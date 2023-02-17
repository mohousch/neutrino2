AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
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
if ENABLE_CI
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbci
endif
endif

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libzapit.a

libzapit_a_SOURCES = \
	bouquets.cpp channel.cpp dvbstring.cpp getservices.cpp \
	scan.cpp zapit.cpp cam.cpp ci.cpp frontend.cpp \
	descriptors.cpp nit.cpp pat.cpp pmt.cpp sdt.cpp

