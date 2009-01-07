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

#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/time.h>
#include <rpm/rpmlib.h>

#include "low-debug.h"
#include "low-transaction.h"
#include "low-repo-rpmdb.h"

/**
 * \page depsolver The Depedency Resolution Algorithm
 *
 * This page documents the dependency resolution algorithm used in Low
 * (as copied from YUM).
 *
 * \section algorithm The Algorithm
 * - WHILE there are unresolved dependencies DO:
 *   - FOR EACH package to be installed DO:
 *     - FOR EACH requires of the package DO:
 *       - IF NOT requires provided by installed packages
 *         OR NOT requires provided by packages in the transaction DO:
 * 	     - Add requires to unresolved requires.
 */

typedef enum _LowTransactionStatus {
	LOW_TRANSACTION_NO_CHANGE,
	LOW_TRANSACTION_PACKAGES_ADDED,
	LOW_TRANSACTION_UNRESOLVABLE
} LowTransactionStatus;

static void
low_transaction_member_free (LowTransactionMember *member)
{
//	low_package_unref (member->pkg);
	free (member);
}

LowTransaction *
low_transaction_new (LowRepo *rpmdb, LowRepoSet *repos) {
	LowTransaction *trans = malloc (sizeof (LowTransaction));

	trans->rpmdb = rpmdb;
	trans->repos = repos;

	trans->install = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free,
						(GDestroyNotify)
						low_transaction_member_free);
	trans->update = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free,
					       (GDestroyNotify)
					       low_transaction_member_free);
	trans->updated = g_hash_table_new_full (g_str_hash, g_str_equal,
						g_free,
						(GDestroyNotify)
						low_transaction_member_free);
	trans->remove = g_hash_table_new_full (g_str_hash, g_str_equal,
					       g_free,
					       (GDestroyNotify)
					       low_transaction_member_free);

	trans->unresolved = g_hash_table_new_full (g_str_hash, g_str_equal,
						   g_free,
						   (GDestroyNotify)
						   low_transaction_member_free);

	return trans;
}
/*
static int
low_transaction_pkg_compare_func (gconstpointer data1, gconstpointer data2)
{
	const LowPackage *pkg1 = (LowPackage *) data1;
	const LowPackage *pkg2 = (LowPackage *) data2;

	if (!strcmp (pkg1->name, pkg2->name) &&
	    !strcmp (pkg1->version, pkg2->version) &&
	    !strcmp (pkg1->release, pkg2->release) &&
	    !strcmp (pkg1->arch, pkg2->arch) &&
	    (!(pkg1->epoch || pkg2->epoch) ||
	     !strcmp (pkg1->epoch, pkg2->epoch))) {
		return 0;
	} else {
		return 1;
	}

}
*/
static gboolean
low_transaction_add_to_hash (GHashTable *hash, LowPackage *pkg,
			     LowPackage *related_pkg)
{
	gboolean res;
	char *key;
	LowTransactionMember *member;

	if (pkg->epoch) {
		key = g_strdup_printf ("%s-%s:%s-%s.%s", pkg->name,
				       pkg->epoch, pkg->version,
				       pkg->release, pkg->arch);
	} else {
		key = g_strdup_printf ("%s-%s-%s.%s", pkg->name,
				       pkg->version, pkg->release,
				       pkg->arch);
	}

	if (!g_hash_table_lookup (hash, key)) {
		member = malloc (sizeof (LowTransactionMember));
		member->pkg = pkg;
		member->related_pkg = related_pkg;
		member->resolved = FALSE;

		g_hash_table_insert (hash, key, member);
		res = TRUE;
	} else {
		/* XXX not the right place for this */
//		low_package_unref (pkg);
		res = FALSE;
	}

	//free (key);

	return res;
}

static void
low_transaction_remove_from_hash (GHashTable *hash, LowPackage *pkg)
{
	char *key;
	LowTransactionMember *found_pkg;

	if (pkg->epoch) {
		key = g_strdup_printf ("%s-%s:%s-%s.%s", pkg->name,
				       pkg->epoch, pkg->version,
				       pkg->release, pkg->arch);
	} else {
		key = g_strdup_printf ("%s-%s-%s.%s", pkg->name,
				       pkg->version, pkg->release,
				       pkg->arch);
	}

	found_pkg = g_hash_table_lookup (hash, key);
	if (found_pkg) {
		g_hash_table_remove (hash, key);
	}

	free (key);
}

