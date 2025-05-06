#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# tools :
AC_PREREQ([2.72])
LT_PREREQ([1.0.0])
AC_REVISION([$REVISION: 1.0.0$])
AC_INIT([gxinterface],[1.0.0],[benhaim_jerome@yahoo.fr])
dnl ================================================================
dnl INFO PACKAGE
dnl ================================================================
AC_SUBST([SO_VERSION], [1:0:0])
AC_SUBST([API_VERSION], [1.0.0])
AC_SUBST([LIB_VERSION], [0.0.1])

AC_CONFIG_AUX_DIR([config])
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([src])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([config.h.in])

dnl ================================================================
dnl checks for programs
dnl ================================================================
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE

m4_ifdef([AM_PROG_AR], [AM_PROG_AR])
m4_ifdef([AC_PROG_AR], [AC_PROG_AR])

AC_CANONICAL_HOST
AS_CASE([$host_os],
    [linux*], [
        pkg_modules="glibmm-2.68 giomm-2.68 gtkmm-4.0 gthread-2.0 cairomm-1.16 sigc++-3.0 pangomm-2.48 fontconfig"
        AC_DEFINE([PLATFORM_LINUX], [1], [Define if the platform is Linux])
        platform_linux=yes
        platform_win32=no
    ],
    [mingw*|windows*|pw32*|cygwin*], [
        pkg_modules="glibmm-2.68 giomm-2.68 gtkmm-4.0 gthread-2.0 cairomm-1.16 sigc++-3.0 pangomm-2.48 fontconfig"
        AC_DEFINE([PLATFORM_WINDOWS], [1], [Define if the platform is Windows])
        platform_win32=yes
        platform_linux=no
        m4_define([lt_option], [win32-dll])
        m4_define([win32], [])
    ],
    [*], [
        AC_MSG_ERROR([Unsupported operating system: $host_os])
    ]
)

AM_CONDITIONAL([PLATFORM_WINDOWS], [test x$platform_win32 = xyes])
AM_CONDITIONAL([PLATFORM_LINUX], [test x$platform_linux = xyes])

ifdef('lt_option',
    [LT_INIT([lt_option])],
    [LT_INIT([shared])])

AC_C_INLINE
AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MAKE_SET

AC_LANG([C++])
AC_LANG_COMPILER([C++])

AC_SUBST(CXXFLAGS)

dnl ================================================================
dnl Check for libraries
dnl ================================================================
AC_SEARCH_LIBS([strerror],[cposix])
dnl ================================================================
dnl checks for header files
dnl ================================================================
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS([iostream])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([libintl.h])
AC_CHECK_HEADERS([fcntl.h])
AC_CHECK_HEADERS([unistd.h])

dnl ================================================================
dnl checks for types
dnl ================================================================
AC_HEADER_STDBOOL
AC_TYPE_INT8_T
AC_TYPE_UINT8_T
AC_TYPE_SIZE_T
dnl ================================================================
dnl checks for structures
dnl ================================================================
dnl ================================================================
dnl checks for compiler characteristics
dnl ================================================================
AC_LANG_COMPILER_REQUIRE
dnl ================================================================
dnl checks for library functions
dnl ================================================================
AC_FUNC_ALLOCA
AC_FUNC_ERROR_AT_LINE
AM_COND_IF([PLATFORM_WINDOWS],[
    AC_CHECK_FUNCS([localtime_s])
],[
    AC_CHECK_FUNCS([localtime_r])
])

PKG_CHECK_MODULES([DEPS], [$pkg_modules])
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_CPPFLAGS)
AC_SUBST(DEPS_LIBS)

dnl ================================================================
dnl checks for system services
dnl ================================================================
dnl ================================================================
dnl Check for windows
dnl ================================================================
AM_COND_IF([PLATFORM_WINDOWS],[
    AC_CHECK_TOOL([WINDRES], [windres], no)
    if test "x$WINDRES" = "xno"; then
        AC_MSG_ERROR([Could not find an implementation of windres in your PATH.])
    fi
])
dnl console activate the console for windows
AC_ARG_ENABLE([console],
[  --enable-console    Turn on console for windows ],
[case "${enableval}" in
  0|no)  console=false ;;
  1|yes) console=true ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-console]) ;;
