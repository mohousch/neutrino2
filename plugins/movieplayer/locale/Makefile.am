LANGS = ar bs de fr it nl pl pt ru sk sv

LANGMO = $(LANGS:=.mo)
LANGPO = $(LANGS:=.po)

EXTRA_DIST = $(LANGPO)

movieplayer.pot: $(top_srcdir)/movieplayer/*.cpp
	$(XGETTEXT) -L C++ --keyword=_ --from-code=utf-8 --no-wrap --add-comments="TRANSLATORS:" -d movieplayer --package-name=@PACKAGE_NAME@ --package-version=@PACKAGE_VERSION@ --msgid-bugs-address='<see README>' --copyright-holder=neutrino2 -s -o $@ $^
	$(SED) --in-place $@ --expression=s/CHARSET/UTF-8/
	
%.po: movieplayer.pot
	if [ -f $@ ]; then \
		$(MSGMERGE) --backup=none --no-wrap --no-location -s -U $@ $< && touch $@; \
		$(MSGATTRIB) --no-wrap --no-obsolete $@ -o $@; \
	else \
		$(MSGINIT) -l $@ -o $@ -i $< --no-translator; \
	fi

.po.mo:
	$(MSGFMT) -o $@ $<

BUILT_SOURCES = $(LANGMO)
CLEANFILES = $(LANGMO) movieplayer.pot

dist-hook: $(LANGPO)

install-data-local: $(LANGMO)
	for lang in $(LANGS); do \
		$(mkinstalldirs) $(DESTDIR)$(PLUGINDIR)/movieplayer/locale/$$lang/LC_MESSAGES; \
		$(INSTALL_DATA) $$lang.mo $(DESTDIR)$(PLUGINDIR)/movieplayer/locale/$$lang/LC_MESSAGES/movieplayer.mo; \
	done

uninstall-local:
	for lang in $(LANGS); do \
		$(RM) $(DESTDIR)$(PLUGINDIR)/movieplayer/locale/$$lang/LC_MESSAGES/movieplayer.mo; \
	done
	
	
