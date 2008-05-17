/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2008 James Bowes <jbowes@dangerouslyinc.com>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
 *  02110-1301  USA
 */

#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include "low-package-rpmdb.h"

union rpm_entry {
	void *p;
	char *string;
	char **list;
	uint_32 *flags;
	uint_32 integer;
};

static LowPackage *
low_package_rpmdb_new_from_header (Header header)
{
	union rpm_entry name, epoch, version, release, arch;
	union rpm_entry size, summary, description, url, license;
	int_32 type, count;

	headerGetEntry(header, RPMTAG_NAME, &type, &name.p, &count);
	headerGetEntry(header, RPMTAG_EPOCH, &type, &epoch.p, &count);
	headerGetEntry(header, RPMTAG_VERSION, &type, &version.p, &count);
	headerGetEntry(header, RPMTAG_RELEASE, &type, &release.p, &count);
	headerGetEntry(header, RPMTAG_ARCH, &type, &arch.p, &count);

	headerGetEntry(header, RPMTAG_SIZE, &type, &size.p, &count);
	headerGetEntry(header, RPMTAG_SUMMARY, &type, &summary.p, &count);
	headerGetEntry(header, RPMTAG_DESCRIPTION, &type, &description.p, &count);
	headerGetEntry(header, RPMTAG_URL, &type, &url.p, &count);
	headerGetEntry(header, RPMTAG_LICENSE, &type, &license.p, &count);

	LowPackage *pkg = malloc (sizeof (LowPackage));

	pkg->name = name.string;
	pkg->epoch = epoch.string;
	pkg->version = version.string;
	pkg->release = release.string;
	pkg->arch = arch.string;

	pkg->size = size.integer;
	pkg->repo = "installed";
	pkg->summary = summary.string;
	pkg->description = description.string;
	pkg->url = url.string;
	pkg->license = license.string;

	return pkg;
}

LowPackageIter *
low_package_iter_next (LowPackageIter *iter)
{
	LowPackageIterRpmdb *iter_rpmdb = (LowPackageIterRpmdb *) iter;
	Header header = rpmdbNextIterator(iter_rpmdb->rpm_iter);

	if (header == NULL) {
		free (iter);
		return NULL;
	}

	if (iter->pkg != NULL) {
		free (iter->pkg);
	}
	iter->pkg = low_package_rpmdb_new_from_header (header);

	return iter;
}
