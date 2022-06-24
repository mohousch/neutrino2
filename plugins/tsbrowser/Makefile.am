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

noinst_LTLIBRARIES = tsbrowser.la

tsbrowser_la_SOURCES = tsbrowser.cpp
	 
tsbrowser_la_LDFLAGS = -rpath $(PLUGINDIR) -module

tsbrowser_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/tsbrowser
	install .libs/tsbrowser.so $(DESTDIR)$(PLUGINDIR)/tsbrowser
	install -m 0644  tsbrowser.cfg $(DESTDIR)$(PLUGINDIR)/tsbrowser
	install -m 0644  tsbrowser.png $(DESTDIR)$(PLUGINDIR)/tsbrowser

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/tsbrowser/tsbrowser.so
	-rm $(DESTDIR)$(PLUGINDIR)/tsbrowser/tsbrowser.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/tsbrowser/tsbrowser.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/tsbrowser
	




