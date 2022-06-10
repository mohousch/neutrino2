/******************************************************************************
 *                        <<< TuxCal - Calendar daemon >>>
 *                (c) Robert "robspr1" Spreitzer 2006 (robert.spreitzer@inode.at)
 ******************************************************************************/
// lots of code is from the tuxmail-project

#include "config.h"

#if !defined(HAVE_DVB_API_VERSION) && defined(HAVE_OST_DMX_H)
#define HAVE_DVB_API_VERSION 1
#endif


#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <time.h>
#include <unistd.h>
#include <netdb.h>
#include <pthread.h>
#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/soundcard.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <sys/un.h>
#include <sys/time.h>
#include <arpa/inet.h>
#include <time.h>
#include <syslog.h>
#include <linux/fb.h>
#include <zlib.h>
#include <malloc.h>
#if HAVE_DVB_API_VERSION == 3
#include <linux/input.h>
#endif

#include <ft2build.h>

#include <plugin.h>

#include FT_FREETYPE_H
#include FT_CACHE_H
#include FT_CACHE_SMALL_BITMAPS_H

#define DSP "/dev/sound/dsp"

#define RIFF	0x46464952
#define WAVE	0x45564157
#define FMT	0x20746D66
#define DATA	0x61746164
#define PCM	1

#define SCKFILE "/tmp/tuxcald.socket"									//! socket-file, connection to daemon
#define RUNFILE "/var/etc/.tuxcald"										//! autostart-file for daemon
#define CFGPATH "/var/tuxbox/config/tuxcal/"					//! config-path
#define TMPPATH "/tmp/"																//! temp-path
#define CFGFILE "tuxcal.conf"													//! config-file
#define EVTFILE "tuxcal.list"													//! database-file
#define PIDFILE "/tmp/tuxcald.pid"										//! PID file
#define CLKFILE "/tmp/tuxcal.clk"											//! clock file
#define NOTIFILE "/tmp/tuxmail.new"										//! notify from tuxmail
#define SNDFILE1 "tuxcal_birthday.wav"								//! birthday sound
#define SNDFILE2 "tuxcal_event.wav"										//! event sound
#define SNDFILE3 "tuxcal_timer.wav"										//! timer sound
#define SHELLFILE	"tuxcal.notify"											//! notify shell file

#define bool char
#define true 1
#define false 0

//----------------------------------------------------
// OSD   different languages

int osdidx = 0;																				// actual used language

#define MAXOSD	2
 
char *monthmsg[12][MAXOSD] = {
	{ "Januar"    , "January" },
	{ "Februar"   , "February" },
	{ "M�rz"      , "March" },
	{ "April"     , "April" },
	{ "Mai"       , "May" },
	{ "Juni"      , "Juni" },
	{ "Juli"      , "July" },
	{ "August"    , "August" },
	{ "September" , "September" },
	{ "Oktober"   , "October" },
	{ "November"  , "November" },
	{ "Dezember"  , "December" }
};

char *vdaysnames[][MAXOSD] = {
	{ "Ostersonntag"  				, "eastern sunday" },
	{ "Ostermontag"  			  	, "eastern monday" },
	{ "Christi Himmelfahrt" 	, "Christi Himmelfahrt" },
	{ "Pfingstsonntag"				, "Pfingsten" },
	{ "Pfingstmontag"					, "Pfingsten" },
	{ "Fronleichnam"					, "Fronleichnam" },
	{ "Aschermittwoch"				, "Aschermittwoch" },
	{ "Muttertag"							, "Muttertag" },
	{ "Sommerzeit"						, "summer time" },
	{ "Winterzeit"						, "winter time" },
	{ "heiliger Abend"				, "christmas" },
	{ "1. Weihnachtsfeiertag"	, "christmas" },
	{ "2. Weihnachtsfeiertag"	, "christmas" },
	{ "heil. 3 K�nige"				, "three kings" },
	{ "Neujahr"								, "new year" },
	{ "Sylvester"							, "sylvester" },
	{ "Valentinstag"					, "valentine" },
	{ "Mai-/Staatsfeiertag"		, "first may" },
	{ "Maria Himmelfahrt"			, "Maria Himmelfahrt" },
	{ "Nikolaus"							, "st. claus" },
	{ "Rosenmontag"						, "Rosenmontag" },
	{ "Gr�ndonnerstag"				, "Gr�ndonnerstag" },
	{ "Karfreitag"						, "Karfreitag" },
	{ "D: Tag der Einheit"		, "D: Tag der Einheit" },
	{ "�: Nationalfeiertag"		, "�: Nationalfeiertag" }
};

