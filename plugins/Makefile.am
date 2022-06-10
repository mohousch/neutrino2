AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	test \
	systeminfo \
	tuxcom \
	fileplayer \
	fileexplorer \
	mediaportal \
	icecast \
	internetradio \
	tsbrowser \
	moviebrowser \
	picviewer \
	audioplayer \
	nfilm \
	ntvshows

#testing
if ENABLE_TESTING
SUBDIRS += satipclient \
	opkg_manager \
	tuxcal \
	netzkino \
	youtube \
	upnp \
	netzkino_hd \
	mtv \
	arte_concert
endif

if ENABLE_PYTHON
SUBDIRS += python
endif

if ENABLE_LUA
SUBDIRS += lua \
	media_one \
	plutotv \
	netzkino_hd
endif




