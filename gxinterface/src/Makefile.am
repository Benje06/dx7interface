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
	debug.h\
	common.h common.cc\
	gxthread.h gxthread.cc\
	gxmodule.h gxmodule.cc\
	gemod.h gemod.cc\
	gxinterface.h gxinterface.cc\
	main.h main.cc\
	lang.h

AM_CPPFLAGS = \
	$(DEPS_CFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS)

gxinterface_LDADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)

gxinterface_LDFLAGS = -static -no-undefined -Wl,--no-undefined,--gc-sections

## Platform-specific additions

if PLATFORM_WINDOWS
## Windows/MinGW PE-specific flags ,-rpath,ucrt64\lib
gxinterface_LDADD += gxinterfaceicon.o
gxinterface_LDFLAGS += -static-libgcc -static-libstdc++ -Wl,--export-all-symbols
gxinterfaceicon.o: seticon.rc ../../dx7interface/data/images/dx7interface.ico
	$(WINDRES) -i $< -o $@

else
  ## Linux/ELF Equivalent symbol visibility control
  gxinterface_LDFLAGS += -rdynamic
  ## Expands to -Wl,--export-dynamic
endif
