#                                               -*- Autoconf -*-
# Process this file with autoconf to produce a configure script.
# tools :

AC_PREREQ([2.72])
AC_REVISION([$REVISION: 1.0.0$])
AC_INIT([draw],[1.0.0],[benhaim_jerome@yahoo.fr])
dnl ================================================================
dnl INFO PACKAGE
dnl ================================================================
AC_SUBST([SO_VERSION], [1:0:0])
AC_SUBST([API_VERSION], [1.0.0])
AC_SUBST([LIB_VERSION], [0.0.1])
AC_SUBST([MASTER_LOADER], [gxinterface])
AC_SUBST([MASTER_LOADER_VERSION], [1.0.0])
PROGRAMNAME_LIBDIR=[${datadir}/$PACKAGE_NAME/$PACKAGE_VERSION/]
AC_SUBST([libdir],[$PROGRAMNAME_LIBDIR])


AC_CONFIG_AUX_DIR([config])

dnl ================================================================
dnl checks for programs
dnl ================================================================
AM_INIT_AUTOMAKE
AM_MAINTAINER_MODE
AC_CONFIG_MACRO_DIR([m4])
AC_CONFIG_SRCDIR([config.h.in])
AC_CONFIG_HEADERS([config.h])
AC_CONFIG_SRCDIR([src])

LT_PREREQ([1.0.0])
LT_INIT
AC_LANG([C++])

AC_PROG_CC
AC_PROG_CXX
AC_PROG_CXXCPP
AC_PROG_CPP
AC_PROG_AWK
AC_PROG_LN_S
AC_PROG_RANLIB
AC_PROG_INSTALL
AC_PROG_MAKE_SET
m4_ifdef([AC_PROG_AR], [AC_PROG_AR])

dnl ================================================================
dnl Check for libraries
dnl ================================================================
AC_SEARCH_LIBS([strerror],[cposix])
dnl ================================================================
dnl checks for header files
dnl ================================================================
# Autoupdate added the next two lines to ensure that your configure
# script's behavior did not change.  They are probably safe to remove.
AC_CHECK_INCLUDES_DEFAULT
AC_PROG_EGREP

AC_CHECK_HEADERS([iostream])
AC_CHECK_HEADERS([stdlib.h])
AC_CHECK_HEADERS([libintl.h])

dnl ================================================================
dnl checks for types
dnl ================================================================
AC_HEADER_STDBOOL
AC_TYPE_UINT8_T
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
#LT_INIT([dlopen],[win32-dll])
AC_FUNC_ALLOCA
AC_TYPE_SIZE_T
AC_CHECK_FUNCS([pow])
AC_FUNC_ERROR_AT_LINE

pkg_modules="alsa glibmm-2.68 gtkmm-4.0 gthread-2.0 gxinterface cairomm-1.16"
PKG_CHECK_MODULES(DEPS, [$pkg_modules])
AC_SUBST(DEPS_CFLAGS)
AC_SUBST(DEPS_LIBS)

dnl ================================================================
dnl checks for system services
dnl ================================================================
dnl ================================================================
dnl Check for windows
dnl ================================================================
case $host_os in
  *mingw* | pw32* | cygwin*)
    platform_win32=yes
    ;;
  *)
    platform_win32=no
    ;;
esac
AM_CONDITIONAL(PLATFORM_WIN32, test x"$platform_win32" = "xyes")

case $host_os in
  *mingw*)
    native_win32=yes
    ;;
  *)
    native_win32=no
    ;;
esac
AM_CONDITIONAL(NATIVE_WIN32, test x"$native_win32" = "xyes")

if test "$native_win32" = "yes"; then
  AC_CHECK_TOOL(WINDRES, windres, no)
  if test "$WINDRES" = no; then
    AC_MSG_ERROR([*** Could not find an implementation of windres in your PATH.])
    
  fi
fi

