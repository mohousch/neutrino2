SUBDIRS = locale

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

noinst_LTLIBRARIES = moviebrowser.la

moviebrowser_la_SOURCES = moviebrowser.cpp
	 
moviebrowser_la_LDFLAGS = -rpath $(PLUGINDIR) -module

moviebrowser_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/moviebrowser
	install .libs/moviebrowser.so $(DESTDIR)$(PLUGINDIR)/moviebrowser
	install -m 0644 moviebrowser.cfg $(DESTDIR)$(PLUGINDIR)/moviebrowser
	install -m 0664 moviebrowser.png $(DESTDIR)$(PLUGINDIR)/moviebrowser

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/moviebrowser/moviebrowser.so
	-rm $(DESTDIR)$(PLUGINDIR)/moviebrowser/moviebrowser.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/moviebrowser/moviebrowser.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/moviebrowser

