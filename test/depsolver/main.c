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
#include <stdio.h>
#include <string.h>

#include <glib.h>
#include <syck.h>

#include "low-debug.h"
#include "low-package.h"
#include "low-repo-set.h"
#include "low-repo-rpmdb.h"
#include "low-repo-sqlite.h"
#include "low-transaction.h"
#include "low-fake-repo.h"

FAKE_RPMDB;
FAKE_SQLITE_REPO;

/**********************************************************************
 * YAML parsing functions
 **********************************************************************/

/**
 * Our symbol table for syck.
 */
GHashTable *symbols;

/**
 * An id we increment for the symbol table
 */
unsigned long int global_id = 0;

static void
parse_string (SyckNode *node)
{
	g_hash_table_insert (symbols, GINT_TO_POINTER (node->id),
			     g_strdup (node->data.str->ptr));
}

static void
parse_list (SyckNode *node)
{
	GSList *list = NULL;
	int i;

	for (i = 0; i < syck_seq_count (node); i++) {
		gpointer sym_entry = GINT_TO_POINTER (syck_seq_read (node, i));
		gpointer entry = g_hash_table_lookup (symbols, sym_entry);

		list  =	g_slist_append (list, entry);
	}
	g_hash_table_insert (symbols, GINT_TO_POINTER (node->id), list);
}

static void
parse_hash (SyckNode *node)
{
	GHashTable *table = g_hash_table_new (g_str_hash, g_str_equal);
	int i;

	for (i = 0; i < syck_map_count (node); i++) {
		SYMID key = syck_map_read (node, map_key, i);
		SYMID value = syck_map_read (node, map_value, i);

		gpointer skey =
			g_hash_table_lookup (symbols, GINT_TO_POINTER (key));
		gpointer svalue =
			g_hash_table_lookup (symbols, GINT_TO_POINTER (value));
		g_hash_table_insert (table, skey, svalue);
	}

	g_hash_table_insert (symbols, GINT_TO_POINTER (node->id), table);
}

static SYMID
node_handler (SyckParser *parser G_GNUC_UNUSED, SyckNode *node)
{
	node->id = global_id++;
	if (node->kind == syck_str_kind) {
		parse_string (node);
	} else if (node->kind == syck_seq_kind) {
		parse_list (node);
	} else if (node->kind == syck_map_kind) {
		parse_hash (node);
	} else {
		printf ("UNKNOWN NODE TYPE\n");
		exit (1);
	}

	return node->id;
}

static GHashTable *
parse_yaml (const char *file_name)
{
	SYMID top_symid;
	GHashTable *top_hash;
	SyckParser *parser = syck_new_parser ();
	FILE *file;

	symbols = g_hash_table_new (NULL, NULL);

	syck_parser_handler (parser, node_handler);

	file = fopen (file_name, "r");
	syck_parser_file (parser, file, NULL);

	top_symid = syck_parse (parser);

	syck_free_parser (parser);

	top_hash = g_hash_table_lookup (symbols, GINT_TO_POINTER (top_symid));

	return top_hash;
}

/**********************************************************************
 * A fake package type that holds its own dep info
 **********************************************************************/

typedef struct _LowFakePackage {
	LowPackage super;

	LowPackageDependency **provides;
	LowPackageDependency **requires;
	LowPackageDependency **conflicts;
	LowPackageDependency **obsoletes;

	char **files;
} LowFakePackage;

static size_t
package_dependency_lenv (LowPackageDependency **dependencies)
{
	unsigned int i;

	for (i = 0; dependencies[i] != NULL; i++) ;

	return i;
}
static LowPackageDependency **
package_dependency_dupv (LowPackageDependency **dependencies)
{
	int i;
	LowPackageDependency **dep_copy;

	for (i = 0; dependencies[i] != NULL; i++) ;

	dep_copy = malloc (sizeof (LowPackageDependency *) * (i + 1));

	for (i = 0; dependencies[i] != NULL; i++) {
		dep_copy[i] =
			low_package_dependency_new (dependencies[i]->name,
						    dependencies[i]->sense,
						    dependencies[i]->evr);
	}
	dep_copy[i] = NULL;

	return dep_copy;
}

