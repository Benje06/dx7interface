#!/bin/sh
# Run this to generate all the initial makefiles, etc.
srcdir=`dirname $0`
test -z "$srcdir" && srcdir='.'
DIE=0

#function check_system(){
    if test "${MSYSTEM}" = "UCRT64" ; then
        prefix_path="/ucrt64"
    else
        prefix_path="/usr"
    fi
#}

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
	cp ${prefix_path}/share/aclocal/* "${srcdir}/m4"
}

(test -d "${srcdir}/config")||{
	mkdir "${srcdir}/config"
}

rep=`pwd`
pname=${rep##/*/}
ACLOCAL_FLAGS="-I ${prefix_path}/share/aclocal -I /usr/share/gettext/m4 ${ACLOCAL_FLAGS}"

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

(grep "^AM_GNU_GETTEXT" $srcdir/configure.ac >/dev/null) && {
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

(grep "^IT_PROG_INTLTOOL" $srcdir/configure.ac >/dev/null) && {
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
            if grep "^AM_GNU_GETTEXT" configure.ac >/dev/null; then
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
            if grep "^IT_PROG_INTLTOOL" configure.ac >/dev/null; then
                echo "Running intltoolize..."
                intltoolize --copy --force --automake
            fi
            if grep "^AM_PROG_XML_I18N_TOOLS" configure.ac >/dev/null; then
                echo "Running xml-i18n-toolize..."
                xml-i18n-toolize --copy --force --automake
            fi

            echo "Running aclocal $aclocalinclude ..."
            aclocal ${aclocalinclude}

            echo ""
            echo "Running autoreconf ..."
            autoreconf --force --install -I config -I m4 --verbose --warnings=all "$srcdir"

            if grep "^AC_CONFIG_HEADER" configure.ac >/dev/null; then
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
            echo ""
            echo "Running intltool-update & gettext ..."
            cd po
            echo ""
            echo "/*** Extract string to header files (intltool-update -s)***/"
            intltool-update -x -s
            echo ""
            echo "/*** Analyse potfiles (intltool-update -m) ***/"
            intltool-update -x -m
            echo ""
            echo "/*** generate $pname.pot ***/"
            intltool-update -x -p -g $pname
            sed -i 's/CHARSET/UTF-8/' ${pname}.pot
            echo ""
            echo "/*** Extract string from ui files (intltool-extract --type=gettext/glade --update)***/"
            for ui in $(cat POTFILES.in|grep "\.ui$") ; do
                intltool-extract --update ../${ui} --type=gettext/glade
                xgettext -o ${pname}.pot --from-code=utf-8 -j -a ../${ui}.h ${pname}.pot
            done
            if test -f LINGUAS && test -r LINGUAS; then
                for i in $(cat LINGUAS | grep -v '#') ; do
                    if [ ! -f ${i}.po ]; then
                        msginit --no-translator --locale ${i} -i ${pname}.pot
                        intltool-update -d -g $pname ${i}
                    else
                        msgmerge ${i}.po ${pname}.pot --output-file=${i}.po
                    fi
                done
            fi
            cd ..
		)
	fi
done


if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi


 
