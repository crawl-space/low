/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2008, 2009 James Bowes <jbowes@repl.ca>
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
#include <stdarg.h>
#include <glib.h>
#include "low-debug.h"

static gboolean debugging_enabled = FALSE;

void
low_debug_impl (const char *file, const char *func, const int line,
		const char *format, ...)
{
	va_list args;

	if (!debugging_enabled) {
		return;
	}

	fprintf (stderr, "%s:%d (%s) : ", file, line, func);

	va_start (args, format);
	vfprintf (stderr, format, args);
	va_end (args);

	fprintf (stderr, "\n");
}

void
low_debug_init (void)
{
	char *debug = getenv ("LOW_DEBUG");

	if (debug != NULL) {
		debugging_enabled = TRUE;
	}
}

/* vim: set ts=8 sw=8 noet: */
