#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <stdio.h>

#include <algorithm>
#include <cstdlib>

#include <string>
#include <vector>
#include <list>

#include <gui/filebrowser.h>

#include <gui/widget/hintbox.h>
#include <gui/widget/helpbox.h>
#include <gui/widget/messagebox.h>
#include <gui/widget/stringinput_ext.h>
#include <gui/movieinfo.h>

#include <dirent.h>
#include <sys/stat.h>

#include <unistd.h>

#include <neutrino.h>
#include <gui/widget/stringinput.h>
#include <gui/widget/widget_helpers.h>

#include <sys/vfs.h> // for statfs
#include <sys/types.h>
#include <fcntl.h>

#include <utime.h>

#include <system/debug.h>

#include <gui/movieinfo.h>

#include <driver/color.h>
#include <driver/framebuffer.h>

#include <system/settings.h>


#define my_scandir scandir64
#define my_alphasort alphasort64
typedef struct stat64 stat_struct;
typedef struct dirent64 dirent_struct;
#define my_stat stat64

//static CProgressBar * timescale;

// tstool
off64_t get_full_len(char * startname)
{
        off64_t fulllength=0;
        struct stat64 s;
        char spart[255];
        int part = 0;

        stat64(startname, &s);
        do {
                fulllength +=s.st_size;
                sprintf(spart, "%s.%03d", startname, ++part);
        } while (!stat64(spart, &s));
	
        return fulllength;
}

static void reset_atime(char * path, time_t tt)
{
	struct utimbuf ut;
	ut.actime = tt-1;
	ut.modtime = tt-1;
	utime(path, &ut);
}

#define BUF_SIZE 1395*188
#define SAFE_GOP 1395*188
#define MP_TS_SIZE 262072 // ~0.5 sec
#define MINUTEOFFSET 117*262072
#define SECONDOFFSET MP_TS_SIZE*2
off64_t truncate_movie(MI_MOVIE_INFO * minfo)
{
	struct stat64 s;
	char spart[255];
	int part = 0, tpart = 0;
	bool found = 0;
	char * name = (char *) minfo->file.Name.c_str();
	off64_t size = minfo->file.Size;
	int len = minfo->length;
	int seconds = minfo->bookmarks.end;
	off64_t minuteoffset = len ? size / len : MINUTEOFFSET;
	minuteoffset = (minuteoffset / MP_TS_SIZE) * MP_TS_SIZE;
	if(minuteoffset < 10000000 || minuteoffset > 90000000)
		minuteoffset = MINUTEOFFSET;
	off64_t secsize = minuteoffset/60;
	off64_t secoffset = secsize * seconds;
	off64_t newsize = secoffset;

	//printf("truncate: name %s size %lld len %d sec truncate to %d sec, new size %lld\n", name, size, len, seconds, secoffset);
	
	sprintf(spart, "%s", name);
	while (!stat64(spart, &s)) 
	{
		if(found) 
		{
			//printf("truncate: check part %d file %s - TO REMOVE\n", part, spart);
			
			unlink(spart);
		} 
		else 
		{
			//printf("truncate: check part %d file %s - OK\n", part, spart);
			
			if(secoffset < s.st_size) 
			{
				tpart = part;
				found = 1;
			} 
			else
				secoffset -= s.st_size;
		}
		sprintf(spart, "%s.%03d", name, ++part);
	}
	
	if(found) 
	{
		if(tpart)
			sprintf(spart, "%s.%03d", name, tpart);
		else
			sprintf(spart, "%s", name);
		
		printf("truncate: part %s to size %lld\n", spart, secoffset);
		
		truncate(spart, secoffset);
		minfo->file.Size = newsize;
		minfo->length = minfo->bookmarks.end/60;
		minfo->bookmarks.end = 0;
		reset_atime(spart, minfo->file.Time);
		return newsize;
	}
	return 0;
}

