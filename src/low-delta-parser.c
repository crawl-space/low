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

#include "low-delta-parser.h"

enum {
	DELTA_STATE_BEGIN,
	DELTA_STATE_PACKAGE,
	DELTA_STATE_DELTA,
	DELTA_STATE_FILENAME,
	DELTA_STATE_SEQUENCE,
	DELTA_STATE_SIZE,
	DELTA_STATE_CHECKSUM,
};

struct delta_context {
	int state;
	LowPackageDelta *pkg_delta;
	LowDelta *delta;
	char *buf;
	size_t buf_size;
	size_t str_len;
};

static void
low_delta_handle_newpackage_start (struct delta_context *ctx, const char **atts)
{
	int i;

	ctx->pkg_delta = malloc (sizeof (LowPackageDelta));

	for (i = 0; atts[i]; i += 2) {
		if (strcmp (atts[i], "name") == 0) {
			ctx->pkg_delta->name = strdup (atts[i + 1]);
		} else if (strcmp (atts[i], "arch") == 0) {
			ctx->pkg_delta->arch = strdup (atts[i + 1]);
		} else if (strcmp (atts[i], "epoch") == 0) {
			ctx->pkg_delta->new_epoch = strdup (atts[i + 1]);
		} else if (strcmp (atts[i], "version") == 0) {
			ctx->pkg_delta->new_version = strdup (atts[i + 1]);
		} else if (strcmp (atts[i], "release") == 0) {
			ctx->pkg_delta->new_release = strdup (atts[i + 1]);
		}
	}
}

static void
low_delta_handle_delta_start (struct delta_context *ctx, const char **atts)
{
	int i;

	for (i = 0; atts[i]; i += 2) {
		if (strcmp (atts[i], "oldepoch") == 0) {
			ctx->pkg_delta->old_epoch = strdup (atts[i + 1]);
		} else if (strcmp (atts[i], "oldversion") == 0) {
			ctx->pkg_delta->old_version = strdup (atts[i + 1]);
		} else if (strcmp (atts[i], "oldrelease") == 0) {
			ctx->pkg_delta->old_release = strdup (atts[i + 1]);
		}
	}
}

static void
low_delta_handle_checksum_start (struct delta_context *ctx, const char **atts)
{
	int i;

	for (i = 0; atts[i]; i += 2) {
		if (strcmp (atts[i], "type") == 0) {
			ctx->pkg_delta->digest_type =
				low_util_digest_type_from_string (atts[i + 1]);
		}
	}
}

static void
low_delta_start_element (void *data, const char *name, const char **atts)
{
	struct delta_context *ctx = data;

	if (strcmp (name, "newpackage") == 0) {
		ctx->state = DELTA_STATE_PACKAGE;
		low_delta_handle_newpackage_start (ctx, atts);
	} else if (strcmp (name, "delta") == 0) {
		ctx->state = DELTA_STATE_DELTA;
		low_delta_handle_delta_start (ctx, atts);
	} else if (strcmp (name, "filename") == 0) {
		ctx->state = DELTA_STATE_FILENAME;
	} else if (strcmp (name, "sequence") == 0) {
		ctx->state = DELTA_STATE_SEQUENCE;
	} else if (strcmp (name, "size") == 0) {
		ctx->state = DELTA_STATE_SIZE;
	} else if (strcmp (name, "checksum") == 0) {
		ctx->state = DELTA_STATE_CHECKSUM;
		low_delta_handle_checksum_start (ctx, atts);
	}
}

static char *
low_delta_make_key (LowPackageDelta *pkg_delta)
{
	return g_strdup_printf ("%s-%s:%s-%s.%s", pkg_delta->name,
				pkg_delta->new_epoch, pkg_delta->new_version,
				pkg_delta->new_release, pkg_delta->arch);
}

static void
low_delta_end_element (void *data, const char *name)
{
	struct delta_context *ctx = data;

	if (strcmp (name, "newpackage") == 0) {
		ctx->state = DELTA_STATE_BEGIN;
		g_hash_table_insert (ctx->delta->deltas,
				     low_delta_make_key (ctx->pkg_delta),
				     ctx->pkg_delta);
	} else if (strcmp (name, "delta") == 0) {
		ctx->state = DELTA_STATE_PACKAGE;
	} else if (strcmp (name, "filename") == 0) {
		ctx->state = DELTA_STATE_DELTA;
		ctx->pkg_delta->filename = strndup (ctx->buf, ctx->str_len);
		ctx->pkg_delta->filename =
			g_strstrip (ctx->pkg_delta->filename);
	} else if (strcmp (name, "sequence") == 0) {
		ctx->state = DELTA_STATE_DELTA;
		ctx->pkg_delta->sequence = strndup (ctx->buf, ctx->str_len);
		ctx->pkg_delta->sequence =
			g_strstrip (ctx->pkg_delta->sequence);
	} else if (strcmp (name, "size") == 0) {
		ctx->state = DELTA_STATE_DELTA;
		ctx->pkg_delta->size = strtoul (ctx->buf, NULL, 10);
	} else if (strcmp (name, "checksum") == 0) {
		ctx->state = DELTA_STATE_DELTA;
		ctx->pkg_delta->digest = strndup (ctx->buf, ctx->str_len);
		ctx->pkg_delta->digest = g_strstrip (ctx->pkg_delta->digest);
	}

	ctx->str_len = 0;
}