char *infotype[][MAXOSD] = {
	{ "*** G e b u r t s t a g  %s (%d)%%0A"  , 	"*** B i r t h d a y  %s (%d)%%0A" },
	{ "*** T e r m i n  %s%%0A"      					,   "*** D a t e  %s%%0A" },
	{ "*** T e r m i n  %s um %02d:%02d%%0A"  ,   "*** D a t e  %s at %02d:%02d%%0A" },
	{ "*** F e i e r t a g   %s%%0A"      		,   "*** H o l i d a y   %s%%0A" },
};

char *infomsg[][MAXOSD] = {
	{ "%d.%m.%Y"    , "%m/%d/%Y" }
};

char *infomsgclock[] = { 
	"%H:%M"		, "%H:%M:%S" 
};

#if HAVE_DVB_API_VERSION == 3
// longer breaking-line for dBox
char *http_br = {"**************************************************%0A"};
#else
// shorter breaking-line for dreambox
char *http_br = {"**********************************%0A"};
#endif

//----------------------------------------------------
// defines for database
#define MAXINFOLEN				80
#define MAXENTRYS					200
#define MAXCHECKS					10
#define MAXCHECKDAYS			5
#define MAXSTIMER					5

char *http_ln = {"%0A"};
char *http_lines[MAXCHECKDAYS+1][MAXOSD] = {
	{"*** Heute am %d. %s %d ***%%0A"					,"*** Today at %s, %d %d ***%%0A"},
	{"*** Morgen am %d. %s %d ***%%0A"				,"*** Tomorrow at %s, %d %d ***%%0A"},
	{"*** In zwei Tagen am %d. %s %d ***%%0A" ,"*** in two days at %s, %d %d ***%%0A"},
	{"*** In drei Tagen am %d. %s %d ***%%0A" ,"*** in three days at %s, %d %d ***%%0A"},
	{"*** In vier Tagen am %d. %s %d ***%%0A" ,"*** in four days at %s, %d %d ***%%0A"},
	{"*** Jetzt um %02d:%02d ***%%0A"				,"*** Now at %02d:%02d ***%%0A"}
};

//----------------------------------------------------
// calendar calculations

// for calculation the day of the week
const int monthcode[12] = {
	6, 	2, 	2, 	5, 	0, 	3, 	5, 	1, 	4, 	6, 	2, 	4
};

// days per month
const int monthdays[2][12] = {
	{ 31, 28, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 },								// Normal years.
	{ 31, 29, 31, 30, 31, 30, 31, 31, 30, 31, 30, 31 }								// Leap years. 
};

// How many days come before each month (0-12).
const int __mon_yday[2][13] =
  {    
    { 0, 31, 59, 90, 120, 151, 181, 212, 243, 273, 304, 334, 365 },  // Normal years.
    { 0, 31, 60, 91, 121, 152, 182, 213, 244, 274, 305, 335, 366 }	 // Leap years.  
  };
  

//----------------------------------------------------
// functions

void ShowMessage(int message);
int IsEvent(int day, int month, int year);
int WeekNumber( int y, int m, int d );
int LeapYear(int year);

//----------------------------------------------------
// freetype stuff

#define FONT FONTDIR "/pakenham.ttf"

// definitions for string-rendering and size
enum {LEFT, CENTER, RIGHT, FIXEDLEFT, FIXEDCENTER, FIXEDRIGHT};
enum {SMALL, NORMAL, BIG};

FT_Library		library;
FTC_Manager		manager;
FTC_SBitCache		cache;
FTC_SBit		sbit;
#if FREETYPE_MAJOR  == 2 && FREETYPE_MINOR == 0
FTC_ImageDesc		desc;
#else
FTC_ImageTypeRec	desc;
#endif
FT_Face			face;
FT_UInt			prev_glyphindex;
FT_Bool			use_kerning;