static LowPackageDependency **
low_fake_package_get_provides (LowPackage *pkg) {
	return package_dependency_dupv (((LowFakePackage *) pkg)->provides);
}

static LowPackageDependency **
low_fake_package_get_requires (LowPackage *pkg) {
	return package_dependency_dupv (((LowFakePackage *) pkg)->requires);
}

static LowPackageDependency **
low_fake_package_get_conflicts (LowPackage *pkg) {
	return package_dependency_dupv (((LowFakePackage *) pkg)->conflicts);
}

static LowPackageDependency **
low_fake_package_get_obsoletes (LowPackage *pkg) {
	return package_dependency_dupv (((LowFakePackage *) pkg)->obsoletes);
}

static char **
low_fake_package_get_files (LowPackage *pkg) {
	return g_strdupv (((LowFakePackage *) pkg)->files);
}

/**********************************************************************
 * Repo & Package creation from parsed YAML functions
 **********************************************************************/

static void
parse_evr (const char *evr, char **epoch, char **version, char **release)
{
	char *didx;
	const char *cidx = index (evr, ':');

	if (cidx == NULL) {
		*epoch = NULL;
		cidx = evr - 1;
	} else {
		*epoch = g_strndup (evr, cidx - evr);
	}

	didx = rindex (evr, '-');

	if (didx == NULL) {
		*version = g_strdup (cidx + 1);
	} else {
		*version = g_strndup (cidx + 1, didx - (cidx + 1));
		*release = g_strdup (didx + 1);
	}
}

static LowPackageDependency **
parse_package_dep (GHashTable *hash, const char *dep_type)
{
	GSList *dep = g_hash_table_lookup (hash, dep_type);
	LowPackageDependency **deps = malloc (sizeof (LowPackageDependency *) *
					      (g_slist_length (dep) + 1));
	int i = 0;

	for (; dep != NULL; dep = dep->next) {
		char **parts = g_strsplit (dep->data, " ", 0);
		LowPackageDependencySense sense = DEPENDENCY_SENSE_NONE;
		char *evr = NULL;

		if (g_strv_length (parts) == 3) {
			sense = low_package_dependency_sense_from_string (parts[1]);
			evr = parts[2];
		}

		deps[i++] = low_package_dependency_new (parts[0],
							sense,
							evr);
	}
	deps[i] = NULL;

	return deps;
}

static char **
parse_package_files (GHashTable *hash)
{
	GSList *file = g_hash_table_lookup (hash, "files");
	char **files = malloc (sizeof (char *) * (g_slist_length (file) + 1));
	int i = 0;

	for (; file != NULL; file = file->next) {
		files[i++] = file->data;
	}
	files[i] = NULL;

	return files;
}

static LowPackage *
low_package_from_hash (GHashTable *hash)
{
	LowFakePackage *fake_pkg = malloc (sizeof (LowFakePackage));
	LowPackage *pkg = (LowPackage *) fake_pkg;
	GHashTable *nevra_hash;
	const char *evr;
	char **empty_dep;

	// We always keep a ref to the test pkgs
	low_package_ref_init (pkg);

	/* Default empty deps for prco */
	empty_dep = malloc (sizeof (char *));
	empty_dep[0] = NULL;

	nevra_hash = g_hash_table_lookup (hash, "package");
	evr = g_hash_table_lookup (nevra_hash, "evr");

	pkg->name = g_hash_table_lookup (nevra_hash, "name");
	pkg->arch = g_hash_table_lookup (nevra_hash, "arch");

	parse_evr (evr, &pkg->epoch, &pkg->version, &pkg->release);

	/* Have to add the implicit provides on the pkg's name */
	fake_pkg->provides = parse_package_dep (hash, "provides");
	fake_pkg->provides =
		realloc (fake_pkg->provides,
			 (package_dependency_lenv (fake_pkg->provides) + 1) *
			 sizeof (char *));

	fake_pkg->provides[package_dependency_lenv (fake_pkg->provides) + 1] =
		NULL;
	fake_pkg->provides[package_dependency_lenv (fake_pkg->provides)] =
		low_package_dependency_new (pkg->name, DEPENDENCY_SENSE_EQ,
					    evr);

	fake_pkg->requires = parse_package_dep (hash, "requires");
	fake_pkg->conflicts = parse_package_dep (hash, "conflicts");
	fake_pkg->obsoletes = parse_package_dep (hash, "obsoletes");
	fake_pkg->files = parse_package_files (hash);

	pkg->get_provides = low_fake_package_get_provides;
	pkg->get_requires = low_fake_package_get_requires;
	pkg->get_conflicts = low_fake_package_get_conflicts;
	pkg->get_obsoletes = low_fake_package_get_obsoletes;
	pkg->get_files = low_fake_package_get_files;

	low_debug_pkg ("found package", pkg);

	return pkg;
}

