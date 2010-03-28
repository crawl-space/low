/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2008 - 2010 James Bowes <jbowes@repl.ca>
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
#include <stdio.h>
#include <string.h>
#include <sqlite3.h>
#include <unistd.h>
#include <glob.h>
#include "low-debug.h"
#include "low-repo-sqlite.h"
#include "low-repomd-parser.h"

#define SELECT_FIELDS_FROM "SELECT p.pkgKey, p.name, p.arch, p.version, " \
			   "p.release, p.epoch, p.size_package, " \
			   "p.location_href, p.pkgId, p.checksum_type FROM "

typedef struct _LowRepoSqlite {
	LowRepo super;
	LowMirrorList *mirrors;
	LowDelta *delta;
	sqlite3 *primary_db;
	sqlite3 *filelists_db;
	GHashTable *table;
	GHashTable *obsoletes;
} LowRepoSqlite;

/* XXX clean these up */
typedef bool (*LowPackageIterFilterFn) (LowPackage *pkg, gpointer data);
typedef void (*LowPackageIterFilterDataFree) (gpointer data);

typedef struct _LowPackageIterSqlite {
	LowPackageIter super;
	sqlite3_stmt *pp_stmt;
	LowPackageIterFilterFn func;
	gpointer filter_data;
	LowPackageIterFilterDataFree filter_data_free_func;
} LowPackageIterSqlite;

LowPackageDetails *low_sqlite_package_get_details (LowPackage *pkg);

LowPackageDependency **low_sqlite_package_get_provides (LowPackage *pkg);
LowPackageDependency **low_sqlite_package_get_requires (LowPackage *pkg);
LowPackageDependency **low_sqlite_package_get_conflicts (LowPackage *pkg);
LowPackageDependency **low_sqlite_package_get_obsoletes (LowPackage *pkg);

char **low_sqlite_package_get_files (LowPackage *pkg);

static void
low_repo_sqlite_open_db (const char *db_file, sqlite3 **db)
{
	if (sqlite3_open (db_file, db)) {
		sqlite3_close (*db);
	}
}

