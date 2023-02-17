AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

AM_CPPFLAGS += -fno-rtti

noinst_LIBRARIES = libsectionsd.a

libsectionsd_a_SOURCES = \
	abstime.c \
	sectionsd.cpp \
	dmxapi.cpp \
	dmx.cpp \
	SIsections.cpp \
	SIevents.cpp \
	SIutils.cpp \
	SIlanguage.cpp \
	edvbstring.cpp \
	freesatv2.cpp

installdir = $(CONFIGDIR)

install_DATA = \
	freesat.t1 \
	freesat.t2