esac],[console=false])
AM_CONDITIONAL([WINDOWS_CONSOLE], [test x$console = xtrue])
AM_COND_IF([WINDOWS_CONSOLE],[
],[
if test x$platform_win32 == xyes; then
CXXFLAGS="$CXXFLAGS -mwindows"
AC_SUBST(CXXFLAGS)
fi
])
dnl ================================================================
dnl Gettext stuff.
dnl ================================================================
IT_PROG_INTLTOOL([0.35.0])
AC_CHECK_FUNCS([setlocale])
GETTEXT_PACKAGE=$PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[$GETTEXT_PACKAGE])
AM_XGETTEXT_OPTION([--from-code=utf-8])
AM_GNU_GETTEXT([external])
AM_GNU_GETTEXT_VERSION([0.19.8])

dnl ================================================================
dnl Develloper stuff
dnl ================================================================
dnl debug set the message format define in debug.h
AC_ARG_ENABLE([log],
 [AS_HELP_STRING([--enable-log@<:@=LEVEL@:>@],
    [Set debug log format (0=none, 1=funct name, 2=class::funct + file + line number) @<:@default=no@:>@])],
[case "${enableval}" in
  0|no)  debug=false; debug_deep=false ;;
  1|yes) debug=true; debug_deep=false;;
  2|deep) debug=true; debug_deep=true;;
  *) AC_MSG_ERROR([Invalid debug log format: use 0/1/2 or no/yes/deep]) ;;
esac],[debug=false; debug_deep=false])

AM_CONDITIONAL([DEBUG_TOOL], [test x$debug = xtrue])
AM_CONDITIONAL([DEBUG_TOOL_AS_ERROR], [test x$debug_deep = xtrue])

AM_COND_IF([DEBUG_TOOL],[
AC_DEFINE([DEBUG_TOOL], [], [Activate debug print])
],[])
AM_COND_IF([DEBUG_TOOL_AS_ERROR],[
AC_DEFINE([DEBUG_TOOL_AS_ERROR], [], [Activate deep debug print])
],[])

AC_ARG_ENABLE([dev],
[  --enable-dev    Turn on folder from dev directory ],
[case "${enableval}" in
  yes) dev=true ;;
  no)  dev=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-dev]) ;;
esac],[dev=false])
AM_CONDITIONAL([DEV], [test x$dev = xtrue])

AM_COND_IF([DEV],[
PROGRAMNAME_LOCALEDIR=[~/dev/git/gtk4/$PACKAGE_NAME/po]
PROGRAMNAME_DATA_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/data/]
PROGRAMNAME_IMG_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/data/images]
PROGRAMNAME_UI_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/data/ui/]
PROGRAMNAME_MOD_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/src/]
],[
PROGRAMNAME_LOCALEDIR=[${datadir}/locale]
PROGRAMNAME_DATA_DIR=[${datadir}/$PACKAGE_NAME/$VERSION/data/]
PROGRAMNAME_IMG_DIR=[${datadir}/$PACKAGE_NAME/$VERSION/data/images/]
PROGRAMNAME_UI_DIR=[${datadir}/$PACKAGE_NAME/$VERSION/data/ui/]
PROGRAMNAME_MOD_DIR=[${datadir}/$PACKAGE_NAME/modules/]
])

