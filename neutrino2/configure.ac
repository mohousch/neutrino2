AC_INIT([neutrino2],[5.0],[https://github.com/mohousch/neutrino2])
AM_INIT_AUTOMAKE([nostdinc])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY
TUXBOX_APPS_DVB
TUXBOX_APPS_DRIVER
TUXBOX_BOXTYPE

AX_PTHREAD

AC_PROG_CC
AC_PROG_CXX

AC_DISABLE_STATIC
AM_PROG_LIBTOOL

AM_PROG_CC_C_O

#
AC_PATH_PROG(MSGFMT, msgfmt, AC_MSG_ERROR(Could not find msgfmt))
AC_PATH_PROG(MSGINIT, msginit, AC_MSG_ERROR(Could not find msginit))
AC_PATH_PROG(MSGMERGE, msgmerge, AC_MSG_ERROR(Could not find msgmerge))
AC_PATH_PROG(MSGUNIQ, msguniq, AC_MSG_ERROR(Could not find msguniq))
AC_PATH_PROG(MSGATTRIB, msgattrib, AC_MSG_ERROR(Could not find msgattrib))
AC_PATH_PROG(XGETTEXT, xgettext, AC_MSG_ERROR(Could not find xgettext))

#
PKG_CHECK_MODULES(CURL, libcurl)
PKG_CHECK_MODULES(FREETYPE,freetype2)
PKG_CHECK_MODULES(ID3TAG, id3tag)
PKG_CHECK_MODULES(MAD, mad)
PKG_CHECK_MODULES(PNG,libpng)
PKG_CHECK_MODULES(AVCODEC,libavcodec)
PKG_CHECK_MODULES(AVUTIL,libavutil)
PKG_CHECK_MODULES(AVFORMAT,libavformat)
PKG_CHECK_MODULES(FRIBIDI,fribidi)

# keybaord
AC_ARG_ENABLE(keyboard-no-rc,
         [  --enable-keyboard-no-rc enable keyboard control, disable rc control],
         [AC_DEFINE(KEYBOARD_INSTEAD_OF_REMOTE_CONTROL,1,[enable keyboard control, disable rc control])])

# opengl framebuffer (x86)
AC_ARG_ENABLE(opengl,
	AS_HELP_STRING(--enable-opengl,include opengl framebuffer support for x86),
	,[enable_opengl=no])
AM_CONDITIONAL(USE_OPENGL,test "$enable_opengl" = "yes")

if test "$enable_opengl" = "yes"; then
AC_DEFINE(USE_OPENGL,1,[include opengl engine])
PKG_CHECK_MODULES(GL, gl)
#PKG_CHECK_MODULES(GLUT, glut)
PKG_CHECK_MODULES(GLEW, glew)
fi

# gstreamer / opengl overlay
AC_ARG_ENABLE(overlay,
	AS_HELP_STRING(--enable-overlay, include gstreamer overlay window into opengl window),
	,[enable_overlay=no])
	
if test "$enable_overlay" = "yes"; then
AC_DEFINE(USE_OVERLAY,1,[include overlay])
fi

# gstreamer
AC_ARG_ENABLE(gstreamer,
	AS_HELP_STRING(--enable-gstreamer,include gstreamer as player engine support),
	,[enable_gstreamer=no])
AM_CONDITIONAL(ENABLE_GSTREAMER,test "$enable_gstreamer" = "yes")

if test "$enable_gstreamer" = "yes"; then
AC_DEFINE(ENABLE_GSTREAMER,1,[include gstreamer as player engine support])

AC_ARG_WITH(gstversion,
	AS_HELP_STRING([--with-gstversion],[use gstreamer version (major.minor)]),
	[GST_MAJORMINOR=$withval],[GST_MAJORMINOR=1.0])

PKG_CHECK_MODULES(GSTREAMER, gstreamer-$GST_MAJORMINOR gstreamer-pbutils-$GST_MAJORMINOR)
# to overlay video window
#PKG_CHECK_MODULES(GSTREAMER, gstreamer-$GST_MAJORMINOR gstreamer-pbutils-$GST_MAJORMINOR gstreamer-video-$GST_MAJORMINOR)
fi

# lcd
AC_ARG_ENABLE(lcd,
	AS_HELP_STRING(--enable-lcd,include lcd support),
	,[enable_lcd=no])
AM_CONDITIONAL(ENABLE_LCD,test "$enable_lcd" = "yes")

if test "$enable_lcd" = "yes"; then
AC_DEFINE(ENABLE_LCD,1,[include lcd support])
fi

# tftlcd
AC_ARG_ENABLE(tftlcd,
	AS_HELP_STRING(--enable-tftlcd,include tftlcd support),
	,[enable_tftlcd=no])
AM_CONDITIONAL(ENABLE_TFTLCD,test "$enable_tftlcd" = "yes")

if test "$enable_tftlcd" = "yes"; then
AC_DEFINE(ENABLE_TFTLCD,1,[include tftlcd support])
fi

# 4 digits
AC_ARG_ENABLE(4digits,
	AS_HELP_STRING(--enable-4digits,include 5 segment lcd support),
	,[enable_4digits=no])
AM_CONDITIONAL(ENABLE_4DIGITS,test "$enable_4digits" = "yes")

if test "$enable_4digits" = "yes"; then
AC_DEFINE(ENABLE_4DIGITS,1,[include 5 segment lcd support])
fi

# scart output
AC_ARG_ENABLE(scart,
	AS_HELP_STRING(--enable-scart,enable scart output),
	,[enable_scart=no])
AM_CONDITIONAL(ENABLE_SCART,test "$enable_scart" = "yes")

if test "$enable_scart" = "yes"; then
AC_DEFINE(ENABLE_SCART,1,[include scart output support])
fi

# ci-cam
AC_ARG_ENABLE(ci,
	AS_HELP_STRING(--enable-ci,enable ci cam),
	,[enable_ci=no])
AM_CONDITIONAL(ENABLE_CI,test "$enable_ci" = "yes")

if test "$enable_ci" = "yes"; then
AC_DEFINE(ENABLE_CI,1,[include ci cam support])
fi

# function keys
AC_ARG_ENABLE(functionkeys,
	AS_HELP_STRING(--enable-functionkeys,include RC functions keys support),
	,[enable_functionkeys=no])
AM_CONDITIONAL(ENABLE_FUNCTIONKEYS,test "$enable_functionkeys" = "yes")

if test "$enable_functionkeys" = "yes"; then
AC_DEFINE(ENABLE_FUNCTIONKEYS,1,[include RC function keys support])
fi

# python
AC_ARG_ENABLE(python,
	AS_HELP_STRING(--enable-python,include python interface),
	,[enable_python=no])
AM_CONDITIONAL(ENABLE_PYTHON,test "$enable_python" = "yes")

if test "$enable_python" = "yes"; then
AM_PATH_PYTHON
AX_PKG_SWIG
AX_SWIG_ENABLE_CXX
AX_SWIG_PYTHON
AX_PYTHON_DEVEL

AC_DEFINE(ENABLE_PYTHON,1,[include python as interface support])
fi

# lua
AC_ARG_ENABLE(lua,
	AS_HELP_STRING(--enable-lua,include lua interface),
	,[enable_lua=no])
AM_CONDITIONAL(ENABLE_LUA,test "$enable_lua" = "yes")

if test "$enable_lua" = "yes"; then
AX_PKG_SWIG
AX_SWIG_ENABLE_CXX
AX_PROG_LUA
AX_LUA_HEADERS
AX_LUA_LIBS

AC_DEFINE(ENABLE_LUA,1,[include lua interface support])
fi

# fake tuner
AC_ARG_ENABLE(fake_tuner,
	AS_HELP_STRING(--enable-fake_tuner,include fake tuner support for testing),
	,[enable_fake_tuner=no])
AM_CONDITIONAL(ENABLE_FAKE_TUNER,test "$enable_fake_tuner" = "yes")

if test "$enable_fake_tuner" = "yes"; then
AC_DEFINE(ENABLE_FAKE_TUNER,1,[include fake tuner support for testing])
fi

# testing
AC_ARG_ENABLE(testing,
	AS_HELP_STRING(--enable-testing,include testing plugins support),
	,[enable_testing=no])
AM_CONDITIONAL(ENABLE_TESTING,test "$enable_testing" = "yes")

AC_MSG_RESULT([
Boxtype          	: $BOXTYPE
Box Model		: $BOXMODEL
OpenGL			: $enable_opengl
Gstreamer		: $enable_gstreamer
SCART			: $enable_scart
LCD			: $enable_lcd
TFTLCD			: $enable_tftlcd
4 Digits		: $enable_4digits
CI CAM			: $enable_ci
F-Keys			: $enable_functionkeys
Python			: $enable_python
Lua			: $enable_lua
Fake Tuner		: $enable_fake_tuner
Testing      		: $enable_testing
])

AC_OUTPUT([
Makefile
lib/Makefile
lib/libconfigfile/Makefile
lib/connection/Makefile
lib/libeventserver/Makefile
lib/libmd5sum/Makefile
lib/libnet/Makefile
lib/libxmltree/Makefile
lib/libtuxtxt/Makefile
lib/libdvbsub/Makefile
lib/liblcddisplay/Makefile
lib/libdvbapi/Makefile
lib/libeplayer3/Makefile
lib/jsoncpp/Makefile
lib/libdvbci/Makefile
lib/libiw/Makefile
src/nhttpd/Makefile
src/nhttpd/web/Makefile
src/nhttpd/web/images/Makefile
src/nhttpd/web/scripts/Makefile
src/nhttpd/web/languages/Makefile
src/nhttpd/tuxboxapi/Makefile
src/nhttpd/yhttpd_core/Makefile
src/nhttpd/yhttpd_mods/Makefile
src/Makefile
src/daemonc/Makefile
src/driver/pictureviewer/Makefile
src/driver/audiodec/Makefile
src/driver/Makefile
src/gui/Makefile
src/gui/bedit/Makefile
src/gui/widget/Makefile
src/system/Makefile
data/Makefile
data/fonts/Makefile
data/icons/Makefile
data/icons/buttons/Makefile
data/icons/filetypes/Makefile
data/icons/headers/Makefile
data/icons/hints/Makefile
data/icons/spinner/Makefile
data/icons/locale/Makefile
data/icons/movieplayer/Makefile
data/icons/radar/Makefile
data/icons/slider/Makefile
data/icons/status/Makefile
data/icons/various/Makefile
data/icons/weather/Makefile
data/iso-codes/Makefile
data/config/Makefile
data/lcd/Makefile
data/lcd/icons/Makefile
data/lcd/clock/Makefile
data/zapit/Makefile
data/webtv/Makefile
data/scripts/Makefile
data/skins/Makefile
data/skins/neutrino/Makefile
data/skins/neutrino/buttons/Makefile
data/skins/neutrino/hints/Makefile
data/skins/KravenHD/Makefile
data/skins/KravenHD/background/Makefile
data/skins/MetrixHD/Makefile
data/skins/MetrixHD/buttons/Makefile
data/skins/MetrixHD/hints/Makefile
data/skins/titan/Makefile
data/skins/neutrino2/Makefile
data/locale/Makefile
src/sectionsd/Makefile
src/timerd/Makefile
src/zapit/Makefile
src/interfaces/Makefile
src/interfaces/python/Makefile
src/interfaces/lua/Makefile
])

