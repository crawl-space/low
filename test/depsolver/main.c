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
unsigned long int id = 0;

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
	node->id = id++;
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

static LowPackage *
low_package_from_hash (GHashTable *hash)
{
	LowPackage *pkg = malloc (sizeof (LowPackage));

	hash = g_hash_table_lookup (hash, "package");

	pkg->name = g_hash_table_lookup (hash, "name");
	pkg->arch = g_hash_table_lookup (hash, "arch");

	parse_evr (g_hash_table_lookup (hash, "evr"), &pkg->epoch,
		   &pkg->version, &pkg->release);

	low_debug_pkg ("found package", pkg);

	return pkg;
}

static LowRepo *
low_repo_from_list (char *name, char *id, gboolean enabled, GSList *list)
{
	LowRepo *repo = low_fake_repo_initialize (name, id, enabled);
	LowPackage **packages =
		malloc (sizeof (LowPackage *) * g_slist_length (list));
	GSList *cur = list;
	int i = 0;

	while (cur != NULL) {
		packages[i++] = low_package_from_hash (cur->data);
		cur = cur->next;
	}

	((LowFakeRepo *) repo)->packages = packages;
	return repo;
}

static void
schedule_transactions (LowTransaction *trans G_GNUC_UNUSED, GSList *list G_GNUC_UNUSED)
{

}

static void
run_test (GHashTable *test)
{
	LowRepo *installed;
	LowRepo *available;
	LowRepoSet *repo_set;
	LowTransaction *trans;
	GSList *list;


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

	schedule_transactions (trans, list);
	low_transaction_resolve (trans);

	low_transaction_free (trans);
}

int
main (int argc, char *argv[])
{
	GHashTable *top_hash;

	if (argc != 2) {
		printf ("Usage: %s FILE\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	low_debug_init ();

	top_hash = parse_yaml (argv[1]);

	run_test (top_hash);

	return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 noet: */
