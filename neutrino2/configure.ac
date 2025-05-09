AC_INIT([neutrino2],[5.2],[https://github.com/mohousch/neutrino2])
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
m4_ifdef([LT_INIT], [LT_INIT], [AC_PROG_LIBTOOL])

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
PKG_CHECK_MODULES(FRIBIDI,fribidi)
PKG_CHECK_MODULES(PNG,libpng)
PKG_CHECK_MODULES(AVCODEC,libavcodec)
PKG_CHECK_MODULES(AVUTIL,libavutil)
PKG_CHECK_MODULES(AVFORMAT,libavformat)
PKG_CHECK_MODULES([SWRESAMPLE], [libswresample])
PKG_CHECK_MODULES([SWSCALE], [libswscale])

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
fi

# gstreamer / opengl overlay
AC_ARG_ENABLE(overlay,
	AS_HELP_STRING(--enable-overlay, include gstreamer overlay window into opengl window),
	,[enable_overlay=no])
	
if test "$enable_overlay" = "yes"; then
AC_DEFINE(USE_OVERLAY,1,[include overlay])
PKG_CHECK_MODULES(GSTREAMER, gstreamer-$GST_MAJORMINOR gstreamer-pbutils-$GST_MAJORMINOR gstreamer-video-$GST_MAJORMINOR)
fi

# vfd
AC_ARG_ENABLE(vfd,
	AS_HELP_STRING(--enable-vfd,include vfd support),
	,[enable_vfd=no])
AM_CONDITIONAL(ENABLE_VFD,test "$enable_vfd" = "yes")

if test "$enable_vfd" = "yes"; then
AC_DEFINE(ENABLE_VFD,1,[include vfd support])
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

# graphlcd
AC_ARG_ENABLE(graphlcd,
	AS_HELP_STRING(--enable-graphlcd,include graphlcd support),
	,[enable_graphlcd=no])
AM_CONDITIONAL(ENABLE_GRAPHLCD,test "$enable_graphlcd" = "yes")

if test "$enable_graphlcd" = "yes"; then
AC_DEFINE(ENABLE_GRAPHLCD,1,[include graphlcd support])
fi

# lirc
AC_ARG_ENABLE(lirc,
	AS_HELP_STRING(--enable-lirc,include lirc support),
	,[enable_lirc=no])
AM_CONDITIONAL(ENABLE_LIRC,test "$enable_lirc" = "yes")

if test "$enable_lirc" = "yes"; then
AC_DEFINE(ENABLE_LIRC,1,[include lirc support])
#AC_DEFINE(HAVE_KERNEL_LIRC,1,[include lirc kernel api support])
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

# testing
AC_ARG_ENABLE(testing,
	AS_HELP_STRING(--enable-testing,include testing stuff support),
	,[enable_testing=no])
AM_CONDITIONAL(ENABLE_TESTING,test "$enable_testing" = "yes")

if test "$enable_testing" = "yes"; then
AC_DEFINE(ENABLE_TESTING,1,[include testing stuff support])
fi

AC_MSG_RESULT([
Boxtype          	: $BOXTYPE
Box Model		: $BOXMODEL
OpenGL			: $enable_opengl
Gstreamer		: $enable_gstreamer
SCART			: $enable_scart
VFD			: $enable_vfd
LCD			: $enable_lcd
TFTLCD			: $enable_tftlcd
4 Digits		: $enable_4digits
NGLCD			: $enable_graphlcd
CI CAM			: $enable_ci
F-Keys			: $enable_functionkeys
Python			: $enable_python
Lua			: $enable_lua
Testing      		: $enable_testing
LIRC			: $enable_lirc
])

AC_OUTPUT([
Makefile
lib/Makefile
lib/libconfigfile/Makefile
lib/connection/Makefile
lib/libmd5sum/Makefile
lib/libnet/Makefile
lib/libxmltree/Makefile
lib/libtuxtxt/Makefile
lib/libsubtitle/Makefile
lib/libdvbapi/Makefile
lib/libeplayer3/Makefile
lib/jsoncpp/Makefile
lib/libdvbci/Makefile
lib/libiw/Makefile
lib/libngpng/Makefile
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
src/driver/gdi/Makefile
src/driver/lcd/Makefile
src/driver/Makefile
src/gui/Makefile
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
data/icons/lcd/Makefile
data/iso-codes/Makefile
data/config/Makefile
data/zapit/Makefile
data/webtv/Makefile
data/scripts/Makefile
data/skins/Makefile
data/skins/standard/Makefile
data/skins/KravenHD/Makefile
data/skins/KravenHD/background/Makefile
data/skins/MetrixHD/Makefile
data/skins/MetrixHD/buttons/Makefile
data/skins/MetrixHD/hints/Makefile
data/skins/titan/Makefile
data/skins/neutrino2/Makefile
data/skins/elgato/Makefile
data/skins/elgato/images/Makefile
data/skins/ice/Makefile
data/skins/ice/buttons/Makefile
data/skins/ice/icons/Makefile
data/skins/ice/hints/Makefile
data/skins/ice/fonts/Makefile
data/locale/Makefile
src/sectionsd/Makefile
src/timerd/Makefile
src/zapit/Makefile
src/interfaces/Makefile
src/interfaces/python/Makefile
src/interfaces/lua/Makefile
])

