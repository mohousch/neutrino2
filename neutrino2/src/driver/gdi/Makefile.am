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
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libdvbapi \
	@FREETYPE_CFLAGS@ \
	@FRIBIDI_CFLAGS@

if ENABLE_LCD
AM_CPPFLAGS += -I$(top_srcdir)/lib/liblcddisplay
endif

if ENABLE_GSTREAMER
AM_CPPFLAGS += @GSTREAMER_CFLAGS@
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_driver_gdi.a 

libneutrino2_driver_gdi_a_SOURCES = \
	fontrenderer.cpp \
	framebuffer.cpp \
	color.cpp

if USE_OPENGL
libneutrino2_driver_gdi_a_SOURCES += glthread.cpp
endif