dnl ================================================================
dnl Check for GDK Quartz and MacOSX integration package 
dnl ================================================================
_gdk_tgt=`$PKG_CONFIG --variable=target gdk-3.0`
AM_CONDITIONAL([GDK_TARGET_QUARTZ], [test x$_gdk_tgt = xquartz])
if test "x$_gdk_tgt" = xquartz; then 
   PKG_CHECK_MODULES(IGE_MAC, ige-mac-integration)

   IGE_MAC_BUNDLE_FLAG=

   AC_ARG_ENABLE(mac-bundle,
      AS_HELP_STRING([--enable-mac-bundle], [enable mac bundling]),
      build_bundle=yes, build_bundle=no)

   if test "x$build_bundle" = xyes; then
      AC_MSG_NOTICE([enableing mac bundle..])

      IGE_MAC_BUNDLE_FLAG=-DMAC_BUNDLE
   fi      

   AC_SUBST(IGE_MAC_BUNDLE_FLAG)
   AC_SUBST(IGE_MAC_LIBS)
   AC_SUBST(IGE_MAC_CFLAGS)
fi

dnl ================================================================
dnl Gnome init
dnl ================================================================
dnl GNOME_COMMON_INIT
dnl GNOME_DEBUG_CHECK
dnl GNOME_COMPILE_WARNINGS([maximum])
dnl GNOME_MAINTAINER_MODE_DEFINES

dnl ================================================================
dnl Gettext stuff.
dnl ================================================================
AC_CHECK_FUNCS([setlocale])
GETTEXT_PACKAGE=$PACKAGE_NAME
AC_SUBST(GETTEXT_PACKAGE)
AC_DEFINE_UNQUOTED(GETTEXT_PACKAGE, "$GETTEXT_PACKAGE",[$GETTEXT_PACKAGE])
ALL_LINGUAS="de en es fr jap languagecode ru"
dnl getoff 		"xgettext: Chaîne non-ASCII repérée à [FILE].cc:[n°_line].
dnl Veuillez spécifier l'encodage de la source avec l'option « --from-code »."
AM_XGETTEXT_OPTION([--from-code=utf-8])
dnl IT_PROG_INTLTOOL
AM_GLIB_GNU_GETTEXT

AC_ARG_ENABLE([dev],
[  --enable-dev    Replace path with dev directory],
[case "${enableval}" in
  yes) dev=true ;;
  no)  dev=false ;;
  *) AC_MSG_ERROR([bad value ${enableval} for --enable-dev]) ;;
esac],[dev=false])
AM_CONDITIONAL([DEV], [test x$dev = xtrue])

AM_COND_IF([DEV],[
PROGRAMNAME_LOCALEDIR=[~/dev/git/gtk4/$PACKAGE_NAME/locale]
PROGRAMNAME_UI_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/src/]
PROGRAMNAME_MOD_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/src/]
PROGRAMNAME_IMG_DIR=[~/dev/git/gtk4/$PACKAGE_NAME/data/images/]
],[
PROGRAMNAME_LOCALEDIR=[${datadir}/locale/]
PROGRAMNAME_UI_DIR=[${datadir}/$PACKAGE_NAME/$PACKAGE_VERSION/]
PROGRAMNAME_MOD_DIR=[${datadir}/$PACKAGE_NAME/$PACKAGE_VERSION/]
PROGRAMNAME_IMG_DIR=[${datadir}/$PACKAGE_NAME/$PACKAGE_VERSION/images/]
])

AC_SUBST([PROGRAMNAME_LOCALEDIR])
AC_SUBST([PROGRAMNAME_UI_DIR])
AC_SUBST([PROGRAMNAME_MOD_DIR])
AC_SUBST([PROGRAMNAME_IMG_DIR])

AC_SUBST([AM_CXXFLAGS])
AC_SUBST([AM_CFLAGS])
AC_SUBST([AM_CPPFLAGS])
AC_SUBST([AM_LDFLAGS])

AC_CONFIG_FILES([Makefile
                 src/Makefile
                 src/draw.pc
])
AC_OUTPUT

echo "
Configuration:

    Source code location:	 ${srcdir}
    Compiler:		 C : ${CC},		C++ : ${CXX}
    GNOME UI Widgets:		 ${have_gnome}

    CFLAGS:  ${CFLAGS}
    CPPFLAGS: ${CPPFLAGS}
    CXXFLAGS: ${CXXFLAGS}
    LDFLAGS: ${LDFLAGS}

    PROGRAMNAME_LOCALEDIR: ${PROGRAMNAME_LOCALEDIR}
    PROGRAMNAME_UI_DIR: ${PROGRAMNAME_UI_DIR}
    PROGRAMNAME_MOD_DIR: ${PROGRAMNAME_MOD_DIR}
    PROGRAMNAME_IMG_DIR: ${PROGRAMNAME_IMG_DIR}

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
