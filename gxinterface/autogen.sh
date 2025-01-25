#!/bin/sh
# Run this to generate all the initial makefiles, etc.
srcdir=`dirname $0`
test -z "$srcdir" && srcdir='.'
DIE=0

if test "x`cd "${srcdir}" 2>/dev/null && pwd`" != "x`pwd`"
#`cd "${srcdir}" 2>/dev/null && pwd` != `pwd` ]]
then
    echo "This script must be executed directly from the source directory."
    DIE=1
fi

rm -f config.cache acconfig.h

(test -d "${srcdir}/m4")||{
	mkdir "${srcdir}/m4"
	echo "copy all m4 file from /usr/share/aclocal/ to m4 directory"
	cp /usr/share/aclocal/* "${srcdir}/m4"
}

(test -d "${srcdir}/config")||{
	mkdir "${srcdir}/config"
}

rep=`pwd`
pname=${rep##/*/}
ACLOCAL_FLAGS="-I /usr/share/aclocal $ACLOCAL_FLAGS"

if [ -n "$GNOME2_DIR" ]; then
	ACLOCAL_FLAGS="-I $GNOME2_DIR/share/aclocal $ACLOCAL_FLAGS"
	LD_LIBRARY_PATH="$GNOME2_DIR/lib:$LD_LIBRARY_PATH"
	PATH="$GNOME2_DIR/bin:$PATH"
	export PATH
	export LD_LIBRARY_PATH
fi

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory "\`$srcdir\'" does not look like the"
    echo " top-level package directory"
    exit 1
}

(autoconf --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`autoconf' installed."
  echo "Download the appropriate package for your distribution,"
  echo "or get the source tarball at ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}


(grep "^AM_PROG_XML_I18N_TOOLS" $srcdir/configure.ac >/dev/null) && {
  (xml-i18n-toolize --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`xml-i18n-toolize' installed."
    echo "You can get it from:"
    echo "  ftp://ftp.gnome.org/pub/GNOME/"
    DIE=1
  }
}

(grep "^AM_PROG_LIBTOOL" $srcdir/configure.ac >/dev/null) && {
  (libtool --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`libtool' installed."
    echo "You can get it from: ftp://ftp.gnu.org/pub/gnu/"
    DIE=1
  }
}

(grep "^AM_GLIB_GNU_GETTEXT" $srcdir/configure.ac >/dev/null) && {
  (grep "sed.*POTFILES" $srcdir/configure.ac) > /dev/null || \
  (glib-gettextize --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`glib' installed."
    echo "You can get it from: ftp://ftp.gtk.org/pub/gtk"
    DIE=1
  }
}

