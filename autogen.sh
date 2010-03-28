#!/bin/sh
#
#  Low: a yum-like package manager
#
#  Copyright (C) 2008 - 2010 James Bowes <jbowes@repl.ca>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#  02110-1301  USA
#
#  Run this to generate all the initial makefiles, etc.

srcdir=`dirname $0`
test -z "$srcdir" && srcdir=.

(test -f $srcdir/configure.ac) || {
    echo -n "**Error**: Directory \"\'$srcdir\'\" does not look like the"
    echo " top-level package directory"
    exit 1
}

if ([ -z "$*" ] && [ "x$NOCONFIGURE" = "x" ]) ; then
  echo "**Warning**: I am going to run 'configure' with no arguments."
  echo "If you wish to pass any to it, please specify them on the"
  echo "'$0' command line."
  echo
fi

(cd $srcdir && aclocal) || exit 1
(cd $srcdir && autoheader) || exit 1
(cd $srcdir && automake --foreign --add-missing) || exit 1
(cd $srcdir && autoconf) || exit 1

# conf_flags="--enable-maintainer-mode"

if test x$NOCONFIGURE = x; then
  echo Running $srcdir/configure $conf_flags "$@" ...
  $srcdir/configure $conf_flags "$@" \
  && echo Now type \`make\' to compile. || exit 1
else
  echo Skipping configure process.
fi
