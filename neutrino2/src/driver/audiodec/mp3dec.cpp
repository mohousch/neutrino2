/*
	Neutrino-GUI  -   DBoxII-Project
	
	$Id: mp3dec.cpp 2013/10/12 mohousch Exp $

	Copyright (C) 2002 Bjoern Kalkbrenner <terminar@cyberphoria.org>
	(C) 2002,2003,2004 Zwen <Zwen@tuxbox.org>
   
	libmad MP3 low-level core
	Homepage: http://www.cyberphoria.org/

	Kommentar:

	based on
	************************************
	*** madlld -- Mad low-level      ***  v 1.0p1, 2002-01-08
	*** demonstration/decoder        ***  (c) 2001, 2002 Bertrand Petit
	************************************

	License: GPL

	This program is free software; you can redistribute it and/or modify
	it under the terms of the GNU General Public License as published by
	the Free Software Foundation; either version 2 of the License, or
	(at your option) any later version.

	This program is distributed in the hope that it will be useful,
	but WITHOUT ANY WARRANTY; without even the implied warranty of
	MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
	GNU General Public License for more details.

	You should have received a copy of the GNU General Public License
	along with this program; if not, write to the Free Software
	Foundation, Inc., 675 Mass Ave, Cambridge, MA 02139, USA.
*/


/****************************************************************************
 * Includes																	*
 ****************************************************************************/
#ifdef HAVE_CONFIG_H
#include <config.h>
#endif

#include <string.h>
#include <errno.h>
#include <string>
#include <driver/audiodec/mp3dec.h>
#include <linux/soundcard.h>
#include <assert.h>
#include <cmath>

#include <id3tag.h>

#include <global.h>
#include <neutrino2.h>

#include <system/debug.h>

/* libid3tag extension: This is neccessary in order to call fclose
   on the file. Normally libid3tag closes the file implicit.
   For the netfile extension to work properly netfiles fclose must be called.
   To close an id3 file (usually by calling id3_file_close) without fclosing it,
   call following i3_finish_file function. It's just a copy of libid3tags
   finish_file function. */
extern "C"
{
void id3_tag_addref(struct id3_tag *);
void id3_tag_delref(struct id3_tag *);

struct filetag {
	struct id3_tag *tag;
	unsigned long location;
	id3_length_t length;
};

struct id3_file {
	FILE *iofile;
	enum id3_file_mode mode;
	char *path;
	int flags;
	struct id3_tag *primary;
	unsigned int ntags;
	struct filetag *tags;
};

void id3_finish_file(struct id3_file* file);
}

// Frames to skip in ff/rev mode
#define FRAMES_TO_SKIP 50 // 75 
// nr of frames to play after skipping in rev/ff mode
#define FRAMES_TO_PLAY 5

#define ProgName "CMP3Dec"

/****************************************************************************
 * Global variables.														*
 ****************************************************************************/

/****************************************************************************
 * Return an error string associated with a mad error code.					*
 ****************************************************************************/
/* Mad version 0.14.2b introduced the mad_stream_errorstr() function.
 * For previous library versions a replacement is provided below.
 */
#if (MAD_VERSION_MAJOR>=1) || \
    ((MAD_VERSION_MAJOR==0) && \
     (((MAD_VERSION_MINOR==14) && \
       (MAD_VERSION_PATCH>=2)) || \
      (MAD_VERSION_MINOR>14)))
