## Process this file with automake to produce Makefile.in
## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = subdir-objects

## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

#### LIB ####
lib_LTLIBRARIES = \
    libgxinterface-0.0.1.la

libgxinterface_0_0_1_la_SOURCES = \
	../src/lang.h\
	../src/debug.h\
	../src/common.h ../src/common.cc\
	../src/gxthread.h ../src/gxthread.cc\
	../src/gxmodule.h ../src/gxmodule.cc\
	../src/gemod.h ../src/gemod.cc\
	../src/gxinterface.cc ./src/gxinterface.h
	
libgxinterface_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-rdynamic -no-undefined -Wl,--no-undefined
## win32 ,--export-all-symbols
	
##  -rdynamics -no-undefined -Wl,--export-dynamic 

libgxinterface_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxinterface_0_0_1_la_CPPFLAGS = \
	$(AM_CPPFLAGS) -fPIC $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\"  \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\"

## libgxinterface_0_0_1_la_LIBADD = $(DEPS_LIBS) $(PTHREAD_LIBS) $(INTLLIBS) /ucrt64/bin/libintl-8.dll $(GTKMM_LIBS) $(GLIBMM_LIBS)

libgxinterface_0_0_1_la_LIBADD = $(DEPS_LIBS) $(INTLLIBS)

#### HEADERS IN INCLUDE ####			
## This only works if the directory hierarchy in the
## source tree matches the hierarchy at the install location, however.
library_includedir = $(includedir)/${PACKAGE_NAME}/$(LIB_VERSION)

library_include_HEADERS = \
	../src/common.h \
	../src/debug.h \
	../src/gemod.h \
	../src/gxmodule.h\
	../src/gxinterface.h \
	../src/gxthread.h \
	../src/lang.h

libgxinterface_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include

pkgconfigdir = $(datadir)/pkgconfig
pkgconfig_DATA = ${PACKAGE_NAME}.pc

## File created by the gnome-build tools

