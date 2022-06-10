install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/arte_concert
	install -m 0644 arte_concert.cfg $(DESTDIR)$(PLUGINDIR)/arte_concert
	install -m 0644 arte_concert.lua $(DESTDIR)$(PLUGINDIR)/arte_concert
	install -m 0644 arte_concert_hint.png $(DESTDIR)$(PLUGINDIR)/arte_concert

uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/arte_concert/arte_concert.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/arte_concert/arte_concert.lua
	-rm $(DESTDIR)$(PLUGINDIR)/arte_concert/arte_concert_hint.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/arte_concert



