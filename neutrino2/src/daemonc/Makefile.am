AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/xmltree \
	-I$(top_srcdir)/lib/connection \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_daemonc.a

libneutrino2_daemonc_a_SOURCES = remotecontrol.cpp

