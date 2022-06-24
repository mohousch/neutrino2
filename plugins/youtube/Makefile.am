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
	-I$(top_srcdir)/../neutrino2/lib/jsoncpp/include \
	@FREETYPE_CFLAGS@ 

noinst_LTLIBRARIES = youtube.la

youtube_la_SOURCES = youtube.cpp
	 
youtube_la_LDFLAGS = -rpath $(PLUGINDIR) -module

youtube_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/youtube
	install .libs/youtube.so $(DESTDIR)$(PLUGINDIR)/youtube
	install -m 0644 youtube.cfg $(DESTDIR)$(PLUGINDIR)/youtube
	install -m 0644 youtube.png $(DESTDIR)$(PLUGINDIR)/youtube
	install -m 0644 youtube_small.png $(DESTDIR)$(PLUGINDIR)/youtube

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/youtube/youtube.so
	-rm $(DESTDIR)$(PLUGINDIR)/youtube/youtube.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/youtube/youtube.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/youtube



