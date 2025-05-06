## Process this file with automake to produce Makefile.in
## Place generated object files (.o) into the same directory as their source
## files, in order to avoid collisions when non-recursive make is used.
AUTOMAKE_OPTIONS = subdir-objects

## to provide a way for the user to supply additional arguments.
ACLOCAL_AMFLAGS = ${ACLOCAL_FLAGS}

## LIBS

## LOGMANAGER
lib_LTLIBRARIES = \
	libgxlogmanager-0.0.1.la

libgxlogmanager_0_0_1_la_SOURCES = \
	../src/logger.h ../src/logger.cc

libgxlogmanager_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxlogmanager_0_0_1_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	$(AM_CPPFLAGS) $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DGXLOG_EXPORTS

if PLATFORM_LINUX
libgxlogmanager_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\" \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\" \
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

libgxlogmanager_0_0_1_la_LIBADD = \
	$(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)

libgxlogmanager_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-no-undefined -Wl,--no-undefined

if PLATFORM_WINDOWS
libgxlogmanager_0_0_1_la_LDFLAGS += \
	-Wl,--export-all-symbols \
	-lgdi32
else
libgxlogmanager_0_0_1_la_LDFLAGS += \
	-rdynamic
endif

libgxlogmanager_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include

## LIB GXINTERFACE
lib_LTLIBRARIES += \
	libgxinterface-0.0.1.la

libgxinterface_0_0_1_la_SOURCES = \
	../src/gxmodule.h ../src/gxmodule.cc \
	../src/gemod.h ../src/gemod.cc

libgxinterface_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS) -fPIC
libgxinterface_0_0_1_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	$(AM_CPPFLAGS) $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\"\
	-DGXLOG_IMPORTS

if PLATFORM_LINUX
libgxinterface_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\" \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\" \
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

libgxinterface_0_0_1_la_LIBADD = \
	libgxlogmanager-0.0.1.la \
	$(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)

libgxinterface_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-no-undefined -Wl,--no-undefined

if PLATFORM_WINDOWS
libgxinterface_0_0_1_la_LDFLAGS += \
	-Wl,--export-all-symbols \
	-lgdi32
else
libgxinterface_0_0_1_la_LDFLAGS += \
	-rdynamic
endif

## LIB GXTHREAD
lib_LTLIBRARIES += \
	libgxthread-0.0.1.la

libgxthread_0_0_1_la_SOURCES = \
	../src/gxthread.h ../src/gxthread.cc

libgxthread_0_0_1_la_CXXFLAGS = $(AM_CXXFLAGS)
libgxthread_0_0_1_la_CPPFLAGS = \
	-I$(top_srcdir)/src \
	-I$(top_srcdir)/lib \
	$(AM_CPPFLAGS) $(DEPS_CFLAGS) $(DEPS_CPPFLAGS) $(GTKMM_CFLAGS) $(GLIBMM_CFLAGS) \
	-DGETTEXT_PACKAGE=\"$(GETTEXT_PACKAGE)\" \
	-DGXLOG_IMPORTS

if PLATFORM_LINUX
libgxthread_0_0_1_la_CPPFLAGS += \
	-DPROGRAMNAME_LOCALEDIR=\"${PROGRAMNAME_LOCALEDIR}\" \
	-DPROGRAMNAME_UI_DIR=\"${PROGRAMNAME_UI_DIR}\" \
	-DPROGRAMNAME_MOD_DIR=\"${PROGRAMNAME_MOD_DIR}\" \
	-DPROGRAMNAME_IMG_DIR=\"${PROGRAMNAME_IMG_DIR}\" \
	-DPROGRAMNAME_DATA_DIR=\"${PROGRAMNAME_DATA_DIR}\"
endif

libgxthread_0_0_1_la_LIBADD = \
	libgxinterface-0.0.1.la \
	$(DEPS_LIBS) $(INTLLIBS) $(GTKMM_LIBS) $(GLIBMM_LIBS)


libgxthread_0_0_1_la_LDFLAGS = \
	-version-info $(SO_VERSION) \
	-no-undefined -Wl,--no-undefined

if PLATFORM_WINDOWS
libgxthread_0_0_1_la_LDFLAGS += \
	-Wl,--export-all-symbols \
	-lgdi32
else
libgxthread_0_0_1_la_LDFLAGS += \
	-rdynamic
endif

libgxthread_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include

#### HEADERS IN INCLUDE ####			
## This only works if the directory hierarchy in the
## source tree matches the hierarchy at the install location, however.

library_include_HEADERS = \
	../src/common.h \
	../src/debug.h \
	../src/logger.h \
	../src/gemod.h \
	../src/gxmodule.h \
	../src/gxinterface.h \
	../src/gxthread.h \
	../src/lang.h

libgxinterface_0_0_1_libincludedir = $(libdir)/${PACKAGE_NAME}/$(LIB_VERSION)/include

if PLATFORM_WINDOWS
library_includedir = /ucrt64/include/${PACKAGE_NAME}/$(LIB_VERSION)
pkgconfigdir = /ucrt64/share/pkgconfig
else
library_includedir = $(includedir)/${PACKAGE_NAME}/$(LIB_VERSION)
pkgconfigdir = $(datadir)/pkgconfig
endif
pkgconfig_DATA = ${PACKAGE_NAME}.pc	

## File created by the gnome-build tools

