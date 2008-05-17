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

#include <glib.h>
#include "low-config.h"
#include "low-repo.h"

#ifndef _LOW_REPO_SET_H_
#define _LOW_REPO_SET_H_

typedef struct _LowRepoSet {
	GHashTable *repos;
} LowRepoSet;

typedef enum {
	ENABLED,
	DISABLED,
	ALL
} LowRepoSetFilter;

typedef gboolean (*LowRepoSetFunc) 						(LowRepo *repo,
														 gpointer data);

LowRepoSet *    low_repo_set_initialize_from_config 	(LowConfig *config);
void            low_repo_set_free                      	(LowRepoSet *repo_set);

void            low_repo_set_for_each                  	(LowRepoSet *repo_set,
														 LowRepoSetFilter filter,
														 LowRepoSetFunc func,
														 gpointer data);

LowPackageIter * 	low_repo_set_search_provides      	(LowRepoSet *repo_set,
														 const char *provides);

#endif /* _LOW_REPO_SET_H_ */

/* vim: set ts=8 sw=8 noet: */
