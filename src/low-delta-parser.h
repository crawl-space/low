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

#include <glib.h>

#include "low-package.h"
#include "low-util.h"

#ifndef _LOW_DELTA_PARSER_H_
#define _LOW_DELTA_PARSER_H_

typedef struct _LowPackageDelta {
	char *name;
	char *arch;
	char *new_epoch;
	char *new_version;
	char *new_release;
	char *old_epoch;
	char *old_version;
	char *old_release;
	char *filename;
	size_t size;
	LowDigestType digest_type;
	char *digest;
	char *sequence;
} LowPackageDelta;

typedef struct _LowDelta {
	GHashTable *deltas;
} LowDelta;

LowDelta *low_delta_parse (const char *delta);
void low_delta_free (LowDelta *delta);

LowPackageDelta *low_delta_find_delta (LowDelta *delta, LowPackage *new_pkg,
				       LowPackage *old_pkg);

#endif /* _LOW_DELTA_PARSER_H_ */

/* vim: set ts=8 sw=8 noet: */
