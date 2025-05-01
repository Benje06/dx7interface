## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = subdir-objects

## Additional flags to pass to aclocal when it is invoked automatically at
## make time. The ${ACLOCAL_FLAGS} variable is picked up from the environment
## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

bin_PROGRAMS = \
	gxinterface

gxinterface_SOURCES = \
	gxinterface.h gxinterface.cc \
	main.h main.cc

gxinterface_CXXFLAGS = $(AM_CXXFLAGS)
gxinterface_CPPFLAGS = \
	-I$(top_srcdir)/lib
	$(AM_CPPFLAGS) -fPIC $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DGXLOG_IMPORTS

## $(AM_CPPFLAGS) $(DEPS_CFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS)

if PLATFORM_LINUX
gxinterface_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\" \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\" \
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

gxinterface_LDADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS) \
	-L../lib/.libs $(top_builddir)/lib/.libs/libgxlogmanager-0.0.1-1.dll \
	-L../lib/.libs $(top_builddir)/lib/.libs/libgxinterface-0.0.1-1.dll

gxinterface_LDFLAGS = \
	-no-undefined \
	-Wl,--no-undefined,--gc-sections

## Platform-specific additions

if PLATFORM_WINDOWS
gxinterface_LDADD += gxinterfaceicon.o
gxinterface_LDFLAGS += \
	-static-libgcc \
	-static-libstdc++ \
	-Wl,--export-all-symbols \
	-lgdi32

gxinterfaceicon.o: seticon.rc ../../dx7interface/data/images/dx7interface.ico
	$(WINDRES) -i $< -o $@
install-exec-hook:
	cp /ucrt64/bin/gdbus.exe $(DESTDIR)/usr/bin
	cp /c/WINDOWS/System32/ucrtbase.dll $(DESTDIR)/usr/bin
	ldd $(DESTDIR)/usr/bin/gdbus.exe | grep /ucrt64 | awk '{print $$3}' | xargs -I {} cp {} $(DESTDIR)/usr/bin/
	ldd $(DESTDIR)/usr/bin/gxinterface.exe | grep /ucrt64 | awk '{print $$3}' | xargs -I {} cp {} $(DESTDIR)/usr/bin/
	mkdir -p /d/gxinterface/package/usr/share/glib-2.0/schemas/
	glib-compile-schemas --targetdir=/d/gxinterface/package/usr/share/glib-2.0/schemas/ /ucrt64/share/glib-2.0/schemas
else
gxinterface_LDFLAGS += -rdynamic 
## Expands to -Wl,--export-dynamic
endif