struct mybook {
        off64_t pos;
        off64_t len;
	bool ok;
};
#define REAL_CUT 1

static int check_pes_start (unsigned char *packet)
{
	// PCKT: 47 41 91 37 07 50 3F 14 BF 04 FE B9 00 00 01 EA 00 00 8C ...
	if (packet[0] == 0x47 &&                    // sync byte 0x47
	    (packet[1] & 0x40))                     // pusi == 1
	{
		/* good, now we have to check if it is video stream */
		unsigned char *pes = packet + 4;
		if (packet[3] & 0x20)                   // adaptation field is present
			pes += packet[4] + 1;

		if (!memcmp(pes, "\x00\x00\x01", 3) && (pes[3] & 0xF0) == 0xE0) // PES start & video type
		{
			//return 1; //(pes[4] << 8) | pes[5];       // PES packet len
			pes += 4;
			while (pes < (packet + 188 - 4))
				if (!memcmp (pes, "\x00\x00\x01\xB8", 4)) // GOP detect
					return 1;
				else
					pes++;
		}
	}
	
	return 0;
}

int find_gop(unsigned char *buf, int r)
{
	for(int j = 0; j < r/188; j++) 
	{
		if(check_pes_start(&buf[188*j])) 
		{
			return 188*j;
		}
	}
	return -1;
}

off64_t fake_read(int fd, unsigned char *buf, size_t size, off64_t fsize)
{
	off64_t cur = lseek64 (fd, 0, SEEK_CUR);

	buf[0] = 0x47;
	if((cur + size) > fsize)
		return (fsize - cur);
	else
		return size;
}

#define PSI_SIZE 188*3
static int read_psi(char * spart, unsigned char * buf)
{
	int srcfd = open (spart, O_RDONLY | O_LARGEFILE);
	if(srcfd >= 0) 
	{
		/* read psi */
		int r = read (srcfd, buf, PSI_SIZE);
		close(srcfd);
		if(r != PSI_SIZE) 
		{
			perror("read psi");
			return -1;
		}
		return 0;
	}
	return -1;
}

static void save_info(CMovieInfo * cmovie, MI_MOVIE_INFO * minfo, char * dpart, off64_t spos, off64_t secsize)
{
	MI_MOVIE_INFO ninfo;
	
	cmovie->copy(minfo, &ninfo);
	ninfo.file.Name = dpart;
	ninfo.file.Size = spos;
	ninfo.length = spos/secsize/60;
	ninfo.bookmarks.end = 0;
	ninfo.bookmarks.start = 0;
	ninfo.bookmarks.lastPlayStop = 0;
	for(int book_nr = 0; book_nr < MI_MOVIE_BOOK_USER_MAX; book_nr++) {
		if( ninfo.bookmarks.user[book_nr].pos != 0 && ninfo.bookmarks.user[book_nr].length > 0 ) {
			ninfo.bookmarks.user[book_nr].pos = 0;
			ninfo.bookmarks.user[book_nr].length = 0;
		}
	}
	cmovie->saveMovieInfo(ninfo);
	cmovie->clearMovieInfo(&ninfo);
	reset_atime(dpart, minfo->file.Time);
}

static void find_new_part(char * npart, char * dpart)
{
	struct stat64 s;
	int dp = 0;
	sprintf(dpart, "%s_%d.ts", npart, dp);
	while (!stat64(dpart, &s)) {
		sprintf(dpart, "%s_%d.ts", npart, ++dp);
	}
}

int compare_book(const void *x, const void *y)
{
        struct mybook * px, * py;
	int dx, dy;
        px = (struct mybook*) x;
        py = (struct mybook*) y;
	dx = px->pos / (off64_t) 1024;
	dy = py->pos / (off64_t) 1024;
	int res = dx - dy;
	//printf("SORT: %lld and %lld res %d\n", px->pos, py->pos, res);
	return res;
}

static int get_input(bool * stop)
{
	neutrino_msg_data_t data;
	neutrino_msg_t msg;
	int retval = 0;
	* stop = false;
	g_RCInput->getMsg(&msg, &data, 1, false);
	
	if(msg == RC_home) 
	{
		if(MessageBox(_("Information"), _("Cancel movie cut/split ?"), mbrNo, mbYes | mbNo) == mbrYes) 
		{
			* stop = true;
		}
	}
	
	if(msg != RC_timeout)
		retval |= 1;
	if (CNeutrinoApp::getInstance()->handleMsg(msg, data) & messages_return::cancel_all)
		retval |= 2;

	//printf("input: msg %d (%x) ret %d\n", msg, msg, retval);

	return retval;
}

off64_t cut_movie(MI_MOVIE_INFO * minfo, CMovieInfo * cmovie)
{
	struct mybook books[MI_MOVIE_BOOK_USER_MAX+2];
	int bcount = 0;
	int dstfd, srcfd;
	int part = 0;
	struct stat64 s;
	char spart[255];
	char dpart[255];
	char npart[255];
	unsigned char * buf;
	unsigned char psi[PSI_SIZE];
	int r, i;
	off64_t sdone, spos;
	off64_t newsize;
	time_t tt;
	int percent = 0;
	char * name = (char *) minfo->file.Name.c_str();
	CFile file;
	MI_MOVIE_INFO ninfo;
	bool need_gop = 0;
	off64_t tdone = 0;
	int was_cancel = 0;
	int retval = 0;
	time_t tt1;
	off64_t bpos, bskip;

	buf = (unsigned char *) malloc(BUF_SIZE);
	if(buf == 0) {
		perror("malloc");
		return 0;
	}

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();

	//if(!timescale) 
	//	timescale = new CProgressBar(200, 15, 0, 100, 0);
	
        int dx = 256;
        int x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
        int y = g_settings.screen_EndY - 50;
	
	frameBuffer->paintBoxRel (x + 40, y+12, 200, 15, COL_INFOBAR_PLUS_0);

	//timescale->setPosition(x + 41, y + 12, 200, 15);
	//timescale->paint(x + 41, y + 12, percent);
	
	int len = minfo->length;
	off64_t size = minfo->file.Size;
	//off64_t secsize = len ? size/len/60 : 511040;
	off64_t minuteoffset = len ? size / len : MINUTEOFFSET;
	minuteoffset = (minuteoffset / MP_TS_SIZE) * MP_TS_SIZE;
	if(minuteoffset < 5000000 || minuteoffset > 190000000)
		minuteoffset = MINUTEOFFSET;
	off64_t secsize = minuteoffset/60;
	newsize = size;

	if(minfo->bookmarks.start != 0) 
	{
			books[bcount].pos = 0;
			books[bcount].len = (minfo->bookmarks.start * secsize)/188 * 188;
			if(books[bcount].len > SAFE_GOP)
				books[bcount].len -= SAFE_GOP;
			books[bcount].ok = 1;
			printf("cut: start bookmark %d at %lld len %lld\n", bcount, books[bcount].pos, books[bcount].len);
			bcount++;
	}
	
	for(int book_nr = 0; book_nr < MI_MOVIE_BOOK_USER_MAX; book_nr++) 
	{
		if( minfo->bookmarks.user[book_nr].pos != 0 && minfo->bookmarks.user[book_nr].length > 0 ) 
		{
			books[bcount].pos = (minfo->bookmarks.user[book_nr].pos * secsize)/188 * 188;
			books[bcount].len = (minfo->bookmarks.user[book_nr].length * secsize)/188 * 188;
			if(books[bcount].len > SAFE_GOP)
				books[bcount].len -= SAFE_GOP;
			books[bcount].ok = 1;
			printf("cut: jump bookmark %d at %lld len %lld -> skip to %lld\n", bcount, books[bcount].pos, books[bcount].len, books[bcount].pos+books[bcount].len);
			bcount++;
		}
	}
	
	if(minfo->bookmarks.end != 0) 
	{
			books[bcount].pos = ((off64_t) minfo->bookmarks.end * secsize)/188 * 188;
			books[bcount].len = size - books[bcount].pos;
			//if(books[bcount].pos > SAFE_GOP)
			//	books[bcount].pos -= SAFE_GOP;
			books[bcount].ok = 1;
			printf("cut: end bookmark %d at %lld\n", bcount, books[bcount].pos);
			bcount++;
	}
	printf("\n");
	if(!bcount) return 0;
	qsort(books, bcount, sizeof(struct mybook), compare_book);
	for(i = 0; i < bcount; i++) 
	{
		if(books[i].ok) 
		{
			printf("cut: bookmark %d at %lld len %lld -> skip to %lld\n", i, books[i].pos, books[i].len, books[i].pos+books[i].len);
			newsize -= books[i].len;
			off64_t curend = books[i].pos + books[i].len;
			for(int j = i + 1; j < bcount; j++) 
			{
				if((books[j].pos > books[i].pos) && (books[j].pos < curend)) 
				{
					off64_t newend = books[j].pos + books[j].len;
					if(newend > curend) 
					{
						printf("cut: bad bookmark %d, position %lld len %lld, ajusting..\n", j, books[j].pos, books[j].len);
						books[j].pos = curend;
						books[j].len = newend - curend;
					} 
					else 
					{
						printf("cut: bad bookmark %d, position %lld len %lld, skipping..\n", j, books[j].pos, books[j].len);
						books[j].ok = 0;
					}
				}
			}
		}
	}
	sprintf(npart, "%s", name);
	char * ptr = strstr(npart, ".ts");
	if(ptr)
		*ptr = 0;
	find_new_part(npart, dpart);
	tt = time(0);
	printf("\n********* new file %s expected size %lld, start time %s", dpart, newsize, ctime (&tt));
	
	dstfd = open (dpart, O_CREAT|O_WRONLY|O_TRUNC| O_LARGEFILE, 0x644);

	if(dstfd < 0) 
	{
		perror(dpart);
		return 0;
	}
	part = 0;
	i = 0;
	off64_t offset = 0;
	spos = 0;
	sprintf(spart, "%s", name);
	if(read_psi(spart, &psi[0])) 
	{
		perror(spart);
		goto ret_err;
	}
	write(dstfd, psi, PSI_SIZE);
	bpos = books[i].pos;
	bskip = books[i].len;
	while (!stat64(spart, &s)) 
	{
		printf("cut: open part %d file %s size %lld offset %lld book pos %lld\n", part, spart, s.st_size, offset, bpos);
		srcfd = open (spart, O_RDONLY | O_LARGEFILE);
		if(srcfd < 0) {
			perror(spart);
			goto ret_err;
		}
		if(offset >= s.st_size) 
		{
			offset -= s.st_size;
			bpos -= s.st_size;
			goto next_file;
		}
		lseek64 (srcfd, offset, SEEK_SET);
		sdone = offset;
		while(true) 
		{
			off64_t until = bpos;
			printf("\ncut: reading from %lld to %lld (%lld) want gop %d\n", sdone, until, until - sdone, need_gop);
			while(sdone < until) 
			{
				bool stop;
				int msg = get_input(&stop);
				was_cancel = msg & 2;
				
				if(stop) 
				{
					close(srcfd);
					unlink(dpart);
					retval = 1;
					goto ret_err;
				}
				
				if(msg) 
				{
					//timescale->reset();
					frameBuffer->paintBoxRel (x + 40, y+12, 200, 15, COL_INFOBAR_PLUS_0);
				}
				size_t toread = (until-sdone) > BUF_SIZE ? BUF_SIZE : until - sdone;
#if REAL_CUT
				r = read (srcfd, buf, toread);
#else
				r = fake_read (srcfd, buf, toread, s.st_size);
#endif
				if(r > 0) 
				{
					int wptr = 0;
					// FIXME: TEST
					if(r != BUF_SIZE) 
						printf("****** short read ? %d\n", r);
					
					if(buf[0] != 0x47) 
						printf("cut: buffer not aligned at %lld\n", sdone);
					
					if(need_gop) 
					{
						int gop = find_gop(buf, r);
						if(gop >= 0) 
						{
							printf("cut: GOP found at %lld offset %d\n", (off64_t)(sdone+gop), gop);
							newsize -= gop;
							wptr = gop;
						} 
						else
							printf("cut: GOP needed, but not found\n");
						need_gop = 0;
					}
					sdone += r;
					spos += r - wptr;
					percent = (int) ((float)(spos)/(float)(newsize)*100.);

					//timescale->setPosition(x + 41, y + 12, 200 , 15);
					//timescale->paint(x + 41, y + 12, percent);
#if REAL_CUT
					int wr = write(dstfd, &buf[wptr], r-wptr);
					if(wr < (r-wptr)) 
					{
						perror(dpart);
						close(srcfd);
						goto ret_err;
					}
#endif
				} 
				else if(sdone < s.st_size) 
				{
					/* read error ? */
					close(srcfd);
					perror(spart);
					goto ret_err;
				} 
				else 
				{
					printf("cut: next file -> sdone %lld spos %lld bpos %lld\n", sdone, spos, bpos);
					offset = 0;
					bpos -= sdone;
					goto next_file;
				}
			}
			printf("cut: current file pos %lld write pos %lld book pos %lld still to read %lld\n", sdone, spos, bpos, sdone - bpos);
			need_gop = 1;
			offset = bpos + bskip;
			i++;
			while(i < bcount) 
			{
				if(books[i].ok)
					break;
				else
					i++;
			}
			if(i < bcount) 
			{
				bpos = books[i].pos;
				bskip = books[i].len;
			} 
			else
				bpos = size;
			printf("cut: next bookmark pos: %lld abs %lld relative next file pos %lld cur file size %lld\n", bpos, bpos - tdone, offset, s.st_size);
			bpos -= tdone; /* all books from 0, converting to 0 + total size skipped */
			if(offset >= s.st_size) 
			{
				offset -= s.st_size;
				bpos -= s.st_size;
				goto next_file;
			}
			lseek64 (srcfd, offset, SEEK_SET);
			sdone = offset;
		}
next_file:
		tdone += s.st_size;
		close(srcfd);
		sprintf(spart, "%s.%03d", name, ++part);
	}
	 tt1 = time(0);
	printf("********* total written %lld tooks %ld secs end time %s", spos, tt1-tt, ctime (&tt1));

	save_info(cmovie, minfo, dpart, spos, secsize);
	retval = 1;
	lseek64 (dstfd, 0, SEEK_SET);
ret_err:
	close(dstfd);
	free(buf);
	if(was_cancel)
		g_RCInput->postMsg(RC_home, 0);
	return retval;
}

