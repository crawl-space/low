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

#ifndef _LOW_UTIL_H_
#define _LOW_UTIL_H_

#include <stdbool.h>

/**
 * Package digest types we understand
 */
typedef enum {
	DIGEST_MD5,
	DIGEST_SHA1,
	DIGEST_SHA256,
	DIGEST_UNKNOWN,
	DIGEST_NONE
} LowDigestType;

char **low_util_word_wrap (const char *text, int width);

bool low_util_parse_nevra (const char *nevra, char **name, char **epoch,
			   char **version, char **release, char **arch);

int low_util_evr_cmp (const char *evr1, const char *evr2);

#endif /* _LOW_UTIL_H_ */

/* vim: set ts=8 sw=8 noet: */
