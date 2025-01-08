AUTOMAKE_OPTIONS = gnu

SUBDIRS = \
	test \
	systeminfo \
	fileexplorer \
	mediaportal \
	icecast \
	internetradio \
	movieplayer \
	picviewer \
	audioplayer \
	nfilm \
	ntvshows \
	upnp \
	tuxcom \
	moviebrowser

#testing
if ENABLE_TESTING
SUBDIRS += \
	netzkino \
	youtube
endif

if ENABLE_PYTHON
SUBDIRS += python
endif

if ENABLE_LUA
SUBDIRS += \
	lua \
	media_one \
	plutotv \
	netzkino_hd
if ENABLE_TESTING	
	 \ mtv \
	arte_concert
endif	
endif

