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

#include "low-util.h"

#include <stdlib.h>
#include <stdio.h>
#include <string.h>
#include <ctype.h>
#include <glib.h>

#define MAXLINE 10000
#define MAXWORD 1000

#define MAXOUTPUT 10000

char **
low_util_word_wrap (const char *text, int width)
{
	char c = '\0';
	char cc = '\0';
	char d, line[MAXLINE + 2], word[MAXWORD + 2];
	int newline = 0, line_pos = 0, word_pos = 0;
	unsigned int i;
	int lineno = 0;

	/* XXX make this dynamic */
	char **output = malloc (sizeof (char *) * MAXOUTPUT);

	for (i = 0; i < strlen (text) + 1; i++) {
		d = cc;
		cc = text[i];
		c = cc;

		if (c == '\r') {
			continue;
		}
		if (c == '\n') {
			if (d == '\n')
				newline = 1;
			else
				c = ' ';
		}

		line[line_pos++] = word[word_pos++] = c;

		if (isspace (c)) {
			word_pos = 0;
		}

		if (word_pos > MAXWORD) {
			word[word_pos] = '\0';
			printf
			    ("ERROR: \"%s\" exceeds maximum word length (%d)\n",
			     word, MAXWORD);
			goto quit;
		}

		if (newline || line_pos > width) {
			newline = 0;
			line_pos -= word_pos;
			while (isspace (line[--line_pos])
			       && line[line_pos] != '\n') ;
			line[line_pos + 1] = '\0';
			output[lineno++] = g_strdup (line);
			strncpy (line, word, word_pos);
			line_pos = word_pos;
		}
	}

	if (line_pos > 0) {
		line[line_pos] = '\0';
		output[lineno++] = g_strdup (line);
	}

quit:
	output[lineno] = NULL;
	return output;
}

gboolean
low_util_parse_nevra (const char *nevra, char **name G_GNUC_UNUSED, char **epoch G_GNUC_UNUSED,
		      char **version G_GNUC_UNUSED, char **release G_GNUC_UNUSED, char **arch G_GNUC_UNUSED)
{
	if (strlen (nevra) == 0) {
		return FALSE;
	}
	*name = g_strdup (nevra);

	return TRUE;
}

/* vim: set ts=8 sw=8 noet: */