static void
attach_db (sqlite3 *db, const char *db_file)
{
	const char *stmt = "ATTACH :db_file AS :name";

	sqlite3_stmt *pp_stmt;

	sqlite3_prepare (db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_text (pp_stmt, 1, db_file, -1, SQLITE_STATIC);
	sqlite3_bind_text (pp_stmt, 2, "filelists", -1, SQLITE_STATIC);

	sqlite3_step (pp_stmt);
	sqlite3_finalize (pp_stmt);
}

static void
detach_db (sqlite3 *db)
{
	const char *stmt = "DETACH :name";

	sqlite3_stmt *pp_stmt;

	sqlite3_prepare (db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_text (pp_stmt, 1, "filelists", -1, SQLITE_STATIC);

	sqlite3_step (pp_stmt);
	sqlite3_finalize (pp_stmt);
}

static void
low_repo_sqlite_regexp (sqlite3_context *ctx, int argc G_GNUC_UNUSED,
			sqlite3_value **args)
{
	bool matched;

	const char *expr = (const char *) sqlite3_value_text (args[0]);
	const char *item = (const char *) sqlite3_value_text (args[1]);

	/* XXX Can we compile this and free it when done? */
	matched = g_regex_match_simple (expr, item, 0, 0);
	sqlite3_result_int (ctx, matched);
}

static void
low_repo_sqlite_filename_match (sqlite3_context *ctx, int argc G_GNUC_UNUSED,
				sqlite3_value **args)
{
	const char *list = (const char *) sqlite3_value_text (args[0]);
	const char *filename = (const char *) sqlite3_value_text (args[1]);
	const char *location = strstr (list, filename);

	while (location) {
		low_debug ("%s", list);
		if ((location == list || *(location - 1) == '/') &&
		    (*(location + strlen (filename)) == '\0' ||
		     *(location + strlen (filename)) == '/')) {
			sqlite3_result_int (ctx, TRUE);
			return;
		}

		location = strstr (location + strlen(filename), filename);
	}

	sqlite3_result_int (ctx, FALSE);
}

typedef void (*sqlFunc) (sqlite3_context *, int, sqlite3_value **);
typedef void (*sqlFinal) (sqlite3_context *);

#define LOCAL_CACHE "/var/cache/yum"

static char *
xstrdup (const char *in)
{
	if (in == NULL) {
		return NULL;
	}

	return strdup (in);
}

LowRepo *
low_repo_sqlite_initialize (const char *id, const char *name,
			    const char *baseurl, const char *mirror_list,
			    bool enabled, bool bind_dbs)
{
	LowRepoSqlite *repo = malloc (sizeof (LowRepoSqlite));

	char *repomd_file;
	LowRepomd *repomd;

	repomd_file = g_strdup_printf (LOCAL_CACHE "/%s/repomd.xml", id);
	repomd = low_repomd_parse (repomd_file);
	free (repomd_file);

	/* Will need a way to flick this on later */
	/* XXX return some error when repomd is null */
	if (enabled && bind_dbs && repomd != NULL) {
		/* XXX don't assume .bz2 */
		char tmp;
		char *primary_db;
		char *filelists_db;

		tmp = repomd->primary_db[strlen (repomd->primary_db) - 4];
		repomd->primary_db[strlen (repomd->primary_db) - 4] = '\0';
		/* XXX don't assume 'repodata/' */
		primary_db = g_strdup_printf (LOCAL_CACHE "/%s/%s", id,
					      repomd->primary_db + 9);
		repomd->primary_db[strlen (repomd->primary_db) - 4] = tmp;

		tmp = repomd->filelists_db[strlen (repomd->filelists_db) - 4];
		repomd->filelists_db[strlen (repomd->filelists_db) - 4] = '\0';
		filelists_db = g_strdup_printf (LOCAL_CACHE "/%s/%s", id,
						repomd->filelists_db + 9);
		repomd->filelists_db[strlen (repomd->filelists_db) - 4] = tmp;

		low_debug ("Opening %s - %s\n", id, primary_db);
		low_debug ("Opening %s - %s\n", id, filelists_db);

		if (access (primary_db, R_OK) || access (filelists_db, R_OK)) {
			printf ("Can't open db files for repo '%s'! (try running 'yum makecache')\n", id);

			free (filelists_db);
			free (primary_db);
			low_repomd_free (repomd);
			free (repo);

			return NULL;
		}

		low_repo_sqlite_open_db (primary_db, &repo->primary_db);
		attach_db (repo->primary_db, filelists_db);
		sqlite3_create_function (repo->primary_db, "regexp", 2,
					 SQLITE_ANY, NULL,
					 low_repo_sqlite_regexp,
					 (sqlFunc) NULL, (sqlFinal) NULL);

		sqlite3_create_function (repo->primary_db, "filename_match", 2,
					 SQLITE_ANY, NULL,
					 low_repo_sqlite_filename_match,
					 (sqlFunc) NULL, (sqlFinal) NULL);


		/* XXX do this lazily */
		if (repomd->delta_xml != NULL) {
			char *delta_xml;

			tmp = repomd->delta_xml[strlen (repomd->delta_xml) - 3];
			repomd->delta_xml[strlen (repomd->delta_xml) - 3] =
				'\0';
			delta_xml = g_strdup_printf (LOCAL_CACHE "/%s/%s", id,
						     repomd->delta_xml + 9);
			repomd->delta_xml[strlen (repomd->delta_xml) - 3] =
				tmp;

			repo->delta = low_delta_parse (delta_xml);
			free (delta_xml);
		} else {
			repo->delta = NULL;
		}

		free (primary_db);
		free (filelists_db);
	} else {
		repo->primary_db = NULL;
		repo->filelists_db = NULL;
		repo->delta = NULL;
	}

	repo->table = NULL;
	repo->obsoletes = NULL;
	repo->mirrors = NULL;

	repo->super.id = strdup (id);
	repo->super.name = strdup (name);
	repo->super.baseurl = xstrdup (baseurl);
	repo->super.mirror_list = xstrdup (mirror_list);
	repo->super.enabled = enabled;

	low_repomd_free (repomd);

	return (LowRepo *) repo;
}

void
low_repo_sqlite_shutdown (LowRepo *repo)
{
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;

	free (repo->id);
	free (repo->name);
	free (repo->mirror_list);

	if (repo_sqlite->mirrors) {
		low_mirror_list_free (repo_sqlite->mirrors);
	}

	if (repo_sqlite->primary_db) {
		detach_db (repo_sqlite->primary_db);
		sqlite3_close (repo_sqlite->primary_db);
	}

	if (repo_sqlite->delta) {
		low_delta_free (repo_sqlite->delta);
	}

	if (repo_sqlite->table) {
		g_hash_table_destroy (repo_sqlite->table);
	}

	free (repo);
}

static LowPackage *
low_package_sqlite_new_from_row (sqlite3_stmt *pp_stmt, LowRepo *repo)
{
	int i = 0;
	int key;
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackage *pkg;

	if (!repo_sqlite->table) {
		low_debug ("initializing hash table\n");
		repo_sqlite->table =
			g_hash_table_new_full (g_int_hash, g_int_equal,
					       NULL,
					       (GDestroyNotify)
					       low_package_unref);
	}

	key = sqlite3_column_int (pp_stmt, 0);
	pkg = g_hash_table_lookup (repo_sqlite->table, &key);
	if (pkg) {
		low_debug ("CACHE HIT, for %d", key);
		low_package_ref (pkg);
		return pkg;
	}

	pkg = malloc (sizeof (LowPackage));
	low_package_ref_init (pkg);
	low_package_ref (pkg);

	/* XXX kind of hacky */
	pkg->id = malloc (sizeof (int));
	*((int *) pkg->id) = sqlite3_column_int (pp_stmt, i++);

	low_debug ("CACHE MISS, inserting %d", GPOINTER_TO_INT (pkg->id));
	g_hash_table_insert (repo_sqlite->table, pkg->id, pkg);

	pkg->name = strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->arch =
		low_arch_from_str ((const char *) sqlite3_column_text (pp_stmt,
								       i++));
	pkg->version =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->release =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->epoch = strdup ((const char *) sqlite3_column_text (pp_stmt, i++));

	pkg->size = sqlite3_column_int (pp_stmt, i++);
	pkg->repo = repo;

	pkg->location_href =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));

	pkg->digest =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	pkg->digest_type =
		low_util_digest_type_from_string ((const char *)
						  sqlite3_column_text
						  (pp_stmt, i));

	pkg->get_details = low_sqlite_package_get_details;

	pkg->get_provides = low_sqlite_package_get_provides;
	pkg->get_requires = low_sqlite_package_get_requires;
	pkg->get_conflicts = low_sqlite_package_get_conflicts;
	pkg->get_obsoletes = low_sqlite_package_get_obsoletes;

	pkg->provides = NULL;
	pkg->requires = NULL;
	pkg->conflicts = NULL;
	pkg->obsoletes = NULL;

	pkg->get_files = low_sqlite_package_get_files;

	return pkg;
}

