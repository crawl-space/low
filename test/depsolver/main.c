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

GHashTable *symbols;
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

static void
print_package (gpointer pkg)
{
	GHashTable *table = (GHashTable *) pkg;

	GHashTableIter iter;
	gpointer key, value;

	g_hash_table_iter_init (&iter, table);
	while (g_hash_table_iter_next (&iter, &key, &value)) {
		printf ("%s - %s\n", (char *) key, (char *) value);
	}

}

static void
print_repo (gpointer repo)
{
	GSList *list = (GSList *) repo;

	while (list != NULL) {
		print_package (list->data);
		list = list->next;
	}
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

int
main (int argc, char *argv[])
{
	GHashTable *top_hash;

	if (argc != 2) {
		printf ("Usage: %s FILE\n", argv[0]);
		exit (EXIT_FAILURE);
	}

	top_hash = parse_yaml (argv[1]);

	print_repo (g_hash_table_lookup (top_hash, "installed"));

	return EXIT_SUCCESS;
}

/* vim: set ts=8 sw=8 noet: */
