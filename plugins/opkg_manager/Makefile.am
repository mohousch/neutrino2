AM_CPPFLAGS =  \
	-I$(srcdir) \
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
	-I$(top_builddir)/upnpbrowser/libupnpclient \
	@FREETYPE_CFLAGS@ 

noinst_LTLIBRARIES = opkg_manager.la

opkg_manager_la_SOURCES = opkg_manager.cpp
	 
opkg_manager_la_LDFLAGS = -rpath $(PLUGINDIR) -module

opkg_manager_la_CXXFLAGS = -fno-rtti -fno-exceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/opkg_manager
	install .libs/opkg_manager.so $(DESTDIR)$(PLUGINDIR)/opkg_manager
	install -m 0644 opkg_manager.cfg $(DESTDIR)$(PLUGINDIR)/opkg_manager
	install -m 0644 opkg_manager.png $(DESTDIR)$(PLUGINDIR)/opkg_manager
	install -m 0644 opkg_manager_small.png $(DESTDIR)$(PLUGINDIR)/opkg_manager

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/opkg_manager/opkg_manager.so
	-rm $(DESTDIR)$(PLUGINDIR)/opkg_manager/opkg_manager.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/opkg_manager/opkg_manager.png
	-rm $(DESTDIR)$(PLUGINDIR)/opkg_manager/opkg_manager_small.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/opkg_manager

	