#define MadErrorString(x) mad_stream_errorstr(x)
#else
const char *CMP3Dec::MadErrorString(const struct mad_stream *Stream)
{
	switch(Stream->error)
	{
		/* Generic unrecoverable errors. */
		case MAD_ERROR_BUFLEN:
			return("input buffer too small (or EOF)");
		case MAD_ERROR_BUFPTR:
			return("invalid (null) buffer pointer");
		case MAD_ERROR_NOMEM:
			return("not enough memory");

		/* Frame header related unrecoverable errors. */
		case MAD_ERROR_LOSTSYNC:
			return("lost synchronization");
		case MAD_ERROR_BADLAYER:
			return("reserved header layer value");
		case MAD_ERROR_BADBITRATE:
			return("forbidden bitrate value");
		case MAD_ERROR_BADSAMPLERATE:
			return("reserved sample frequency value");
		case MAD_ERROR_BADEMPHASIS:
			return("reserved emphasis value");

		/* Recoverable errors */
		case MAD_ERROR_BADCRC:
			return("CRC check failed");
		case MAD_ERROR_BADBITALLOC:
			return("forbidden bit allocation value");
		case MAD_ERROR_BADSCALEFACTOR:
			return("bad scalefactor index");
		case MAD_ERROR_BADFRAMELEN:
			return("bad frame length");
		case MAD_ERROR_BADBIGVALUES:
			return("bad big_values count");
		case MAD_ERROR_BADBLOCKTYPE:
			return("reserved block_type");
		case MAD_ERROR_BADSCFSI:
			return("bad scalefactor selection info");
		case MAD_ERROR_BADDATAPTR:
			return("bad main_data_begin pointer");
		case MAD_ERROR_BADPART3LEN:
			return("bad audio data length");
		case MAD_ERROR_BADHUFFTABLE:
			return("bad Huffman table select");
		case MAD_ERROR_BADHUFFDATA:
			return("Huffman data overrun");
		case MAD_ERROR_BADSTEREO:
			return("incompatible block_type for JS");

		/* Unknown error. This swich may be out of sync with libmad's
		 * defined error codes.
		 */
		default:
			return("Unknown error code");
	}
}
#endif

//
// Converts a sample from mad's fixed point number format to a signed
// short (16 bits).
//
#if 1
struct audio_dither {
	mad_fixed_t error[3];
	mad_fixed_t random;
};
static struct audio_dither left_dither, right_dither;
static inline
unsigned long prng(unsigned long state)
{
  	return (state * 0x0019660dL + 0x3c6ef35fL) & 0xffffffffL;
}

inline signed short CMP3Dec::MadFixedToSShort(const mad_fixed_t Fixed, bool left)
{
	unsigned int scalebits;
	mad_fixed_t output, mask, random;
	struct audio_dither *dither = left ? &left_dither : &right_dither;
	unsigned int bits = 16;
	mad_fixed_t sample = Fixed;
	
	enum {
	MIN = -MAD_F_ONE,
	MAX =  MAD_F_ONE - 1
	};
	
	/* noise shape */
	sample += dither->error[0] - dither->error[1] + dither->error[2];
	
	dither->error[2] = dither->error[1];
	dither->error[1] = dither->error[0] / 2;
	
	/* bias */
	output = sample + (1L << (MAD_F_FRACBITS + 1 - bits - 1));
	
	scalebits = MAD_F_FRACBITS + 1 - bits;
	mask = (1L << scalebits) - 1;
	
	/* dither */
	random  = prng(dither->random);
	output += (random & mask) - (dither->random & mask);
	
	dither->random = random;

	/* clip */
#if 0
	if (output >= MAD_F_ONE)
		output = 32767;
	else if (output < -MAD_F_ONE)
		output = -32768;
#endif

	if (output > MAX)
		output = MAX;
	else if (output < MIN)
		output = MIN;
	
	/* quantize */
	output &= ~mask;
	
	/* error feedback */
	dither->error[0] = sample - output;
	
	/* scale */
	return (signed short) (output >> scalebits);
}
#else
inline signed short CMP3Dec::MadFixedToSShort(const mad_fixed_t Fixed)
{
	/* A fixed point number is formed of the following bit pattern:
	 *
	 * SWWWFFFFFFFFFFFFFFFFFFFFFFFFFFFF
	 * MSB                          LSB
	 * S ==> Sign (0 is positive, 1 is negative)
	 * W ==> Whole part bits
	 * F ==> Fractional part bits
	 *
	 * This pattern contains MAD_F_FRACBITS fractional bits, one
	 * should alway use this macro when working on the bits of a fixed
	 * point number. It is not guaranteed to be constant over the
	 * different platforms supported by libmad.
	 *
	 * The unsigned short value is formed by the least significant
	 * whole part bit, followed by the 15 most significant fractional
	 * part bits. Warning: this is a quick and dirty way to compute
	 * the 16-bit number, madplay includes much better algorithms.
	 */
	if (Fixed >= MAD_F_ONE)
		return 32767;
	else if (Fixed < -MAD_F_ONE)
		return -32768;
 
	return (signed short)(Fixed >> (MAD_F_FRACBITS + 1 - 16));
}
#endif

