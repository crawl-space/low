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


#ifndef _LOW_FAKE_REPO_H_
#define _LOW_FAKE_REPO_H_

#include "low-package.h"
#include "low-repo.h"

typedef struct _LowFakeRepo {
	LowRepo super;
	LowPackage **packages;
} LowFakeRepo;
LowRepo *		low_fake_repo_initialize 	(const char *id,
							 const char *name,
							 gboolean enabled);
void 			low_fake_repo_shutdown 		(LowRepo *repo);

LowPackageIter *	low_fake_repo_list_all 	(LowRepo *repo);
LowPackageIter *	low_fake_repo_list_by_name 	(LowRepo *repo,
							 const char *name);
LowPackageIter *	low_fake_repo_search_provides 	(LowRepo *repo,
							 const char *provides);
LowPackageIter * 	low_fake_repo_search_requires  (LowRepo *repo,
							const char *requires);
LowPackageIter * 	low_fake_repo_search_conflicts (LowRepo *repo,
							const char *conflicts);
LowPackageIter *	low_fake_repo_search_obsoletes (LowRepo *repo,
							const char *obsoletes);
LowPackageIter *	low_fake_repo_search_files 	(LowRepo *repo,
							 const char *file);

LowPackageIter *	low_fake_repo_search_details 	(LowRepo *repo,
							 const char *querystr);

char **			low_fake_repo_get_provides 	(LowRepo *repo,
							 LowPackage *pkg);
char **			low_fake_repo_get_requires 	(LowRepo *repo,
							 LowPackage *pkg);
char **			low_fake_repo_get_conflicts 	(LowRepo *repo,
							 LowPackage *pkg);
char **			low_fake_repo_get_obsoletes 	(LowRepo *repo,
							 LowPackage *pkg);

char **			low_fake_repo_get_files 	(LowRepo *repo,
							 LowPackage *pkg);

#define DELEGATE_SEARCH_FUNCTION(name, func) \
	LowPackageIter * \
	low_repo_ ## name ## _ ## func (LowRepo *repo, \
					const char *searchstr) { \
		return low_fake_repo_ ## func (repo, searchstr); \
	}

#define DELEGATE_GET_FUNCTION(name, func) \
	char ** \
	low_repo_ ## name ## _get_ ## func (LowRepo *repo, \
					    LowPackage *pkg) { \
		return low_fake_repo_get_ ## func (repo, pkg); \
	}


#define FAKE_REPO(name, studley_name) \
	\
	typedef LowFakeRepo LowRepo ## studley_name ## Fake; \
	\
	void \
	low_repo_ ## name ## _shutdown (LowRepo *repo) \
	{ \
		return low_fake_repo_shutdown (repo); \
	} \
	\
	LowPackageIter * \
	low_repo_ ## name ## _list_all (LowRepo *repo) { \
		return low_fake_repo_list_all (repo); \
	} \
	\
	DELEGATE_SEARCH_FUNCTION(name, list_by_name) \
	DELEGATE_SEARCH_FUNCTION(name, search_provides) \
	DELEGATE_SEARCH_FUNCTION(name, search_requires) \
	DELEGATE_SEARCH_FUNCTION(name, search_conflicts) \
	DELEGATE_SEARCH_FUNCTION(name, search_obsoletes) \
	DELEGATE_SEARCH_FUNCTION(name, search_files) \
	DELEGATE_SEARCH_FUNCTION(name, search_details) \
	\
//	DELEGATE_GET_FUNCTION(name, provides)
//	DELEGATE_GET_FUNCTION(name, requires)
//	DELEGATE_GET_FUNCTION(name, conflicts)
//	DELEGATE_GET_FUNCTION(name, obsoletes)
//	DELEGATE_GET_FUNCTION(name, files)

#define FAKE_SQLITE_REPO \
	FAKE_REPO (sqlite, Sqlite) \
	\
	LowRepo * \
	low_repo_sqlite_initialize (const char *name, const char *id, \
				    gboolean enabled) { \
		return low_fake_repo_initialize (name, id, enabled); \
	}

#define FAKE_RPMDB \
	FAKE_REPO (rpmdb, Rpmdb) \
	\
	LowRepo * \
	low_repo_rpmdb_initialize (void) { \
		return low_fake_repo_initialize ("rpmdb", "installed", TRUE); \
	}

#endif /* _LOW_FAKE_REPO_H_ */

/* vim: set ts=8 sw=8 noet: */
