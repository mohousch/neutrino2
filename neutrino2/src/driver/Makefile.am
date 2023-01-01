SUBDIRS = pictureviewer audiodec

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree \
	@FREETYPE_CFLAGS@ \
	@FRIBIDI_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

if ENABLE_LCD
AM_CPPFLAGS += -I$(top_srcdir)/lib/liblcddisplay
endif

if ENABLE_GSTREAMER
AM_CPPFLAGS += @GSTREAMER_CFLAGS@
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_driver.a 

libneutrino2_driver_a_SOURCES = \
	encoding.cpp \
	fontrenderer.cpp \
	framebuffer.cpp \
	color.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	audioplay.cpp \
	stream2file.cpp \
	file.cpp \
	audiometadata.cpp \
	audiofile.cpp \
	shutdown_count.cpp \
	genpsi.c \
	streamts.cpp \
	radiotext.cpp \
	radiotools.cpp \
	png.cpp \
	jpeg.cpp \
	bmp.cpp \
	gif.cpp \
	crw.cpp \
	svg.cpp

if ENABLE_LCD
libneutrino2_driver_a_SOURCES += lcdd.cpp	
else
libneutrino2_driver_a_SOURCES += vfd.cpp
endif

if USE_OPENGL
libneutrino2_driver_a_SOURCES += \
	glthread.cpp
endif