static void
low_sqlite_package_iter_free (LowPackageIter *iter)
{
	LowPackageIterSqlite *iter_sqlite = (LowPackageIterSqlite *) iter;

	sqlite3_finalize (iter_sqlite->pp_stmt);

	if (iter_sqlite->filter_data_free_func) {
		iter_sqlite->filter_data_free_func (iter_sqlite->filter_data);
	}

	free (iter_sqlite);
}

static LowPackageIter *
low_sqlite_package_iter_next (LowPackageIter *iter)
{
	LowPackageIterSqlite *iter_sqlite = (LowPackageIterSqlite *) iter;

	if (sqlite3_step (iter_sqlite->pp_stmt) == SQLITE_DONE) {
		low_sqlite_package_iter_free (iter);
		return NULL;
	}

	iter->pkg = low_package_sqlite_new_from_row (iter_sqlite->pp_stmt,
						     iter->repo);
	if (iter_sqlite->func != NULL) {
		/* move on to the next package if this one fails the filter */
		if (!iter_sqlite->func (iter->pkg, iter_sqlite->filter_data)) {
			low_package_unref (iter->pkg);
			return low_package_iter_next (iter);
		}
	}
	return iter;
}

static LowPackageIterSqlite *
low_package_iter_sqlite_new (LowRepo *repo)
{
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));

	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.free_func = low_sqlite_package_iter_free;
	iter->super.pkg = NULL;

	iter->func = NULL;
	iter->filter_data_free_func = NULL;
	iter->filter_data = NULL;

	return iter;
}