//
// Print human readable informations about an audio MPEG frame
//
void CMP3Dec::CreateInfo(CAudioMetaData * m, int FrameNumber)
{
	if ( !m )
		return;

	if ( !m->hasInfoOrXingTag )
	{
		m->total_time = m->avg_bitrate != 0 ? static_cast<int>( round( static_cast<double>( m->filesize ) / m->avg_bitrate ) ) : 0;
	}

	if ( FrameNumber == 1 )
	{
		using namespace std;
		string Layer, Mode;

		/* Convert the layer number to it's printed representation. */
		switch(m->layer)
		{
			case MAD_LAYER_I:
				Layer="I";
				break;
			case MAD_LAYER_II:
				Layer="II";
				break;
			case MAD_LAYER_III:
				Layer="III";
				break;
			default:
				Layer="?";
				break;
		}

		/* Convert the audio mode to it's printed representation. */
		switch(m->mode)
		{
			case MAD_MODE_SINGLE_CHANNEL:
				Mode="single channel";
				break;
			case MAD_MODE_DUAL_CHANNEL:
				Mode="dual channel";
				break;
			case MAD_MODE_JOINT_STEREO:
				Mode="joint stereo";
				break;
			case MAD_MODE_STEREO:
				Mode="normal stereo";
				break;
			default:
				Mode="unkn. mode";
				break;
		}

#ifdef INCLUDE_UNUSED_STUFF
		const char *Emphasis, *Vbr;
	
		/* Convert the emphasis to it's printed representation. */
		switch(m->emphasis)
		{
			case MAD_EMPHASIS_NONE:
				Emphasis="no";
				break;
			case MAD_EMPHASIS_50_15_US:
				Emphasis="50/15 us";
				break;
			case MAD_EMPHASIS_CCITT_J_17:
				Emphasis="CCITT J.17";
				break;
			default:
				Emphasis="(unexpected emphasis value)";
				break;
		}

		if(m->vbr)
			Vbr="VBR ";
		else
			Vbr="";
#endif /* INCLUDE_UNUSED_STUFF */

		m->type_info = string("MPEG Layer ") + Layer + string(" / ") + Mode;
	}

	m->changed = true;
}

CMP3Dec* CMP3Dec::getInstance()
{
	static CMP3Dec *MP3Dec = NULL;
	
	if(MP3Dec == NULL)
	{
		MP3Dec = new CMP3Dec();
	}
	
	return MP3Dec;
}

bool CMP3Dec::GetMetaData(FILE* in, const bool nice, CAudioMetaData* const m)
{
	bool res;
	
	if ( in && m )
	{
		//mp3 infos
		res = GetMP3Info(in, nice, m);
		
		// id3tag infos
		GetID3(in, m);
	}
	else
	{
		res = false;
	}

	return res;
}

/*
 * Scans MP3 header for Xing, Info and Lame tag.  Returns -1 on failure,
 * >= 0 on success.  The returned value specifies the location of the
 * first audio frame.
 *
 * @author Christian Schlotter
 * @date   2004
 *
 * Based on scan_header() from Robert Leslie's "MAD Plug-in for Winamp".
 */
