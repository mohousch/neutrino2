AC_INIT(neutrinohd2-plugins,4.2)
AM_INIT_AUTOMAKE([nostdinc])

m4_ifdef([AM_SILENT_RULES], [AM_SILENT_RULES([yes])])

TUXBOX_APPS
TUXBOX_APPS_DIRECTORY
TUXBOX_APPS_DVB
TUXBOX_BOXTYPE

AC_PROG_CC
AC_PROG_CXX
AC_DISABLE_STATIC

AM_PROG_LIBTOOL

#
AC_PATH_PROG(MSGFMT, msgfmt, AC_MSG_ERROR(Could not find msgfmt))
AC_PATH_PROG(MSGINIT, msginit, AC_MSG_ERROR(Could not find msginit))
AC_PATH_PROG(MSGMERGE, msgmerge, AC_MSG_ERROR(Could not find msgmerge))
AC_PATH_PROG(MSGUNIQ, msguniq, AC_MSG_ERROR(Could not find msguniq))
AC_PATH_PROG(MSGATTRIB, msgattrib, AC_MSG_ERROR(Could not find msgattrib))
AC_PATH_PROG(XGETTEXT, xgettext, AC_MSG_ERROR(Could not find xgettext))

PKG_CHECK_MODULES(CURL,libcurl)
PKG_CHECK_MODULES(FREETYPE,freetype2)
PKG_CHECK_MODULES(PNG,libpng)

# testing
AC_ARG_ENABLE(testing,
	AS_HELP_STRING(--enable-testing,include testing plugins support),
	,[enable_testing=no])
AM_CONDITIONAL(ENABLE_TESTING,test "$enable_testing" = "yes")

# python
AC_ARG_ENABLE(python,
	AS_HELP_STRING(--enable-python,include python interface),
	,[enable_python=no])
AM_CONDITIONAL(ENABLE_PYTHON,test "$enable_python" = "yes")

# lua
AC_ARG_ENABLE(lua,
	AS_HELP_STRING(--enable-lua,include lua interface),
	,[enable_lua=no])
AM_CONDITIONAL(ENABLE_LUA,test "$enable_lua" = "yes")

AC_OUTPUT([
Makefile
test/Makefile
netzkino/Makefile
youtube/Makefile
systeminfo/Makefile
tuxcom/Makefile
fileplayer/Makefile
tuxcal/Makefile
tuxcal/daemon/Makefile
fileexplorer/Makefile
mediaportal/Makefile
satipclient/Makefile
icecast/Makefile
internetradio/Makefile
tsbrowser/Makefile
moviebrowser/Makefile
picviewer/Makefile
audioplayer/Makefile
opkg_manager/Makefile
nfilm/Makefile
ntvshows/Makefile
upnp/Makefile
python/Makefile
lua/Makefile
netzkino_hd/Makefile
mtv/Makefile
media_one/Makefile
arte_concert/Makefile
plutotv/Makefile
])

