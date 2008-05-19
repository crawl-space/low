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
	$(GLIB_CFLAGS) \
	$(SQLITE_CFLAGS) \
	$(NULL)

bin_PROGRAMS = low

low_SOURCES = \
	low.c \
	low-config.c \
	low-config.h \
	low-package.h \
	low-package-rpmdb.c \
	low-package-rpmdb.h \
	low-package-sqlite.c \
	low-package-sqlite.h \
	low-repo.h \
	low-repo-rpmdb.c \
	low-repo-rpmdb.h \
	low-repo-set.c \
	low-repo-set.h \
	low-repo-sqlite.c \
	low-repo-sqlite.h \
	low-transaction.c \
	low-transaction.h \
	low-download.h \
	low-download.c \
	main.c \
	$(NULL)

low_LDADD = \
	$(GLIB_LIBS) \
	$(SQLITE_LIBS) \
	$(RPM_LIBS) \
	-lcurl \
	$(NULL)
