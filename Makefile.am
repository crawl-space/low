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

EXTRA_DIST = HACKING low.spec low-completion.bash

.PHONY: doxygen

doxygen: doxygen/doxygen.conf
	doxygen $<

clean-local:
	-rm -rf doxygen/html

# Automatically convert trailing spaces, as found in 'trailinglint'
eolspace:
	find \( -name '*.c' -o -name '*.h' -o -name 'Makefile.am' \) -exec perl -i -pe 's/\s+\n$$/\1\n/;' \{\} \;

# Check for spaces followed by tabs before characters
tablint:
	@! find \( -name \*.c -o -name '*.h' -o -name 'Makefile.am' \) \
		| xargs grep -nP "^\s* \t"

# Check for all spaces before characters
leadingspacelint:
	@! find \( -name \*.c -o -name '*.h' -o -name 'Makefile.am' \) \
		| xargs grep -nP "^  *\w\w*[^:]$$"

# Check for trailing whitespace
trailinglint:
	@! find \( -name \*.c -o -name '*.h' -o -name 'Makefile.am' \) \
		| xargs grep -nP "[ \t]$$"

whitespacelint: tablint trailinglint leadingspacelint

CLASSPATH=/usr/share/java/pmd.jar
cpd:
	@CLASSPATH=$(CLASSPATH) java net.sourceforge.pmd.cpd.CPD  --language c \
		--minimum-tokens 100 \
		--files src \
		--files test

TAGS=$(shell awk 'BEGIN{RS=""} { for(i=1;i<=NF;i++) printf "-T "$$(i)" "; print ""}' typedefs.txt)

# XXX watch for pointers and custom typedefs with this
indent:
	@find \( -name \*.c -o -name '*.h' \) \
		| xargs indent $(TAGS) -linux -pcs -psl -ci8 -cs -cli8 -cp0

todo:
	@find \( -name \*.c -o -name '*.h' \) \
		| xargs grep -nP "(XXX)|(TODO)|(FIXME)"

smoke-test:
	@cd test && ./smoke-test.sh && cd ../

rpm: dist
	rpmbuild --clean -ta ${distdir}.tar.bz2

srpm: dist
	rpmbuild --clean -ts ${distdir}.tar.bz2

INCLUDES = \
	$(GLIB_CFLAGS) \
	$(SQLITE_CFLAGS) \
	$(NSS_CFLAGS) \
	$(NULL)

bin_PROGRAMS = src/low

src_low_SOURCES = \
	src/low-config.c \
	src/low-config.h \
	src/low-debug.c \
	src/low-debug.h \
	src/low-package.c \
	src/low-package.h \
	src/low-repo.h \
	src/low-repo-rpmdb.c \
	src/low-repo-rpmdb.h \
	src/low-repo-set.c \
	src/low-repo-set.h \
	src/low-repo-sqlite.c \
	src/low-repo-sqlite.h \
	src/low-repomd-parser.c \
	src/low-repomd-parser.h \
	src/low-transaction.c \
	src/low-transaction.h \
	src/low-util.c \
	src/low-util.h \
	src/low-download.h \
	src/low-download.c \
	src/low-mirror-list.h \
	src/low-mirror-list.c \
	src/low-metalink-parser.h \
	src/low-metalink-parser.c \
	src/low-delta-parser.h \
	src/low-delta-parser.c \
	src/low-arch.h \
	src/low-arch.c \
	src/low-parse-options.h \
	src/low-parse-options.c \
	src/main.c \
	$(NULL)

src_low_LDADD = \
	$(GLIB_LIBS) \
	$(SQLITE_LIBS) \
	$(RPM_LIBS) \
	$(CURL_LIBS) \
	$(EXPAT_LIBS) \
	$(BZIP2_LIBS) \
	$(Z_LIBS) \
	$(NSS_LIBS) \
	$(NULL)

TESTS =
noinst_PROGRAMS =

if HAVE_CHECK

TESTS += test/unit/check_low

noinst_PROGRAMS += test/unit/check_low

INCLUDES += \
	@CHECK_CFLAGS@ \
	-I${top_srcdir}/src/ \
	$(NULL)

test_unit_check_low_SOURCES = \
		test/unit/check_low.c \
		test/unit/low-fake-repo.c \
		test/unit/low-fake-repo.h \
		test/unit/low-repo-sqlite-fake.h \
		test/unit/low-config-fake.c \
		test/unit/low-config-fake.h \
		$(NULL)

test_unit_check_low_LDADD = \
		@CHECK_LIBS@ \
		$(GLIB_LIBS) \
		$(RPM_LIBS) \
		${top_builddir}/src/low-debug.o \
		${top_builddir}/src/low-package.o \
		${top_builddir}/src/low-repo-set.o \
		${top_builddir}/src/low-util.o \
		${top_builddir}/src/low-arch.o \
		$(NULL)

CLEANFILES = check_low.log
endif

if HAVE_YAML
EXTRA_DIST += test/depsolver/run-depsolver-tests.sh test/depsolver/yaml

INCLUDES += \
	-I${top_srcdir}/src/ \
	-I${top_srcdir}/test/unit/ \
	$(NULL)

noinst_PROGRAMS += test/depsolver/test_depsolver

test_depsolver_test_depsolver_SOURCES = test/depsolver/main.c

test_depsolver_test_depsolver_LDADD = \
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

test/depsolver/run-depsolver-tests.sh: test/depsolver/test_depsolver

TESTS += test/depsolver/run-depsolver-tests.sh
endif