LowPackageIter *
low_repo_sqlite_list_all (LowRepo *repo)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = low_package_iter_sqlite_new (repo);

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	return (LowPackageIter *) iter;
}


LowPackageIter *
low_repo_sqlite_list_by_name (LowRepo *repo, const char *name)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p WHERE name = :name";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = low_package_iter_sqlite_new (repo);

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, name, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

/* XXX duplicated */
typedef struct _DepFilterData {
	LowPackageDependency *dep;
	LowPackageGetDependency dep_func;
} DepFilterData;

static void
dep_filter_data_free_fn (gpointer data)
{
	DepFilterData *filter_data = (DepFilterData *) data;
	low_package_dependency_free (filter_data->dep);
	free (filter_data);
}

static bool
low_repo_sqlite_search_dep_filter_fn (LowPackage *pkg, gpointer data)
{
	DepFilterData *filter_data = (DepFilterData *) data;
	bool res = false;
	LowPackageDependency **deps = filter_data->dep_func (pkg);
	int i;

	for (i = 0; deps[i] != NULL; i++) {
		if (low_package_dependency_satisfies (filter_data->dep,
						      deps[i])) {
			res = true;
			break;
		}
	}

	return res;
}

static LowPackageIterSqlite *
low_package_iter_sqlite_new_with_filter (LowRepo *repo)
{
	DepFilterData *data = malloc (sizeof (DepFilterData));
	LowPackageIterSqlite *iter = malloc (sizeof (LowPackageIterSqlite));

	iter->super.repo = repo;
	iter->super.next_func = low_sqlite_package_iter_next;
	iter->super.free_func = low_sqlite_package_iter_free;
	iter->super.pkg = NULL;

	iter->func = low_repo_sqlite_search_dep_filter_fn;
	iter->filter_data_free_func = dep_filter_data_free_fn;
	iter->filter_data = (gpointer) data;

	return iter;
}

