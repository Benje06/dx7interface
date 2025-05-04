## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = \
	subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = \
	${ACLOCAL_FLAGS}

## GTKCLASS
lib_LTLIBRARIES = \
	libgxgtkclass-0.0.1.la

### LIB GtkClass ####
libgxgtkclass_0_0_1_la_SOURCES = \
	../src/GtkClass.h ../src/GtkClass.cc


libgxgtkclass_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxgtkclass_0_0_1_la_CPPFLAGS = \
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


## SYNTH
lib_LTLIBRARIES += \
	libgxsynth-0.0.1.la

### LIB Synth ####
libgxsynth_0_0_1_la_SOURCES = \
	../src/synth.h ../src/synth.cc


libgxsynth_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxsynth_0_0_1_la_CPPFLAGS = \
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
else
libgxsynth_0_0_1_la_LDFLAGS += \
	-rdynamic
endif

libgxsynth_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include


