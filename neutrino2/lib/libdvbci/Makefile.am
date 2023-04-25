AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib/connection \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libeventserver \
	@FREETYPE_CFLAGS@

noinst_LIBRARIES = libdvbci.a

libdvbci_a_SOURCES = \
	ca_ci.cpp \
	dvbci_session.cpp \
	dvbci_mmi.cpp \
	dvbci_camgr.cpp \
	misc.cpp \
	descrambler.cpp \
	dh_rsa_misc.cpp \
	aes_xcbc_mac.cpp \
	dvbci_ccmgr.cpp \
	dvbci_appmgr.cpp \
	dvbci_datetimemgr.cpp \
	dvbci_resmgr.cpp

