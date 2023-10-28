AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libdvbapi \
	@FREETYPE_CFLAGS@

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_gui_bedit.a

libneutrino2_gui_bedit_a_SOURCES = \
	bouqueteditor_bouquets.cpp \
	bouqueteditor_channels.cpp \
	bouqueteditor_chanselect.cpp

