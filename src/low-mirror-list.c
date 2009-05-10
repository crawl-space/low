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
#include "low-metalink-parser.h"

LowMirrorList *
low_mirror_list_new (void)
{
	LowMirrorList *mirrors = malloc (sizeof (LowMirrorList));
	mirrors->current_weight = 100;
	mirrors->mirrors = NULL;

	return mirrors;
}

static void
free_g_list_node (gpointer data_ptr, gpointer ignored G_GNUC_UNUSED)
{
	LowMirror *mirror = (LowMirror *) data_ptr;

	free (mirror->url);
	free (mirror);
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
				LowMirror *mirror = malloc (sizeof (LowMirror));
				mirror->url = url->str;
				/* txt mirrors are unweighted */
				mirror->weight = 100;
				mirrors->mirrors =
					g_list_append (mirrors->mirrors,
						       mirror);
			}
			g_string_free (url, FALSE);
			url = g_string_new ("");
			continue;
		}
		url = g_string_append_c (url, x);
	}
	fclose (file);

	return mirrors;
}

LowMirrorList *
low_mirror_list_new_from_metalink (const char *mirrorlist_txt)
{
	LowMirrorList *mirrors = low_mirror_list_new ();

	mirrors->mirrors = low_metalink_parse (mirrorlist_txt);

	return mirrors;
}

/* Generate a random integer between 0 and upper bound, including 0 */
static int
random_int (int upper)
{
	/* Not the most random thing in the world but do we care? */
	unsigned int iseed = (unsigned int) time (NULL);
	srand (iseed);
	return rand () % upper;
}

const gchar *
low_mirror_list_lookup_random_mirror (LowMirrorList *mirrors)
{
	int number_at_current_weight = 0;
	int count = 0;
	int choice;
	LowMirror *mirror = NULL;
	GList *cur;

	for (cur = mirrors->mirrors; cur != NULL; cur = cur->next) {
		mirror = (LowMirror *) cur->data;
		if (mirror->weight == mirrors->current_weight) {
			number_at_current_weight++;
		}
	}

	choice = random_int (number_at_current_weight);
	for (cur = mirrors->mirrors; cur != NULL; cur = cur->next) {
		mirror = (LowMirror *) cur->data;
		if (mirror->weight == mirrors->current_weight) {
			if (count == choice) {
				break;
			} else {
				count++;
			}
		}
	}

	return mirror->url;
}

/* vim: set ts=8 sw=8 noet: */