AM_COND_IF([PLATFORM_WINDOWS],[
PROGRAMNAME_LOCALEDIR=[usr/share/locale/]
PROGRAMNAME_DATA_DIR=[usr/share/$PACKAGE_NAME/$VERSION/data/]
PROGRAMNAME_IMG_DIR=[usr/share/$PACKAGE_NAME/$VERSION/data/images/]
PROGRAMNAME_UI_DIR=[usr/share/$PACKAGE_NAME/$VERSION/data/ui/]
PROGRAMNAME_MOD_DIR=[usr/share/$PACKAGE_NAME/$VERSION/modules/]

AC_DEFINE_UNQUOTED([PROGRAMNAME_LOCALEDIR], ["$PROGRAMNAME_LOCALEDIR"], [Translation directory])
AC_DEFINE_UNQUOTED([PROGRAMNAME_DATA_DIR], ["$PROGRAMNAME_DATA_DIR"], [Data directory])
AC_DEFINE_UNQUOTED([PROGRAMNAME_IMG_DIR], ["$PROGRAMNAME_IMG_DIR"], [Images directory])
AC_DEFINE_UNQUOTED([PROGRAMNAME_UI_DIR], ["$PROGRAMNAME_UI_DIR"], [UI directory])
AC_DEFINE_UNQUOTED([PROGRAMNAME_MOD_DIR], ["$PROGRAMNAME_MOD_DIR"], [Module directory])
])

AC_SUBST(PROGRAMNAME_LOCALEDIR)
AC_SUBST(PROGRAMNAME_DATA_DIR)
AC_SUBST(PROGRAMNAME_IMG_DIR)
AC_SUBST(PROGRAMNAME_UI_DIR)
AC_SUBST(PROGRAMNAME_MOD_DIR)

dnl ================================================================
dnl Flags
dnl ================================================================

AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([Makefile
                po/Makefile.in
                lib/Makefile
                src/Makefile
                data/Makefile
                data/images/Makefile
                data/ui/Makefile
                data/launcher/Makefile
                data/launcher/gxinterface.desktop
                lib/gxinterface.pc
])
AC_OUTPUT

echo "

Configuration:

  Source code location:	 ${srcdir}
  libdir location:	     ${libdir}
  Compiler:		 C : ${CC}		C++ : ${CXX}

  env:      ${host_os}
  Windows:  ${platform_win32}   console:    ${console}
  Linux:    ${platform_linux}
  Log format:   minimal: ${debug}
                extended:  ${debug_deep}

  CPPFLAGS: ${CPPFLAGS}
  CFLAGS:  ${CFLAGS}
  CXXFLAGS: ${CXXFLAGS}

  DEPS_CPPFLAGS: ${DEPS_CPPFLAGS}
  DEPS_CFLAGS: ${DEPS_CFLAGS}
  DEPS_CXXFLAGS: ${DEPS_CXXFLAGS}

  LDFLAGS: ${LDFLAGS}
  DEPS_LIBS: ${DEPS_LIBS}

  GETTEXT_PACKAGE: ${GETTEXT_PACKAGE}
    
  PROGRAMNAME_LOCALEDIR: ${PROGRAMNAME_LOCALEDIR}
  PROGRAMNAME_UI_DIR: ${PROGRAMNAME_UI_DIR}
  PROGRAMNAME_MOD_DIR: ${PROGRAMNAME_MOD_DIR}
  PROGRAMNAME_IMG_DIR: ${PROGRAMNAME_IMG_DIR}
  PROGRAMNAME_DATA_DIR: ${PROGRAMNAME_DATA_DIR}
  datadir: ${datadir}
  datarootdir: ${daterootdir}
  pkgdatadir: ${pkgdatadir}
"

