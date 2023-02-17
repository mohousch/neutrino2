AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	@AVCODEC_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

AM_CPPFLAGS += -D__STDC_CONSTANT_MACROS

noinst_LIBRARIES = libdvbsub.a

libdvbsub_a_SOURCES = dvbsub.cpp dvbsubtitle.cpp \
	tools.cpp PacketQueue.cpp helpers.cpp Debug.cpp
