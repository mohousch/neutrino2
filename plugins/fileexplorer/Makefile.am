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

noinst_LTLIBRARIES = fileexplorer.la

fileexplorer_la_SOURCES = fileexplorer.cpp
	 
fileexplorer_la_LDFLAGS = -rpath $(PLUGINDIR) -module

fileexplorer_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/fileexplorer
	install .libs/fileexplorer.so $(DESTDIR)$(PLUGINDIR)/fileexplorer
	install -m 0644  fileexplorer.cfg $(DESTDIR)$(PLUGINDIR)/fileexplorer
	install -m 0644  fileexplorer.png $(DESTDIR)$(PLUGINDIR)/fileexplorer

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/fileexplorer/fileexplorer.so
	-rm $(DESTDIR)$(PLUGINDIR)/fileexplorer/fileexplorer.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/fileexplorer/fileexplorer.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/fileexplorer
	




