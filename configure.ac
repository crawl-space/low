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
	CPPFLAGS="$CPPFLAGS -Wall -Wextra -Wformat-security"
	CPPFLAGS="$CPPFLAGS -Wmissing-prototypes -Wmissing-declarations"
	CPPFLAGS="$CPPFLAGS -Wredundant-decls -Wshadow"
fi

dnl ---------------------------------------------------------------------------
dnl - Library dependencies
dnl ---------------------------------------------------------------------------
GLIB_REQUIRED=2.16.3
CURL_REQUIRED=7.18.1
CHECK_REQUIRED=0.9.5

dnl ---------------------------------------------------------------------------
dnl - Make above strings available for packaging files (e.g. rpm spec files)
dnl ---------------------------------------------------------------------------
AC_SUBST(GLIB_REQUIRED)
AC_SUBST(CURL_REQUIRED)
AC_SUBST(CHECK_REQUIRED)

dnl ---------------------------------------------------------------------------
dnl - Check library dependencies
dnl ---------------------------------------------------------------------------

dnl Check for glib (required)
PKG_CHECK_MODULES(GLIB, glib-2.0 >= $GLIB_REQUIRED)
AC_SUBST(GLIB_CFLAGS)
AC_SUBST(GLIB_LIBS)

dnl Check for sqlite3 (required)
PKG_CHECK_MODULES(SQLITE, sqlite3)
AC_SUBST(SQLITE_CFLAGS)
AC_SUBST(SQLITE_LIBS)

dnl Check for rpm (required)
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

dnl Check for libcurl (required)
LIBCURL_CHECK_CONFIG(yes,$CURL_REQUIRED,,AC_MSG_ERROR(*** curl was not found))
CURL_LIBS=$LIBCURL
AC_SUBST(CURL_LIBS)

dnl Check for expat xml parser (required)
EXPAT_LIB=""
AC_ARG_WITH(expat, [  --with-expat=<dir>      Use expat from here],
                      [
                      expat=$withval
                      CPPFLAGS="$CPPFLAGS -I$withval/include"
                      LDFLAGS="$LDFLAGS -L$withval/lib"
                      ]
                      )
AC_CHECK_HEADERS(expat.h, [AC_DEFINE(HAVE_EXPAT_H)],
		 [AC_MSG_ERROR([Can't find expat.h. Please install expat.])])
AC_CHECK_LIB(expat, XML_ParserCreate, [EXPAT_LIBS="-lexpat"],
	     [AC_MSG_ERROR([Can't find expat library. Please install expat.])])
AC_SUBST(EXPAT_LIBS)

dnl Check for bzip2 (required)
BZIP2_LIB=""
AC_ARG_WITH(bzip2, [  --with-bzip2=<dir>      Use bzip2 from here],
                      [
                      bzlib2=$withval
                      CPPFLAGS="$CPPFLAGS -I$withval/include"
                      LDFLAGS="$LDFLAGS -L$withval/lib"
                      ]
                      )
AC_CHECK_HEADERS(bzlib.h, [AC_DEFINE(HAVE_BZLIB_H)],
		 [AC_MSG_ERROR([Can't find bzlib.h. Please install bzip2.])])
AC_CHECK_LIB(bz2, BZ2_bzReadOpen, [BZIP2_LIBS="-lbz2"],
	     [AC_MSG_ERROR([Can't find bz2 library. Please install bzip2.])])
AC_SUBST(BZIP2_LIBS)

dnl Check for check (unit testing library, optional)
PKG_CHECK_MODULES(CHECK, check >= $CHECK_REQUIRED, HAVE_CHECK=yes,
                  HAVE_CHECK=no)
AM_CONDITIONAL(HAVE_CHECK, test x$HAVE_CHECK = xyes)
AC_SUBST(CHECK_CFLAGS)
AC_SUBST(CHECK_LIBS)

dnl Check for syck yaml parser (Used in depsolver test harness, optional)
AC_CHECK_HEADER(syck.h,
            AC_CHECK_LIB(syck, syck_new_parser,
                 [HAVE_YAML=yes
                  SYCK_LIBS="-lsyck"
                 ],HAVE_YAML=no),HAVE_YAML=no)
AM_CONDITIONAL(HAVE_YAML, test x$HAVE_YAML = xyes)
AC_SUBST(SYCK_LIBS)

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
dnl - Developer switches
dnl ---------------------------------------------------------------------------
AC_ARG_ENABLE(gcov, AS_HELP_STRING([--enable-gcov],
              [compile with gcov code coverage support (gcc only)]),
              enable_gcov=$enableval,enable_gcov=no)
AC_ARG_ENABLE(gprof, AS_HELP_STRING([--enable-gprof],
              [compile with gprof profiling support (gcc only)]),
              enable_gprof=$enableval,enable_gprof=no)

if test x$enable_gcov = xyes; then
	## so that config.h changes when you toggle gcov support
	AC_DEFINE_UNQUOTED(LOW_BUILD_GCOV, 1, [Defined if gcov is enabled to force a rebuild due to config.h changing])

	AC_MSG_CHECKING([for gcc 3.3 version of gcov file format])
	have_gcc33_gcov=no
	AC_RUN_IFELSE( [AC_LANG_PROGRAM( , [[ if (__GNUC__ >=3 && __GNUC_MINOR__ >= 3) exit (0); else exit (1); ]])],
			have_gcc33_gcov=yes)
	if test x$have_gcc33_gcov = xyes ; then
		AC_DEFINE_UNQUOTED(LOW_HAVE_GCC33_GCOV, 1, [Defined if we have gcc 3.3 and thus the new gcov format])
	fi
	CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
	AC_MSG_RESULT($have_gcc33_gcov)
fi
AM_CONDITIONAL(LOW_BUILD_GCOV, test x$enable_gcov = xyes)

if test x$enable_gprof = xyes; then
	## so that config.h changes when you toggle gprof support
	AC_DEFINE_UNQUOTED(LOW_BUILD_GPROF, 1, [Defined if gprof is enabled to force a rebuild due to config.h changing])
	CPPFLAGS="$CPPFLAGS -pg"
	LDFLAGS="$LDFLAGS -pg"
    # don't need these twice
    if test x$enable_gcov = xno; then
        CFLAGS="$CFLAGS -fprofile-arcs -ftest-coverage"
    fi
	AC_MSG_RESULT($enable_gprof)
fi
AM_CONDITIONAL(LOW_BUILD_GPROF, test x$enable_gprof = xyes)


dnl ---------------------------------------------------------------------------
dnl - Makefiles, etc.
dnl ---------------------------------------------------------------------------
AC_OUTPUT([
low.spec
Makefile
doxygen/Makefile
doxygen/doxygen.conf
src/Makefile
test/Makefile
test/unit/Makefile
test/depsolver/Makefile
])

dnl ---------------------------------------------------------------------------
dnl - Configure summary
dnl ---------------------------------------------------------------------------

echo "
                        Low $VERSION
                  ====================

        prefix:                         ${prefix}
        compiler:                       ${CC}
        cflags:                         ${CFLAGS}
        Building 'check' unit tests:    ${HAVE_CHECK}
        Building depsolver tests:       ${HAVE_YAML}
        GCC coverage profiling:         ${enable_gcov}
        GCC time profiling:             ${enable_gprof}
"
