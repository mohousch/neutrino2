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

noinst_LTLIBRARIES = fileplayer.la

fileplayer_la_SOURCES = fileplayer.cpp
	 
fileplayer_la_LDFLAGS = -rpath $(PLUGINDIR) -module

fileplayer_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/fileplayer
	install .libs/fileplayer.so $(DESTDIR)$(PLUGINDIR)/fileplayer
	install -m 0644 fileplayer.cfg $(DESTDIR)$(PLUGINDIR)/fileplayer
	install -m 0644 fileplayer.png $(DESTDIR)$(PLUGINDIR)/fileplayer

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/fileplayer/fileplayer.so
	-rm $(DESTDIR)$(PLUGINDIR)/fileplayer/fileplayer.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/fileplayer/fileplayer.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/fileplayer





