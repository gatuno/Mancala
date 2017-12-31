SUBDIRS = src data

.PHONY: macapp
macapp:
	cd src && $(MAKE) $(AM_MAKEFLAGS) bundle && $(MAKE) $(AM_MAKEFLAGS) bundle-fw

.PHONY: macdmg
macdmg:
	cd src && $(MAKE) $(AM_MAKEFLAGS) bundle-dmg