(automake --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: You must have \`automake' installed."
  echo "You can get it from: ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
  NO_AUTOMAKE=yes
}


# if no automake, don't bother testing for aclocal
test -n "$NO_AUTOMAKE" || (aclocal --version) < /dev/null > /dev/null 2>&1 || {
  echo
  echo "**Error**: Missing \`aclocal'.  The version of \`automake'"
  echo "installed doesn't appear recent enough."
  echo "You can get automake from ftp://ftp.gnu.org/pub/gnu/"
  DIE=1
}

(grep "^AC_PROG_INTLTOOL" $srcdir/configure.ac >/dev/null) && {
  (intltoolize --version) < /dev/null > /dev/null 2>&1 || {
    echo
    echo "**Error**: You must have \`intltool' installed."
    echo "You can get it from:"
    echo "  ftp://ftp.gnome.org/pub/GNOME/"
    DIE=1
  }
}
if test "$DIE" -eq 1; then
  exit 1
fi

if test -z "$*"; then
  echo "**Warning**: I am going to run \`configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo \`$0\'" command line."
  echo
fi

case $CC in
xlc )
  am_opt=--include-deps;;
g++ )
  am_opt=-Wall;;
esac

for coin in `find $srcdir -name configure.ac -print`
do
	dr=`dirname $coin`
	if test -f $dr/NO-AUTO-GEN; then
		echo skipping $dr -- flagged as no auto-gen
	else
		echo processing $dr
		( cd $dr
		aclocalinclude="$ACLOCAL_FLAGS"
		if grep "^AM_GLIB_GNU_GETTEXT" configure.ac >/dev/null; then
			echo "Creating $dr/aclocal.m4 ..."
			test -r $dr/aclocal.m4 || touch $dr/aclocal.m4
			echo "Running glib-gettextize...  Ignore non-fatal messages."
			echo "no" | glib-gettextize --force --copy
			echo "Making $dr/aclocal.m4 writable ..."
			test -r $dr/aclocal.m4 && chmod u+w $dr/aclocal.m4
		fi
        	if grep "^AM_PROG_LIBTOOL" configure.ac >/dev/null; then
			if test -z "$NO_LIBTOOLIZE" ; then
				echo "Running libtoolize..."
				libtoolize --force --copy
			fi
		fi
		if grep "^AC_PROG_INTLTOOL" configure.ac >/dev/null; then
			echo "Running intltoolize..."
			intltoolize --copy --force --automake
		fi
		if grep "^AM_PROG_XML_I18N_TOOLS" configure.ac >/dev/null; then
			echo "Running xml-i18n-toolize..."
			xml-i18n-toolize --copy --force --automake
		fi

		echo "Running aclocal $aclocalinclude ..."
		aclocal $aclocalinclude

		echo ""
		echo "Running autoreconf ..."
		autoreconf --force --install -I config -I m4
		if grep "^AM_CONFIG_HEADER" configure.ac >/dev/null; then
			echo ""
			echo "Running autoheader..."
			autoheader
		fi
		#autoconf
		echo ""
		echo "Running automake --gnu $am_opt ..."
		automake --add-missing --gnu $am_opt

		#autoscan
		echo ""
		echo "Running autoscan ..."
		autoscan

		#autoupdate
		echo ""
		echo "Running autoupdate ..."
		autoupdate

		# intltool
		# i18n
		update_languagecode(){
			# xgettext
			echo ""
			echo "/*** Running xgettext  ***/"
			cd src
				# s sort, F by file,c add comment,a extract all, f from list of po file, o output file,
				if test -f ../po/languagecode.po && test -w ../po/languagecode.po ;then
					echo "Running xgettext  -j -F -c -a -n --no-wrap -f ../po/POTFILES --from-code=utf-8 -o ../po/languagecode.po --omit-header *.cc *.h..."
					xgettext -j -F -c -a -n --no-wrap -f ../po/POTFILES --from-code=utf-8 -o ../po/languagecode.po --omit-header *.cc *.h
				else
					echo "Running xgettext -F -c -a -n --no-wrap --from-code=utf-8 -o ../po/languagecode.po --omit-header *.cc *.h..."
					xgettext -F -c -a -n --no-wrap --from-code=utf-8 -o ../po/languagecode.po --omit-header *.cc *.h
				fi
			cd ..
		}
		echo ""
		echo "Running intltool-update & gettext ..."
		update_languagecode
		cd po
			echo ""
			echo "/*** Extract string to header files (intltool-update -s)***/"
			intltool-update -x -s
			echo ""
			echo "/*** Analyse potfiles (intltool-update -m) ***/"
			intltool-update -x -m
			if test -f $pname.pot && test -w $pname.pot ; then
				echo ""
				echo "/*** $pname.pot present merge with languagecode.po ***/"
				intltool-update -x -d -g $pname -o languagecode.po languagecode
				echo "/*** Create new and merge with languagecode.po ***/"
				intltool-update -x -p -g $pname
				intltool-update -x -d -g $pname -o languagecode.po languagecode
			else
				echo ""
				echo "/*** $pname.pot not present ***/"
				intltool-update -x -g $pname languagecode
			fi
			echo ""
			echo "/*** Extract string to header files (intltool-update -s) ***/"
			intltool-update -x -s
			if test -f LINGUAS && test -r LINGUAS; then
    			for i in $(cat LINGUAS | grep -v '#') ; do
    			    msginit --no-translator --locale ${i} -i ${pname}.pot
    			done
    		fi
			cd ..
		)
	fi
done

conf_flags="--enable-maintainer-mode"

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi



