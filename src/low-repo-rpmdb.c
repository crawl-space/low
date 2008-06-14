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

#include <stdlib.h>
#include <fcntl.h>
#include <string.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include "low-debug.h"
#include "low-repo-rpmdb.h"

typedef struct _LowRepoRpmdb {
	LowRepo super;
	rpmdb db;
} LowRepoRpmdb;

/* XXX clean these up */
typedef gboolean (*LowPackageIterFilterFn) (LowPackage *pkg, gpointer data);
typedef void 	 (*LowPackageIterFilterDataFree) (gpointer data);

typedef struct _LowPackageIterRpmdb {
	LowPackageIter super;
	rpmdbMatchIterator rpm_iter;
	LowPackageIterFilterFn func;
	gpointer filter_data;
	LowPackageIterFilterDataFree filter_data_free_func;
} LowPackageIterRpmdb;

LowPackageIter * low_package_iter_rpmdb_next (LowPackageIter *iter);

LowPackageDetails *	low_rpmdb_package_get_details	(LowPackage *pkg);

LowPackageDependency **	low_rpmdb_package_get_provides	(LowPackage *pkg);
LowPackageDependency **	low_rpmdb_package_get_requires	(LowPackage *pkg);
LowPackageDependency **	low_rpmdb_package_get_conflicts	(LowPackage *pkg);
LowPackageDependency **	low_rpmdb_package_get_obsoletes	(LowPackage *pkg);

char **		low_rpmdb_package_get_files 		(LowPackage *pkg);

LowRepo *
low_repo_rpmdb_initialize ()
{
	LowRepoRpmdb *repo = malloc (sizeof (LowRepoRpmdb));

	repo->super.id = "installed";
	repo->super.name = "Installed Packages";
	repo->super.enabled = TRUE;

	rpmReadConfigFiles (NULL, NULL);
	if (rpmdbOpen ("", &repo->db, O_RDONLY, 0644) != 0) {
		fprintf (stderr, "Cannot open rpm database\n");
		exit (1);
	}

	return (LowRepo *) repo;
}

void
low_repo_rpmdb_shutdown (LowRepo *repo)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;

	rpmdbClose (repo_rpmdb->db);
	rpmFreeRpmrc ();

	free (repo);
}

static LowPackageIter *
low_repo_rpmdb_search (LowRepo *repo, int_32 tag, const char *querystr)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.next_func = low_package_iter_rpmdb_next;
	iter->super.pkg = NULL;

	iter->func = NULL;
	iter->filter_data_free_func = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, tag, querystr, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_list_all (LowRepo *repo)
{
	return low_repo_rpmdb_search (repo, 0, NULL);
}

LowPackageIter *
low_repo_rpmdb_list_by_name (LowRepo *repo, const char *name)
{
	return low_repo_rpmdb_search (repo, RPMTAG_NAME, name);
}

typedef struct _DepFilterData {
	LowPackageDependency *dep;
	LowPackageGetDependency dep_func;
} DepFilterData;

static void
dep_filter_data_free_fn (gpointer data) {
	DepFilterData *filter_data = (DepFilterData *) data;
	free (filter_data->dep);
	free (filter_data);
}

static gboolean
low_repo_rpmdb_search_dep_filter_fn (LowPackage *pkg, gpointer data)
{
	DepFilterData *filter_data = (DepFilterData *) data;
	gboolean res = FALSE;
	LowPackageDependency **deps = (filter_data->dep_func) (pkg);
	int i;

	for (i = 0; deps[i] != NULL; i++) {
		if (low_package_dependency_satisfies (filter_data->dep,
						      deps[i])) {
			res = TRUE;
			break;
		}
	}

//	low_package_dependency_list_free (deps);

	return res;
}

static LowPackageIter *
low_repo_rpmdb_search_dep (LowRepo *repo, int_32 tag,
			   const LowPackageDependency *dep,
			   LowPackageGetDependency dep_func)
{
	DepFilterData *data = malloc (sizeof (DepFilterData));
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.next_func = low_package_iter_rpmdb_next;
	iter->super.pkg = NULL;

	iter->func = low_repo_rpmdb_search_dep_filter_fn;
	iter->filter_data_free_func = dep_filter_data_free_fn;
	iter->filter_data = (gpointer) data;
	/* XXX might need to copy this */
	data->dep = low_package_dependency_new (dep->name, dep->sense,
						dep->evr);
	data->dep_func = dep_func;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, tag, dep->name, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_search_provides (LowRepo *repo,
				const LowPackageDependency *provides)
{
	return low_repo_rpmdb_search_dep (repo, RPMTAG_PROVIDENAME, provides,
					  low_package_get_provides);
}

LowPackageIter *
low_repo_rpmdb_search_requires (LowRepo *repo,
				const LowPackageDependency *requires)
{
	return low_repo_rpmdb_search_dep (repo, RPMTAG_REQUIRENAME, requires,
					  low_package_get_requires);
}

LowPackageIter *
low_repo_rpmdb_search_conflicts (LowRepo *repo,
				 const LowPackageDependency *conflicts)
{
	return low_repo_rpmdb_search_dep (repo, RPMTAG_CONFLICTNAME, conflicts,
					  low_package_get_conflicts);
}

LowPackageIter *
low_repo_rpmdb_search_obsoletes (LowRepo *repo,
				 const LowPackageDependency *obsoletes)
{
	/* XXX This seems to be broken in RPM itself. */
	return low_repo_rpmdb_search_dep (repo, RPMTAG_OBSOLETENAME, obsoletes,
					  low_package_get_obsoletes);
}

LowPackageIter *
low_repo_rpmdb_search_files (LowRepo *repo, const char *file)
{
	return low_repo_rpmdb_search (repo, RPMTAG_BASENAMES, file);
}

static gboolean
low_repo_rpmdb_search_details_filter_fn (LowPackage *pkg, gpointer data)
{
	gchar *querystr = (gchar *) data;

	/* url can be NULL, so check first. */
	if (strstr (pkg->name, querystr)) {
		return TRUE;
	} else {
		gboolean res;
		LowPackageDetails *details = low_package_get_details (pkg);

		if (strstr (details->summary, querystr) ||
		    strstr (details->description, querystr) ||
		    (details->url != NULL && strstr (details->url, querystr))) {
			res = TRUE;
		} else {
			res = FALSE;
		}

		low_package_details_free (details);

		return res;
	}
}

LowPackageIter *
low_repo_rpmdb_search_details (LowRepo *repo, const char *querystr)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.repo = repo;
	iter->super.next_func = low_package_iter_rpmdb_next;
	iter->super.pkg = NULL;

	iter->func = low_repo_rpmdb_search_details_filter_fn;
	iter->filter_data_free_func = NULL;
	iter->filter_data = (gpointer) querystr;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, 0, NULL, 0);
	return (LowPackageIter *) iter;
}

/* XXX Duplicated */
union rpm_entry {
	void *p;
	char *string;
	char **list;
	uint_32 *int_list;
	uint_32 *flags;
	uint_32 *integer;
};

static LowPackageDetails *
low_repo_rpmdb_get_details (LowRepo *repo, LowPackage *pkg)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	rpmdbMatchIterator iter;
	Header header;
	LowPackageDetails *details = malloc (sizeof (LowPackageDetails));
	union rpm_entry summary, description, url, license;
	int_32 type, count;

	iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_PKGID, pkg->id, 16);
	header = rpmdbNextIterator (iter);

	rpmHeaderGetEntry(header, RPMTAG_SUMMARY, &type, &summary.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_DESCRIPTION, &type, &description.p,
			  &count);
	rpmHeaderGetEntry(header, RPMTAG_URL, &type, &url.p, &count);
	rpmHeaderGetEntry(header, RPMTAG_LICENSE, &type, &license.p, &count);

	/* XXX need to confirm that we don't need to dup these */
	details->summary = summary.string;
	details->description = description.string;

	details->url = g_strdup (url.string);
	details->license = strdup (license.string);

	rpmdbFreeIterator (iter);

	return details;
}

static LowPackageDependencySense
rpm_to_low_dependency_sense (uint_32 flag)
{
	switch (flag & (RPMSENSE_LESS | RPMSENSE_EQUAL | RPMSENSE_GREATER)) {
		case RPMSENSE_LESS:
			return DEPENDENCY_SENSE_LT;
		case RPMSENSE_LESS|RPMSENSE_EQUAL:
			return DEPENDENCY_SENSE_LE;
		case RPMSENSE_EQUAL:
			return DEPENDENCY_SENSE_EQ;
		case RPMSENSE_GREATER|RPMSENSE_EQUAL:
			return DEPENDENCY_SENSE_GE;
		case RPMSENSE_GREATER:
			return DEPENDENCY_SENSE_GT;
	}

	return DEPENDENCY_SENSE_NONE;
}

static LowPackageDependency **
low_repo_rpmdb_get_deps (LowRepo *repo, LowPackage *pkg, uint_32 name_tag,
			 uint_32 flag_tag, uint_32 version_tag)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	rpmdbMatchIterator iter;
	Header header;
	LowPackageDependency **deps;
	union rpm_entry name, flag, version;
	int_32 type, count, i;

	iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_PKGID, pkg->id, 16);
	header = rpmdbNextIterator (iter);

	rpmHeaderGetEntry (header, name_tag, &type, &name.p, &count);
	rpmHeaderGetEntry (header, flag_tag, &type, &flag.p, &count);
	rpmHeaderGetEntry (header, version_tag, &type, &version.p, &count);

	deps = malloc (sizeof (char *) * (count + 1));
	for (i = 0; i < count; i++) {
		LowPackageDependencySense sense =
			rpm_to_low_dependency_sense (flag.int_list[i]);
		deps[i] = low_package_dependency_new (name.list[i],
						      sense,
						      version.list[i]);
	}
	deps[count] = NULL;

	headerFreeTag (header, name.p, type);
	headerFreeTag (header, version.p, type);
	rpmdbFreeIterator (iter);

	return deps;
}

