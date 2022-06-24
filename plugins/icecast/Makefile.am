AM_CPPFLAGS =  \
	-I$(srcdir) \
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

noinst_LTLIBRARIES = icecast.la

icecast_la_SOURCES = icecast.cpp
	 
icecast_la_LDFLAGS = -rpath $(PLUGINDIR) -module

icecast_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/icecast
	install .libs/icecast.so $(DESTDIR)$(PLUGINDIR)/icecast
	install -m 0644 icecast.cfg $(DESTDIR)$(PLUGINDIR)/icecast
	install -m 0644 icecast.png $(DESTDIR)$(PLUGINDIR)/icecast
	install -m 0644 icecast_small.png $(DESTDIR)$(PLUGINDIR)/icecast

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/icecast/icecast.so
	-rm $(DESTDIR)$(PLUGINDIR)/icecast/icecast.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/icecast/icecast.png
	-rm $(DESTDIR)$(PLUGINDIR)/icecast/icecast_small.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/icecast

	




