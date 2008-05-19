#  Low: a yum-like package manager
#
#  Copyright (C) 2008 James Bowes <jbowes@dangerouslyinc.com>
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

AC_PREREQ(2.52)

AC_INIT(low, 0.0.1)
AC_CONFIG_SRCDIR(src)
AM_INIT_AUTOMAKE
AM_CONFIG_HEADER(config.h)

AC_PROG_CC
AC_PROG_INSTALL
AC_ISC_POSIX
AC_HEADER_STDC
AM_PROG_CC_C_O

dnl ---------------------------------------------------------------------------
dnl - Extra verbose warning switches
dnl ---------------------------------------------------------------------------
if test "$GCC" = "yes"; then
	CPPFLAGS="$CPPFLAGS -Werror -Wcast-align -Wno-uninitialized"
	CPPFLAGS="$CPPFLAGS -Wall -Wformat-security"
fi

dnl ---------------------------------------------------------------------------
dnl - Library dependencies
dnl ---------------------------------------------------------------------------
GLIB_REQUIRED=2.14.0

dnl ---------------------------------------------------------------------------
dnl - Make above strings available for packaging files (e.g. rpm spec files)
dnl ---------------------------------------------------------------------------
AC_SUBST(GLIB_REQUIRED)

dnl ---------------------------------------------------------------------------
dnl - Check library dependencies
dnl ---------------------------------------------------------------------------
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

PKG_CHECK_MODULES(SQLITE, sqlite3)
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)

AC_CHECK_HEADER(rpm/rpmlib.h,
            AC_CHECK_LIB(rpm, rpmdbOpen,
                 [have_rpm=yes
                  RPM_LIBS="$LIBS -lrpm"
                  AC_DEFINE(HAVE_RPM, 1,
                  [Defined if rpm is available])
                 ]))

if test "$have_rpm" != yes; then
    AC_MSG_ERROR(
    *** rpm was not found)
fi
AC_SUBST(RPM_LIBS)

dnl ---------------------------------------------------------------------------
dnl - Make paths available for source files
dnl ---------------------------------------------------------------------------
AC_SUBST(SYSCONFDIR, $sysconfdir)
AC_SUBST(LIBDIR, $libdir)
AC_SUBST(LIBEXECDIR, $libexecdir)
AC_SUBST(DATADIR, $datadir)
AC_SUBST(BINDIR, $bindir)
AC_SUBST(SBINDIR, $sbindir)
AC_SUBST(LOCALSTATEDIR, $localstatedir)

dnl ---------------------------------------------------------------------------
dnl - Makefiles, etc.
dnl ---------------------------------------------------------------------------
AC_OUTPUT([
low.spec
Makefile
doxygen/Makefile
doxygen/doxygen.conf
src/Makefile
])
