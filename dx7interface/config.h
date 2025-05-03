/* config.h.  Generated from config.h.in by configure.  */
/* config.h.in.  Generated from configure.ac by autoheader.  */

/* Define to 1 if using 'alloca.c'. */
/* #undef C_ALLOCA */

/* activate debug tool */
/* #undef DEBUG_TOOL */

/* Activate deep debug print */
/* #undef DEBUG_TOOL_AS_ERROR */

/* Define to 1 if translation of program messages to the user's native
   language is requested. */
#define ENABLE_NLS 1

/* $GETTEXT_PACKAGE */
#define GETTEXT_PACKAGE "dx7interface"

/* Define to 1 if you have 'alloca', as a function or macro. */
#define HAVE_ALLOCA 1

/* Define to 1 if <alloca.h> works. */
/* #undef HAVE_ALLOCA_H */

/* Define to 1 if you have the Mac OS X function CFLocaleCopyCurrent in the
   CoreFoundation framework. */
/* #undef HAVE_CFLOCALECOPYCURRENT */

/* Define to 1 if you have the Mac OS X function CFPreferencesCopyAppValue in
   the CoreFoundation framework. */
/* #undef HAVE_CFPREFERENCESCOPYAPPVALUE */

/* Define if the GNU dcgettext() function is already present or preinstalled.
   */
#define HAVE_DCGETTEXT 1

/* Define to 1 if you have the declaration of 'M_PI', and to 0 if you don't.
   */
#define HAVE_DECL_M_PI 0

/* Define to 1 if you have the <dlfcn.h> header file. */
/* #undef HAVE_DLFCN_H */

/* Define to 1 if you have the <fcntl.h> header file. */
#define HAVE_FCNTL_H 1

/* Define if the GNU gettext() function is already present or preinstalled. */
#define HAVE_GETTEXT 1

/* Define if you have the iconv() function and it works. */
#define HAVE_ICONV 1

/* Define to 1 if you have the <inttypes.h> header file. */
#define HAVE_INTTYPES_H 1

/* Define to 1 if you have the <iostream> header file. */
#define HAVE_IOSTREAM 1

/* Define to 1 if you have the <libintl.h> header file. */
#define HAVE_LIBINTL_H 1

/* Define to 1 if you have the 'pow' function. */
#define HAVE_POW 1

/* Define to 1 if you have the 'setlocale' function. */
#define HAVE_SETLOCALE 1

/* Define to 1 if stdbool.h conforms to C99. */
#define HAVE_STDBOOL_H 1

/* Define to 1 if you have the <stdint.h> header file. */
#define HAVE_STDINT_H 1

/* Define to 1 if you have the <stdio.h> header file. */
#define HAVE_STDIO_H 1

/* Define to 1 if you have the <stdlib.h> header file. */
#define HAVE_STDLIB_H 1

/* Define to 1 if you have the <strings.h> header file. */
#define HAVE_STRINGS_H 1

/* Define to 1 if you have the <string.h> header file. */
#define HAVE_STRING_H 1

/* Define to 1 if you have the <sys/stat.h> header file. */
#define HAVE_SYS_STAT_H 1

/* Define to 1 if you have the <sys/types.h> header file. */
#define HAVE_SYS_TYPES_H 1

/* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_UNISTD_H 1

/* Define to 1 if the system has the type '_Bool'. */
/* #undef HAVE__BOOL */

/* Define to the sub-directory where libtool stores uninstalled libraries. */
#define LT_OBJDIR ".libs/"

/* Name of package */
#define PACKAGE "dx7interface"

/* Define to the address where bug reports for this package should be sent. */
#define PACKAGE_BUGREPORT "benhaim_jerome@yahoo.fr"

/* Define to the full name of this package. */
#define PACKAGE_NAME "dx7interface"

/* Define to the full name and version of this package. */
#define PACKAGE_STRING "dx7interface 1.0.0"

/* Define to the one symbol short name of this package. */
#define PACKAGE_TARNAME "dx7interface"

/* Define to the home page for this package. */
#define PACKAGE_URL ""

/* Define to the version of this package. */
#define PACKAGE_VERSION "1.0.0"

/* Define if the platform is Linux */
/* #undef PLATFORM_LINUX */

/* Define if the platform is Windows */
#define PLATFORM_WINDOWS 1

/* Data directory */
#define PROGRAMNAME_DATA_DIR "usr/share/dx7interface/1.0.0/data/"

/* Images directory */
#define PROGRAMNAME_IMG_DIR "usr/share/dx7interface/1.0.0/data/images/"

/* Translation directory */
#define PROGRAMNAME_LOCALEDIR "usr/share/locale/"

/* Module directory */
#define PROGRAMNAME_MOD_DIR "usr/share/dx7interface/1.0.0/modules/"

/* UI directory */
#define PROGRAMNAME_UI_DIR "usr/share/dx7interface/1.0.0/data/ui/"

/* If using the C implementation of alloca, define if you know the
   direction of stack growth for your system; otherwise it will be
   automatically deduced at runtime.
	STACK_DIRECTION > 0 => grows toward higher addresses
	STACK_DIRECTION < 0 => grows toward lower addresses
	STACK_DIRECTION = 0 => direction of growth unknown */
/* #undef STACK_DIRECTION */

/* Define to 1 if all of the C89 standard headers exist (not just the ones
   required in a freestanding environment). This macro is provided for
   backward compatibility; new code need not use it. */
#define STDC_HEADERS 1

/* Version number of package */
#define VERSION "1.0.0"

/* Define for Solaris 2.5.1 so the uint8_t typedef from <sys/synch.h>,
   <pthread.h>, or <semaphore.h> is not used. If the typedef were allowed, the
   #define below would cause a syntax error. */
/* #undef _UINT8_T */

/* Define __ALSA__ for ALSA specific code */
/* #undef __ALSA__ */

/* Define __JUCE__ for JUCE-specific code */
/* #undef __JUCE__ */

/* Define __RtMidi__ for RtMidi-specific code */
#define __RtMidi__ 1

/* Define to '__inline__' or '__inline' if that's what the C compiler
   calls it, or to nothing if 'inline' is not supported under any name.  */
#ifndef __cplusplus
/* #undef inline */
#endif

/* Define to the type of a signed integer type of width exactly 8 bits if such
   a type exists and the standard includes do not define it. */
/* #undef int8_t */

/* Define as 'unsigned int' if <stddef.h> doesn't define. */
/* #undef size_t */

/* Define to the type of an unsigned integer type of width exactly 8 bits if
   such a type exists and the standard includes do not define it. */
/* #undef uint8_t */