#define BUFFER_SIZE (2*8192) // big enough to skip id3 tags containing jpegs
long CMP3Dec::scanHeader(FILE* input, struct mad_header* const header, struct _tag* const ftag, const bool nice)
{
	struct mad_stream stream;
	struct mad_frame frame;
	unsigned char buffer[BUFFER_SIZE];
	unsigned int buflen = 0;
	int count = 0;
	short refillCount = 4; /* buffer may be refilled refillCount times */
	long filePos = 0; /* return value */

	mad_stream_init( &stream );
	mad_frame_init( &frame );

	if ( ftag )
		tag_init( ftag );

	while ( true )
	{
		if ( buflen < sizeof(buffer) )
		{
			if ( nice )
				usleep( 15000 );

			filePos = ftell( input ); /* remember where reading started */
			if ( filePos == -1 )
			{
				perror( "ftell()" );
			}

			/* fill buffer */
			int readbytes = fread( buffer+buflen, 1, sizeof(buffer)-buflen, input );

			if ( readbytes <= 0 )
			{
				if ( readbytes == -1 )
					filePos = -1;
				break;
			}

			buflen += readbytes;
		}

		mad_stream_buffer( &stream, buffer, buflen );

		while ( true )
		{
			const unsigned char* const actualFrame = stream.this_frame;
			if ( mad_frame_decode( &frame, &stream ) == -1 )
			{
				if ( !MAD_RECOVERABLE( stream.error ) )
					break;

				// check if id3 tag is in the way
				long tagsize = id3_tag_query( stream.this_frame,stream.bufend - stream.this_frame );

				if ( tagsize > 0 ) // id3 tag recognized 
				{
					mad_stream_skip( &stream, tagsize );
					continue;
				}
				else if ( mad_stream_sync( &stream ) != -1 ) // try to sync
				{
					continue;
				}
				else // syncing attempt failed
				{
					// we have to set some limit here, otherwise we would scan junk files completely
					if ( refillCount-- )
					{
						stream.error = MAD_ERROR_BUFLEN;
					}
					break;
				}
			}

			if ( count++ || ( ftag && tag_parse(ftag, &stream) == -1 ) )
			{
				filePos += actualFrame - buffer; /* start of audio data */
				break;
			}
		}

		if ( count || stream.error != MAD_ERROR_BUFLEN )
			break;

		if ( refillCount-- )
		{
			memmove( buffer, stream.next_frame,
					 buflen = &buffer[buflen] - stream.next_frame );
		}
		else
		{
			break;
		}
	}

	if ( count )
	{
		if ( header )
		{
			*header = frame.header;
		}
	}
	else
	{
		filePos = -1;
	}

	mad_frame_finish( &frame );
	mad_stream_finish( &stream );

	return filePos;
}

/*
 * Function retrieves MP3 metadata.  Returns false on failure, true on success.
 *
 * Inspired by get_fileinfo() from Robert Leslie's "MAD Plug-in for Winamp" and
 * decode_filter() from Robert Leslie's "madplay".
 */
bool CMP3Dec::GetMP3Info(FILE* input, const bool nice, CAudioMetaData* const meta)
{
	dprintf(DEBUG_DEBUG, "CMP3Dec::GetMP3Info\n");
	
	struct mad_header header;
	struct _tag ftag;
	mad_header_init( &header );
	tag_init( &ftag );
	bool result = true;

	if ( ( meta->audio_start_pos = scanHeader(input, &header, &ftag, nice) ) != -1 )
	{
		meta->type = CAudioMetaData::MP3;
		meta->bitrate = header.bitrate;
		meta->layer = header.layer;
		meta->mode = header.mode;
		meta->samplerate = header.samplerate;

		if (fseek( input, 0, SEEK_END ))
		{
			perror( "fseek()" );
			result = false;
		}
		/* 
		this is still not 100% accurate, because it does not take
		id3 tags at the end of the file in account 
		*/
		meta->filesize = ( ftell( input ) - meta->audio_start_pos ) * 8;
		
		/* valid Xing vbr tag present? */
		if ( ( ftag.flags & TAG_XING ) &&
			 ( ftag.xing.flags & TAG_XING_FRAMES ) )
		{
			meta->hasInfoOrXingTag = true;
			mad_timer_t timer = header.duration;
			mad_timer_multiply( &timer, ftag.xing.frames );

			meta->total_time = mad_timer_count( timer, MAD_UNITS_SECONDS );
		}
		else /* no valid Xing vbr tag present */
		{
			meta->total_time = header.bitrate != 0 ? meta->filesize / header.bitrate : 0;
		}

		/* vbr file */
		if ( ftag.flags & TAG_VBR )
		{
			meta->vbr = true;
			meta->avg_bitrate = meta->total_time != 0
				? static_cast<int>( round( static_cast<double>(meta->filesize)
										   / meta->total_time ) )
				: 0;
		}
		else /* we do not know wether the file is vbr or not */
		{
			meta->vbr = false;
			meta->avg_bitrate = header.bitrate;

		}
	}
	else /* scanning failed */
	{
		result = false;
	}

	if ( !result )
	{
		meta->clear();
	}

	return result;
}