LowPackageIter *
low_repo_sqlite_search_provides (LowRepo *repo,
				 const LowPackageDependency *provides)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, provides pr "
			   "WHERE pr.pkgKey = p.pkgKey AND pr.name = :provides";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter =
		low_package_iter_sqlite_new_with_filter (repo);
	DepFilterData *data = (DepFilterData *) iter->filter_data;

	/* XXX might need to copy this */
	data->dep = low_package_dependency_new (provides->name, provides->sense,
						provides->evr);
	data->dep_func = low_package_get_provides;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, provides->name, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_requires (LowRepo *repo,
				 const LowPackageDependency *requires)
{
	const char *stmt = SELECT_FIELDS_FROM " packages p, requires req "
			   "WHERE req.pkgKey = p.pkgKey "
			   "AND req.name = :requires";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter =
		low_package_iter_sqlite_new_with_filter (repo);
	DepFilterData *data = (DepFilterData *) iter->filter_data;

	/* XXX might need to copy this */
	data->dep = low_package_dependency_new (requires->name, requires->sense,
						requires->evr);
	data->dep_func = low_package_get_requires;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, requires->name, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_conflicts (LowRepo *repo,
				  const LowPackageDependency *conflicts)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, conflicts conf "
			   "WHERE conf.pkgKey = p.pkgKey "
			   "AND conf.name = :conflicts";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter =
		low_package_iter_sqlite_new_with_filter (repo);
	DepFilterData *data = (DepFilterData *) iter->filter_data;

	/* XXX might need to copy this */
	data->dep = low_package_dependency_new (conflicts->name,
						conflicts->sense,
						conflicts->evr);
	data->dep_func = low_package_get_conflicts;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, conflicts->name, -1,
			   SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

static LowPackageDependencySense
low_package_dependency_sense_from_sqlite_flags (const unsigned char *flags)
{
	const char *sensestr = (const char *) flags;

	if (sensestr == NULL) {
		return DEPENDENCY_SENSE_NONE;
	} else if (!strcmp ("EQ", sensestr)) {
		return DEPENDENCY_SENSE_EQ;
	} else if (!strcmp ("GT", sensestr)) {
		return DEPENDENCY_SENSE_GT;
	} else if (!strcmp ("GE", sensestr)) {
		return DEPENDENCY_SENSE_GE;
	} else if (!strcmp ("LT", sensestr)) {
		return DEPENDENCY_SENSE_LT;
	} else if (!strcmp ("LE", sensestr)) {
		return DEPENDENCY_SENSE_LE;
	} else {
		/* XXX error here instead */
		return DEPENDENCY_SENSE_NONE;
	}
}

static char *
build_evr (const unsigned char *epoch, const unsigned char *version,
	   const unsigned char *release)
{
	char *evr;

	if (epoch && version && release) {
		evr = g_strdup_printf ("%s:%s-%s", epoch, version, release);
	} else if (epoch && version) {
		evr = g_strdup_printf ("%s:%s", epoch, version);
	} else if (version && release) {
		evr = g_strdup_printf ("%s-%s", version, release);
	} else {
		evr = xstrdup ((const char *) version);
	}

	return evr;
}

static void
add_dep_to_hash (GHashTable *table, const char *dep_name, int pkg_id)
{
	GSList *deps = g_hash_table_lookup (table, dep_name);
	deps = g_slist_prepend (deps, GINT_TO_POINTER (pkg_id));

	g_hash_table_replace (table, strdup (dep_name), deps);
}

static void
low_repo_sqlite_initialize_obsoletes (LowRepoSqlite *repo_sqlite)
{
	const char *stmt = "SELECT name, pkgKey from obsoletes";
	sqlite3_stmt *pp_stmt;

	repo_sqlite->obsoletes = g_hash_table_new (g_str_hash, g_str_equal);

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &pp_stmt, NULL);

	while (sqlite3_step (pp_stmt) != SQLITE_DONE) {
		const char *dep_name =
			(const char *) sqlite3_column_text (pp_stmt, 0);

		add_dep_to_hash (repo_sqlite->obsoletes, dep_name,
				 sqlite3_column_int (pp_stmt, 1));
	}

	sqlite3_finalize (pp_stmt);
}

LowPackageIter *
low_repo_sqlite_search_obsoletes (LowRepo *repo,
				  const LowPackageDependency *obsoletes)
{
	char *stmt;
	GSList *obs;
	char *tmp;

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter =
		low_package_iter_sqlite_new_with_filter (repo);
	DepFilterData *data = (DepFilterData *) iter->filter_data;

	if (repo_sqlite->obsoletes == NULL) {
		low_repo_sqlite_initialize_obsoletes (repo_sqlite);
	}

	obs = g_hash_table_lookup (repo_sqlite->obsoletes, obsoletes->name);
	if (obs == NULL) {
		/* Just an ID we don't find */
		tmp = strdup ("-1);");
	} else {
		char *tmp2;
		tmp = g_strdup_printf ("%d", GPOINTER_TO_INT (obs->data));
		obs = obs->next;

		while (obs != NULL) {
			tmp2 = g_strdup_printf ("%s, %d", tmp,
						GPOINTER_TO_INT (obs->data));
			free (tmp);
			tmp = tmp2;
			obs = obs->next;
		}

		tmp2 = g_strdup_printf ("%s)", tmp);
		free (tmp);
		tmp = tmp2;
	}
	stmt = g_strdup_printf (SELECT_FIELDS_FROM "packages p "
			   "WHERE p.pkgKey IN (%s", tmp);

	/* XXX might need to copy this */
	data->dep = low_package_dependency_new (obsoletes->name,
						obsoletes->sense,
						obsoletes->evr);
	data->dep_func = low_package_get_obsoletes;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	free (stmt);

	return (LowPackageIter *) iter;
}

static LowPackageIter *
low_repo_sqlite_search_primary_files (LowRepo *repo, const char *file)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p, files f "
			   "WHERE f.pkgKey = p.pkgKey "
			   "AND f.name = :file";

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = low_package_iter_sqlite_new (repo);

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, file, -1, SQLITE_STATIC);
	return (LowPackageIter *) iter;
}

