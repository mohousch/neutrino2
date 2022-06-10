/******************************************************************************
 *                        <<< TuxCal - Calendar Plugin >>>
 *                (c) Robert "robspr1" Spreitzer 2006 (robert.spreitzer@inode.at)
 ******************************************************************************/
// lots of code is from the tuxmail-project

#include <config.h>

#define _FILE_OFFSET_BITS 64

#include <errno.h>
#include <fcntl.h>
#include <signal.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>


#include <sys/ioctl.h>
#include <sys/mman.h>
#include <sys/socket.h>
#include <sys/un.h>
#include <sys/time.h>
#include <time.h>

#include <plugin.h>

#define SCKFILE 	"/tmp/tuxcald.socket"						//! socket-file, connection to daemon
#define RUNFILE 	"/var/etc/.tuxcald"						//! autostart-file for daemon
#define CFGPATH 	PLUGINDIR "/tuxcal"						//! config-path
#define CFGFILE 	CFGPATH "/tuxcal.conf"							//! config-file
#define EVTFILE 	CFGPATH "/tuxcal.list"							//! database-file

// OSD   different languages
int osdidx = 0;																				// actual used language

#define MAXOSD	2
 
const char *days[7][MAXOSD] = {
	{ "Mo", "Mo" },
	{ "Di", "Tu" },
	{ "Mi", "We" },
	{ "Do", "Th" },
	{ "Fr", "Fr" },
	{ "Sa", "Sa" },
	{ "So", "So" }
};

