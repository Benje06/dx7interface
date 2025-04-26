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
	dx7interface-0.0.1.la

##        tx81zinterface-0.0.1.la

### LIB DX7 ####
dx7interface_0_0_1_la_SOURCES = \
	synth.h synth.cc\
	GtkClass.h GtkClass.cc\
	dx7sysex.h dx7interface.h dx7interface.cc

dx7interface_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
dx7interface_0_0_1_la_CPPFLAGS = \
	$(AM_CPPFLAGS) -fPIC $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\"

if PLATFORM_LINUX
dx7interface_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\"\
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"\
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\"\
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\"\
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

dx7interface_0_0_1_la_LIBADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS)

dx7interface_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-module -no-undefined -Wl,--no-undefined,--gc-sections

## Platform-specific additions
if PLATFORM_WINDOWS
## Windows/MinGW: PE-specific flags ; ,-rpath,ucrt64\lib
dx7interface_0_0_1_la_LDFLAGS += -Wl,--export-all-symbols
install-exec-hook:
	sed -i "s|\(libdir='\)/\(.*'\)|\1\2|" "$(DESTDIR)$(libdir)/dx7interface-0.0.1.la"

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
