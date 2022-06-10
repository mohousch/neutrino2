AM_CPPFLAGS =  \
	-I$(top_srcdir)/../nhd2-exp \
	-I$(top_srcdir)/../nhd2-exp/src \
	-I$(top_srcdir)/../nhd2-exp/src/zapit/include/zapit \
	-I$(top_srcdir)/../nhd2-exp/lib \
	-I$(top_srcdir)/../nhd2-exp/lib/connection \
	-I$(top_srcdir)/../nhd2-exp/lib/libeventserver \
	-I$(top_srcdir)/../nhd2-exp/lib/libconfigfile \
	-I$(top_srcdir)/../nhd2-exp/lib/libnet \
	-I$(top_srcdir)/../nhd2-exp/lib/libxmltree \
	-I$(top_srcdir)/../nhd2-exp/lib/libmd5sum \
	-I$(top_srcdir)/../nhd2-exp/lib/libdvbapi \
	@FREETYPE_CFLAGS@ 

noinst_LTLIBRARIES = picviewer.la

picviewer_la_SOURCES = picviewer.cpp
	 
picviewer_la_LDFLAGS = -rpath $(PLUGINDIR) -module

picviewer_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/picviewer
	install .libs/picviewer.so $(DESTDIR)$(PLUGINDIR)/picviewer
	install -m 0644 picviewer.cfg $(DESTDIR)$(PLUGINDIR)/picviewer
	install -m 0644 picviewer.png $(DESTDIR)$(PLUGINDIR)/picviewer

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/picviewer/picviewer.so
	-rm $(DESTDIR)$(PLUGINDIR)/picviewer/picviewer.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/picviewer/picviewer.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/picviewer
	




