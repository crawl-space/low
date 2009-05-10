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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <expat.h>
#include <glib.h>

#include "low-metalink-parser.h"
#include "low-mirror-list.h"

enum {
	METALINK_STATE_URL,
	METALINK_STATE_OTHER,
};

struct metalink_context {
	int state;
	GList *mirrors;
	int weight;
};

static void
low_metalink_start_element (void *data, const char *name, const char **atts)
{
	struct metalink_context *ctx = data;
	int i;

	ctx->state = METALINK_STATE_OTHER;

	if (strcmp (name, "url") == 0) {
		for (i = 0; atts[i]; i += 2) {
			if (strcmp (atts[i], "type") == 0) {
				if (strcmp (atts[i + 1], "http") == 0 ||
				    strcmp (atts[i + 1], "ftp") == 0) {
					ctx->state = METALINK_STATE_URL;
				}
			} else if (strcmp (atts[i], "preference") == 0) {
				ctx->weight = atol (atts[i + 1]);
			}

		}
	}
}

static void
low_metalink_end_element (void *data, const char *name G_GNUC_UNUSED)
{
	struct metalink_context *ctx = data;

	ctx->state = METALINK_STATE_OTHER;
	ctx->weight = 100;
}

static void
low_metalink_character_data (void *data, const XML_Char *s, int len)
{
	struct metalink_context *ctx = data;

	LowMirror *mirror;

	switch (ctx->state) {
		case METALINK_STATE_URL:
			mirror = malloc (sizeof (LowMirror));
			/* drop the 'repodata/repomd.xml' */
			mirror->url = strndup (s, len - 19);
			/* XXX parse this */
			mirror->weight = ctx->weight;
			ctx->mirrors = g_list_append (ctx->mirrors, mirror);
			break;
		default:
			break;
	}
}

#define XML_BUFFER_SIZE 4096


/*
 * XXX this isn't very fault tolerant
 * XXX we should also store the preference weight
 */
GList *
low_metalink_parse (const char *metalink)
{
	struct metalink_context ctx;
	void *buf;
	int len;
	int metalink_file;
	XML_Parser parser;
	XML_ParsingStatus status;

	ctx.mirrors = NULL;
	ctx.state = METALINK_STATE_OTHER;
	ctx.weight = 100;

	parser = XML_ParserCreate (NULL);
	XML_SetUserData (parser, &ctx);
	XML_SetElementHandler (parser,
			       low_metalink_start_element,
			       low_metalink_end_element);
	XML_SetCharacterDataHandler (parser, low_metalink_character_data);

	metalink_file = open (metalink, O_RDONLY);
	if (metalink_file < 0) {
		return NULL;
	}

	do {
		XML_GetParsingStatus (parser, &status);
		switch (status.parsing) {
			case XML_SUSPENDED:
			case XML_PARSING:
			case XML_INITIALIZED:
				buf = XML_GetBuffer (parser, XML_BUFFER_SIZE);
				len = read (metalink_file, buf,
					    XML_BUFFER_SIZE);
				if (len < 0) {
					fprintf (stderr,
						 "couldn't read input: %s\n",
						 strerror (errno));
					return NULL;
				}

				XML_ParseBuffer (parser, len, len == 0);
				break;
			case XML_FINISHED:
			default:
				break;
		}
	} while (status.parsing != XML_FINISHED);

	XML_ParserFree (parser);

	close (metalink_file);
	return ctx.mirrors;
}

/* vim: set ts=8 sw=8 noet: */
