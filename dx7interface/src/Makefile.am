## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = \
        subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = \
        ${ACLOCAL_FLAGS}

lib_LTLIBRARIES = \
        dx7interface-0.0.1.la\
        tx81zinterface-0.0.1.la

### LIB DX7 ####
dx7interface_0_0_1_la_SOURCES = \
        dx7interface.h dx7interface.cc\
	synth.h synth.cc\
	GtkClass.h GtkClass.cc\
        dx7sysex.h

dx7interface_0_0_1_la_LDFLAGS = \
        -version-info $(SO_VERSION) \
        -module -Wl,--export-dynamic -rdynamics

dx7interface_0_0_1_la_CPPFLAGS = \
        $(DEPS_CFLAGS) $(GTKMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\"\
        -DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"\
        -DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"\
        -DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\"\
        -DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\"\
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"

dx7interface_0_0_1_la_LIBADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS)

##dx7interface_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include
##library_includedir = $(includedir)/${PACKAGE_NAME}/$(LIB_VERSION)
##library_include_HEADERS = \
##        dx7interface.h  \
##        dx7sysex.h

#### LIB tx81z ####
tx81zinterface_0_0_1_la_SOURCES = \
        tx81z.h tx81z.cc\
        synth.h synth.cc\
	GtkClass.h GtkClass.cc\
        tx81zsysex.h

## Lib DX7 interface for sysex editing as module
tx81zinterface_0_0_1_la_LDFLAGS = \
        -version-info $(SO_VERSION) \
        -module -Wl,--export-dynamic -rdynamics -no-undefined

tx81zinterface_0_0_1_la_CPPFLAGS =\
        $(DEPS_CFLAGS) $(GTKMM_CFLAGS)\
        -DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"\
        -DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"\
        -DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\"\
        -DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\"\
        -DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"

tx81zinterface_0_0_1_la_LIBADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS)

tx81zinterface_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include
##library_includedir = $(includedir)/${PACKAGE_NAME}/$(LIB_VERSION)
##library_include_HEADERS = \
##        tx81z.h\
##        tx81zsysex.h
## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information.  Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
##pkgconfigdir = $(datadir)/pkgconfig
##pkgconfig_DATA = ${PACKAGE_NAME}.pc

## Install the generated pkg-config file (.pc) into the expected location for
## architecture-dependent package configuration information.  Occasionally,
## pkg-config files are also used for architecture-independent data packages,
## in which case the correct install location would be $(datadir)/pkgconfig.
pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = ${PACKAGE_NAME}.pc

## File created by the gnome-build tools
