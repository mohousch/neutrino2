install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/mtv
	install -m 0644 mtv.cfg $(DESTDIR)$(PLUGINDIR)/mtv
	install -m 0644 mtv.lua $(DESTDIR)$(PLUGINDIR)/mtv
	install -m 0644 mtv_hint.png $(DESTDIR)$(PLUGINDIR)/mtv


uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/mtv/mtv.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/mtv/mtv.lua
	-rm $(DESTDIR)$(PLUGINDIR)/mtv/mtv_hint.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/mtv



