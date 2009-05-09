/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2009 James Bowes <jbowes@dangerouslyinc.com>
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

#include <stdio.h>
#include <stdlib.h>
#include "low-mirror-list.h"

LowMirrorList *
low_mirror_list_new (void)
{
	LowMirrorList *mirrors = malloc (sizeof (LowMirrorList));
	mirrors->mirrors = NULL;

	return mirrors;
}

static void
free_g_list_node (gpointer data_ptr, gpointer ignored G_GNUC_UNUSED)
{
	g_string_free ((GString *) data_ptr, TRUE);
}

void
low_mirror_list_free (LowMirrorList *mirrors)
{
	/* XXX free list entries */

	g_list_foreach (mirrors->mirrors, free_g_list_node, NULL);
	g_list_free (mirrors->mirrors);

	g_free (mirrors);
}

LowMirrorList *
low_mirror_list_new_from_txt_file (const char *mirrorlist_txt)
{
	LowMirrorList *mirrors = low_mirror_list_new ();
	gchar x;
	GString *url;

	FILE *file = fopen (mirrorlist_txt, "r");
	if (file == 0) {
		printf ("Error opening file: %s\n", mirrorlist_txt);
		return NULL;
	}

	url = g_string_new ("");
	while ((x = fgetc (file)) != EOF) {
		if (x == '\n') {
			/* g_print ("Final string: %s\n", url->str); */
			/* Ignore lines commented out: */
			if (url->str[0] != '#') {
				mirrors->mirrors =
					g_list_append (mirrors->mirrors,
						       (gpointer) url);
			}
			url = g_string_new ("");
			continue;
		}
		url = g_string_append_c (url, x);
	}
	fclose (file);

	return mirrors;
}

/* Generate a random integer between 0 and upper bound. (inclusive) */
static int
random_int (int upper)
{
	/* Not the most random thing in the world but do we care? */
	unsigned int iseed = (unsigned int) time (NULL);
	srand (iseed);
	return rand () % (upper + 1);
}

const gchar *
low_mirror_list_lookup_random_mirror (LowMirrorList *mirrors)
{
	int choice;
	const GString *random_url;

	choice = random_int (g_list_length (mirrors->mirrors) - 1);
	random_url = (GString *) g_list_nth_data (mirrors->mirrors, choice);

	return random_url->str;
}
/* vim: set ts=8 sw=8 noet: */