off64_t copy_movie(MI_MOVIE_INFO * minfo, CMovieInfo * cmovie, bool onefile)
{
	struct mybook books[MI_MOVIE_BOOK_USER_MAX+2];
	int bcount = 0;
	int dstfd = -1, srcfd;
	int part = 0;
	struct stat64 s;
	char spart[255];
	char dpart[255];
	char npart[255];
	unsigned char * buf;
	unsigned char psi[PSI_SIZE];
	int r, i;
	off64_t sdone, spos = 0, btotal = 0;
	off64_t newsize;
	time_t tt;
	int percent = 0;
	char * name = (char *) minfo->file.Name.c_str();
	CFile file;
	bool need_gop = 0;
	bool dst_done = 0;
	bool was_cancel = 0;
	int retval = 0;

	buf = (unsigned char *) malloc(BUF_SIZE);
	if(buf == 0) 
	{
		perror("malloc");
		return 0;
	}

	int len = minfo->length;
	off64_t size = minfo->file.Size;
	off64_t minuteoffset = len ? size / len : MINUTEOFFSET;
	minuteoffset = (minuteoffset / MP_TS_SIZE) * MP_TS_SIZE;
	if(minuteoffset < 5000000 || minuteoffset > 190000000)
		minuteoffset = MINUTEOFFSET;
	off64_t secsize = minuteoffset/60;
	//off64_t secsize = len ? size/len/60 : 511040;
	printf("copy: len %d minute %lld second %lld\n", len, len ? size/len : 511040*60, secsize);

	CFrameBuffer * frameBuffer = CFrameBuffer::getInstance();
	//if(!timescale)
	//	timescale = new CProgressBar(200, 15, 0, 100, 0);
        int dx = 256;
        int x = (((g_settings.screen_EndX- g_settings.screen_StartX)- dx) / 2) + g_settings.screen_StartX;
        int y = g_settings.screen_EndY - 50;
	frameBuffer->paintBoxRel (x + 40, y+12, 200, 15, COL_INFOBAR_PLUS_0);

	//timescale->setPosition(x + 41, y + 12, 200, 15);
	//timescale->paint(x + 41, y + 12, percent);

	newsize = 0;
	for(int book_nr = 0; book_nr < MI_MOVIE_BOOK_USER_MAX; book_nr++) 
	{
		if( minfo->bookmarks.user[book_nr].pos != 0 && minfo->bookmarks.user[book_nr].length > 0 ) 
		{
			books[bcount].pos = (minfo->bookmarks.user[book_nr].pos * secsize)/188 * 188;
			if(books[bcount].pos > SAFE_GOP)
				books[bcount].pos -= SAFE_GOP;
			books[bcount].len = (minfo->bookmarks.user[book_nr].length * secsize)/188 * 188;
			books[bcount].ok = 1;
			printf("copy: jump bookmark %d at %lld len %lld\n", bcount, books[bcount].pos, books[bcount].len);
			newsize += books[bcount].len;
			bcount++;
		}
	}
	if(!bcount) return 0;

	tt = time(0);
	printf("********* %d boormarks, to %s file(s), expected size to copy %lld, start time %s", bcount, onefile ? "one" : "many", newsize, ctime (&tt));
	sprintf(npart, "%s", name);
	char * ptr = strstr(npart, ".ts");
	if(ptr)
		*ptr = 0;
	sprintf(spart, "%s", name);
	srcfd = open (spart, O_RDONLY | O_LARGEFILE);
	if(read_psi(spart, &psi[0])) 
	{
		perror(spart);
		goto ret_err;
	}
	
	for(i = 0; i < bcount; i++) 
	{
		printf("\ncopy: processing bookmark %d at %lld len %lld\n", i, books[i].pos, books[i].len);
		off64_t bpos = books[i].pos;
		off64_t bskip = books[i].len;
		part = 0;
		sprintf(spart, "%s", name);
		int sres;
		while (!(sres = stat64(spart, &s))) 
		{
			if(bpos >= s.st_size) 
			{
				bpos -= s.st_size;
				sprintf(spart, "%s.%03d", name, ++part);
				//printf("copy: check src part %s\n", spart);
				continue;
			}
			break;
		}
		if(sres != 0) {
			printf("file for bookmark %d with offset %lld not found\n", i, books[i].pos);
			continue;
		}
		if(!dst_done || !onefile) 
		{
			find_new_part(npart, dpart);
			
			dstfd = open (dpart, O_CREAT|O_WRONLY|O_TRUNC| O_LARGEFILE, 0x644);

			printf("copy: new file %s fd %d\n", dpart, dstfd);
			if(dstfd < 0) 
			{
				printf("failed to open %s\n", dpart);
				goto ret_err;;
			}
			dst_done = 1;
			spos = 0;
			write(dstfd, psi, PSI_SIZE);
		}
		need_gop = 1;
next_file:
		stat64(spart, &s);
		printf("copy: open part %d file %s size %lld offset %lld\n", part, spart, s.st_size, bpos);
		srcfd = open (spart, O_RDONLY | O_LARGEFILE);
		if(srcfd < 0) 
		{
			printf("failed to open %s\n", spart);
			close(dstfd);
			goto ret_err;
		}
		lseek64 (srcfd, bpos, SEEK_SET);
		sdone = bpos;
		off64_t until = bpos + bskip;
		
		printf("copy: read from %lld to %lld read size %d want gop %d\n", bpos, until, BUF_SIZE, need_gop);
		
		while(sdone < until) 
		{
			size_t toread = (until-sdone) > BUF_SIZE ? BUF_SIZE : until - sdone;
			bool stop;
			int msg = get_input(&stop);
			was_cancel = msg & 2;
			if(stop) 
			{
				close(srcfd);
				close(dstfd);
				unlink(dpart);
				retval = 1;
				goto ret_err;
			}
			
			if(msg) 
			{
				frameBuffer->paintBoxRel (x + 40, y+12, 200, 15, COL_INFOBAR_PLUS_0);

				//timescale->reset();
			}
#if REAL_CUT
			r = read (srcfd, buf, toread);
#else
			r = fake_read (srcfd, buf, toread, s.st_size);
#endif
			if(r > 0) 
			{
				int wptr = 0;
				// FIXME: TEST
				if(r != BUF_SIZE) 
					printf("****** short read ? %d\n", r);
				
				if(buf[0] != 0x47) 
					printf("copy: buffer not aligned at %lld\n", sdone);
				
				if(need_gop) 
				{
					int gop = find_gop(buf, r);
					if(gop >= 0) 
					{
						printf("cut: GOP found at %lld offset %d\n", (off64_t)(sdone+gop), gop);
						newsize -= gop;
						wptr = gop;
					} 
					else
						printf("cut: GOP needed, but not found\n");
					need_gop = 0;
				}
				sdone += r;
				bskip -= r;
				spos += r - wptr;
				btotal += r;
				percent = (int) ((float)(btotal)/(float)(newsize)*100.);

				//timescale->setPosition(x + 41, y + 12, 200, 15);
				//timescale->paint(x + 41, y + 12, percent);
#if REAL_CUT
				int wr = write(dstfd, &buf[wptr], r-wptr);
				if(wr < (r-wptr)) 
				{
					printf("write to %s failed\n", dpart);
					close(srcfd);
					close(dstfd);
					goto ret_err;
				}
#endif
			} 
			else if(sdone < s.st_size) 
			{
				/* read error ? */
				printf("%s: read failed\n", spart);
				close(srcfd);
				close(dstfd);
				goto ret_err;
			} 
			else 
			{
				printf("copy: -> next file, file pos %lld written %lld left %lld\n", sdone, spos, bskip);
				bpos = 0;
				close(srcfd);
				sprintf(spart, "%s.%03d", name, ++part);
				goto next_file;
			}
		} /* while(sdone < until) */
		close(srcfd);

		if(!onefile) 
		{
			close(dstfd);
			save_info(cmovie, minfo, dpart, spos, secsize);
			time_t tt1 = time(0);
			printf("copy: ********* %s: total written %lld took %ld secs\n", dpart, spos, tt1-tt);
		}
	} /* for all books */
	
	if(onefile) 
	{
		close(dstfd);
		save_info(cmovie, minfo, dpart, spos, secsize);
		time_t tt1 = time(0);
		printf("copy: ********* %s: total written %lld took %ld secs\n", dpart, spos, tt1-tt);
	}
	retval = 1;
ret_err:
	free(buf);
	if(was_cancel)
		g_RCInput->postMsg(RC_home, 0);
	return retval;
}

