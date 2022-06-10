install-exec-local:
	install -d $(DESTDIR)$(PLUGINDIR)/python
	install -m 0644 python.cfg $(DESTDIR)$(PLUGINDIR)/python
	install -m 0644 python.py $(DESTDIR)$(PLUGINDIR)/python
	install -m 0644 python.png $(DESTDIR)$(PLUGINDIR)/python


uninstall-local:
	-rm $(DESTDIR)$(PLUGINDIR)/python/python.cfg
	-rm $(DESTDIR)$(PLUGINDIR)/python/python.py
	-rm $(DESTDIR)$(PLUGINDIR)/python/python.png
	-rmdir $(DESTDIR)$(PLUGINDIR)/python



