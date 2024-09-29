installdir = $(bindir)

install_DATA = \
	backup.sh \
	restore.sh \
	init_hdd.sh

install-data-hook:
	chmod 0755 $(DESTDIR)$(bindir)/backup.sh
	chmod 0755 $(DESTDIR)$(bindir)/restore.sh
	chmod 0755 $(DESTDIR)$(bindir)/init_hdd.sh
