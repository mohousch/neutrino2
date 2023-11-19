AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/system \
	-I$(top_srcdir)/src/gui/widget \
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
	-I$(top_srcdir)/lib/libdvbapi \
	@FREETYPE_CFLAGS@

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

AM_CXXFLAGS = -fno-rtti -fno-exceptions -std=c++98

noinst_LIBRARIES = libneutrino2_system.a

libneutrino2_system_a_SOURCES = \
	safe_system.c \
	localize.cpp \
	setting_helpers.cpp \
	ping.c \
	flashtool.cpp \
	httptool.cpp \
	lastchannel.cpp \
	configure_network.cpp \
	fsmounter.cpp \
	helpers.cpp \
	channellogo.cpp \
	tmdbparser.cpp \
	ytparser.cpp \
	weather.cpp

