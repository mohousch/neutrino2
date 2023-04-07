SUBDIRS = \
	connection \
	libeventserver \
	libconfigfile \
	libmd5sum \
	libnet \
	libxmltree \
	libtuxtxt \
	libdvbsub \
	jsoncpp \
	libiw

if !BOXTYPE_COOLSTREAM
SUBDIRS += libdvbapi
endif

if !ENABLE_GSTREAMER
SUBDIRS += libeplayer3
endif

if ENABLE_LCD
SUBDIRS += liblcddisplay
endif

if ENABLE_CI
SUBDIRS += libdvbci
endif





