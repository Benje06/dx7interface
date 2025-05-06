## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = \
	subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = \
	${ACLOCAL_FLAGS}

### LIB GtkClass ####


#if PLATFORM_WINDOWS
#libgxgtkclass_0_0_1_ladir = /usr/bin
#else
#libgxgtkclass_0_0_1_ladir = $(libdir)
#endif

##libgxgtkclass_0_0_1_la_LTLIBRARIES = \
##	libgxgtkclass-0.0.1.la

lib_LTLIBRARIES = \
	libgxgtkclass-0.0.1.la

libgxgtkclass_0_0_1_la_SOURCES = \
	../src/GtkClass.h ../src/GtkClass.cc

libgxgtkclass_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxgtkclass_0_0_1_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(AM_CPPFLAGS) -fPIC $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DGXLOG_IMPORTS

if PLATFORM_LINUX
libgxgtkclass_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\" \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\" \
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

libgxgtkclass_0_0_1_la_LIBADD = \
	$(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)

## -L$(top_builddir)/../gxinterface/lib/ $(top_builddir)/../gxinterface/lib/libgxthread-0.0.1.la
## -L$(top_builddir)/../gxinterface/lib/ $(top_builddir)/../gxinterface/lib/libgxinterface-0.0.1.la

## LIBS
libgxgtkclass_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-no-undefined -Wl,--no-undefined,--gc-sections

if PLATFORM_WINDOWS
libgxgtkclass_0_0_1_la_LDFLAGS += \
	-Wl,--export-all-symbols \
	-lgdi32
else
libgxgtkclass_0_0_1_la_LDFLAGS += \
	-rdynamic
endif

libgxgtkclass_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include

### LIB Synth ####

## if PLATFORM_WINDOWS
## libgxsynth_0_0_1_ladir = /usr/bin
## else
## libgxsynth_0_0_1_ladir = $(libdir)
## endif

## libgxsynth_0_0_1_la_LTLIBRARIES += \
##	libgxsynth-0.0.1.la

lib_LTLIBRARIES += \
	libgxsynth-0.0.1.la

libgxsynth_0_0_1_la_SOURCES = \
	../src/synth.h ../src/synth.cc

libgxsynth_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxsynth_0_0_1_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(AM_CPPFLAGS) -fPIC $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DGXLOG_IMPORTS

## -I$(top_srcdir)/../gxinterface/lib

if PLATFORM_LINUX
libgxsynth_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\" \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\" \
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

libgxsynth_0_0_1_la_LIBADD = \
	$(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)

## LIBS
libgxsynth_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-no-undefined -Wl,--no-undefined,--gc-sections

if PLATFORM_WINDOWS
libgxsynth_0_0_1_la_LDFLAGS += \
	-Wl,--export-all-symbols \
	-lgdi32
install-data-hook:
	cp -f $(DESTDIR)/ucrt64/bin/libgxgtkclass-0.0.1-1.dll $(DESTDIR)/usr/bin
	cp -f $(DESTDIR)/ucrt64/bin/libgxsynth-0.0.1-1.dll $(DESTDIR)/usr/bin
	ldd $(DESTDIR)/usr/bin/libgxgtkclass-0.0.1-1.dll | grep /ucrt64 | awk '{print $$3}' | xargs -I {} cp {} $(DESTDIR)/usr/bin/
	ldd $(DESTDIR)/usr/bin/libgxsynth-0.0.1-1.dll | grep /ucrt64 | awk '{print $$3}' | xargs -I {} cp {} $(DESTDIR)/usr/bin/
	## sed -i "s|\(libdir='\)/\(.*'\)|\1\2|" "$(DESTDIR)$(libdir)/usr/bin/libgxgtkclass-0.0.1.la"
	## @rm -f $(DESTDIR)$(libgxgtkclass_0_0_1_ladir)/libgxgtkclass-0.0.1.la
	## @rm -f $(DESTDIR)$(libgxgtkclass_0_0_1_ladir)/libgxgtkclass-0.0.1.a
	## @rm -f $(DESTDIR)$(libgxgtkclass0_0_1_ladir)/libgxgtkclass-0.0.1.dll.a
	## sed -i "s|\(libdir='\)/\(.*'\)|\1\2|" "$(DESTDIR)$(libdir)/usr/bin/libgxsynth-0.0.1.la"
	## @rm -f $(DESTDIR)$(libgxsynth_0_0_1_ladir)/libgxsynth-0.0.1.la
	## @rm -f $(DESTDIR)$(libgxsynth_0_0_1_ladir)/libgxsynth-0.0.1.a
	## @rm -f $(DESTDIR)$(libgxsynth_0_0_1_ladir)/libgxsynth-0.0.1.dll.a
else
libgxsynth_0_0_1_la_LDFLAGS += \
	-rdynamic
endif

libgxsynth_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include

## library_includedir = $(includedir)/${PACKAGE_NAME}/$(LIB_VERSION)
## library_include_HEADERS = \
##        gtkclass.h  \
##        gxsynth.h

## pkgconfigdir = $(datadir)/pkgconfig
## pkgconfig_DATA = gxsynth.pc
