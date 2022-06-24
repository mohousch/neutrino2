AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/src/system \
	-I$(top_srcdir)/src/zapit/include/zapit \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libeventserver \
	@FREETYPE_CFLAGS@

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libdvbci.a

libdvbci_a_SOURCES = dvb-ci.cpp \
		dvbci_session.cpp \
		dvbci_mmi.cpp \
		dvbci_camgr.cpp \
		dvbci_appmgr.cpp \
		dvbci_datetimemgr.cpp \
		dvbci_resmgr.cpp