static LowRepo *
low_repo_from_list (const char *name, const char *id, gboolean enabled,
		    GSList *list)
{
	LowRepo *repo = low_fake_repo_initialize (name, id, enabled);
	LowPackage **packages =
		malloc (sizeof (LowPackage *) * (g_slist_length (list) + 1));
	GSList *cur;
	int i;

	for (cur = list, i = 0; cur != NULL; cur = cur->next, i++) {
		packages[i] = low_package_from_hash (cur->data);
	}
	packages[i] = NULL;

	((LowFakeRepo *) repo)->packages = packages;
	return repo;
}

static LowPackage *
find_package (LowRepo *repo, GHashTable *hash)
{
	char *name = g_hash_table_lookup (hash, "name");
	char *arch = g_hash_table_lookup (hash, "arch");
	char *evr = g_hash_table_lookup (hash, "evr");

	char *epoch = NULL;
	char *version = NULL;
	char *release = NULL;

	LowPackageIter *iter;

	if (evr != NULL) {
		parse_evr (evr, &epoch, &version, &release);
	}

	/* We need at least the name. */
	iter = low_fake_repo_list_all (repo);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;

		if (strcmp (name, pkg->name)) {
			low_package_unref (pkg);
			continue;
		}
		if (arch && strcmp (arch, pkg->arch)) {
			low_package_unref (pkg);
			continue;
		}
		if (epoch && strcmp (epoch, pkg->epoch)) {
			low_package_unref (pkg);
			continue;
		}
		if (version && strcmp (version, pkg->version)) {
			low_package_unref (pkg);
			continue;
		}
		if (release && strcmp (release, pkg->release)) {
			low_package_unref (pkg);
			continue;
		}

		return pkg;
	}

	printf ("could not find matching package\n");
	exit (EXIT_FAILURE);
}

static void
schedule_transactions (LowTransaction *trans, LowRepo *installed G_GNUC_UNUSED,
		       LowRepo *available G_GNUC_UNUSED, GSList *list)
{
	GSList *cur;
	LowPackage *pkg;

	for (cur = list; cur != NULL; cur = cur->next) {
		GHashTable *table = cur->data;
		/* There should only be a single key */
		GList *keys = g_hash_table_get_keys (table);
		char *op = keys->data;
		if (!strcmp (op, "update")) {
			low_debug ("scheduling update");
			pkg = find_package (installed,
					    g_hash_table_lookup (table, op));
			low_transaction_add_update (trans, pkg);
		} else if (!strcmp (op, "install")) {
			low_debug ("scheduling install");
			pkg = find_package (available,
					    g_hash_table_lookup (table, op));
			low_transaction_add_install (trans, pkg);
		} else if (!strcmp (op, "remove")) {
			low_debug ("scheduling remove");
			pkg = find_package (installed,
					    g_hash_table_lookup (table, op));
			low_transaction_add_remove (trans, pkg);
		} else {
			printf ("Unknown operation %s\n", op);
			exit (EXIT_FAILURE);
		}
	}

}