: variables='
$ACLOCAL
$ALL_LINGUAS
$AUTOCONF
$AUTOHEADER
$AUTOMAKE 
$AWK
$BASH
$BASHOPTS
$BASH_ARGC
$BASH_ARGV
$CATALOGS
$CATOBJEXT
$CC
$CCDEPMODE
$CFLAGS
$CONFIG_SHELL
$CONFIG_STATUS
$CPP
$CXX
$CXXCPP
$CXXDEPMODE
$CXXFLAGS
$CYGPATH_W
$DATADIRNAME
$DBUS_SESSION_BUS_ADDRESS
$DEFAULTS_PATH
$DEFS
$DEPDIR
$DEPS_CFLAGS
$DEPS_LIBS
$DESKTOP_SESSION
$DIRSTACK
$DISPLAY
$DUALCASE
$ECHO_C
$ECHO_N
$ECHO_T
$EGREP
$EUID
$EXEEXT
$GCC
$GDK_TARGET_QUARTZ_FALSE
$GDK_TARGET_QUARTZ_TRUE
$GDMSESSION
$GDM_KEYBOARD_LAYOUT
$GDM_LANG
$GETTEXT_PACKAGE
$GMOFILES
$GMSGFMT
$GNOME_DESKTOP_SESSION_ID
$GNOME_KEYRING_CONTROL
$GNOME_KEYRING_PID
$GREP
$GROUPS
$GTK_MODULES
$GXX
$HISTCONTROL
$HOME
$HOSTNAME
$HOSTTYPE
$IFS
$INSTALL
$INSTALL_DATA
$INSTALL_PROGRAM
$INSTALL_SCRIPT
$INSTALL_STRIP_PROGRAM
$INSTOBJEXT
$INTLLIBS
$LANG
$LANGUAGE
$LC_ALL
$LESSCLOSE
$LESSOPEN
$LIBOBJS
$LIBS
$LINENO
$LINGUAS
$LN_S
$LOGNAME
$LS_COLORS
$LTLIBOBJS
$MACHTYPE
$MAINT
$MAINTAINER_MODE_FALSE
$MAINTAINER_MODE_TRUE
$MAKEFLAGS
$MAKEINFO
$MANDATORY_PATH
$MFLAGS
$MISSING
$MKDIR_P
$MKINSTALLDIRS
$MSGFMT
$MSGFMT_OPTS
$NATIVE_WIN32_FALSE
$NATIVE_WIN32_TRUE
$NEW_LINGUAS
$OBJEXT
$OLDPWD
$OPTERR
$OPTIND
$ORBIT_SOCKETDIR
$OSTYPE
$PACKAGE
$PACKAGE_BUGREPORT
$PACKAGE_NAME
$PACKAGE_STRING
$PACKAGE_TARNAME
$PACKAGE_URL
$PACKAGE_VERSION
$PATH
$PATH_SEPARATOR
$PIPESTATUS
$PKG_CONFIG
$PLATFORM_WIN32_FALSE
$PLATFORM_WIN32_TRUE
$POFILES
$POSIXLY_CORRECT
$POSUB
$PPID
$PROGRAMNAME_LOCALEDIR
$PROGRAMNAME_UI_DIR
$PS1
$PS2
$PS4
$PWD
$RANLIB
$SESSION_MANAGER
$SET_MAKE
$SHELL
$SHELLOPTS
$SHLVL
$SPEECHD_PORT
$SSH_AGENT_PID
$SSH_AUTH_SOCK
$TERM
$UID
$USER
$USERNAME
$USE_MAINTAINER_MODE
$USE_NLS
$VERSION
$WINDOWID
$XAUTHORITY
$XDG_CONFIG_DIRS
$XDG_DATA_DIRS
$XDG_SESSION_COOKIE
$XGETTEXT
$XGETTEXT_EXTRA_OPTIONS
$_
$_am_result
$_gdk_tgt
$_pkg_min_version
$ac_abs_confdir
$ac_arg
$ac_aux_dir
$ac_c_preproc_warn_flag
$ac_c_werror_flag
$ac_cache_corrupted
$ac_clean_files
$ac_clean_files_save
$ac_compile
$ac_compiler
$ac_compiler_gnu
$ac_confdir
$ac_config_commands
$ac_config_files
$ac_config_headers
$ac_config_libobj_dir
$ac_config_status_args
$ac_configure_args
$ac_cpp
$ac_cs_success
$ac_ct_CC
$ac_ct_CXX
$ac_ct_RANLIB
$ac_cv_c_compiler_gnu
$ac_cv_cxx_compiler_gnu
$ac_cv_env_CCC_set
$ac_cv_env_CCC_value
$ac_cv_env_CC_set
$ac_cv_env_CC_value
$ac_cv_env_CFLAGS_set
$ac_cv_env_CFLAGS_value
$ac_cv_env_CPPFLAGS_set
$ac_cv_env_CPPFLAGS_value
$ac_cv_env_CPP_set
$ac_cv_env_CPP_value
$ac_cv_env_CXXCPP_set
$ac_cv_env_CXXCPP_value
$ac_cv_env_CXXFLAGS_set
$ac_cv_env_CXXFLAGS_value
$ac_cv_env_CXX_set
$ac_cv_env_CXX_value
$ac_cv_env_DEPS_CFLAGS_set
$ac_cv_env_DEPS_CFLAGS_value
$ac_cv_env_DEPS_LIBS_set
$ac_cv_env_DEPS_LIBS_value
$ac_cv_env_IGE_MAC_CFLAGS_set
$ac_cv_env_IGE_MAC_CFLAGS_value
$ac_cv_env_IGE_MAC_LIBS_set
$ac_cv_env_IGE_MAC_LIBS_value
$ac_cv_env_LDFLAGS_set
$ac_cv_env_LDFLAGS_value
$ac_cv_env_LIBS_set
$ac_cv_env_LIBS_value
$ac_cv_env_PKG_CONFIG_set
$ac_cv_env_PKG_CONFIG_value
$ac_cv_env_build_alias_set
$ac_cv_env_build_alias_value
$ac_cv_env_host_alias_set
$ac_cv_env_host_alias_value
$ac_cv_env_target_alias_set
$ac_cv_env_target_alias_value
$ac_cv_func_bind_textdomain_codeset
$ac_cv_func_dcgettext
$ac_cv_func_setlocale
$ac_cv_header_inttypes_h
$ac_cv_header_iostream
$ac_cv_header_libintl_h
$ac_cv_header_locale_h
$ac_cv_header_memory_h
$ac_cv_header_stdc
$ac_cv_header_stdint_h
$ac_cv_header_stdlib_h
$ac_cv_header_string_h
$ac_cv_header_strings_h
$ac_cv_header_sys_stat_h
$ac_cv_header_sys_types_h
$ac_cv_header_unistd_h
$ac_cv_objext
$ac_cv_path_EGREP
$ac_cv_path_GMSGFMT
$ac_cv_path_GREP
$ac_cv_path_MSGFMT
$ac_cv_path_XGETTEXT
$ac_cv_path_ac_pt_PKG_CONFIG
$ac_cv_path_install
$ac_cv_path_mkdir
$ac_cv_prog_AWK
$ac_cv_prog_CPP
$ac_cv_prog_CXXCPP
$ac_cv_prog_ac_ct_CC
$ac_cv_prog_ac_ct_CXX
$ac_cv_prog_ac_ct_RANLIB
$ac_cv_prog_cc_c89
$ac_cv_prog_cc_c99
$ac_cv_prog_cc_g
$ac_cv_prog_cc_stdc
$ac_cv_prog_cxx_g
$ac_cv_prog_make_make_set
$ac_cv_search_strerror
$ac_cxx_preproc_warn_flag
$ac_cxx_werror_flag
$ac_dashdash
$ac_default_prefix
$ac_delim
$ac_delim_n
$ac_delim_num
$ac_dir
$ac_env_CCC_set
$ac_env_CCC_value
$ac_env_CC_set
$ac_env_CC_value
$ac_env_CFLAGS_set
$ac_env_CFLAGS_value
$ac_env_CPPFLAGS_set
$ac_env_CPPFLAGS_value
$ac_env_CPP_set
$ac_env_CPP_value
$ac_env_CXXCPP_set
$ac_env_CXXCPP_value
$ac_env_CXXFLAGS_set
$ac_env_CXXFLAGS_value
$ac_env_CXX_set
$ac_env_CXX_value
$ac_env_DEPS_CFLAGS_set
$ac_env_DEPS_CFLAGS_value
$ac_env_DEPS_LIBS_set
$ac_env_DEPS_LIBS_value
$ac_env_IGE_MAC_CFLAGS_set
$ac_env_IGE_MAC_CFLAGS_value
$ac_env_IGE_MAC_LIBS_set
$ac_env_IGE_MAC_LIBS_value
$ac_env_LDFLAGS_set
$ac_env_LDFLAGS_value
$ac_env_LIBS_set
$ac_env_LIBS_value
$ac_env_PKG_CONFIG_set
$ac_env_PKG_CONFIG_value
$ac_env_build_alias_set
$ac_env_build_alias_value
$ac_env_host_alias_set
$ac_env_host_alias_value
$ac_env_target_alias_set
$ac_env_target_alias_value
$ac_exec_ext
$ac_exeext
$ac_ext
$ac_file
$ac_files
$ac_func
$ac_func_search_save_LIBS
$ac_header
$ac_header_compiler
$ac_header_preproc
$ac_hostname
$ac_i
$ac_includes_default
ac_includes_default=
$ac_init_help
$ac_init_version
$ac_install_sh
$ac_last_try
$ac_lib
$ac_libobjs
$ac_link
$ac_link_default
$ac_ls_di
$ac_ltlibobjs
$ac_make
$ac_msg
$ac_must_keep_next
$ac_new_set
$ac_new_val
$ac_objext
$ac_old_set
$ac_old_val
$ac_optarg
$ac_option
$ac_pass
$ac_path_GREP
$ac_path_GREP_found
$ac_precious_vars
ac_precious_vars=
$ac_preproc_ok
$ac_prev
$ac_prog
$ac_pt_PKG_CONFIG
$ac_pwd
$ac_pwd_ls_di
$ac_res
$ac_retval
$ac_rmfiles
$ac_save_CC
$ac_save_CFLAGS
$ac_save_CXXFLAGS
$ac_save_c_werror_flag
$ac_save_cxx_werror_flag
$ac_save_ifs
ac_save_ifs=
$ac_script
$ac_signal
$ac_site_file
$ac_site_file1
$ac_site_file2
$ac_srcdir_defaulted
$ac_status
$ac_subst_files
$ac_subst_vars
ac_subst_vars=
$ac_t
$ac_test_CFLAGS
$ac_test_CXXFLAGS
$ac_tool_prefix
$ac_try
$ac_try_echo
$ac_unique_file
$ac_unrecognized_opts
$ac_unrecognized_sep
$ac_user_opts
ac_user_opts=
$ac_val

