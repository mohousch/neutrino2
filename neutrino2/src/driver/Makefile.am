SUBDIRS = audiodec gfx lcd

AM_CPPFLAGS = \
	-I$(top_builddir) \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libeventserver \
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

noinst_LIBRARIES = libneutrino2_driver.a 

libneutrino2_driver_a_SOURCES = \
	encoding.cpp \
	rcinput.cpp \
	vcrcontrol.cpp \
	audioplay.cpp \
	pictureviewer.cpp \
	file.cpp \
	audiometadata.cpp \
	audiofile.cpp \
	shutdown_count.cpp \
	genpsi.cpp \
	streamts.cpp \
	radiotext.cpp \
	radiotools.cpp

if !BOXTYPE_DGS 
if !BOXTYPE_FULAN 
if !BOXTYPE_KATHREIN 
if !BOXTYPE_IPBOX
if !BOXTYPE_HL101 
if !BOXTYPE_TOPFIELD 
if !BOXTYPE_FORTIS_HDBOX 
if !BOXMODEL_OCTAGON1008 
if !BOXTYPE_ATEVIO 
if !BOXTYPE_ADB_BOX 
if !BOXTYPE_WHITEBOX 
if !BOXTYPE_VIP 
if !BOXTYPE_HOMECAST 
if !BOXTYPE_TECHNOMATE
libneutrino2_driver_a_SOURCES += hdmi_cec.cpp
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif
endif

