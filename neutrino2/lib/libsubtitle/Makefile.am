AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libdvbapi \
	@AVCODEC_CFLAGS@

AM_CPPFLAGS += -D__STDC_CONSTANT_MACROS

noinst_LIBRARIES = libsubtitle.a

libsubtitle_a_SOURCES = subtitle.cpp dvbsubtitle.cpp tools.cpp
