## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

lib_LTLIBRARIES = \
    draw-0.0.1.la

draw_0_0_1_la_SOURCES = \
	draw.cc           \
	draw.h

EXTRA_DIST = \
	Makefile.am \
	draw.pc.in

## Lib draw
draw_0_0_1_la_LDFLAGS = \
    -version-info $(SO_VERSION) \
    -module -Wl,--export-dynamic -rdynamics

draw_0_0_1_la_CPPFLAGS = \
    $(DEPS_CFLAGS) $(GTKMM_CFLAGS) \
    -DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
    -DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"  \
    -DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
    -DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\"
    
draw_0_0_1_la_LIBADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS)
draw_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}-$(API_VERSION)/include

library_includedir = $(includedir)/${PACKAGE_NAME}-$(API_VERSION)
library_include_HEADERS = \
    draw.h

## The generated configuration header is installed in its own subdirectory of
## $(libdir).  The reason for this is that the configuration information put
## into this header file describes the target platform the installed library
## has been built for.  Thus the file must not be installed into a location
## intended for architecture-independent files, as defined by the Filesystem
## Hierarchy Standard (FHS).
## The nodist_ prefix instructs Automake to not generate rules for including
## the listed files in the distribution on 'make dist'.  Files that are listed
## in _HEADERS variables are normally included in the distribution, but the
## configuration header file is generated at configure time and should not be
## shipped with the source tarball.
## libgxsynth_libincludedir = ${PROGRAMNAME_MOD_DIR}/${PACKAGE_NAME}-$(API_VERSION)/include
#nodist_libgxsynth_libinclude_HEADERS = gxsynth.h

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information.  Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = draw.pc

### UI ###
ui_DATA = \
    draw-0.0.1.ui

uidir = \
	$(PROGRAMNAME_UI_DIR)

## File created by the gnome-build tools