static void
low_delta_character_data (void *data, const XML_Char *s, int len)
{
	struct delta_context *ctx = data;

	if (ctx->str_len + len > ctx->buf_size) {
		ctx->buf = realloc (ctx->buf, ctx->buf_size + len);
		ctx->buf_size += len;
	}

	strncpy (ctx->buf + ctx->str_len, s, len);
	ctx->str_len += len;
}

#define XML_BUFFER_SIZE 4096

LowDelta *
low_delta_parse (const char *delta)
{
	struct delta_context ctx;
	void *buf;
	int len;
	int delta_file;
	XML_Parser parser;
	XML_ParsingStatus status;

	ctx.state = DELTA_STATE_BEGIN;

	parser = XML_ParserCreate (NULL);
	XML_SetUserData (parser, &ctx);
	XML_SetElementHandler (parser,
			       low_delta_start_element, low_delta_end_element);
	XML_SetCharacterDataHandler (parser, low_delta_character_data);

	delta_file = open (delta, O_RDONLY);
	if (delta_file < 0) {
		XML_ParserFree (parser);
		return NULL;
	}

	ctx.delta = malloc (sizeof (LowDelta));

	ctx.delta->deltas = g_hash_table_new (g_str_hash, g_str_equal);

	ctx.buf = NULL;
	ctx.buf_size = 0;
	ctx.str_len = 0;

	do {
		XML_GetParsingStatus (parser, &status);
		switch (status.parsing) {
			case XML_SUSPENDED:
			case XML_PARSING:
			case XML_INITIALIZED:
				buf = XML_GetBuffer (parser, XML_BUFFER_SIZE);
				len = read (delta_file, buf, XML_BUFFER_SIZE);
				if (len < 0) {
					fprintf (stderr,
						 "couldn't read input: %s\n",
						 strerror (errno));
					XML_ParserFree (parser);
					low_delta_free (ctx.delta);
					free (ctx.buf);
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
	free (ctx.buf);

	close (delta_file);
	return ctx.delta;
}

static void
low_package_delta_free (gpointer key G_GNUC_UNUSED, gpointer value,
			gpointer data G_GNUC_UNUSED)
{
	LowPackageDelta *pkg_delta = (LowPackageDelta *) value;

	free (pkg_delta->name);
	free (pkg_delta->arch);
	free (pkg_delta->new_epoch);
	free (pkg_delta->new_version);
	free (pkg_delta->new_release);
	free (pkg_delta->old_epoch);
	free (pkg_delta->old_version);
	free (pkg_delta->old_release);
	free (pkg_delta->filename);
	free (pkg_delta->digest);
	free (pkg_delta->sequence);

	free (pkg_delta);
}

void
low_delta_free (LowDelta *delta)
{
	if (delta != NULL) {
		g_hash_table_foreach (delta->deltas, low_package_delta_free,
				      NULL);
		free (delta);
	}
}

static char *
low_delta_make_key_from_pkg (LowPackage *pkg)
{

	return g_strdup_printf ("%s-%s:%s-%s.%s", pkg->name, pkg->epoch,
				pkg->version, pkg->release,
				low_arch_to_str (pkg->arch));
}

static bool
compare_epochs (char *e1, char *e2)
{
	if (e1 == NULL && e2 == NULL) {
		return true;
	} else if (e1 == NULL) {
		return strcmp ("0", e2) == 0;
		return true;
	} else if (e2 == NULL) {
		return strcmp ("0", e1) == 0;
	} else {
		return strcmp (e1, e2) == 0;
	}
}

LowPackageDelta *
low_delta_find_delta (LowDelta *delta, LowPackage *new_pkg, LowPackage *old_pkg)
{
	char *key = low_delta_make_key_from_pkg (new_pkg);
	LowPackageDelta *pkg_delta = g_hash_table_lookup (delta->deltas, key);

	free (key);

	if (pkg_delta != NULL &&
	    compare_epochs (old_pkg->epoch, pkg_delta->old_epoch) &&
	    strcmp (old_pkg->version, pkg_delta->old_version) == 0 &&
	    strcmp (old_pkg->release, pkg_delta->old_release) == 0) {
		return pkg_delta;
	}

	return NULL;
}

/* vim: set ts=8 sw=8 noet: */
