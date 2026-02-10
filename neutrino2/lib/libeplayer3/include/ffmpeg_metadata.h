/*
 * ffmpeg_metadata.h
 *
 * This program is free software; you can redistribute it and/or modify
 * it under the terms of the GNU General Public License as published by
 * the Free Software Foundation; either version 2 of the License, or
 * (at your option) any later version.
 *
 * This program is distributed in the hope that it will be useful,
 * but WITHOUT ANY WARRANTY; without even the implied warranty of
 * MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 * GNU General Public License for more details.
 *
 * You should have received a copy of the GNU General Public License
 * along with this program; if not, write to the Free Software
 * Foundation, Inc., 59 Temple Place, Suite 330, Boston, MA 02111-1307 USA
 *
 */
 
 #ifndef _ffmpeg_metadata_123
#define _ffmpeg_metadata_123

/* these file contains a list of metadata tags which can be used by applications
 * to stream specific information. it maps the tags to ffmpeg specific tags.
 *
 * fixme: if we add other container for some resons later (maybe some other libs
 * support better demuxing or something like this), then we should think on a
 * more generic mechanism!
 */

/* metatdata map list:
 */
char * metadata_map[] =
{
	/* our tags      ffmpeg tag / id3v2 */  
	"Title",       "TIT2",
	"Title",       "TT2",
	"Artist",      "TPE1",
	"Artist",      "TP1",
	"AlbumArtist", "TPE2",
	"AlbumArtist", "TP2",
	"Album",       "TALB",
	"Album",       "TAL",
	"Year",        "TDRL",  /* fixme */
	"Year",        "TDRC",  /* fixme */
	"Comment",     "unknown",
	"Track",       "TRCK",
	"Track",       "TRK",
	"Copyright",   "TCOP",
	"Composer",    "TCOM",
	"Genre",       "TCON",
	"Genre",       "TCO",   
	"EncodedBy",   "TENC",   
	"EncodedBy",   "TEN", 
	"Language",    "TLAN",
	"Performer",   "TPE3",
	"Performer",   "TP3",
	"Publisher",   "TPUB",
	"Encoder",     "TSSE",
	"Disc",        "TPOS",
	NULL
};

#endif

