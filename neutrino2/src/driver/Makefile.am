SUBDIRS = gdi lcd

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libdvbapi \
	@FREETYPE_CFLAGS@ \
	@FRIBIDI_CFLAGS@

if ENABLE_GSTREAMER
AM_CPPFLAGS += @GSTREAMER_CFLAGS@
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_driver.a 

libneutrino2_driver_a_SOURCES = \
	encoding.cpp \
	rcinput.cpp \
	genpsi.cpp \
	record.cpp \
	pictureviewer.cpp \
	file.cpp \
	shutdown_count.cpp \
	streamts.cpp \
	radiotext.cpp \
	audiofile.cpp \
	audioplay.cpp \
	movieinfo.cpp \
	lcdd.cpp

if ENABLE_CEC
libneutrino2_driver_a_SOURCES += hdmi_cec.cpp
endif

