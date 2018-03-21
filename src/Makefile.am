# Automake file for Mancala

gamedatadir = $(pkgdatadir)/data

bin_PROGRAMS = mancala
mancala_SOURCES = mancala.c mancala.h \
	resolv.c resolv.h \
	juego.c juego.h \
	draw-text.c draw-text.h \
	path.c path.h \
	background.c background.h \
	ventana.c ventana.h \
	zoom.c zoom.h \
	inputbox.c inputbox.h \
	netplay.c netplay.h \
	utf8.c utf8.h \
	sdl2_rect.c sdl2_rect.h

if MACOSX
mancala_SOURCES += SDLMain.m SDLMain.h
endif

#EXTRA_DIST = mancala.rc SDLMain.m SDLMain.h

if MINGW32
mancala_ico.o: mancala.rc ../data/mancala.ico
	$(WINDRES) $(srcdir)/mancala.rc -O coff -o mancala_ico.o

mingw_ldadd = mancala_ico.o -lws2_32 -lwsock32
else # NOT MINGW
mingw_ldadd =
endif

mancala_CPPFLAGS = -DGAMEDATA_DIR=\"$(gamedatadir)/\" -DLOCALEDIR=\"$(localedir)\" $(AM_CPPFLAGS)
mancala_CFLAGS = $(SDL_CFLAGS) $(SDL_image_CFLAGS) $(SDL_mixer_CFLAGS) $(SDL_ttf_CFLAGS) $(AM_CFLAGS)
if MACOSX
# En MAC OS X, hay que ligar/compilar contra los frameworks
mancala_LDFLAGS = -Wl,-rpath,@loader_path/../Frameworks
else
mancala_LDADD = -lm $(SDL_LIBS) $(SDL_image_LIBS) $(SDL_mixer_LIBS) $(SDL_ttf_LIBS) $(mingw_ldadd)
endif

#------------------ Packaging rules for Mac OSX ------------------------

bundle_root = $(top_builddir)/etc/macfiles
bundle_name = $(bundle_root)/Mancala.app
# Special target to copy all needed game files to Mancala.app/Contents/Resources.
# For Mac OS X bundle.
.PHONY: bundle
bundle:
	-rm -rf $(bundle_root)
	mkdir -p $(bundle_name)/Contents/MacOS
	mkdir -p $(bundle_name)/Contents/Resources/data/
	echo "APPL????" > $(bundle_name)/Contents/PkgInfo
	cp $(top_builddir)/etc/Info.plist $(bundle_name)/Contents/
	cp $(top_builddir)/etc/mancala.icns $(bundle_name)/Contents/Resources/
	mkdir -p $(bundle_name)/Contents/Resources/data/images
	cp -R $(top_builddir)/data/images/* $(bundle_name)/Contents/Resources/data/images
	mkdir -p $(bundle_name)/Contents/Resources/data/sounds
	cp $(top_builddir)/data/sounds/* $(bundle_name)/Contents/Resources/data/sounds
	cp $(top_builddir)/data/*.ttf $(bundle_name)/Contents/Resources/data
#	for cat in $(ALL_LINGUAS); do \
#	  dir=$(bundle_name)/Contents/Resources/locale/$$cat/LC_MESSAGES; \
#	  mkdir -p $$dir; \
#	  echo "installing $$cat as $$dir/mancala.mo"; \
#	  cp $(top_builddir)/po/$$cat.gmo $$dir/mancala.mo; \
#	done

# Special target to copy the binary to Mancala.app/Contents/MacOS.
.PHONY: bundle-fw
bundle-fw: bundle
	cp mancala $(bundle_name)/Contents/MacOS/mancala
	mkdir  $(bundle_name)/Contents/Frameworks
	for i in SDL SDL_mixer SDL_image SDL_ttf CPStamp; do \
	  cp -a /Library/Frameworks/$${i}.framework $(bundle_name)/Contents/Frameworks/; \
	done
	strip $(bundle_name)/Contents/MacOS/mancala

# Special target to make a disk image of the bundled app
.PHONY: bundle-dmg
bundle-dmg: bundle-fw
	ln -s /Applications $(bundle_root)/Applications
	hdiutil create  -ov -srcfolder $(bundle_root) -volname Mancala -imagekey zlib-level=6 $(top_builddir)/etc/mancala.dmg

