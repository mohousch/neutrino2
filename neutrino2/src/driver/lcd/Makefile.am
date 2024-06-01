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

if ENABLE_GSTREAMER
AM_CPPFLAGS += @GSTREAMER_CFLAGS@
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_driver_lcd.a

libneutrino2_driver_lcd_a_SOURCES = lcdd.cpp

if ENABLE_LCD
libneutrino2_driver_lcd_a_SOURCES += lcddisplay.pp fontrenderer.cpp
endif

if ENABLE_TFTLCD
libneutrino2_driver_lcd_a_SOURCES += tftlcd.cpp
endif	

if ENABLE_GRAPHLCD
libneutrino2_driver_lcd_a_SOURCES += nglcd.cpp
endif

