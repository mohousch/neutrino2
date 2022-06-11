installdir =  $(DATADIR)/httpd/scripts

install_DATA= api.sh _Y_Globals.sh _Y_Library.sh Y_Live.sh Y_Tools.sh

install-data-hook:
	chmod 0755 $(DESTDIR)$(DATADIR)/httpd/scripts/api.sh
	chmod 0755 $(DESTDIR)$(DATADIR)/httpd/scripts/Y_*.sh
