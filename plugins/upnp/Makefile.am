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
	-I$(top_srcdir)/../nhd2-exp/lib/libupnpclient \
	@FREETYPE_CFLAGS@ 

noinst_LTLIBRARIES = upnp.la

upnp_la_SOURCES = UPNPSocket.cpp UPNPDevice.cpp UPNPService.cpp upnp.cpp
	 
upnp_la_LDFLAGS = -rpath $(PLUGINDIR) -module

upnp_la_CXXFLAGS = -fno-rtti -fexceptions

install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/upnp
	install .libs/upnp.so $(DESTDIR)$(PLUGINDIR)/upnp
	install -m 0644 upnp.cfg $(DESTDIR)$(PLUGINDIR)/upnp
	install -m 0644 upnp.png $(DESTDIR)$(PLUGINDIR)/upnp
	install -m 0644 upnp_small.png $(DESTDIR)$(PLUGINDIR)/upnp

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/upnp/upnp.so
	-rm $(DESTDIR)$(PLUGINDIR)/upnp/upnp.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/upnp/upnp.png
	-rm $(DESTDIR)$(PLUGINDIR)/upnp/upnp_small.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/upnp

	




