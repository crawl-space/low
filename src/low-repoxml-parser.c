/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2010 James Bowes <jbowes@repl.ca>
 *
 *  Based on razor's import-yum.c.
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
#include <stdint.h>
#include <stdio.h>
#include <string.h>
#include <unistd.h>

#include <expat.h>

#include "low-sqlite-importer.h"

#include "low-repoxml-parser.h"

enum {
	REPOXML_STATE_BEGIN,
	REPOXML_STATE_PACKAGE_NAME,
	REPOXML_STATE_PACKAGE_ARCH,
	REPOXML_STATE_SUMMARY,
	REPOXML_STATE_DESCRIPTION,
	REPOXML_STATE_URL,
	REPOXML_STATE_LICENSE,
	REPOXML_STATE_CHECKSUM,
	REPOXML_STATE_REQUIRES,
	REPOXML_STATE_PROVIDES,
	REPOXML_STATE_OBSOLETES,
	REPOXML_STATE_CONFLICTS,
	REPOXML_STATE_FILE
};

struct repoxml_context {
	int state;

	int total;
	int current;

	LowSqliteImporter *importer;

	XML_Parser primary_parser;
	XML_Parser filelists_parser;
	XML_Parser current_parser;

        char name[256];
	char arch[64];
	char summary[512];
	char description[4096];
	char url[256];
	char license[64];
	char buffer[512];
	char *p;

	char pkgid[128];
	uint32_t dependency_type;
};


static void
repoxml_primary_start_element(void *data, const char *name, const char **atts)
{
	struct repoxml_context *ctx = data;
	const char *n, *epoch, *version, *release;
	bool pre;
	const char *relation;
	int i;

	if (strcmp(name, "metadata") == 0) {
		for (i = 0; atts[i]; i += 2) {
			if (strcmp(atts[i], "packages") == 0)
				ctx->total = atoi(atts[i + 1]);
		}
	} else if (strcmp(name, "name") == 0) {
		ctx->state = REPOXML_STATE_PACKAGE_NAME;
		ctx->p = ctx->name;
	} else if (strcmp(name, "arch") == 0) {
		ctx->state = REPOXML_STATE_PACKAGE_ARCH;
		ctx->p = ctx->arch;
	} else if (strcmp(name, "version") == 0) {
		epoch = NULL;
		version = NULL;
		release = NULL;
		for (i = 0; atts[i]; i += 2) {
			if (strcmp(atts[i], "epoch") == 0)
				epoch = atts[i + 1];
			else if (strcmp(atts[i], "ver") == 0)
				version = atts[i + 1];
			else if (strcmp(atts[i], "rel") == 0)
				release = atts[i + 1];
		}
		if (version == NULL || release == NULL) {
			fprintf(stderr, "invalid version tag, "
				"missing version or  release attribute\n");
			return;
		}

		low_sqlite_importer_begin_package (ctx->importer, ctx->name,
						   ctx->arch, epoch, version,
						   release);
	} else if (strcmp(name, "summary") == 0) {
		ctx->p = ctx->summary;
		ctx->state = REPOXML_STATE_SUMMARY;
	} else if (strcmp(name, "description") == 0) {
		ctx->p = ctx->description;
		ctx->state = REPOXML_STATE_DESCRIPTION;
	} else if (strcmp(name, "url") == 0) {
		ctx->p = ctx->url;
		ctx->state = REPOXML_STATE_URL;
	} else if (strcmp(name, "checksum") == 0) {
		ctx->p = ctx->pkgid;
		ctx->state = REPOXML_STATE_CHECKSUM;
	} else if (strcmp(name, "rpm:license") == 0) {
		ctx->p = ctx->license;
		ctx->state = REPOXML_STATE_LICENSE;
	} else if (strcmp(name, "rpm:requires") == 0) {
		ctx->state = REPOXML_STATE_REQUIRES;
		ctx->dependency_type = DEPENDENCY_TYPE_REQUIRES;
	} else if (strcmp(name, "rpm:provides") == 0) {
		ctx->state = REPOXML_STATE_PROVIDES;
		ctx->dependency_type = DEPENDENCY_TYPE_PROVIDES;
	} else if (strcmp(name, "rpm:obsoletes") == 0) {
		ctx->state = REPOXML_STATE_OBSOLETES;
		ctx->dependency_type = DEPENDENCY_TYPE_OBSOLETES;
	} else if (strcmp(name, "rpm:conflicts") == 0) {
		ctx->state = REPOXML_STATE_CONFLICTS;
		ctx->dependency_type = DEPENDENCY_TYPE_CONFLICTS;
	} else if (strcmp(name, "rpm:entry") == 0 &&
		   ctx->state != REPOXML_STATE_BEGIN) {
		n = NULL;
		epoch = NULL;
		version = NULL;
		release = NULL;
		relation = "";
		pre = 0;
		for (i = 0; atts[i]; i += 2) {
			if (strcmp(atts[i], "name") == 0)
				n = atts[i + 1];
			else if (strcmp(atts[i], "epoch") == 0)
				epoch = atts[i + 1];
			else if (strcmp(atts[i], "ver") == 0)
				version = atts[i + 1];
			else if (strcmp(atts[i], "rel") == 0)
				release = atts[i + 1];
			else if (strcmp(atts[i], "flags") == 0)
				relation = atts[i + 1];
			else if (strcmp(atts[i], "pre") == 0)
				pre = true;
		}

		if (n == NULL) {
			fprintf (stderr, "invalid rpm:entry, "
				 "missing name or version attributes\n");
			return;
		}

		low_sqlite_importer_add_dependency (ctx->importer, n, relation,
						    ctx->dependency_type, pre,
						    epoch, version, release);
	}
}

