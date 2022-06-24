AM_CPPFLAGS =  \
	-I$(top_srcdir)/../neutrino2 \
	-I$(top_srcdir)/../neutrino2/src \
	-I$(top_srcdir)/../neutrino2/src/zapit/include/zapit \
	-I$(top_srcdir)/../neutrino2/lib \
	-I$(top_srcdir)/../neutrino2/lib/connection \
	-I$(top_srcdir)/../neutrino2/lib/libeventserver \
	-I$(top_srcdir)/../neutrino2/lib/libconfigfile \
	-I$(top_srcdir)/../neutrino2/lib/libnet \
	-I$(top_srcdir)/../neutrino2/lib/libxmltree \
	-I$(top_srcdir)/../neutrino2/lib/libmd5sum \
	-I$(top_srcdir)/../neutrino2/lib/libdvbapi \
	@FREETYPE_CFLAGS@ 

noinst_LTLIBRARIES = ntvshows.la

ntvshows_la_SOURCES = ntvshows.cpp nseasons.cpp nepisodes.cpp
	 
ntvshows_la_LDFLAGS = -rpath $(PLUGINDIR) -module

nfilm_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/ntvshows
	install .libs/ntvshows.so $(DESTDIR)$(PLUGINDIR)/ntvshows
	install -m 0644  ntvshows.cfg $(DESTDIR)$(PLUGINDIR)/ntvshows
	install -m 0644  ntvshows.png $(DESTDIR)$(PLUGINDIR)/ntvshows

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/ntvshows/ntvshows.so
	-rm $(DESTDIR)$(PLUGINDIR)/ntvshows/ntvshows.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/ntvshows/ntvshows.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/ntvshows
	