#if (FREETYPE_MAJOR > 2 || (FREETYPE_MAJOR == 2 && (FREETYPE_MINOR > 1 || (FREETYPE_MINOR == 1 && FREETYPE_PATCH >= 8))))
#  define FT_NEW_CACHE_API
#endif

//----------------------------------------------------
// config
char osd = 'G';														//! OSD language
int skin = 1;															//! which skin to use
int startdelay = 30;											//! startdelay for daemon to check event
char logging = 'Y';												//! logging to file
char audio = 'Y';													//! signal event per audio
int video=1;															//! signal event per video (different types)
int sigtype=1;														//! signal type 
int sigmode=0;														//! signal mode
char sigtime[80];													//! fix signal-times
int webport=80;														//! webport for using webinterface
char webuser[32] = "";										//! for using webinterface
char webpass[32] = "";										//! for using webinterface
char disp_date = 'N';											//! display the date
char disp_clock = 'Y';										//! display the clock
char disp_sec = 'Y';											//! display the second
char disp_size = 'S';											//! display size 'S'mall, 'N'ormal, 'B'ig
int disp_color = 1;												//! display color
int disp_back = 2;												//! display back-color
int disp_detect = 1;											//! detect color-map
char disp_mail = 'Y';											//! display mail notification
int iFontSize;														//! Fontsize, converted from config
int iFont;																//! Fonttype, converted from config 

FILE *fd_pid;															//! PID file
int slog = 0;															//! logging to sys
int pid;																	//! the PID
int sock;																	//! socket
int intervall;														//! check every x seconds
char show_clock = 'Y';										//! show the clock
char show_clockatstart = 'Y';
int nodelay=0;														//! startup-delay
char encodedstring[512], decodedstring[512];	//! for web-authentification

char *szFmtStr[] = {
	"%02u" , "%04u"
};


// waveheader
struct WAVEHEADER
{
	unsigned long		ChunkID1;
	unsigned long		ChunkSize1;
	unsigned long		ChunkType;
	unsigned long		ChunkID2;
	unsigned long		ChunkSize2;
	unsigned short	Format;
	unsigned short	Channels;
	unsigned long		SampleRate;
	unsigned long		BytesPerSecond;
	unsigned short	BlockAlign;
	unsigned short	BitsPerSample;
	unsigned long		ChunkID3;
	unsigned long		ChunkSize3;
};

struct CHUNK
{
	unsigned long	ChunkID;
	unsigned long	ChunkSize;
};

//----------------------------------------------------
//----------------------------------------------------
// defines for setting the output
#define FONTSIZE_SMALL  24
#define FONTSIZE_NORMAL 32
#define FONTSIZE_BIG 		40
#define MAXCLOCKINFOLEN 20

//----------------------------------------------------
//----------------------------------------------------
#define DAEMON_ON_NOSIGNAL		0
#define DAEMON_ON_SIGNAL			1
#define DAEMON_OFF						2

enum {SIGNORMAL, SIGPERIOD, SIGHOLIDAY, SIGALL };						//! used by sigmode
typedef struct tagTIMER
{
	int hour;
	int min;
} STIMER, *PSTIMER;

//----------------------------------------------------
// variables
struct tm *at;															//! actual time
time_t tt;																	//! actual time
int tCheck_year;														//! year to check
int tCheck_mon;															//! month to check 
int tCheck_day;															//! day to check 
int tCheck_hour;														//! hour to check 
int tCheck_min;															//! minute to check 
int oldyear = 0;														//! last date/time info
int oldmonth = 0;
int oldday = 0; 
int oldhour = -1; 
int oldmin = -1;
int iBlack = -1;
int iWhite = -1;
int iEventType[MAXCHECKDAYS+2][MAXCHECKS];	//! structure filled with event-index by IsEvent(), 0=today, 1 is today with time, 2 is next day, 3 is the following day
int iCntEntries;														//! total number of entries in database
int iCntEvents[MAXCHECKDAYS+1];							//! found events
int iCntTmEvents;														//! found events at this minute
STIMER tSignal[MAXSTIMER];									//! signal at certain times
int nEditStyle = 1;													//! style for editing (RC, KB)
char online;																//! are we connected to the daemon
int iFB = 0;																//! is the framebuffer ok?
char timeinfo[22];													//! string for time
char versioninfo_d[12] = "?.??";						//! daemon version