static void
repoxml_primary_end_element (void *data, const char *name)
{
	struct repoxml_context *ctx = data;

	switch (ctx->state) {
		case REPOXML_STATE_PACKAGE_NAME:
		case REPOXML_STATE_PACKAGE_ARCH:
		case REPOXML_STATE_SUMMARY:
		case REPOXML_STATE_DESCRIPTION:
		case REPOXML_STATE_URL:
		case REPOXML_STATE_LICENSE:
		case REPOXML_STATE_CHECKSUM:
		case REPOXML_STATE_FILE:
			ctx->state = REPOXML_STATE_BEGIN;
			break;
		default:
			break;
	}

	if (strcmp(name, "package") == 0) {
		low_sqlite_importer_add_details(ctx->importer, ctx->summary,
					   ctx->description, ctx->url,
					   ctx->license);

		XML_StopParser(ctx->current_parser, XML_TRUE);
		ctx->current_parser = ctx->filelists_parser;

		printf("\rimporting %d/%d", ++ctx->current, ctx->total);
		fflush(stdout);
	}
}

static void
repoxml_character_data (void *data, const XML_Char *s, int len)
{
	struct repoxml_context *ctx = data;

	switch (ctx->state) {
		case REPOXML_STATE_PACKAGE_NAME:
		case REPOXML_STATE_PACKAGE_ARCH:
		case REPOXML_STATE_SUMMARY:
		case REPOXML_STATE_DESCRIPTION:
		case REPOXML_STATE_URL:
		case REPOXML_STATE_LICENSE:
		case REPOXML_STATE_CHECKSUM:
		case REPOXML_STATE_FILE:
			memcpy(ctx->p, s, len);
			ctx->p += len;
			*ctx->p = '\0';
			break;
		default:
			break;
	}
}