static int
assert_package (GHashTable *pkgs, GHashTable *hash)
{
	char *name = g_hash_table_lookup (hash, "name");
	char *arch = g_hash_table_lookup (hash, "arch");
	char *evr = g_hash_table_lookup (hash, "evr");

	char *epoch = NULL;
	char *version = NULL;
	char *release = NULL;

	GList *cur;

	if (evr != NULL) {
		parse_evr (evr, &epoch, &version, &release);
	}

	for (cur = g_hash_table_get_values (pkgs); cur != NULL;
	     cur = cur->next) {
		LowTransactionMember *member = cur->data;
		LowPackage *pkg = member->pkg;
		low_debug_pkg ("on package", pkg);
		if (!strcmp (name, pkg->name) &&
		    !strcmp (version, pkg->version) &&
		    !strcmp (release, pkg->release) &&
		    !strcmp (arch, pkg->arch) &&
		    (!(epoch || pkg->epoch) || !strcmp (epoch, pkg->epoch))) {
			return 0;
		}
	}
	printf ("No matching package found\n");

	return 1;
}

static int
compare_results (LowTransactionResult trans_res, LowTransaction *trans,
		 GSList *list)
{
	GSList *cur;
	int res;

	unsigned int expected_updates = 0;
	unsigned int expected_installs = 0;
	unsigned int expected_removals = 0;
	unsigned int expected_unresolved = 0;

	for (cur = list; cur != NULL; cur = cur->next) {
		GHashTable *table = cur->data;
		/* There should only be a single key */
		GList *keys = g_hash_table_get_keys (table);
		char *op = keys->data;
		if (!strcmp (op, "update")) {
			expected_updates++;
			res = assert_package (trans->update,
					      g_hash_table_lookup (table, op));
		} else if (!strcmp (op, "install")) {
			expected_installs++;
			res = assert_package (trans->install,
					      g_hash_table_lookup (table, op));
		} else if (!strcmp (op, "remove")) {
			expected_removals++;
			res = assert_package (trans->remove,
					      g_hash_table_lookup (table, op));
		} else if (!strcmp (op, "unresolved")) {
			expected_unresolved++;
			res = assert_package (trans->unresolved,
					      g_hash_table_lookup (table, op));
		} else {
			printf ("Unknown operation %s\n", op);
			exit (EXIT_FAILURE);
		}

		if (res != 0) {
			return res;
		}
	}

	if (expected_installs != g_hash_table_size (trans->install)) {
		printf ("Unexpected number of packages marked for install\n");
		return 1;
	}
	if (expected_removals != g_hash_table_size (trans->remove)) {
		printf ("Unexpected number of packages marked for remove\n");
		return 1;
	}
	if (expected_updates != g_hash_table_size (trans->update)) {
		printf ("Unexpected number of packages marked for update\n");
		return 1;
	}

	if (trans_res == LOW_TRANSACTION_OK && expected_unresolved != 0) {
		printf ("Expected transaction to fail but it succeeded\n");
		return 1;
	}
	if (trans_res == LOW_TRANSACTION_UNRESOLVED && expected_installs != 0
	    && expected_removals != 0 && expected_updates != 0) {
		return 1;
	}

	return 0;
}

static int
run_test (GHashTable *test)
{
	LowRepo *installed;
	LowRepo *available;
	LowRepoSet *repo_set;
	LowTransaction *trans;
	LowTransactionResult trans_res;
	GSList *list;
	int res;


	installed =
		low_repo_from_list ("installed", "installed", TRUE,
				    g_hash_table_lookup (test, "installed"));

	available =
		low_repo_from_list ("available", "available", TRUE,
				    g_hash_table_lookup (test, "available"));

	repo_set = malloc (sizeof (LowRepoSet));
	repo_set->repos = g_hash_table_new (NULL, NULL);

	g_hash_table_insert (repo_set->repos, available->id, available);

	trans = low_transaction_new (installed, repo_set);

	list = g_hash_table_lookup (test, "transaction");

	schedule_transactions (trans, installed, available, list);
	trans_res = low_transaction_resolve (trans);

	list = g_hash_table_lookup (test, "results");
	res = compare_results (trans_res, trans, list);

	low_transaction_free (trans);

	return res;
}

int
main (int argc, char *argv[])
{
	int res;
	GHashTable *top_hash;

	if (argc != 2) {
		printf ("Usage: %s FILE\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	printf("Starting test\n");

	low_debug_init ();

	top_hash = parse_yaml (argv[1]);

	res = run_test (top_hash);

	if (!res) {
		printf ("Test passed\n");
	} else {
		printf ("Test failed\n");
	}

	return res;
}

/* vim: set ts=8 sw=8 noet: */
