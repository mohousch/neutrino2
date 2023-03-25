LANGS = ar bs cs de fr it nl pl pt ru sk sv

LANGMO = $(LANGS:=.mo)
LANGPO = $(LANGS:=.po)

EXTRA_DIST = $(LANGPO)

neutrino2.pot: $(top_srcdir)/src/*.cpp \
	$(top_srcdir)/src/gui/*.cpp \
	$(top_srcdir)/src/gui/widget/*.cpp \
	$(top_srcdir)/src/gui/bedit/*.cpp \
	$(top_srcdir)/src/system/*.cpp \
	$(top_srcdir)/src/driver/*.cpp \
	$(top_srcdir)/src/daemonc/*.cpp \
	$(top_srcdir)/src/zapit/*.cpp \
	$(top_srcdir)/src/sectionsd/*.cpp \
	$(top_srcdir)/../plugins/test/*.cpp \
	$(top_srcdir)/../plugins/audioplayer/*.cpp \
	$(top_srcdir)/../plugins/fileexplorer/*.cpp \
	$(top_srcdir)/../plugins/fileplayer/*.cpp \
	$(top_srcdir)/../plugins/icecast/*.cpp \
	$(top_srcdir)/../plugins/internetradio/*.cpp \
	$(top_srcdir)/../plugins/mediaportal/*.cpp \
	$(top_srcdir)/../plugins/moviebrowser/*.cpp \
	$(top_srcdir)/../plugins/netzkino/*.cpp \
	$(top_srcdir)/../plugins/nfilm/*.cpp \
	$(top_srcdir)/../plugins/ntvshows/*.cpp \
	$(top_srcdir)/../plugins/opkg_manager/*.cpp \
	$(top_srcdir)/../plugins/picviewer/*.cpp \
	$(top_srcdir)/../plugins/satipclient/*.cpp \
	$(top_srcdir)/../plugins/systeminfo/*.cpp \
	$(top_srcdir)/../plugins/movieplayer/*.cpp \
	$(top_srcdir)/../plugins/upnp/*.cpp \
	$(top_srcdir)/../plugins/youtube/*.cpp
	$(XGETTEXT) -L C++ --keyword=_ --keyword=N_ --from-code=utf-8 --no-wrap --add-comments="TRANSLATORS:" -d @PACKAGE_NAME@ -s -o $@ $^
	$(SED) --in-place $@ --expression=s/CHARSET/UTF-8/
	
%.po: neutrino2.pot
	if [ -f $@ ]; then \
		$(MSGMERGE) --backup=none --no-wrap --no-location -s -U $@ $< && touch $@; \
		$(MSGATTRIB) --no-wrap --no-obsolete $@ -o $@; \
	else \
		$(MSGINIT) -l $@ -o $@ -i $< --no-translator; \
	fi

.po.mo:
	$(MSGFMT) -o $@ $<

BUILT_SOURCES = $(LANGMO)
CLEANFILES = $(LANGMO) neutrino2.pot

dist-hook: $(LANGPO)

install-data-local: $(LANGMO)
	for lang in $(LANGS); do \
		$(mkinstalldirs) $(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES; \
		$(INSTALL_DATA) $$lang.mo $(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/@PACKAGE_NAME@.mo; \
	done

uninstall-local:
	for lang in $(LANGS); do \
		$(RM) $(DESTDIR)$(LOCALEDIR)/@PACKAGE_NAME@-$$lang.po; \
		$(RM) $(DESTDIR)$(LOCALEDIR)/$$lang/LC_MESSAGES/@PACKAGE_NAME@.mo; \
	done
	
	