const char *monthmsg[12][MAXOSD] = {
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

const char* infomsg1[][MAXOSD] = {
	{ "Eintrag"               	, "entry" },
	{ "selektierten l�schen?" 	, "delete selected?" },
	{ "neuen hinzuf�gen?"     	, "add new?" },
	{ "�nderungen �bernehmen?"  , "save changes" },
	{ "�nderungen verwerfen?"  	, "lose changes" }
};

const char *infotype[][MAXOSD] = {
	{ "Geburtstag"    , 	"Birthday" },
	{ "Eintr�ge"      ,   "Entries" },
	{ "Zeitraum"      ,   "Period" },
	{ "Feiertag"      , 	"Holiday" }
};

const char *infohelp[][MAXOSD] = {
	{ "l�schen"       , "delete" },
	{ "markieren"     , "select" },
	{ "einf�gen"      , "insert" },
	{ "bearbeiten"    , "edit" },
	{ "[OK]  Eintr�ge anzeigen    [dBox/Men�]  Uhrzeit ein/ausblenden    [0]  heute" ,  "[OK]  show entrys    [dBox/menu]  show/hide clock    [0]  today" }
};

const char *szEditBoxInfo[][MAXOSD] ={
	{ "Typ ausw�hlen"							, "select event type" },
	{ "Jahreszahl ein/ausblenden" , "toogle year" },
	{ "Uhrzeit ein/ausblenden"    , "toogle time" },
	{ "Enddatum ein/ausblenden" 	, "toogle end-date" },
	{ "Tag  Monat  Jahr  Stunde Minute", "day  month  year   hour  minute" },
	{ "Startdatum"								, "start date" },
	{ "Enddatum"								, "end date" }
};

const char *vdaysnames[][MAXOSD] = {
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

// ShowMessage output
enum {NODAEMON, STARTDONE, STARTFAIL, STOPDONE, STOPFAIL, BOOTON, BOOTOFF, DATE, CLOCKFAIL, CLOCKOK, INFO};

const char *infomsg[][MAXOSD] = {
	{ "Daemon ist nicht geladen!" , "Daemon not running!" },
	{ "Abfrage wurde gestartet."  , "Polling started." },
	{ "Start ist fehlgeschlagen!" , "Start failed!" },
	{ "Abfrage wurde gestoppt."   , "Polling stopped." },
	{ "Stop ist fehlgeschlagen!"  , "Stop failed!" },
	{ "Autostart aktiviert."      , "Autostart enabled." },
	{ "Autostart deaktiviert."    , "Autostart disabled." },
	{ "%d.%m.%Y %H:%M:%S"         , "%m/%d/%Y %H:%M:%S" },
	{ "Uhr ist fehlgeschlagen!"   , "Clock failed!" },
	{ "Uhranzeige umgeschaltet."  , "displaying clock changed" }
};



// remote-control and keyboard
//unsigned short rccode;				//! remote-control code

// rc codes
#define RC_0        	RC_0
#define RC_1        	RC_1
#define RC_2        	RC_2
#define RC_3        	RC_3
#define RC_4        	RC_4
#define RC_5        	RC_5
#define RC_6        	RC_6
#define RC_7        	RC_7
#define RC_8        	RC_8
#define RC_9        	RC_9
#define RC_RIGHT    	RC_right
#define RC_LEFT     	RC_left
#define RC_UP       	RC_up
#define RC_DOWN     	RC_down
#define RC_OK       	RC_ok
#define RC_MUTE     	RC_spkr
#define RC_STANDBY  	RC_standby
#define RC_GREEN    	RC_green
#define RC_YELLOW   	RC_yellow
#define RC_RED      	RC_red
#define RC_BLUE     	RC_blue
#define RC_PLUS     	RC_plus
#define RC_MINUS    	RC_minus
#define RC_HELP     	RC_info
#define RC_DBOX     	RC_setup
#define RC_TEXT     	RC_text
#define RC_HOME     	RC_home
#define RC_PAGEUP	RC_page_up
#define RC_PAGEDOWN	RC_page_down

#define REPKEYDELAY	4

// displaying function-keys
#define KEYBOX_KEYS 12

const char *szKeyBoxInfo[KEYBOX_KEYS] = {
	" @!""#$%&'()*+-./[]\\1"   , "ABC2abc" , "DEF3def" ,
	"GHI4ghi" , "JKL5jkl" , "MNO6mno" ,
	"PQRS7pqrs", "TUV8tuv" , "WXYZ9wxyz",
	"0"    ,     "" ,  "" 
} ;

const char *szKeyBoxKey[KEYBOX_KEYS] = {
	"1" , "2" , "3" ,
	"4" , "5" , "6" ,
	"7" , "8" , "9",
	"0" , "+" , "-" 
} ;

const char *szKeyBBoxInfo[KEYBOX_KEYS][MAXOSD] = {
	{ "red"   , "ROT" }  , { "OK"  , "OK" }   , { "entf." , "clr ln" },
	{ "green" , "GR�N" } , { "HOME", "HOME" } , { "leeren", "clr all" } ,
	{ "yellow", "GELB" } , { "Anf.", "pos1" } , { "plus"  , "plus" },
	{ "blue"  , "BLAU" } , { "Ende", "end" }  , { "minus" , "minus"}
} ;

#if HAVE_DVB_API_VERSION == 1
const char *szKeyBBoxKey[KEYBOX_KEYS] = {
	"F1" , "" , "" ,
	"F2" , "" , "" ,
	"M1" , "" , "P+",
	"M2" , "" , "P-" 
} ;
#else
const char *szKeyBBoxKey[KEYBOX_KEYS] = {
	"F1" , "F5" , "F9" ,
	"F2" , "F6" , "F10" ,
	"F3" , "F7" , "Pg+",
	"F4" , "F8" , "Pg-" 
} ;
#endif

const char *szDirectStyle[4] = {
	"ABC", "Abc", "abc", "keyboard" 
};

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
  
// functions
void ShowMessage(int message);
int IsEvent(int day, int month, int year);
int WeekNumber( int y, int m, int d );
int LeapYear(int year);

#define FONT FONTDIR "/neutrino.ttf"

// definitions for string-rendering and size
enum {LEFT, 
	TUXCAL_CENTER, 
	RIGHT, 
	FIXEDLEFT, 
	FIXEDCENTER, 
	FIXEDRIGHT
};

enum {
	SMALL = 20, 
	NORMAL = 20, 
	BIG = 20
};


// config
char osd = 'G';							//! OSD language
int skin = 1;							//! which skin to use
int startdelay = 30;						//! startdelay for daemon to check event
char logging = 'Y';						//! logging to file
char audio = 'Y';						//! signal event per audio
int video=1;							//! signal event per video (different types)
int sigtype=1;							//! signal type 
int sigmode=0;							//! signal mode
char sigtime[80];						//! fix signal-times
int webport=80;							//! webport for using webinterface
char webuser[32] = "";						//! for using webinterface
char webpass[32] = "";						//! for using webinterface
char disp_date = 'N';						//! display the date
char disp_clock = 'Y';						//! display the clock
char disp_sec = 'Y';						//! display the second
char disp_size = 'S';						//! display size 'S'mall, 'N'ormal, 'B'ig
int disp_color = 1;						//! display color
int disp_back = 2;						//! display back-color
int disp_detect = 1;						//! detect color-map
char disp_mail = 'Y';						//! display mail notification
int cstartx = 500;						//! x position for displaying daemon-clock
int cstarty = 30;						//! y position for displaying daemon-clock
char show_clock = 'Y';						//! show the clock

const char *szFmtStr[] = {
	"%02u" , "%04u"
};

// defines for setting the output
#define FONTSIZE_SMALL  	24
#define FONTSIZE_NORMAL 	32
#define FONTSIZE_BIG 		40

#define _MAXSCREEN_X		CFrameBuffer::getInstance()->getScreenWidth() //1200
#define _MAXSCREEN_Y		CFrameBuffer::getInstance()->getScreenHeight() //640

#define MSGBOX_SX		145
#define MSGBOX_SY		175
#define MSGBOX_EX		455
#define MSGBOX_EY		325

#define GRIDLINE		32
#define GRIDLINE_SMALL		24
#define GRIDBOX_X		(_MAXSCREEN_X/7)
#define GRIDBOX_CY1		564 //420
#define GRIDBOX_CY2		464 //300

#define LNWIDTH			2

#define TEXTWIDTH  		(MSGBOX_EX-MSGBOX_SX-4)
#define HEADERSTART  		(MSGBOX_SY+FONTSIZE_BIG+2)
#define HEADERTEXTSTART  	(HEADERSTART-7)
#define TEXTSTART 		((MSGBOX_EY-HEADERSTART)/2-7+HEADERSTART)
#define BUTTONSY  		(MSGBOX_EY-FONTSIZE_SMALL-14)
#define BUTTONX   		50
#define BUTTONSX  		(((MSGBOX_EX-MSGBOX_SX)-3*BUTTONX)/2 + MSGBOX_SX)
#define GRIDCAL			(GRIDLINE+GRIDLINE_SMALL)
#define GRIDLINE_INFO		((GRIDBOX_CY1-GRIDBOX_CY2)/4)

#define KEYBOX_SPACE  		5
#define KEYBOX_HEIGHT 		25
#define KEYBOX_WIDTH  		90

#define EDITFOOTER_Y		(_MAXSCREEN_Y-4*(KEYBOX_HEIGHT+KEYBOX_SPACE)-2*KEYBOX_SPACE)
#define EDITX			20

// defines for database
#define MAXINFOLEN		80
#define MAXENTRYS		500
#define MAXPERDAY		10
#define MAXINFOEDITLEN		64

//
#define DAEMON_ON_NOSIGNAL	0
#define DAEMON_ON_SIGNAL	1
#define DAEMON_OFF		2

// variables
struct tm *at;								//! actual time
time_t tt;								//! actual time
int tShow_year;								//! year to show
int tShow_mon;								//! month to show
int tShow_day;								//! day to show
int iEventType[MAXPERDAY];						//! structure filled with event-index by IsEvent()
int iCntEntries;							//! total number of entries in database
int nEditStyle = 1;							//! style for editing (RC, KB)
int intervall;								//! update clock-info every x seconds
char online;								//! are we connected to the daemon
char versioninfo_p[12];							//! plugin version
char versioninfo_d[12] = "?.??";					//! daemon version

// database for all events
enum {
	FREE, 
	BIRTHDAY, 
	EVENT, 
	PERIOD, 
	HOLIDAY, 
	COMMENT, 
	UNUSED, 
	SPACE
};

typedef struct tagEVT_DB
{
	int type;							//! type of event: BIRTHDAY, EVENT, PERIOD, HOLIDAY or FREE (not used)
	int year;							//! year for the event , for birthday the birth-year, 0 for all years
	int month;							//! month of the event
	int day;							//! day of the event
	int hour;							//! hour of the event, -1 for all-day event
	int min;							//! minute of the event, ignore if hour == -1
	int days;							//! days since 1.1.
	int eyear;							//! end-year for the event 
	int emonth;							//! end-month of the event
	int eday;							//! end-day of the event
	int ehour;							//! end-hour of the event, -1 for all-day event
	int emin;							//! end-minute of the event, ignore if hour == -1
	int edays;							//! days since 1.1.
	char info[MAXINFOLEN];						//! info for the event
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
#define OFFSET_SZ 9							//! Sommerzeit
#define OFFSET_WZ 10							//! Winterzeit
#define OFFSET_W0 11							//! heiliger abend
#define OFFSET_W1 12							//! 1. Weihnachtstag
#define OFFSET_W2 13							//! 2. Weihnachtstag
#define OFFSET_3K 14							//! hl. 3 Koenige
#define OFFSET_N 	15						//! Neujahr
#define OFFSET_S 	16						//! Silvester
#define OFFSET_V	17						//! Valentinstag
#define OFFSET_1M	18						//! 1. may
#define OFFSET_MH	19						//! maria himmelfahrt
#define OFFSET_NI	20						//! nikolaus
#define OFFSET_RM	21						//! rosenmontag
#define OFFSET_GD	22						//! gruendonnerstag
#define OFFSET_KF	23						//! karfreitag
#define OFFSET_ND	24						//! tag der deutschen einheit
#define OFFSET_NA	25						//! nationalfeiertag oesterreich
#define NOF_VDAYS	25


// structure for the christian holidays in a year
typedef struct tagVariableDays
{
	int mon; 										// month
	int day;										// year
} VARIABLEDAY, *PVARIABLEDAY;

VARIABLEDAY varaibledays[NOF_VDAYS];

// daemon commands
enum {
	GET_STATUS, 
	SET_STATUS, 
	GET_VERSION, 
	RELOAD_DB, 
	TOGGLE_CLOCK
};

// framebuffer stuff
enum {
	FILL, 
	GRID
};

enum {
	TRANSP = COL_BACKGROUND, 
	WHITE = COL_WHITE, 
	SKIN0, 
	SKIN1, 
	SKIN2 = COL_NAVY, 
	ORANGE = COL_ORANGE, 
	GREEN = COL_GREEN, 
	YELLOW = COL_YELLOW, 
	RED = COL_RED, 
	BLUE = COL_BLUE, 
	GREY = COL_MATTERHORN, 
	DAY1,
	DAY2, 
	DAY3, 
	DAY4, 
	DAY5, 
	SKIN3 = COL_BLUE, 
	BLACK = COL_BLACK, 
	LGREY = COL_NOBEL, 
	MAGENTA = COL_MAGENTA
};

unsigned short rccode;
int startx, starty, sx, ex, sy, ey;
int MAXSCREEN_X, MAXSCREEN_Y;

// object to render
enum {
	OBJ_CIRCLE, 
	OBJ_HEART, 
	OBJ_MARKER, 
	OBJ_SCROLLUP, 
	OBJ_SCROLLDN, 
	OBJ_CLOCK
};

#define OBJ_SX	15					// lines for object
#define OBJ_SY	15					// columns for object

char scroll_up[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1
};

char scroll_dn[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,1,0,0,0,0,0,0
};

char circle[] =
{
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,1,1,1,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0
};

char heart[] =
{
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,1,1,1,0,0,0,1,1,1,0,0,0,
	0,0,1,1,1,1,1,0,1,1,1,1,1,0,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0
};

char marker[] =
{
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,1,1,1,1,1,1,1,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,0,0,1,0,0,0,0,0,0,0,
	0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0,
	0,0,0,0,0,1,1,1,1,1,0,0,0,0,0,
	0,0,0,0,0,0,1,1,1,0,0,0,0,0,0
};

char symbolclock[] =
{
	0,1,1,0,0,0,1,1,1,0,0,0,1,1,0,
	0,1,1,1,0,1,1,1,1,1,0,1,1,1,0,
	0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,
	0,0,1,1,1,1,1,0,1,1,1,1,1,0,0,
	0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,
	0,1,1,1,1,1,1,0,1,1,1,1,1,1,0,
	1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
	1,1,1,1,1,1,0,0,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,1,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,0,1,1,1,1,1,1,
	1,1,1,1,1,1,1,0,1,0,1,1,1,1,1,
	0,1,1,1,1,1,1,1,1,1,0,1,1,1,0,
	0,1,1,1,1,1,1,1,1,1,1,0,1,1,0,
	0,0,1,1,1,1,1,1,1,1,1,1,1,0,0,
	0,0,0,1,1,1,1,1,1,1,1,1,0,0,0
};

