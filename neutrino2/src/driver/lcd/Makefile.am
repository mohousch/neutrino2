AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libdvbapi \
	-I$(top_srcdir)/lib/libeplayer3/include \
	@FREETYPE_CFLAGS@ \
	@FRIBIDI_CFLAGS@

AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_driver_lcd.a

libneutrino2_driver_lcd_a_SOURCES = lcddisplay.cpp fontrenderer.cpp

