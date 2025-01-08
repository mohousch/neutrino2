LANGS = ar bs de fr it nl pl pt ru sk sv

LANGMO = $(LANGS:=.mo)
LANGPO = $(LANGS:=.po)

EXTRA_DIST = $(LANGPO)

media_one.pot: $(top_srcdir)/media_one/*.lua
	$(XGETTEXT) -L lua --keyword=_ --from-code=utf-8 --no-wrap --add-comments="TRANSLATORS:" -d media_one --package-name=@PACKAGE_NAME@ --package-version=@PACKAGE_VERSION@ --msgid-bugs-address='<see README>' --copyright-holder=neutrino2 -s -o $@ $^
	$(SED) --in-place $@ --expression=s/CHARSET/UTF-8/
	
%.po: media_one.pot
	if [ -f $@ ]; then \
		$(MSGMERGE) --backup=none --no-wrap --no-location -s -U $@ $< && touch $@; \
		$(MSGATTRIB) --no-wrap --no-obsolete $@ -o $@; \
	else \
		$(MSGINIT) -l $@ -o $@ -i $< --no-translator; \
	fi

.po.mo:
	$(MSGFMT) -o $@ $<

BUILT_SOURCES = $(LANGMO)
CLEANFILES = $(LANGMO) media_one.pot

dist-hook: $(LANGPO)

install-data-local: $(LANGMO)
	for lang in $(LANGS); do \
		$(mkinstalldirs) $(DESTDIR)$(PLUGINDIR)/media_one/locale/$$lang/LC_MESSAGES; \
		$(INSTALL_DATA) $$lang.mo $(DESTDIR)$(PLUGINDIR)/media_one/locale/$$lang/LC_MESSAGES/media_one.mo; \
	done

uninstall-local:
	for lang in $(LANGS); do \
		$(RM) $(DESTDIR)$(PLUGINDIR)/media_one/locale/$$lang/LC_MESSAGES/media_one.mo; \
	done
	
	
