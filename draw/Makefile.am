## Process this file with automake to produce Makefile.in
              
## TODO: 
SUBDIRS = src

INTLTOOL_FILES = intltool-extract.in \
                 intltool-merge.in \
                 intltool-update.in
                 
DISTCLEANFILES = \
        aclocal.m4\
        autoscan.log\
        config.h\
        config.h.in\
        config.h.in~\
        config.log\
        config.status\
        configure\
	configure~ \
        configure.ac~\
        configure.scan\
        configure~\
        libtool\
        Makefile\
        Makefile.in\
        stamp-h1\
        config/compile\
        config/config.guess\
        config/config.sub\
        config/install-sh\
        config/missing\
        config/depcomp\
        config/ltmain.sh\
	m4/Makefile \
	m4/Makefile.in \
        src/Makefile\
        src/Makefile.in\
	intltool-extract \
	intltool-merge \
	intltool-update \
	po/.intltool-merge-cache\
        po/draw.pot\
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
        po/Makefile.in.in\
        po/notexist\
        po/missing\
        po/POTFILES

distclean-local:
	-rm -rf src/.deps autom4te.cache config

AM_CFLAGS=${CFLAGS}
AM_CPPFLAGS=${CPPFLAGS}
## AM_CFLAGS = $(cairo_CFLAGS) $(fontconfig_CFLAGS)
## LIBS += $(cairo_LIBS) $(fontconfig_LIBS)

EXTRA_DIST = \
	$(m4_DATA) \
	INTLTOOL_FILES \
	autogen.sh \
	configure.ac \
	Makefile.am

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