static void
repoxml_filelists_start_element(void *data, const char *name, const char **atts)
{
	struct repoxml_context *ctx = data;
	const char *pkg, *pkgid;
	int i;

	if (strcmp(name, "package") == 0) {
		pkg = NULL;
		pkgid = NULL;
		for (i = 0; atts[i]; i += 2) {
			if (strcmp(atts[i], "name") == 0)
				pkg = atts[i + 1];
			else if (strcmp(atts[i], "pkgid") == 0)
				pkgid = atts[i + 1];
		}
		if (strcmp(pkgid, ctx->pkgid) != 0)
			fprintf(stderr, "primary.xml and filelists.xml "
				"mismatch for %s: %s vs %s",
				pkg, pkgid, ctx->pkgid);
	} else if (strcmp(name, "file") == 0) {
		ctx->state = REPOXML_STATE_FILE;
		ctx->p = ctx->buffer;
	}
}


static void
repoxml_filelists_end_element (void *data, const char *name)
{
	struct repoxml_context *ctx = data;

	ctx->state = REPOXML_STATE_BEGIN;
	if (strcmp(name, "package") == 0) {
		XML_StopParser(ctx->current_parser, XML_TRUE);
		ctx->current_parser = ctx->primary_parser;
		low_sqlite_importer_finish_package(ctx->importer);
	} else if (strcmp(name, "file") == 0)
		low_sqlite_importer_add_file(ctx->importer, ctx->buffer);

}

#define XML_BUFFER_SIZE 4096

void
low_repoxml_parse (const char *primary, const char *filelists)
{
	struct repoxml_context ctx;
	void *buf;
	int len;
	int primary_file;
	int filelists_file;
	char *directory;
	char *last_slash;
	XML_ParsingStatus status;

	ctx.state = REPOXML_STATE_BEGIN;

	ctx.primary_parser = XML_ParserCreate (NULL);
	XML_SetUserData (ctx.primary_parser, &ctx);
	XML_SetElementHandler (ctx.primary_parser,
			       repoxml_primary_start_element,
			       repoxml_primary_end_element);
	XML_SetCharacterDataHandler (ctx.primary_parser,
				     repoxml_character_data);

	ctx.filelists_parser = XML_ParserCreate (NULL);
	XML_SetUserData (ctx.filelists_parser, &ctx);
	XML_SetElementHandler (ctx.filelists_parser,
			       repoxml_filelists_start_element,
			       repoxml_filelists_end_element);
	XML_SetCharacterDataHandler (ctx.filelists_parser,
				     repoxml_character_data);

	primary_file = open (primary, O_RDONLY);
	if (primary_file < 0) {
		XML_ParserFree (ctx.primary_parser);
		return;
	}

	filelists_file = open (filelists, O_RDONLY);
	if (filelists_file < 0) {
		XML_ParserFree (ctx.primary_parser);
		XML_ParserFree (ctx.filelists_parser);
		close (primary_file);
		return;
	}

	ctx.current_parser = ctx.primary_parser;

	ctx.current = 0;

	last_slash = strrchr (primary, '/');
	directory = strndup (primary, last_slash - primary);
	ctx.importer = low_sqlite_importer_new (directory);

	do {
		XML_GetParsingStatus (ctx.current_parser, &status);
		switch (status.parsing) {
			case XML_SUSPENDED:
				XML_ResumeParser (ctx.current_parser);
				break;
			case XML_PARSING:
			case XML_INITIALIZED:
				buf = XML_GetBuffer (ctx.current_parser,
						     XML_BUFFER_SIZE);
				if (ctx.current_parser == ctx.primary_parser)
					len = read(primary_file, buf,
						   XML_BUFFER_SIZE);
				else
					len = read(filelists_file, buf,
						   XML_BUFFER_SIZE);
				if (len < 0) {
					fprintf (stderr,
						 "couldn't read input: %s\n",
						 strerror(errno));
					return;
				}

				XML_ParseBuffer (ctx.current_parser, len,
						 len == 0);
				break;
			case XML_FINISHED:
			default:
				break;
		}
	} while (status.parsing != XML_FINISHED);

	free (directory);

	XML_ParserFree (ctx.primary_parser);
	XML_ParserFree (ctx.filelists_parser);

	low_sqlite_importer_free (ctx.importer);

	close (primary_file);
	close (filelists_file);

	return;
}
/* vim: set ts=8 sw=8 noet: */