void CMP3Dec::GetID3(FILE* in, CAudioMetaData * const m)
{
	dprintf(DEBUG_DEBUG, "CMP3Dec::GetID3\n");
	
	unsigned int i;
	struct id3_frame const *frame;
	id3_ucs4_t const *ucs4;
	id3_utf8_t *utf8;
	char const spaces[] = "          ";
	
	struct 
		{
		char const *id;
		char const *name;
	} const info[] = 
		{
			{ ID3_FRAME_TITLE,  "Title"},
			{ "TIT3",           0},	 /* Subtitle */
			{ "TCOP",           0,},  /* Copyright */
			{ "TPRO",           0,},  /* Produced */
			{ "TCOM",           "Composer"},
			{ ID3_FRAME_ARTIST, "Artist"},
			{ "TPE2",           "Orchestra"},
			{ "TPE3",           "Conductor"},
			{ "TEXT",           "Lyricist"},
			{ ID3_FRAME_ALBUM,  "Album"},
			{ ID3_FRAME_YEAR,   "Year"},
			{ ID3_FRAME_TRACK,  "Track"},
			{ "TPUB",           "Publisher"},
			{ ID3_FRAME_GENRE,  "Genre"},
			{ "TRSN",           "Station"},
			{ "TENC",           "Encoder"}
		};

	// text information
	struct id3_file * id3file = id3_file_fdopen(fileno(in), ID3_FILE_MODE_READONLY);
	if(id3file != 0)
	{
		id3_tag * tag = id3_file_tag(id3file);
		if(tag)
		{
			//
			for(i = 0; i < sizeof(info) / sizeof(info[0]); ++i)
			{
				union id3_field const *field;
				unsigned int nstrings, namelen, j;
				char const *name;

				frame = id3_tag_findframe(tag, info[i].id, 0);
				if(frame == 0)
					continue;

				field    = &frame->fields[1];
				nstrings = id3_field_getnstrings(field);

				name = info[i].name;
				namelen = name ? strlen(name) : 0;
				assert(namelen < sizeof(spaces));

				for(j = 0; j < nstrings; ++j)
				{
					ucs4 = id3_field_getstrings(field, j);
					assert(ucs4);

					if(strcmp(info[i].id, ID3_FRAME_GENRE) == 0)
						ucs4 = id3_genre_name(ucs4);

					utf8 = id3_ucs4_utf8duplicate(ucs4);
					if (utf8 == NULL)
						goto fail;

					if (j == 0 && name)
					{
						if(strcmp(name, "Title") == 0)
							m->title = (char *) utf8;

						if(strcmp(name, "Artist") == 0)
							m->artist = (char *) utf8;

						if(strcmp(name, "Year") == 0)
							m->date = (char *) utf8;

						if(strcmp(name, "Album") == 0)
							m->album = (char *) utf8;

						if(strcmp(name, "Genre") == 0)
							m->genre = (char *) utf8;
					}
					else
					{
						if(strcmp(info[i].id, "TCOP") == 0 || strcmp(info[i].id, "TPRO") == 0)
						{
							//printf("%s  %s %s\n", spaces, (info[i].id[1] == 'C') ? ("Copyright (C)") : ("Produced (P)"), latin1);
						}
					}

					free(utf8);
				}
			}

			// comments
#ifdef INCLUDE_UNUSED_STUFF
			i = 0;
			while((frame = id3_tag_findframe(tag, ID3_FRAME_COMMENT, i++)))
			{
				id3_utf8_t *ptr, *newline;
				int first = 1;

				ucs4 = id3_field_getstring(&frame->fields[2]);
				assert(ucs4);

				if(*ucs4)
					continue;

				ucs4 = id3_field_getfullstring(&frame->fields[3]);
				assert(ucs4);

				utf8 = id3_ucs4_utf8duplicate(ucs4);
				if (utf8 == 0)
					goto fail;

				ptr = utf8;
				while(*ptr)
				{
					newline = (id3_utf8_t *) strchr((char*)ptr, '\n');
					if(newline)
						*newline = 0;

					if(strlen((char *)ptr) > 66)
					{
						id3_utf8_t *linebreak;

						linebreak = ptr + 66;

						while(linebreak > ptr && *linebreak != ' ')
							--linebreak;

						if(*linebreak == ' ')
						{
							if(newline)
								*newline = '\n';

							newline = linebreak;
							*newline = 0;
						}
					}

					if(first)
					{
						char const *name;
						unsigned int namelen;

						name    = "Comment";
						namelen = strlen(name);
						assert(namelen < sizeof(spaces));
						mp3->Comment = (char *) ptr;
						//printf("%s%s: %s\n", &spaces[namelen], name, ptr);
						first = 0;
					}
					else
						//printf("%s  %s\n", spaces, ptr);

						ptr += strlen((char *) ptr) + (newline ? 1 : 0);
				}

				free(utf8);
				break;
			}
#endif

			// cover
			//const char * coverfile = "/tmp/cover.jpg";
			std::string coverfile = "/tmp/audioplayer/";
			coverfile += m->title;
			coverfile += ".jpg";

			frame = id3_tag_findframe(tag, "APIC", 0);
			
			if (frame)
			{
				// picture file data
				unsigned int j;
				union id3_field const *field;
				
				for (j = 0; (field = id3_frame_field(frame, j)); j++)
				{
					switch (id3_field_type(field))
					{
						case ID3_FIELD_TYPE_BINARYDATA:
							id3_length_t size;
							id3_byte_t const *data;

							data = id3_field_getbinarydata(field, &size);
							if ( data )
							{
								dprintf(DEBUG_DEBUG, "CMP3Dec::SaveCover: Cover found\n");
								
								m->cover = coverfile;

								FILE * pFile;
								pFile = fopen(coverfile.c_str(), "wb");
								fwrite(data, 1 , size , pFile);
								fclose (pFile);
							}	
							break;
							
						case ID3_FIELD_TYPE_INT8:
							//pic->type = id3_field_getint(field);
							break;
							
						default:
							break;
					}
				}
			}
		
			id3_tag_delete(tag);
		}

		id3_finish_file(id3file);
	}
	
	if(0)
	{
fail:
		dprintf(DEBUG_DEBUG, "id3: not enough memory to display tag\n");
	}
}

// this is a copy of static libid3tag function "finish_file"
// which cannot be called from outside
void id3_finish_file(struct id3_file* file)
{
	unsigned int i;
	
	if (file->path)
		free(file->path);

	if (file->primary) 
	{
		id3_tag_delref(file->primary);
		id3_tag_delete(file->primary);
	}
	
	for (i = 0; i < file->ntags; ++i) 
	{
		struct id3_tag *tag;
		
		tag = file->tags[i].tag;
		if (tag) 
		{
			id3_tag_delref(tag);
			id3_tag_delete(tag);
		}
	}
	
	if (file->tags)
		free(file->tags);
	
	free(file);
}	

 
