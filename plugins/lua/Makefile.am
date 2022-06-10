install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/lua
	install -m 0644 lua.cfg $(DESTDIR)$(PLUGINDIR)/lua
	install -m 0644 lua.lua $(DESTDIR)$(PLUGINDIR)/lua
	install -m 0644 lua.png $(DESTDIR)$(PLUGINDIR)/lua


uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/lua/lua.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/lua/lua.lua
	-rm $(DESTDIR)$(PLUGINDIR)/lua/lua.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/lua



