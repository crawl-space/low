/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2009 James Bowes <jbowes@repl.ca>
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
#include <string.h>
#include "low-mirror-list.h"
#include "low-metalink-parser.h"

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

	free (mirrors);
}

LowMirrorList *
low_mirror_list_new_from_baseurl (const char *baseurl)
{
	LowMirrorList *mirrors = low_mirror_list_new ();

	LowMirror *mirror = malloc (sizeof (LowMirror));
	mirror->url = strdup (baseurl);
	mirror->weight = 100;
	mirror->is_bad = false;
	mirrors->mirrors = g_list_append (mirrors->mirrors, mirror);

	return mirrors;
}

#define BUF_SIZE 1024
LowMirrorList *
low_mirror_list_new_from_txt_file (const char *mirrorlist_txt)
{
	LowMirrorList *mirrors = low_mirror_list_new ();
	char *line;
	size_t length;
	ssize_t read;

	FILE *file = fopen (mirrorlist_txt, "r");
	if (file == 0) {
		printf ("Error opening file: %s\n", mirrorlist_txt);
		return NULL;
	}

	length = BUF_SIZE;
	line = malloc (sizeof (char) * length);

	while ((read = getline (&line, &length, file)) != -1) {
		/* Ignore comments and blank lines */
		if (line[0] != '#' && read > 1) {
			size_t to_copy = read + 1;
			LowMirror *mirror = malloc (sizeof (LowMirror));

			if (line[to_copy - 2] == '\n') {
				line[to_copy - 2] = '\0';
				to_copy--;
			}

			mirror->url = malloc (sizeof (char) * to_copy);
			strncpy (mirror->url, line, to_copy);

			/* txt mirrors are unweighted */
			mirror->weight = 100;
			mirror->is_bad = false;
			mirrors->mirrors =
				g_list_append (mirrors->mirrors, mirror);
		}
	}

	fclose (file);
	free (line);

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

const char *
low_mirror_list_lookup_random_mirror (LowMirrorList *mirrors)
{
	int weight = 0;
	int number_at_current_weight = 0;
	int count = 0;
	int choice;
	LowMirror *mirror = NULL;
	GList *cur;

	for (cur = mirrors->mirrors; cur != NULL; cur = cur->next) {
		mirror = (LowMirror *) cur->data;

		if (mirror->is_bad) {
			continue;
		}

		if (mirror->weight > weight) {
			weight = mirror->weight;
			number_at_current_weight = 1;
		} else if (mirror->weight == weight) {
			number_at_current_weight++;
		}
	}

	choice = random_int (number_at_current_weight);
	for (cur = mirrors->mirrors; cur != NULL; cur = cur->next) {
		mirror = (LowMirror *) cur->data;

		if (mirror->is_bad) {
			continue;
		}

		if (mirror->weight == weight) {
			if (count == choice) {
				break;
			} else {
				count++;
			}
		}
	}

	if (mirror == NULL) {
		return NULL;
	}

	return mirror->url;
}

void
low_mirror_list_mark_as_bad (LowMirrorList *mirrors, const char *url)
{
	GList *cur;
	LowMirror *mirror;

	for (cur = mirrors->mirrors; cur != NULL; cur = cur->next) {
		mirror = (LowMirror *) cur->data;
		if (!strcmp (mirror->url, url)) {
			mirror->is_bad = true;
			break;
		}
	}
}

/* vim: set ts=8 sw=8 noet: */
