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

INCLUDES = \
	@CHECK_CFLAGS@ \
	-I${top_builddir}/src/ \
	$(GLIB_CFLAGS) \
	$(NULL)

TESTS = check_low

noinst_PROGRAMS = check_low

check_low_SOURCES = \
		check_low.c \
		low-fake-repo.c \
		low-fake-repo.h \
		low-repo-sqlite-fake.h \
		low-config-fake.c \
		low-config-fake.h \
		$(NULL)

check_low_LDADD = \
		@CHECK_LIBS@ \
		$(GLIB_LIBS) \
		$(RPM_LIBS) \
		${top_builddir}/src/low-debug.o \
		${top_builddir}/src/low-package.o \
		${top_builddir}/src/low-repo-set.o \
		${top_builddir}/src/low-util.o \
		$(NULL)

CLEANFILES = check_low.log
