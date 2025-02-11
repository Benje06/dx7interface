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
	gxmodule.h gxmodule.cc\
	gemod.h gemod.cc\
	gxinterface.h gxinterface.cc\
	main.h main.cc\
	gxthread.h gxthread.cc\
	lang.h

# DEBUG FLAG -g
# CFLAG = -g $(CFLAGS)

AM_CPPFLAGS = \
	$(DEPS_CFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS)\
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"  \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\"\
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\"\
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"

gxinterface_LDADD = $(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)

