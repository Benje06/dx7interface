## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = \
	subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = \
	${ACLOCAL_FLAGS}

## DX7INTERFACE
dx7interface_0_0_1_ladir=${datadir}/${PACKAGE_NAME}/${PACKAGE_VERSION}/

dx7interface_0_0_1_la_LTLIBRARIES = \
	dx7interface-0.0.1.la

### LIB DX7 ####
dx7interface_0_0_1_la_SOURCES = \
	dx7sysex.h dx7interface.h dx7interface.cc

dx7interface_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
dx7interface_0_0_1_la_CPPFLAGS = \
	-I$(top_srcdir)/lib/ \
	$(AM_CPPFLAGS) -fPIC $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DGXLOG_IMPORTS

if PLATFORM_LINUX
dx7interface_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"\
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"\
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\"\
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\"\
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

dx7interface_0_0_1_la_LIBADD = \
	$(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS) \
	-L$(top_builddir)/lib $(top_builddir)/lib/libgxgtkclass-0.0.1.la \
	-L$(top_builddir)/lib $(top_builddir)/lib/libgxsynth-0.0.1.la


if FULL_DEV
dx7interface_0_0_1_la_CPPFLAGS += \
	-I$(top_srcdir)/../gxinterface/lib
dx7interface_0_0_1_la_LIBADD += \
	-L$(top_builddir)/../gxinterface/lib/ $(top_builddir)/../gxinterface/lib/libgxthread-0.0.1.la \
	-L$(top_builddir)/../gxinterface/lib/ $(top_builddir)/../gxinterface/lib/libgxinterface-0.0.1.la
endif

dx7interface_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-module -no-undefined -Wl,--no-undefined,--gc-sections

## Platform-specific additions
if PLATFORM_WINDOWS
## Windows/MinGW: PE-specific flags ; ,-rpath,ucrt64\lib
dx7interface_0_0_1_la_LDFLAGS += \
	-Wl,--export-all-symbols \
	-lgdi32
install-data-hook:
	sed -i "s|\(libdir='\)/\(.*'\)|\1\2|" "$(DESTDIR)$(dx7interface_0_0_1_ladir)/dx7interface-0.0.1.la"
	ldd $(DESTDIR)$(dx7interface_0_0_1_ladir)/dx7interface-0.0.1-1.dll | grep /ucrt64 | awk '{print $$3}' | xargs -I {} cp {} $(DESTDIR)/usr/bin/
	@rm -f $(DESTDIR)$(dx7interface_0_0_1_ladir)/dx7interface-0.0.1.a
	@rm -f $(DESTDIR)$(dx7interface_0_0_1_ladir)/dx7interface-0.0.1.dll.a
endif
if PLATFORM_LINUX
## Linux/ELF: Equivalent symbol visibility control # Expands to -Wl,--export-dynamic
dx7interface_0_0_1_la_LDFLAGS += -rdynamic
endif

## dx7interface_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include
## library_includedir = $(includedir)/${PACKAGE_NAME}/$(LIB_VERSION)
## library_include_HEADERS = \
##        dx7interface.h  \
##        dx7sysex.h

## pkgconfigdir = $(datadir)/pkgconfig
## pkgconfig_DATA = ${PACKAGE_NAME}.pc

## File created by the gnome-build tools
