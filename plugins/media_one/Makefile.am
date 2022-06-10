install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/media_one
	install -m 0644 media_one.cfg $(DESTDIR)$(PLUGINDIR)/media_one
	install -m 0644 media_one.lua $(DESTDIR)$(PLUGINDIR)/media_one
	install -m 0644 media_one.png $(DESTDIR)$(PLUGINDIR)/media_one

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/media_one/media_one.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/media_one/media_one.lua
	-rm $(DESTDIR)$(PLUGINDIR)/media_one/media_one.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/media_one



