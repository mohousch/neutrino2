AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libdvbapi \
	-I$(top_srcdir)/lib/libdvbci \
	@FREETYPE_CFLAGS@

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

noinst_LIBRARIES = libzapit.a

libzapit_a_SOURCES = \
	bouquets.cpp \
	channel.cpp \
	dvbstring.cpp \
	cam.cpp \
	frontend.cpp \
	descriptors.cpp \
	nit.cpp \
	pat.cpp \
	pmt.cpp \
	sdt.cpp \
	zapit.cpp