static LowPackageIter *
low_repo_sqlite_search_filelists_files (LowRepo *repo, const char *file)
{
	/* XXX try optimizing for cases where there's only one filename */
	const char *stmt = SELECT_FIELDS_FROM "packages p, filelist f "
			   "WHERE f.pkgKey = p.pkgKey "
			   "AND f.dirname = :dir "
			   "AND filename_match (f.filenames, :file)";

	char *slash = rindex (file, '/');
	char *filename = strdup ((char *) (slash + 1));
	char *dirname = strndup (file, slash - file);

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = low_package_iter_sqlite_new (repo);

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, dirname, -1, free);
	sqlite3_bind_text (iter->pp_stmt, 2, filename, -1, free);

	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_sqlite_search_files (LowRepo *repo, const char *file)
{
	/* Createrepo puts these files in primary.xml */
	if (strstr (file, "bin/") || g_str_has_prefix (file, "/etc/")
	    || !strcmp (file, "/usr/lib/sendmail")) {
		return low_repo_sqlite_search_primary_files (repo, file);
	} else {
		return low_repo_sqlite_search_filelists_files (repo, file);
	}
}

/**
 * Search name, summary, description and & url for the provided string.
 *
 */
LowPackageIter *
low_repo_sqlite_search_details (LowRepo *repo, const char *querystr)
{
	const char *stmt = SELECT_FIELDS_FROM "packages p "
			   "WHERE p.name LIKE :query OR "
			   "p.summary LIKE :query OR "
			   "p.description LIKE :query OR "
			   "p.url LIKE :query";

	char *like_querystr = g_strdup_printf ("%%%s%%", querystr);

	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;
	LowPackageIterSqlite *iter = low_package_iter_sqlite_new (repo);

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &iter->pp_stmt,
			 NULL);
	sqlite3_bind_text (iter->pp_stmt, 1, like_querystr, -1, free);

	return (LowPackageIter *) iter;
}

static LowPackageDependency **
low_repo_sqlite_get_deps (LowRepo *repo, const char *stmt, LowPackage *pkg)
{
	size_t deps_size = 5;
	LowPackageDependency **deps =
		malloc (sizeof (LowPackageDependency *) * deps_size);
	unsigned int i = 0;

	sqlite3_stmt *pp_stmt;
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_int (pp_stmt, 1, *((int *) pkg->id));

	while (sqlite3_step (pp_stmt) != SQLITE_DONE) {
		const char *dep_name =
			(const char *) sqlite3_column_text (pp_stmt, 0);
		LowPackageDependencySense sense =
			low_package_dependency_sense_from_sqlite_flags (sqlite3_column_text (pp_stmt, 1));
		char *evr = build_evr (sqlite3_column_text (pp_stmt, 2),
				       sqlite3_column_text (pp_stmt, 3),
				       sqlite3_column_text (pp_stmt, 4));

		deps[i++] = low_package_dependency_new (dep_name, sense, evr);

		free (evr);

		if (i == deps_size - 1) {
			deps_size *= 2;
			deps = realloc (deps, sizeof (char *) * deps_size);
		}
	}

	sqlite3_finalize (pp_stmt);

	deps[i] = NULL;
	return deps;
}

static LowMirrorList *
build_mirror_list (LowRepo *repo)
{
	LowMirrorList *all_mirrors;

	if (repo->mirror_list == NULL) {
		all_mirrors = low_mirror_list_new_from_baseurl (repo->baseurl);
	} else if (strstr (repo->mirror_list, "metalink")) {
		char *mirrors_file =
			g_strdup_printf ("/var/cache/yum/%s/metalink.xml",
					 repo->id);

		all_mirrors = low_mirror_list_new_from_metalink (mirrors_file);

		free (mirrors_file);

	} else {
		char *mirrors_file =
			g_strdup_printf ("/var/cache/yum/%s/mirrorlist.txt",
					 repo->id);

		all_mirrors = low_mirror_list_new_from_txt_file (mirrors_file);

		free (mirrors_file);
	}

	return all_mirrors;
}

