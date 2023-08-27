AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	test \
	systeminfo \
	tuxcom \
	fileexplorer \
	mediaportal \
	icecast \
	internetradio \
	movieplayer \
	moviebrowser \
	picviewer \
	audioplayer \
	nfilm \
	ntvshows \
	satipclient \
	opkg_manager \
	tuxcal \
	upnp

#testing
if ENABLE_TESTING
SUBDIRS += netzkino \
	youtube
endif

if ENABLE_PYTHON
SUBDIRS += python
endif

if ENABLE_LUA
SUBDIRS += lua \
	media_one \
	plutotv \
	netzkino_hd
if ENABLE_TESTING	
	 \ mtv \
	arte_concert
endif	
endif

