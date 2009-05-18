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

#ifndef _LOW_MIRROR_LIST_H_
#define _LOW_MIRROR_LIST_H_

typedef struct _LowMirror {
	char *url;
	int weight;
	gboolean is_bad;
} LowMirror;

typedef struct _LowMirrorList {
	GList *mirrors;
} LowMirrorList;

LowMirrorList *low_mirror_list_new (void);
LowMirrorList *low_mirror_list_new_from_baseurl (const char *baseurl);
LowMirrorList *low_mirror_list_new_from_txt_file (const char *mirrorlist_txt);
LowMirrorList *low_mirror_list_new_from_metalink (const char *metalink);
void low_mirror_list_free (LowMirrorList *mirrors);

const char *low_mirror_list_lookup_random_mirror (LowMirrorList *mirrors);
void low_mirror_list_mark_as_bad (LowMirrorList *mirrors, const char *url);

#endif /* _LOW_MIRROR_LIST_H_ */

/* vim: set ts=8 sw=8 noet: */