LowMirrorList *
low_repo_sqlite_get_mirror_list (LowRepo *repo)
{
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;

	if (repo_sqlite->mirrors == NULL) {
		repo_sqlite->mirrors = build_mirror_list (repo);
	}

	return repo_sqlite->mirrors;
}

LowDelta *
low_repo_sqlite_get_delta (LowRepo *repo)
{
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) repo;

	return repo_sqlite->delta;
}

LowPackageDetails *
low_sqlite_package_get_details (LowPackage *pkg)
{
	const char *stmt = "SELECT summary, description, url, rpm_license "
			   "FROM packages where pkgKey=:pkgKey";

	LowPackageDetails *details = malloc (sizeof (LowPackageDetails));
	int i = 0;

	sqlite3_stmt *pp_stmt;
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) pkg->repo;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_int (pp_stmt, 1, *((int *) pkg->id));

	sqlite3_step (pp_stmt);

	details->summary =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	details->description =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));

	details->url =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i++));
	details->license =
		strdup ((const char *) sqlite3_column_text (pp_stmt, i));

	sqlite3_finalize (pp_stmt);

	return details;
}

#define DEP_QUERY(type) "SELECT name, flags, epoch, version, release from " \
			type " WHERE pkgKey=:pkgkey"

LowPackageDependency **
low_sqlite_package_get_provides (LowPackage *pkg)
{
	if (!pkg->provides) {
		const char *stmt = DEP_QUERY ("provides");
		pkg->provides = low_repo_sqlite_get_deps (pkg->repo, stmt, pkg);
	}
	return pkg->provides;
}

LowPackageDependency **
low_sqlite_package_get_requires (LowPackage *pkg)
{
	if (!pkg->requires) {
		const char *stmt = DEP_QUERY ("requires");
		pkg->requires = low_repo_sqlite_get_deps (pkg->repo, stmt, pkg);
	}
	return pkg->requires;
}

LowPackageDependency **
low_sqlite_package_get_conflicts (LowPackage *pkg)
{
	if (!pkg->conflicts) {
		const char *stmt = DEP_QUERY ("conflicts");
		pkg->conflicts = low_repo_sqlite_get_deps (pkg->repo, stmt,
							   pkg);
	}
	return pkg->conflicts;
}

LowPackageDependency **
low_sqlite_package_get_obsoletes (LowPackage *pkg)
{
	if (!pkg->obsoletes) {
		const char *stmt = DEP_QUERY ("obsoletes");
		pkg->obsoletes = low_repo_sqlite_get_deps (pkg->repo, stmt,
							   pkg);
	}
	return pkg->obsoletes;
}

char **
low_sqlite_package_get_files (LowPackage *pkg)
{
	const char *stmt = "SELECT dirname, filenames FROM filelist "
			   "WHERE pkgKey = :pkgKey";

	size_t files_size = 5;
	char **files = malloc (sizeof (char *) * files_size);
	unsigned int i = 0;

	sqlite3_stmt *pp_stmt;
	LowRepoSqlite *repo_sqlite = (LowRepoSqlite *) pkg->repo;

	sqlite3_prepare (repo_sqlite->primary_db, stmt, -1, &pp_stmt, NULL);
	sqlite3_bind_int (pp_stmt, 1, *((int *) pkg->id));

	while (sqlite3_step (pp_stmt) != SQLITE_DONE) {
		int j;
		const unsigned char *dir = sqlite3_column_text (pp_stmt, 0);
		const unsigned char *names = sqlite3_column_text (pp_stmt, 1);
		char **names_split = g_strsplit ((const char *) names, "/", -1);

		for (j = 0; names_split[j] != NULL; j++) {
			files[i++] = g_strdup_printf ("%s/%s", dir,
						      names_split[j]);
			if (i == files_size - 1) {
				files_size *= 2;
				files = realloc (files,
						 sizeof (char *) * files_size);
			}
		}

		g_strfreev (names_split);
	}

	sqlite3_finalize (pp_stmt);

	files[i] = NULL;
	return files;
}

/* vim: set ts=8 sw=8 noet: */
