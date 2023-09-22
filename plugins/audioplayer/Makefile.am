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

noinst_LTLIBRARIES = audioplayer.la

audioplayer_la_SOURCES = audioplayer.cpp
	 
audioplayer_la_LDFLAGS = -rpath $(PLUGINDIR) -module

audioplayer_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/audioplayer
	install .libs/audioplayer.so $(DESTDIR)$(PLUGINDIR)/audioplayer
	install -m 0644 audioplayer.cfg $(DESTDIR)$(PLUGINDIR)/audioplayer
	install -m 0644 audioplayer.png $(DESTDIR)$(PLUGINDIR)/audioplayer

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/audioplayer/audioplayer.so
	-rm $(DESTDIR)$(PLUGINDIR)/audioplayer/audioplayer.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/audioplayer/audioplayer.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/audioplayer
	