static LowPackageDependency **
low_repo_rpmdb_get_provides (LowRepo *repo, LowPackage *pkg)
{
	return low_repo_rpmdb_get_deps (repo, pkg, RPMTAG_PROVIDENAME,
					RPMTAG_PROVIDEFLAGS,
					RPMTAG_PROVIDEVERSION);
}

static LowPackageDependency **
low_repo_rpmdb_get_requires (LowRepo *repo, LowPackage *pkg)
{
	if (!pkg->requires) {
		pkg->requires = low_repo_rpmdb_get_deps (repo, pkg,
							 RPMTAG_REQUIRENAME,
							 RPMTAG_REQUIREFLAGS,
							 RPMTAG_REQUIREVERSION);
	}

	return pkg->requires;
}

static LowPackageDependency **
low_repo_rpmdb_get_conflicts (LowRepo *repo, LowPackage *pkg)
{
	return low_repo_rpmdb_get_deps (repo, pkg, RPMTAG_CONFLICTNAME,
					RPMTAG_CONFLICTFLAGS,
					RPMTAG_CONFLICTVERSION);
}

static LowPackageDependency **
low_repo_rpmdb_get_obsoletes (LowRepo *repo, LowPackage *pkg)
{
	return low_repo_rpmdb_get_deps (repo, pkg, RPMTAG_OBSOLETENAME,
					RPMTAG_OBSOLETEFLAGS,
					RPMTAG_OBSOLETEVERSION);
}

static char **
low_repo_rpmdb_get_files (LowRepo *repo, LowPackage *pkg)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	rpmdbMatchIterator iter;
	Header header;
	char **files;
	union rpm_entry index, dir, name;
	int_32 type, count, i;

	iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_PKGID, pkg->id, 16);
	header = rpmdbNextIterator (iter);

	rpmHeaderGetEntry (header, RPMTAG_DIRINDEXES, &type, &index.p, &count);
	rpmHeaderGetEntry (header, RPMTAG_DIRNAMES, &type, &dir.p, &count);
	rpmHeaderGetEntry (header, RPMTAG_BASENAMES, &type, &name.p, &count);

	files = malloc (sizeof (char *) * (count + 1));
	for (i = 0; i < count; i++) {
		files[i] = g_strdup_printf ("%s%s", dir.list[index.int_list[i]],
					    name.list[i]);
	}
	files[count] = NULL;

	headerFreeTag (header, dir.p, type);
	headerFreeTag (header, name.p, type);
	rpmdbFreeIterator (iter);

	return files;
}

/* XXX add this to the rpmdb repo struct */
static GHashTable *table = NULL;

static guint
id_hash_func (gconstpointer key)
{
	return *((guint *) key);
}

static gboolean
id_equal_func (gconstpointer key1, gconstpointer key2)
{
	if (!strncmp (key1, key2, 16)) {
		return TRUE;
	} else {
		return FALSE;
	}
}

static LowPackage *
low_package_rpmdb_new_from_header (Header header, LowRepo *repo)
{
	if (!table) {
		low_debug ("initializing hash table\n");
		table = g_hash_table_new (id_hash_func, id_equal_func);
	}

	LowPackage *pkg;
	union rpm_entry id, name, epoch, version, release, arch;
	union rpm_entry size;
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

	pkg = g_hash_table_lookup (table, id.p);
	if (pkg) {
		low_package_ref (pkg);
		return pkg;
	}
	low_debug ("CACHE MISS - %s", name.string);

	pkg = malloc (sizeof (LowPackage));

	g_hash_table_insert (table, id.p, pkg);
	low_package_ref_init (pkg);
	low_package_ref (pkg);

	pkg->id = id.p;
	pkg->name = strdup (name.string);
	pkg->epoch = g_strdup (epoch.string);
	pkg->version = strdup (version.string);
	pkg->release = strdup (release.string);
	pkg->arch = strdup (arch.string);

	pkg->size = *size.integer;
	pkg->repo = repo;

	/* installed packages can't be downloaded. */
	pkg->location_href = NULL;

	pkg->requires = NULL;

	pkg->get_details = low_rpmdb_package_get_details;

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

		if (iter_rpmdb->filter_data_free_func) {
			(iter_rpmdb->filter_data_free_func) (iter_rpmdb->filter_data);
		}

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

LowPackageDetails *
low_rpmdb_package_get_details (LowPackage *pkg)
{
	return low_repo_rpmdb_get_details (pkg->repo, pkg);
}

LowPackageDependency **
low_rpmdb_package_get_provides (LowPackage *pkg)
{
	/*
	 * XXX maybe this should all be in the same file,
	 *     or the rpmdb repo struct should be in the header
	 */
	return low_repo_rpmdb_get_provides (pkg->repo, pkg);
}

LowPackageDependency **
low_rpmdb_package_get_requires (LowPackage *pkg)
{
	return low_repo_rpmdb_get_requires (pkg->repo, pkg);
}

LowPackageDependency **
low_rpmdb_package_get_conflicts (LowPackage *pkg)
{
	return low_repo_rpmdb_get_conflicts (pkg->repo, pkg);
}

LowPackageDependency **
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