//----------------------------------------------------
// database for all events
enum {FREE, BIRTHDAY, EVENT, PERIOD, HOLIDAY, COMMENT, UNUSED, SPACE};
typedef struct tagEVT_DB
{
	int type;										//! type of event: BIRTHDAY, EVENT, PERIOD, HOLIDAY or FREE (not used)
	int year;										//! year for the event , for birthday the birth-year, 0 for all years
	int month;									//! month of the event
	int day;										//! day of the event
	int hour;										//! hour of the event, -1 for all-day event
	int min;										//! minute of the event, ignore if hour == -1
	int days;										//! days since 1.1.
	int eyear;									//! end-year for the event 
	int emonth;									//! end-month of the event
	int eday;										//! end-day of the event
	int ehour;									//! end-hour of the event, -1 for all-day event
	int emin;										//! end-minute of the event, ignore if hour == -1
	int edays;									//! days since 1.1.
	char info[MAXINFOLEN];			//! info for the event
} EVT_DB, *PEVT_DB;

EVT_DB eventdb[MAXENTRYS];

#define OFFSET_E	1						//! index for eastern
#define OFFSET_EM	2						//! index for eastern
#define OFFSET_H	3						//! index for "christi himmelfahrt"
#define OFFSET_P	4						//! index for "pfingsten"
#define OFFSET_PM	5						//! index for "pfingsten"
#define OFFSET_F	6						//! index for "fronleichnam"
#define OFFSET_A	7						//! aschermittwoch
#define OFFSET_M	8						//! Muttertag
#define OFFSET_SZ 9						//! Sommerzeit
#define OFFSET_WZ 10					//! Winterzeit
#define OFFSET_W0 11					//! heiliger abend
#define OFFSET_W1 12					//! 1. Weihnachtstag
#define OFFSET_W2 13					//! 2. Weihnachtstag
#define OFFSET_3K 14					//! hl. 3 Koenige
#define OFFSET_N 	15					//! Neujahr
#define OFFSET_S 	16					//! Silvester
#define OFFSET_V	17					//! Valentinstag
#define OFFSET_1M	18					//! 1. may
#define OFFSET_MH	19					//! maria himmelfahrt
#define OFFSET_NI	20					//! nikolaus
#define OFFSET_RM	21					//! rosenmontag
#define OFFSET_GD	22					//! gruendonnerstag
#define OFFSET_KF	23					//! karfreitag
#define OFFSET_ND	24					//! tag der deutschen einheit
#define OFFSET_NA	25					//! nationalfeiertag oesterreich
#define NOF_VDAYS	25


// structure for the christian holidays in a year
typedef struct tagVariableDays
{
	int mon; 										// month
	int day;										// year
} VARIABLEDAY, *PVARIABLEDAY;

VARIABLEDAY varaibledays[NOF_VDAYS];

//----------------------------------------------------
// devs
int fb, fbdev;

//----------------------------------------------------
// framebuffer stuff
enum {FILL, GRID};

unsigned char *lfb = 0, *lbb = 0;

struct fb_fix_screeninfo fix_screeninfo;
struct fb_var_screeninfo var_screeninfo;

struct fb_cmap *colormap = NULL;
char bps;

int startx, starty, sx, ex, sy, ey;

//----------------------------------------------------
// object to render
enum {OBJ_CIRCLE, OBJ_HEART, OBJ_MARKER, OBJ_SCROLLUP, OBJ_SCROLLDN, OBJ_CLOCK, OBJ_LETTER};
#define OBJ_SX	15											// lines for object
#define OBJ_SY	15											// columns for object

char sym_letter[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,
	1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,
	1,0,0,1,0,0,0,0,0,0,0,1,0,0,1,
	1,0,0,0,1,0,0,0,0,0,1,0,0,0,1,
	1,0,0,0,0,1,0,0,0,1,0,0,0,0,1,
	1,0,0,0,1,0,1,0,1,0,1,0,0,0,1,
	1,0,0,1,0,0,0,1,0,0,0,1,0,0,1,
	1,0,1,0,0,0,0,0,0,0,0,0,1,0,1,
	1,1,0,0,0,0,0,0,0,0,0,0,0,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};



