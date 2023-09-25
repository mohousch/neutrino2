AM_CPPFLAGS = -fno-strict-aliasing

SUBDIRS = widget bedit

AM_CPPFLAGS += \
	-I$(top_builddir) \
	-I$(top_builddir)/src \
	-I$(top_srcdir) \
	-I$(srcdir) \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	-I$(top_srcdir)/lib/libeventserver \
	-I$(top_srcdir)/lib/libnet \
	-I$(top_srcdir)/lib/libconfigfile \
	-I$(top_srcdir)/lib/libxmltree \
	-I$(top_srcdir)/lib/libupnpclient \
	-I$(top_srcdir)/lib/connection \
	@CURL_CFLAGS@ \
	@FREETYPE_CFLAGS@ 

if BOXTYPE_COOLSTREAM
AM_CPPFLAGS += -I$(top_srcdir)/src/zapit/include \
	-I$(top_srcdir)/lib/libcoolstream
else
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbapi
if ENABLE_CI
AM_CPPFLAGS += -I$(top_srcdir)/lib/libdvbci
endif
endif

if !ENABLE_GSTREAMER
AM_CPPFLAGS += -I$(top_srcdir)/lib/libeplayer3/include
endif

if ENABLE_LUA
AM_CPPFLAGS += @LUA_INCLUDE@
endif

AM_CPPFLAGS += -fno-rtti -fno-exceptions

noinst_LIBRARIES = libneutrino2_gui.a

libneutrino2_gui_a_SOURCES = \
	bouquetlist.cpp \
	channellist.cpp \
	infoviewer.cpp \
	eventlist.cpp \
	epgview.cpp \
	epgplus.cpp \
	pluginlist.cpp \
	sleeptimer.cpp \
	screensetup.cpp \
	scan.cpp \
	update.cpp \
	filebrowser.cpp \
	audioplayer.cpp \
	nfs.cpp \
	pictureviewer.cpp \
	movieplayer.cpp \
	rc_lock.cpp \
	streaminfo.cpp \
	dboxinfo.cpp \
	plugins.cpp \
	imageinfo.cpp \
	audio_select.cpp \
	movieinfo.cpp \
	scan_setup.cpp \
	zapit_setup.cpp \
	proxyserver_setup.cpp \
	service_menu.cpp \
	video_setup.cpp \
	audio_setup.cpp \
	parentallock_setup.cpp \
	network_setup.cpp \
	movieplayer_setup.cpp \
	osd_setup.cpp \
	audioplayer_setup.cpp \
	pictureviewer_setup.cpp \
	lcd_setup.cpp \
	rc_setup.cpp \
	recording_setup.cpp \
	misc_setup.cpp \
	main_setup.cpp \
	power_menu.cpp \
	mediaplayer.cpp \
	audio_video_select.cpp \
	channel_select.cpp \
	hdd_menu.cpp \
	dvbsub_select.cpp \
	timerlist.cpp \
	epg_menu.cpp \
	motorcontrol.cpp \
	alphasetup.cpp \
	psisetup.cpp \
	vfdcontroler.cpp

if ENABLE_CI
libneutrino2_gui_a_SOURCES += cam_menu.cpp
endif

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
libneutrino2_gui_a_SOURCES += cec_setup.cpp
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

