#  Low: a yum-like package manager
#
#  Copyright (C) 2008, 2009 James Bowes <jbowes@repl.ca>
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

AUTOMAKE_OPTIONS = subdir-objects

TESTS =
noinst_PROGRAMS =

if HAVE_CHECK
unit_check_low_INCLUDES = \
	@CHECK_CFLAGS@ \
	-I${top_srcdir}/src/ \
	$(GLIB_CFLAGS) \
	$(NULL)

TESTS += unit/check_low

noinst_PROGRAMS += unit/check_low

unit_check_low_SOURCES = \
		unit/check_low.c \
		unit/low-fake-repo.c \
		unit/low-fake-repo.h \
		unit/low-repo-sqlite-fake.h \
		unit/low-config-fake.c \
		unit/low-config-fake.h \
		$(NULL)

unit_check_low_LDADD = \
		@CHECK_LIBS@ \
		$(GLIB_LIBS) \
		$(RPM_LIBS) \
		${top_builddir}/src/low-debug.o \
		${top_builddir}/src/low-package.o \
		${top_builddir}/src/low-repo-set.o \
		${top_builddir}/src/low-util.o \
		${top_builddir}/src/low-arch.o \
		$(NULL)

CLEANFILES = unit/check_low.log
endif

if HAVE_YAML
EXTRA_DIST = run-depsolver-tests.sh yaml

INCLUDES = \
	-I${top_srcdir}/src/ \
	-I${top_srcdir}/test/unit/ \
	$(GLIB_CFLAGS) \
	$(NULL)

noinst_PROGRAMS += depsolver/test_depsolver

depsolver_test_depsolver_SOURCES = depsolver/main.c

depsolver_test_depsolver_LDADD = \
		$(GLIB_LIBS) \
		$(RPM_LIBS) \
		$(SYCK_LIBS) \
		${top_builddir}/src/low-debug.o \
		${top_builddir}/src/low-package.o \
		${top_builddir}/src/low-repo-set.o \
		${top_builddir}/src/low-transaction.o \
		${top_builddir}/src/low-util.o \
		${top_builddir}/src/low-arch.o \
		${top_builddir}/test/unit/low-config-fake.o \
		${top_builddir}/test/unit/low-fake-repo.o \
		$(NULL)

depsolver/run-depsolver-tests.sh: depsolver/test_depsolver

TESTS += depsolver/run-depsolver-tests.sh
endif
