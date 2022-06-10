install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/plutotv
	install -m 0644 plutotv.cfg $(DESTDIR)$(PLUGINDIR)/plutotv
	install -m 0644 plutotv.lua $(DESTDIR)$(PLUGINDIR)/plutotv
	install -m 0644 plutotv.png $(DESTDIR)$(PLUGINDIR)/plutotv

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/plutotv/plutotv.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/plutotv/plutotv.lua
	-rm $(DESTDIR)$(PLUGINDIR)/plutotv/plutotv.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/plutotv