gboolean
low_transaction_add_install (LowTransaction *trans, LowPackage *to_install)
{
	if (low_transaction_add_to_hash (trans->install, to_install, NULL)) {
		low_debug_pkg ("Adding for install", to_install);
		return TRUE;
	} else {
		low_debug_pkg ("Not adding already added pkg for install",
			       to_install);
		return FALSE;
	}
}

static LowPackage *
choose_best_for_update (LowRepo *repo_rpmdb, LowRepoSet *repos,
			LowPackage *to_update)
{
	LowPackage *best = to_update;
	LowPackageIter *iter;
	char *best_evr = low_package_evr_as_string(to_update);

	LowPackageDependency *provides =
		low_package_dependency_new (to_update->name,
					    DEPENDENCY_SENSE_GT,
					    best_evr);

	iter = low_repo_set_search_provides (repos, provides);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		char *new_evr = low_package_evr_as_string (iter->pkg);

		if (rpmvercmp (new_evr, best_evr) > 0) {
			low_package_unref (best);
			best = iter->pkg;

			g_free (best_evr);
			best_evr = new_evr;
		} else {
			low_package_unref (iter->pkg);
			g_free (new_evr);
		}

	}

	low_package_dependency_free (provides);

	/* We haven't found anything better */
	if (to_update == best) {
		g_free (best_evr);
		return NULL;
	}

	/* Ensure we don't already have it */
	gboolean found = FALSE;
	iter = low_repo_rpmdb_list_by_name (repo_rpmdb, best->name);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		if (!found) {
			char *this_evr = low_package_evr_as_string(iter->pkg);

			if (rpmvercmp (best_evr, this_evr) == 0) {
				found = TRUE;
			}

			g_free (this_evr);
		}

		low_package_unref (iter->pkg);
	}

	g_free (best_evr);

	if (found) {
		return NULL;
	}

	return best;
}

gboolean
low_transaction_add_update (LowTransaction *trans, LowPackage *to_update)
{
	LowPackage *updating_to = choose_best_for_update (trans->rpmdb,
							  trans->repos,
							  to_update);

	if (updating_to == NULL) {
		low_debug_pkg ("No available update for", to_update);
		return FALSE;
	}

	low_debug_update ("Update found", to_update, updating_to);

	if (low_transaction_add_to_hash (trans->update, updating_to,
					 to_update)) {
		low_transaction_add_to_hash (trans->updated, to_update,
					     updating_to);
		low_debug_pkg ("Adding for update", updating_to);
		return TRUE;
	} else {
		low_debug_pkg ("Not adding already added pkg for update",
			       updating_to);
		return FALSE;
	}
}

gboolean
low_transaction_add_remove (LowTransaction *trans, LowPackage *to_remove)
{
	if (low_transaction_add_to_hash (trans->remove, to_remove, NULL)) {
		low_debug_pkg ("Adding for remove", to_remove);
		return TRUE;
	} else {
		low_debug_pkg ("Not adding already added pkg for remove",
			       to_remove);
		return FALSE;
	}
}

/**
 * Check if a requires is in a list of provides
 */
static gboolean
low_transaction_dep_in_deplist (const LowPackageDependency *needle,
				LowPackageDependency **haystack)
{
	int i;

	for (i = 0; haystack[i] != NULL; i++) {
		if (!strcmp (needle->name, haystack[i]->name)) {
			return TRUE;
		}
	}

	return FALSE;
}

/**
 * Check if a requires is in a list of files
 */
