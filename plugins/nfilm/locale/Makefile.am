LANGS = ar bs de fr it nl pl pt ru sk sv

LANGMO = $(LANGS:=.mo)
LANGPO = $(LANGS:=.po)

EXTRA_DIST = $(LANGPO)

nfilm.pot: $(top_srcdir)/nfilm/*.cpp
	$(XGETTEXT) -L C++ --keyword=_ --from-code=utf-8 --no-wrap --add-comments="TRANSLATORS:" -d nfilm --package-name=@PACKAGE_NAME@ --package-version=@PACKAGE_VERSION@ --msgid-bugs-address='<see README>' --copyright-holder=neutrino2 -s -o $@ $^
	$(SED) --in-place $@ --expression=s/CHARSET/UTF-8/
	
%.po: nfilm.pot
	if [ -f $@ ]; then \
		$(MSGMERGE) --backup=none --no-wrap --no-location -s -U $@ $< && touch $@; \
		$(MSGATTRIB) --no-wrap --no-obsolete $@ -o $@; \
	else \
		$(MSGINIT) -l $@ -o $@ -i $< --no-translator; \
	fi

.po.mo:
	$(MSGFMT) -o $@ $<

BUILT_SOURCES = $(LANGMO)
CLEANFILES = $(LANGMO) nfilm.pot

dist-hook: $(LANGPO)

install-data-local: $(LANGMO)
	for lang in $(LANGS); do \
		$(mkinstalldirs) $(DESTDIR)$(PLUGINDIR)/nfilm/locale/$$lang/LC_MESSAGES; \
		$(INSTALL_DATA) $$lang.mo $(DESTDIR)$(PLUGINDIR)/nfilm/locale/$$lang/LC_MESSAGES/nfilm.mo; \
	done

uninstall-local:
	for lang in $(LANGS); do \
		$(RM) $(DESTDIR)$(PLUGINDIR)/nfilm/locale/$$lang/LC_MESSAGES/nfilm.mo; \
	done
	
	
