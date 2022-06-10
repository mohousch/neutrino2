install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/netzkino_hd
	install -m 0644 netzkino_hd.cfg $(DESTDIR)$(PLUGINDIR)/netzkino_hd
	install -m 0644 netzkino_hd.lua $(DESTDIR)$(PLUGINDIR)/netzkino_hd
	install -m 0644 netzkino.png $(DESTDIR)$(PLUGINDIR)/netzkino_hd


uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/netzkino_hd/netzkino_hd.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/netzkino_hd/netzkino_hd.lua
	-rm $(DESTDIR)$(PLUGINDIR)/netzkino_hd/netzkino.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/netzkino_hd



