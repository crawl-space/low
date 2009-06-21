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

#include <stdbool.h>
#include <string.h>
#include <stdio.h>

#include "low-parse-options.h"

#define INVALID_OPTION -1

static int
low_store_option (const LowOption *option)
{
	switch (option->type) {
		case OPTION_BOOL:
			*(bool *)option->value = true;
		case OPTION_END:
		default:
			break;
	}

	return 0;
}

static int
low_parse_long_opt (const char **argv, const LowOption *options)
{
	const LowOption *option;

	for (option = options; option->type != OPTION_END; option++) {
		/* XXX handle '=' here */
		if (!strcmp (option->long_name, argv[0] + 2)) {
			//return low_store_option (argc + 1, argv++, option);
			return low_store_option (option);
		}
	}

	return INVALID_OPTION;
}

static int
low_parse_short_opt (const char **argv, const LowOption *options)
{
	const LowOption *option;

	for (option = options; option->type != OPTION_END; option++) {
		if (option->short_name == argv[0][1]) {
			return low_store_option (option);
		}
	}

	return INVALID_OPTION;
}

int
low_parse_options (int argc, const char **argv, const LowOption *options)
{
	int i;

	for (i = 0; i < argc; i++) {
		const char *arg = argv[i];
		int consumed;

		if (arg[0] != '-') {
			break;
		}

		if (arg[1] == '-') {
			consumed = low_parse_long_opt (argv + i,
						       options);
		} else {
			consumed = low_parse_short_opt (argv + i,
							options);
		}

		i += consumed;
	}

	return i;
}


/* vim: set ts=8 sw=8 noet: */