$ac_var
$ac_vpsub
ac_vpsub=
$ac_word
$ac_word_re
$ac_write_fail
$am__EXEEXT_FALSE
$am__EXEEXT_TRUE
$am__api_version
$am__fastdepCC_FALSE
$am__fastdepCC_TRUE
$am__fastdepCXX_FALSE
$am__fastdepCXX_TRUE
$am__include
$am__leading_dot
$am__minus_obj
$am__obj
$am__quote
$am__tar
$am__universal
$am__untar
$am_aux_dir
$am_compiler_list
am_compiler_list=
$am_cv_CC_dependencies_compiler_type
$am_cv_CXX_dependencies_compiler_type
$am_cv_val_LC_MESSAGES
$am_depcomp
$am_lf
am_lf=
$am_make
$am_missing_run
$as_ac_Header
$as_basename
$as_cr_LETTERS
$as_cr_Letters
$as_cr_alnum
$as_cr_digits
$as_cr_letters
$as_dir
$as_dirname
$as_executable_p
$as_expr
$as_lineno_1
$as_lineno_1a
$as_lineno_2
$as_lineno_2a
$as_lineno_stack
$as_ln_s
$as_me
$as_mkdir_p
$as_myself
$as_nl
$as_save_IFS
as_save_IFS=
$as_test_x
$as_tr_cpp
$as_tr_sh
$as_unset
$as_val
$as_var
$as_write_fail
$bindir
$build
$cache_file
$cross_compiling
$datadir
$datarootdir
$depcc
$depmode
$desiredlang
$desiredlanguages
$docdir
$dvidir
$exec_prefix
$exit_status
$glib_save_LIBS
$gt_cv_func_dgettext_libc
$gt_cv_func_dgettext_libintl
$gt_cv_func_ngettext_libc
$gt_cv_have_gettext
$host
$htmldir
$i
$includedir
$infodir
$install_sh
$lang
$libdir
$libexecdir
$libintl_extra_libs
$localedir
$localstatedir
$main_mail
$mandir
$mkdir_p
$native_win32
$no_create
$no_recursion
$oldincludedir
$pdfdir
$pkg_cv_DEPS_CFLAGS
$pkg_cv_DEPS_LIBS
$pkg_failed
$pkg_modules
$pkg_name
$pkg_version
$platform_win32
$posrcprefix
$prefix
$presentlang
$program_prefix
$program_suffix
$program_transform_name
$psdir
$sbindir
$sharedstatedir
$silent
$site
$srcdir
$subdirs
$sysconfdir
$target
$useit
$verbose
$x_includes
$x_libraries
$am__EXEEXT_FALSE
$am__EXEEXT_TRUE
$LTLIBOBJS
$LIBOBJS
$AM_LDFLAGS
$AM_CXXFLAGS
$AM_CFLAGS
$AM_CPPFLAGS
$PROGRAMNAME_UI_DIR
$PROGRAMNAME_LOCALEDIR
$MKINSTALLDIRS
$POSUB
$POFILES
$PO_IN_DATADIR_FALSE
$PO_IN_DATADIR_TRUE
$INTLLIBS
$INSTOBJEXT
$GMOFILES
$DATADIRNAME
$CATOBJEXT
$CATALOGS
$XGETTEXT
$GMSGFMT
$MSGFMT_OPTS
$MSGFMT
$USE_NLS
$GETTEXT_PACKAGE
$IGE_MAC_BUNDLE_FLAG
$IGE_MAC_LIBS
$IGE_MAC_CFLAGS
$GDK_TARGET_QUARTZ_FALSE
$GDK_TARGET_QUARTZ_TRUE
$WINDRES
$NATIVE_WIN32_FALSE
$NATIVE_WIN32_TRUE
$PLATFORM_WIN32_FALSE
$PLATFORM_WIN32_TRUE
$DEPS_LIBS
$DEPS_CFLAGS
$PKG_CONFIG
$EGREP
$GREP
$RANLIB
$LN_S
$CPP
$CXXCPP
$am__fastdepCXX_FALSE
$am__fastdepCXX_TRUE
$CXXDEPMODE
$ac_ct_CXX
$CXXFLAGS
$CXX
$am__fastdepCC_FALSE
$am__fastdepCC_TRUE
$CCDEPMODE
$AMDEPBACKSLASH
$AMDEP_FALSE
$AMDEP_TRUE
$am__quote
$am__include
$DEPDIR
$OBJEXT
$EXEEXT
$ac_ct_CC
$CPPFLAGS
$LDFLAGS
$CFLAGS
$CC
$MAINT
$MAINTAINER_MODE_FALSE
$MAINTAINER_MODE_TRUE
$am__untar
$am__tar
$AMTAR
$am__leading_dot
$SET_MAKE
$AWK
$mkdir_p
$MKDIR_P
$INSTALL_STRIP_PROGRAM
$STRIP
$install_sh
$MAKEINFO
$AUTOHEADER
$AUTOMAKE
$AUTOCONF
$ACLOCAL
$VERSION
$PACKAGE
$CYGPATH_W
$am__isrc
$INSTALL_DATA
$INSTALL_SCRIPT
$INSTALL_PROGRAM
$target_alias
$host_alias
$build_alias
$LIBS
$ECHO_T
$ECHO_N
$ECHO_C
$DEFS
$mandir
$localedir
$libdir
$psdir
$pdfdir
$dvidir
$htmldir
$infodir
$docdir
$oldincludedir
$includedir
$localstatedir
$sharedstatedir
$sysconfdir
$datadir
$datarootdir
$libexecdir
$sbindir
$bindir
$program_transform_name
$prefix
$exec_prefix
$PACKAGE_URL
$PACKAGE_BUGREPORT
$PACKAGE_STRING
$PACKAGE_VERSION
$PACKAGE_TARNAME
$PACKAGE_NAME
$PATH_SEPARATOR
$SHELL
$PROGRAMNAME_IMG_DIR
$PROGRAMNAME_MOD_DIR
$MASTER_LOADER
$MASTER_LOADER_VERSION
'

#get with set -x -v
for val in $variables; do 
	if test $val != EOF ; then
		echo $val" = "`eval echo $val`
	fi
done 

