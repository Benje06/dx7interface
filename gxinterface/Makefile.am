## Process this file with automake to produce Makefile.in

## TODO: 
SUBDIRS = \
	po\
	data\
	src\
	lib

INTLTOOL_FILES = \
		intltool-extract.in \
		intltool-merge.in \
		intltool-update.in


## If make built it, and it is commonly something that one would want to rebuild
## (for instance, a .o file), then mostlyclean should delete it.
# MOSTLYCLEANFILES = 

## Otherwise, if make built it, then clean should delete it. 
CLEANFILES = \
	lib/gxinterface.pc
##        data/launcher/dx7interface.desktop

##        po/*.gmo\

## ???      lib/makefile \
##          lib/makefile.in \  
##            m4/Makefile \
##            m4/Makefile.in \

## If configure built it, then distclean should delete it.                 
DISTCLEANFILES = \
	Makefile\
	Makefile.in\
	src/Makefile\
	src/Makefile.in\
	data/Makefile\
	data/Makefile.in\
	data/images/Makefile\
	data/images/Makefile.in\
	data/launcher/Makefile\
	data/launcher/Makefile.in\
	data/launcher/gxinterface.desktop.in.h\
	data/ui/Makefile\
	data/ui/Makefile.in\
	data/ui/interface.ui.h\
	po/gxinterface.pot\
	lib/Makefile \
	lib/Makefile.in\
	aclocal.m4\
	autoscan.log\
	config.h\
	config.h.in\
	config.h.in~\
	config.log\
	config.status\
	config/compile\
	config/config.guess\
	config/config.sub\
	config/install-sh\
	config/missing\
	config/depcomp\
	config/ltmain.sh\
	configure\
	configure.ac~\
	configure.scan\
	configure~\
	libtool\
	stamp-h1\
	intltool-extract.in\
	intltool-merge.in\
	intltool-update.in\
	ABOUT-NLS\
	po/*.gmo\
	po/de.po\
	po/en.po\
	po/es.po\
	po/fr.po\
	po/jap.po\
	po/ru.po\
	po/languagecode.po\
	po/Makefile\
	po/Makefile.in\
	po/Makefile.in.in*\
	po/notexist\
	po/missing\
	po/POTFILES\
	po/*.sed\
	po/*.header\
	po/*.sin\
	po/Rules-quot\
	po/Makevars.template\
	m4/*

# local command need a tabulation
#distclean-local:
#<tab>-rm -rf src/.deps autom4te.cache        
distclean-local:
	-rm -rf src/.deps autom4te.cache config m4

AM_CFLAGS=${CFLAGS}
AM_CPPFLAGS=${CPPFLAGS}
## AM_CFLAGS = $(cairo_CFLAGS) $(fontconfig_CFLAGS)
## LIBS += $(cairo_LIBS) $(fontconfig_LIBS)

EXTRA_DIST = \
	$(m4_DATA) \
	$(po_DATA) \
	$(data_DATA) \
	INTLTOOL_FILES \
	autogen.sh \
	configure.ac \
	Makefile.am

##desktopdir = $(datadir)/applications
##desktop_in_files = gxinterface.desktop.in
##desktop_DATA = $(desktop_in_files:.desktop.in=.desktop)
## @INTLTOOL_DESKTOP_RULE@

install-data-local:
	@$(NORMAL_INSTALL)
	if test -d $(srcdir)/pixmaps; then \
		$(mkinstalldirs) $(DESTDIR)$(pkgdatadir)/pixmaps; \
		for pixmap in $(srcdir)/pixmaps/*; do \
			if test -f $$pixmap; then \
				$(INSTALL_DATA) $$pixmap $(DESTDIR)$(pkgdatadir)/pixmaps; \
			fi \
		done \
	fi

dist-hook:
	if test -d pixmaps; then \
		mkdir $(distdir)/pixmaps; \
		for pixmap in pixmaps/*; do \
			if test -f $$pixmap; then \
				cp -p $$pixmap $(distdir)/pixmaps; \
			fi \
		done \
	fi
