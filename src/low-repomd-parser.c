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

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <expat.h>
#include <glib.h>

#include "low-repomd-parser.h"

enum {
	REPODATA_STATE_BEGIN,
	REPODATA_STATE_PRIMARY,
	REPODATA_STATE_FILELISTS,
	REPODATA_STATE_DELTA,
	REPODATA_STATE_PRIMARY_TIMESTAMP,
	REPODATA_STATE_FILELISTS_TIMESTAMP,
	REPODATA_STATE_DELTA_TIMESTAMP,
};

struct repodata_context {
	int state;
	LowRepomd *repomd;
};

static void
low_repomd_start_element (void *data, const char *name, const char **atts)
{
	struct repodata_context *ctx = data;
	int i;

	if (strcmp (name, "data") == 0) {
		for (i = 0; atts[i]; i += 2) {
			if (strcmp (atts[i], "type") == 0) {
				if (strcmp (atts[i + 1], "primary_db") == 0)
					ctx->state = REPODATA_STATE_PRIMARY;
				else if (strcmp (atts[i + 1], "filelists_db") ==
					 0)
					ctx->state = REPODATA_STATE_FILELISTS;
				else if (strcmp (atts[i + 1], "prestodelta") ==
					 0)
					ctx->state = REPODATA_STATE_DELTA;

			}
		}
	} else if (strcmp (name, "location") == 0) {
		for (i = 0; atts[i]; i += 2) {
			if (strcmp (atts[i], "href") == 0) {
				switch (ctx->state) {
					case REPODATA_STATE_PRIMARY:
						ctx->repomd->primary_db =
							strdup (atts[i + 1]);
						break;
					case REPODATA_STATE_FILELISTS:
						ctx->repomd->filelists_db =
							strdup (atts[i + 1]);
					case REPODATA_STATE_DELTA:
						ctx->repomd->delta_xml =
							strdup (atts[i + 1]);

					default:
						break;
				}

			}
		}
	} else if (strcmp (name, "timestamp") == 0) {
		switch (ctx->state) {
			case REPODATA_STATE_PRIMARY:
				ctx->state = REPODATA_STATE_PRIMARY_TIMESTAMP;
				break;
			case REPODATA_STATE_FILELISTS:
				ctx->state = REPODATA_STATE_FILELISTS_TIMESTAMP;
			case REPODATA_STATE_DELTA:
				ctx->state = REPODATA_STATE_DELTA_TIMESTAMP;
			default:
				break;
		}
	}

}

static void
low_repomd_end_element (void *data, const char *name)
{
	struct repodata_context *ctx = data;

	if (strcmp (name, "data") == 0) {
		ctx->state = REPODATA_STATE_BEGIN;
	}
}

static void
low_repomd_character_data (void *data, const XML_Char *s, int len G_GNUC_UNUSED)
{
	struct repodata_context *ctx = data;

	switch (ctx->state) {
		case REPODATA_STATE_PRIMARY_TIMESTAMP:
			ctx->repomd->primary_db_time = strtoul (s, NULL, 10);
			ctx->state = REPODATA_STATE_PRIMARY;
			break;
		case REPODATA_STATE_FILELISTS_TIMESTAMP:
			ctx->repomd->filelists_db_time = strtoul (s, NULL, 10);
			ctx->state = REPODATA_STATE_FILELISTS;
		case REPODATA_STATE_DELTA_TIMESTAMP:
			ctx->repomd->delta_xml_time = strtoul (s, NULL, 10);
			ctx->state = REPODATA_STATE_DELTA;

		default:
			break;
	}
}

#define XML_BUFFER_SIZE 4096

LowRepomd *
low_repomd_parse (const char *repodata)
{
	struct repodata_context ctx;
	void *buf;
	int len;
	int repodata_file;
	XML_Parser parser;
	XML_ParsingStatus status;

	ctx.state = REPODATA_STATE_BEGIN;

	parser = XML_ParserCreate (NULL);
	XML_SetUserData (parser, &ctx);
	XML_SetElementHandler (parser,
			       low_repomd_start_element,
			       low_repomd_end_element);
	XML_SetCharacterDataHandler (parser, low_repomd_character_data);

	repodata_file = open (repodata, O_RDONLY);
	if (repodata_file < 0) {
		XML_ParserFree (parser);
		return NULL;
	}

	ctx.repomd = malloc (sizeof (LowRepomd));
	memset (ctx.repomd, 0, sizeof (LowRepomd));

	do {
		XML_GetParsingStatus (parser, &status);
		switch (status.parsing) {
			case XML_SUSPENDED:
			case XML_PARSING:
			case XML_INITIALIZED:
				buf = XML_GetBuffer (parser, XML_BUFFER_SIZE);
				len = read (repodata_file, buf,
					    XML_BUFFER_SIZE);
				if (len < 0) {
					fprintf (stderr,
						 "couldn't read input: %s\n",
						 strerror (errno));
					XML_ParserFree (parser);
					low_repomd_free (ctx.repomd);
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

	close (repodata_file);
	return ctx.repomd;
}

void
low_repomd_free (LowRepomd *repomd)
{
	if (repomd != NULL) {
		free (repomd->primary_db);
		free (repomd->filelists_db);
		free (repomd->delta_xml);
		free (repomd);
	}
}

/* vim: set ts=8 sw=8 noet: */
