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

SUBDIRS = src test
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
