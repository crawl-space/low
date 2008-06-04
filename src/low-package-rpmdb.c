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

#include <string.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include "low-package-rpmdb.h"
#include "low-repo-rpmdb.h"

union rpm_entry {
	void *p;
	char *string;
	char **list;
	uint_32 *flags;
	uint_32 *integer;
};

static LowPackage *
low_package_rpmdb_new_from_header (Header header, LowRepo *repo)
{
	union rpm_entry id, name, epoch, version, release, arch;
	union rpm_entry size, summary, description, url, license;
	int_32 type, count;

	rpmHeaderGetEntry(header, RPMTAG_NAME, &type, &name.p, &count);

	/* We don't care about the gpg keys (plus they have missing fields */
	if (!strcmp (name.string, "gpg-pubkey")) {
		return NULL;
	}

	rpmHeaderGetEntry(header, RPMTAG_PKGID, &type, &id.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_EPOCH, &type, &epoch.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_VERSION, &type, &version.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_RELEASE, &type, &release.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_ARCH, &type, &arch.p, &count);

	rpmHeaderGetEntry(header, RPMTAG_SIZE, &type, &size.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_SUMMARY, &type, &summary.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_DESCRIPTION, &type, &description.p,
			  &count);
	rpmHeaderGetEntry(header, RPMTAG_URL, &type, &url.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_LICENSE, &type, &license.p, &count);

	LowPackage *pkg = malloc (sizeof (LowPackage));

	low_package_ref_init (pkg);

	pkg->id = id.p;
	pkg->name = strdup (name.string);
	pkg->epoch = g_strdup (epoch.string);
	pkg->version = strdup (version.string);
	pkg->release = strdup (release.string);
	pkg->arch = strdup (arch.string);

	pkg->size = *size.integer;
	pkg->repo = repo;

	/* XXX need to confirm that we don't need to dup these */
	pkg->summary = summary.string;
	pkg->description = description.string;

	pkg->url = g_strdup (url.string);
	pkg->license = strdup (license.string);
	/* installed packages can't be downloaded. */
	pkg->location_href = NULL;

	pkg->get_provides = low_rpmdb_package_get_provides;
	pkg->get_requires = low_rpmdb_package_get_requires;
	pkg->get_conflicts = low_rpmdb_package_get_conflicts;
	pkg->get_obsoletes = low_rpmdb_package_get_obsoletes;

	pkg->get_files = low_rpmdb_package_get_files;

	return pkg;
}

LowPackageIter *
low_package_iter_rpmdb_next (LowPackageIter *iter)
{
	LowPackageIterRpmdb *iter_rpmdb = (LowPackageIterRpmdb *) iter;
	Header header = rpmdbNextIterator(iter_rpmdb->rpm_iter);

	if (header == NULL) {
		rpmdbFreeIterator (iter_rpmdb->rpm_iter);
		free (iter);
		return NULL;
	}

	iter->pkg = low_package_rpmdb_new_from_header (header, iter->repo);

	/* Ignore the gpg-pubkeys */
	while (iter->pkg == NULL && header) {
		header = rpmdbNextIterator(iter_rpmdb->rpm_iter);
		iter->pkg = low_package_rpmdb_new_from_header (header,
							       iter->repo);
	}
	if (iter_rpmdb->func != NULL) {
		/* move on to the next rpm if this one fails the filter */
		if (!(iter_rpmdb->func) (iter->pkg, iter_rpmdb->filter_data)) {
			low_package_unref (iter->pkg);
			return low_package_iter_next (iter);
		}
	}

	return iter;
}

char **
low_rpmdb_package_get_provides (LowPackage *pkg)
{
	/*
	 * XXX maybe this should all be in the same file,
	 *     or the rpmdb repo struct should be in the header
	 */
	return low_repo_rpmdb_get_provides (pkg->repo, pkg);
}

char **
low_rpmdb_package_get_requires (LowPackage *pkg)
{
	return low_repo_rpmdb_get_requires (pkg->repo, pkg);
}

char **
low_rpmdb_package_get_conflicts (LowPackage *pkg)
{
	return low_repo_rpmdb_get_conflicts (pkg->repo, pkg);
}

char **
low_rpmdb_package_get_obsoletes (LowPackage *pkg)
{
	return low_repo_rpmdb_get_obsoletes (pkg->repo, pkg);
}

char **
low_rpmdb_package_get_files (LowPackage *pkg)
{
	return low_repo_rpmdb_get_files (pkg->repo, pkg);
}

/* vim: set ts=8 sw=8 noet: */
