AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/gui/widget \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/configfile \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libmd5sum \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libupnpclient \
	-I$(top_srcdir)/lib/jsoncpp/include \
	@FREETYPE_CFLAGS@

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
endif

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_system.a

libneutrino2_system_a_SOURCES = \
	safe_system.c \
	localize.cpp setting_helpers.cpp debug.cpp \
	ping.c flashtool.cpp httptool.cpp \
	settings.cpp lastchannel.cpp \
	configure_network.cpp fsmounter.cpp \
	helpers.cpp \
	tmdbparser.cpp \
	ytparser.cpp \
	weather.cpp