static gboolean
low_transaction_dep_in_filelist (const char *needle, char **haystack)
{
	int i;

	for (i = 0; haystack[i] != NULL; i++) {
		if (!strcmp (needle, haystack[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

static LowTransactionStatus
low_transaction_check_removal (LowTransaction *trans, LowPackage *pkg,
			       gboolean from_update)
{
	LowTransactionStatus status = LOW_TRANSACTION_NO_CHANGE;
	LowPackageDependency **provides;
	char **files;
	int i;

	low_debug_pkg ("Checking removal of", pkg);

	provides = low_package_get_provides (pkg);
	files = low_package_get_files (pkg);

	for (i = 0; provides[i] != NULL; i++) {
		LowPackageIter *iter;

		low_debug ("Checking provides %s", provides[i]->name);

		iter = low_repo_rpmdb_search_requires (trans->rpmdb,
						       provides[i]);
		while (iter = low_package_iter_next (iter), iter != NULL) {
			LowPackage *pkg = iter->pkg;

			/* It's a self-requires, skip */
			if (pkg == pkg) {
				continue;
			}

			if (from_update) {
				return LOW_TRANSACTION_UNRESOLVABLE;
			}

			low_debug_pkg ("Adding for removal", pkg);
			if (low_transaction_add_remove (trans, pkg)) {
				status = LOW_TRANSACTION_PACKAGES_ADDED;
			}
		}
	}

	for (i = 0; files[i] != NULL; i++) {
		LowPackageIter *iter;
		LowPackageDependency *file_dep =
			low_package_dependency_new (files[i],
						    DEPENDENCY_SENSE_NONE,
						    NULL);

		low_debug ("Checking file %s", files[i]);

		iter = low_repo_rpmdb_search_requires (trans->rpmdb,
						       file_dep);
		while (iter = low_package_iter_next (iter), iter != NULL) {
			LowPackage *pkg = iter->pkg;

			low_debug_pkg ("Adding for removal", pkg);
			if (low_transaction_add_remove (trans, pkg)) {
				status = LOW_TRANSACTION_PACKAGES_ADDED;
			}
		}
		low_package_dependency_free (file_dep);
	}

//	low_package_dependency_list_free (provides);
	g_strfreev (files);

	return status;
}
static LowTransactionStatus
low_transaction_check_package_requires (LowTransaction *trans, LowPackage *pkg)
{
	LowTransactionStatus status = LOW_TRANSACTION_NO_CHANGE;
	LowPackageDependency **requires;
	LowPackageDependency **provides;
	char **files;
	int i;

	low_debug_pkg ("Checking requires for", pkg);

	requires = low_package_get_requires (pkg);
	provides = low_package_get_provides (pkg);
	files = low_package_get_files (pkg);

	for (i = 0; requires[i] != NULL; i++) {
		LowPackageIter *providing;

		if (low_transaction_dep_in_deplist (requires[i], provides)
		    || low_transaction_dep_in_filelist (requires[i]->name,
						        files)) {
		    low_debug ("Self provided requires %s, skipping",
			       requires[i]->name);
		    continue;
		}
		low_debug ("Checking requires %s", requires[i]->name);

		providing =
			low_repo_rpmdb_search_provides (trans->rpmdb,
							requires[i]);

		providing = low_package_iter_next (providing);
		if (providing != NULL) {
			low_debug_pkg ("Provided by", providing->pkg);
			low_package_unref (providing->pkg);

			/* XXX we just need a free function */
			while (providing = low_package_iter_next (providing),
				   providing != NULL) {
					low_package_unref (providing->pkg);
			}
			continue;
		/* Check files if appropriate */
		} else if (requires[i]->name[0] == '/') {
			providing =
				low_repo_rpmdb_search_files (trans->rpmdb,
							     requires[i]->name);

			providing = low_package_iter_next (providing);
			if (providing != NULL) {
				low_debug_pkg ("Provided by", providing->pkg);
				low_package_unref (providing->pkg);

				/* XXX we just need a free function */
				while (providing = low_package_iter_next (providing),
					   providing != NULL) {
						low_package_unref (providing->pkg);
				}
				continue;
			}
		}

		/* Check available packages */
		providing = low_repo_set_search_provides (trans->repos,
							  requires[i]);

		providing = low_package_iter_next (providing);
		if (providing != NULL) {
			low_debug_pkg ("Provided by", providing->pkg);
			/* XXX this might be an update, as well */
			if (low_transaction_add_install (trans,
							 providing->pkg)) {
				status = LOW_TRANSACTION_PACKAGES_ADDED;
			}
			/* XXX we just need a free function */
			while (providing = low_package_iter_next (providing),
				   providing != NULL) {
					low_package_unref (providing->pkg);
			}

			continue;
		/* Check files if appropriate */
		} else if (requires[i]->name[0] == '/') {
			providing =
				low_repo_set_search_files (trans->repos,
							   requires[i]->name);

			providing = low_package_iter_next (providing);
			if (providing != NULL) {
				low_debug_pkg ("Provided by", providing->pkg);
				if (low_transaction_add_install (trans, providing->pkg)) {
					status = LOW_TRANSACTION_PACKAGES_ADDED;
				}
				/* XXX we just need a free function */
				while (providing = low_package_iter_next (providing),
					   providing != NULL) {
						low_package_unref (providing->pkg);
				}

				continue;
			}

		}

		low_debug ("%s not provided by installed pkg",
			   requires[i]->name);
		return LOW_TRANSACTION_UNRESOLVABLE;
	}

//	low_package_dependency_list_free (provides);
//	low_package_dependency_list_free (requires);
	g_strfreev (files);

	return status;
}

static LowTransactionStatus
low_transaction_check_requires_for_added(LowTransactionStatus status,
					 LowTransaction *trans,
					 GHashTable *hash)
{
	GList *cur = g_hash_table_get_values (hash);

	while (cur != NULL) {
		LowTransactionStatus req_status;
		LowTransactionMember *member =
			(LowTransactionMember *) cur->data;
		LowPackage *pkg = member->pkg;

		if (!member->resolved) {
			req_status = low_transaction_check_package_requires (trans,
									     pkg);

			if (req_status == LOW_TRANSACTION_UNRESOLVABLE) {
				low_debug_pkg ("Adding to unresolved", pkg);
				low_transaction_add_to_hash (trans->unresolved, pkg, NULL);
				low_transaction_remove_from_hash (hash, pkg);
				return req_status;
			} else if (req_status == LOW_TRANSACTION_PACKAGES_ADDED) {
				status = LOW_TRANSACTION_PACKAGES_ADDED;
			}

			member->resolved = TRUE;
		}

		cur = cur->next;
	}

	return status;
}

static LowTransactionStatus
low_transaction_check_requires_for_removing(LowTransactionStatus status,
					    LowTransaction *trans,
					    GHashTable *hash,
					    gboolean from_update)
{
	GList *cur = g_hash_table_get_values (hash);

	while (cur != NULL) {
		LowTransactionMember *member =
			(LowTransactionMember *) cur->data;
		LowPackage *pkg = member->pkg;

		if (!member->resolved) {
			LowTransactionStatus rm_status;
			rm_status = low_transaction_check_removal (trans, pkg,
								   from_update);

			/* Only unresolvable for an update */
			if (rm_status == LOW_TRANSACTION_UNRESOLVABLE) {
				low_debug_pkg ("Adding to unresolved", pkg);
				low_transaction_add_to_hash (trans->unresolved,
							     member->related_pkg,
							     NULL);
				low_transaction_remove_from_hash (hash, pkg);
				low_transaction_remove_from_hash (trans->update,
								  member->related_pkg);
				return rm_status;

			} else if (rm_status == LOW_TRANSACTION_PACKAGES_ADDED) {
				status = LOW_TRANSACTION_PACKAGES_ADDED;
			}

			member->resolved = TRUE;
		}

		cur = cur->next;
	}

	return status;
}

static LowTransactionStatus
low_transaction_check_all_requires (LowTransaction *trans)
{
	LowTransactionStatus status = LOW_TRANSACTION_NO_CHANGE;

	status = low_transaction_check_requires_for_added (status, trans,
							   trans->install);
	status = low_transaction_check_requires_for_added (status, trans,
							   trans->update);

	status = low_transaction_check_requires_for_removing (status, trans,
							      trans->remove,
							      FALSE);
	status = low_transaction_check_requires_for_removing (status, trans,
							     trans->updated,
							     TRUE);

	return status;
}

static LowPackage *
low_transaction_search_provides (GHashTable *hash, char *query)
{
	/*
	 * XXX would it be faster to search the repos then compare against
	 *     our transaction?
	 */
	GList *cur;

	for (cur = g_hash_table_get_values (hash); cur != NULL;
	     cur = cur->next) {
		LowTransactionMember *member =
			(LowTransactionMember *) cur->data;
		LowPackageDependency **provides =
			low_package_get_provides (member->pkg);
		int i;

		for (i = 0; provides[i] != NULL; i++) {
			if (!strcmp (query, provides[i]->name)) {
//				low_package_dependency_list_free (provides);
				return member->pkg;
			}
		}

//		low_package_dependency_list_free (provides);

	}

	return NULL;
}

static LowTransactionStatus
low_transaction_check_all_conflicts (LowTransaction *trans)
{
	LowTransactionStatus status = LOW_TRANSACTION_NO_CHANGE;
	GList *cur = g_hash_table_get_values (trans->install);

	while (cur != NULL) {
		LowTransactionMember *member =
			(LowTransactionMember *) cur->data;
		LowPackage *pkg = member->pkg;

		LowPackageDependency **provides =
			low_package_get_provides (pkg);
		LowPackageDependency **conflicts =
			low_package_get_conflicts (pkg);
		int i;

		low_debug_pkg ("Checking for installed pkgs that conflict",
			       pkg);
		for (i = 0; provides[i] != NULL; i++) {
			LowPackageIter *iter;
			iter = low_repo_rpmdb_search_conflicts (trans->rpmdb,
								provides[i]);

			iter = low_package_iter_next (iter);
			if (iter != NULL) {
				low_debug_pkg ("Conflicted by", iter->pkg);
				low_package_unref (iter->pkg);

				/* XXX we just need a free function */
				while (iter = low_package_iter_next (iter),
				       iter != NULL) {
					low_package_unref (iter->pkg);
				}
				status = LOW_TRANSACTION_UNRESOLVABLE;
				break;
			}

		}

		for (i = 0; conflicts[i] != NULL; i++) {
			LowPackageIter *iter;
			iter = low_repo_rpmdb_search_provides (trans->rpmdb,
							       conflicts[i]);

			iter = low_package_iter_next (iter);
			if (iter != NULL) {
				low_debug_pkg ("Conflicts with", iter->pkg);
				low_package_unref (iter->pkg);

				/* XXX we just need a free function */
				while (iter = low_package_iter_next (iter),
				       iter != NULL) {
					low_package_unref (iter->pkg);
				}
				status = LOW_TRANSACTION_UNRESOLVABLE;
				break;
			}

		}

		/*
		 * We only need to search provides here, because we'll look
		 * at the other pkg anyway.
		 */
		low_debug_pkg ("Checking for other installing pkgs that conflict",
			       pkg);

		for (i = 0; conflicts[i] != NULL; i++) {
			LowPackage *conflicting =
				low_transaction_search_provides (trans->install,
								 conflicts[i]->name);
			if (conflicting) {
				low_debug_pkg ("Conflicted by installing",
					   conflicting);

				low_debug_pkg ("Adding to unresolved",
					       conflicting);
				low_transaction_add_to_hash (trans->unresolved,
							     conflicting, NULL);
				low_transaction_remove_from_hash (trans->install,
								  conflicting);


				status = LOW_TRANSACTION_UNRESOLVABLE;
				break;
			}

		}

//		low_package_dependency_list_free (provides);
		low_package_dependency_list_free (conflicts);

		if (status == LOW_TRANSACTION_UNRESOLVABLE) {
			low_debug_pkg ("Adding to unresolved", pkg);
			low_transaction_add_to_hash (trans->unresolved, pkg,
						     NULL);
			low_transaction_remove_from_hash (trans->install, pkg);
			return status;
		}


		cur = cur->next;
	}

	return status;
}

LowTransactionResult
low_transaction_resolve (LowTransaction *trans G_GNUC_UNUSED)
{
	LowTransactionStatus status = LOW_TRANSACTION_PACKAGES_ADDED;

	struct timeval start;
	struct timeval end;

	low_debug ("Resolving transaction");
	gettimeofday (&start, NULL);

	while (status == LOW_TRANSACTION_PACKAGES_ADDED) {
		LowTransactionStatus conflicts_status, requires_status;

		conflicts_status = low_transaction_check_all_conflicts (trans);
		requires_status = low_transaction_check_all_requires (trans);

		if (conflicts_status == LOW_TRANSACTION_UNRESOLVABLE ||
		    requires_status == LOW_TRANSACTION_UNRESOLVABLE) {
			status = LOW_TRANSACTION_UNRESOLVABLE;
		} else if (conflicts_status == LOW_TRANSACTION_PACKAGES_ADDED ||
			   requires_status == LOW_TRANSACTION_PACKAGES_ADDED) {
			status = LOW_TRANSACTION_PACKAGES_ADDED;
		} else {
			status = LOW_TRANSACTION_NO_CHANGE;
		}
	}


	gettimeofday (&end, NULL);
	low_debug ("Transaction resolved in %.2fs",
		   (int) (end.tv_sec - start.tv_sec) +
		   ((float) (end.tv_usec - start.tv_usec) / 1000000));

	if (status == LOW_TRANSACTION_UNRESOLVABLE) {
		low_debug ("Unresolvable transaction");
		return LOW_TRANSACTION_UNRESOLVED;
	} else {
		low_debug ("Transaction resolved successfully");
		return LOW_TRANSACTION_OK;
	}
}

void
low_transaction_free (LowTransaction *trans)
{
	g_hash_table_unref (trans->install);
	g_hash_table_unref (trans->update);
	g_hash_table_unref (trans->updated);
	g_hash_table_unref (trans->remove);

	g_hash_table_unref (trans->unresolved);

	free (trans);
}

/* vim: set ts=8 sw=8 noet: */
