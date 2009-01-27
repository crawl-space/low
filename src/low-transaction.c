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
	low_package_unref (member->pkg);

	if (member->related_pkg) {
		low_package_unref (member->pkg);
	}

	free (member);
}

LowTransaction *
low_transaction_new (LowRepo *repo_rpmdb, LowRepoSet *repos) {
	LowTransaction *trans = malloc (sizeof (LowTransaction));

	trans->rpmdb = repo_rpmdb;
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

		free (key);
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

static gboolean
low_transaction_is_pkg_in_hash (GHashTable *hash, LowPackage *pkg)
{
	char *key;
	gboolean ret;

	if (pkg->epoch) {
		key = g_strdup_printf ("%s-%s:%s-%s.%s", pkg->name,
				       pkg->epoch, pkg->version,
				       pkg->release, pkg->arch);
	} else {
		key = g_strdup_printf ("%s-%s-%s.%s", pkg->name,
				       pkg->version, pkg->release,
				       pkg->arch);
	}

	ret = g_hash_table_lookup (hash, key) != 0 ? TRUE : FALSE;

	g_free (key);

	return ret;
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

static gboolean
arch_is_compatible (LowPackage *target, LowPackage *pkg)
{
	if (strcmp (target->arch, pkg->arch) == 0) {
		return TRUE;
	}

	if (strcmp (target->arch, "noarch") == 0) {
		return TRUE;
	}
	if (strcmp (pkg->arch, "noarch") == 0) {
		return TRUE;
	}

	return FALSE;
}

static LowPackage *
choose_best_arch (LowPackage *target, LowPackage *pkg1, LowPackage *pkg2)
{
	if (strcmp (target->arch, pkg1->arch) == 0) {
		return pkg1;
	}
	if (strcmp (target->arch, pkg2->arch) == 0) {
		return pkg2;
	}

	if (strcmp (pkg1->arch, "noarch") == 0) {
		return pkg1;
	}
	if (strcmp (pkg2->arch, "noarch") == 0) {
		return pkg2;
	}

	return pkg1;
}

static LowPackage *
choose_best_for_update (LowRepo *repo_rpmdb, LowRepoSet *repos,
			LowPackage *to_update)
{
	LowPackage *best = to_update;
	LowPackageIter *iter;
	char *best_evr = low_package_evr_as_string(to_update);
	gboolean found;

	LowPackageDependency *provides =
		low_package_dependency_new (to_update->name,
					    DEPENDENCY_SENSE_GT,
					    best_evr);
	LowPackageDependency *obsoletes =
		low_package_dependency_new (to_update->name,
					    DEPENDENCY_SENSE_EQ,
					    best_evr);

	iter = low_repo_set_search_provides (repos, provides);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		char *new_evr = low_package_evr_as_string (iter->pkg);
		int cmp = rpmvercmp (new_evr, best_evr);

		/* XXX arch cmp here has to be better */
		if ((cmp > 0 && arch_is_compatible (to_update, iter->pkg)) ||
		    (cmp == 0 &&
		     choose_best_arch (to_update, best, iter->pkg) == iter->pkg)) {
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

	iter = low_repo_set_search_obsoletes (repos, obsoletes);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		char *new_evr = low_package_evr_as_string (iter->pkg);
		int cmp = rpmvercmp (new_evr, best_evr);

		/* XXX arch cmp here has to be better */
		if ((cmp > 0 && arch_is_compatible (to_update, iter->pkg)) ||
		    (cmp == 0 &&
		     choose_best_arch (to_update, best, iter->pkg) == iter->pkg)) {
			low_package_unref (best);
			best = iter->pkg;

			g_free (best_evr);
			best_evr = new_evr;
		} else {
			low_package_unref (iter->pkg);
			g_free (new_evr);
		}

	}

	low_package_dependency_free (obsoletes);

	/* We haven't found anything better */
	if (to_update == best) {
		g_free (best_evr);
		return NULL;
	}

	/* Ensure we don't already have it */
	found = FALSE;
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

static gboolean
add_update_worker (LowTransaction *trans, LowPackage *to_update,
		   LowPackage *updating_to)
{
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
low_transaction_add_update (LowTransaction *trans, LowPackage *to_update)
{
	LowPackage *updating_to = choose_best_for_update (trans->rpmdb,
							  trans->repos,
							  to_update);

	if (updating_to == NULL) {
		low_debug_pkg ("No available update for", to_update);
		return FALSE;
	}

	return add_update_worker (trans, to_update, updating_to);
}

static LowPackage *
find_updated (LowRepo *repo_rpmdb, LowPackage *updating)
{
	LowPackage *updated = NULL;
	LowPackageIter *iter;
	gboolean found = FALSE;
	char *updating_evr = low_package_evr_as_string(updating);

	iter = low_repo_rpmdb_list_by_name (repo_rpmdb, updating->name);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		char *updated_evr = low_package_evr_as_string (iter->pkg);

		/* XXX arch cmp here has to be better */
		if (!found &&
		    rpmvercmp (updating_evr, updated_evr) > 0 &&
		    (strcmp (iter->pkg->arch, updating->arch) == 0 ||
		     strcmp (updating->arch, "noarch") == 0)) {
			updated = iter->pkg;
			found = TRUE;
		} else {
			low_package_unref (iter->pkg);
		}
		g_free (updated_evr);

	}

	g_free (updating_evr);

	return updated;
}
/**
 * Install a package, or mark it for update if it updates an installed package
 **/
static gboolean
low_transaction_add_install_or_update (LowTransaction *trans,
				       LowPackage *to_install)
{
	LowPackage *updated = find_updated (trans->rpmdb, to_install);
	if (updated) {
		return add_update_worker (trans, updated, to_install);
	} else {
		return low_transaction_add_install (trans, to_install);
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
		if (!strcmp (needle->name, haystack[i]->name) &&
		    needle->sense == haystack[i]->sense &&
		    needle->evr != NULL && haystack[i]->evr != NULL &&
		    !strcmp (needle->evr, haystack[i]->evr)) {
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
select_best_provides (LowTransaction *trans, LowPackage *pkg,
		      LowPackageIter *iter, gboolean check_for_existing)
{
	LowTransactionStatus status = LOW_TRANSACTION_UNRESOLVABLE;
	LowPackage *best = NULL;
	char *best_evr = g_strdup ("0");

	/* XXX this is duplicated in main.c */
	while (iter = low_package_iter_next (iter), iter != NULL) {
		char *new_evr = low_package_evr_as_string (iter->pkg);
		int cmp = rpmvercmp (new_evr, best_evr);

		if ((cmp > 0 && arch_is_compatible (pkg, iter->pkg)) ||
		    (cmp == 0 &&
		     choose_best_arch (pkg, best, iter->pkg) == iter->pkg) ||
		    low_transaction_is_pkg_in_hash (trans->install, iter->pkg) ||
		    low_transaction_is_pkg_in_hash (trans->update, iter->pkg)) {
			if (best) {
				low_package_unref (best);
			}

			g_free (best_evr);
			best = iter->pkg;
			best_evr = new_evr;
		} else {
			low_package_unref (iter->pkg);
			g_free (new_evr);
		}

	}

	g_free (best_evr);

	if (best) {
		/* XXX clean this up */
		if (check_for_existing) {
			if (low_transaction_is_pkg_in_hash (trans->install, best) ||
			    low_transaction_is_pkg_in_hash (trans->update, best)) {
				return LOW_TRANSACTION_NO_CHANGE;
			} else {
				return LOW_TRANSACTION_UNRESOLVABLE;
			}
		}
		if (low_transaction_add_install_or_update (trans, best)) {
			status = LOW_TRANSACTION_PACKAGES_ADDED;
		} else {
			low_package_unref (best);
			status = LOW_TRANSACTION_NO_CHANGE;
		}
	}

	return status;
}

static LowTransactionStatus
low_transaction_check_package_requires (LowTransaction *trans, LowPackage *pkg,
					gboolean check_available)
{
	LowTransactionStatus status = LOW_TRANSACTION_NO_CHANGE;
	LowPackageDependency **requires;
	LowPackageDependency **provides;
	char **files;
	int i;
	gboolean found;

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

		if (strncmp (requires[i]->name, "rpmlib(", 7) == 0) {
			low_debug ("rpmlib requires, skipping");
			continue;
		}

		providing =
			low_repo_rpmdb_search_provides (trans->rpmdb,
							requires[i]);

		found = FALSE;
		while (providing = low_package_iter_next (providing),
		       providing != NULL) {
			if (!found) {
				if (low_transaction_is_pkg_in_hash (trans->remove,
								    providing->pkg) ||
				    low_transaction_is_pkg_in_hash (trans->updated,
								    providing->pkg)) {
					low_debug ("Providing package is being removed");
				} else {
					low_debug_pkg ("Provided by", providing->pkg);
					found = TRUE;
				}

			}

			low_package_unref (providing->pkg);
		}
		if (found) {
			continue;
		/* Check files if appropriate */
		} else if (requires[i]->name[0] == '/') {
			providing =
				low_repo_rpmdb_search_files (trans->rpmdb,
							     requires[i]->name);

			found = FALSE;
			while (providing = low_package_iter_next (providing),
			       providing != NULL) {
				if (!found) {
					if (low_transaction_is_pkg_in_hash (trans->remove,
									    providing->pkg) ||
					    low_transaction_is_pkg_in_hash (trans->updated,
									    providing->pkg)) {
						low_debug ("Providing package is being removed");
					} else {
						low_debug_pkg ("Provided by", providing->pkg);
						found = TRUE;
					}

				}

				low_package_unref (providing->pkg);
			}
			if (found) {
				continue;
			}

		}

		/* Check available packages */
		providing = low_repo_set_search_provides (trans->repos,
							  requires[i]);
		status = select_best_provides (trans, pkg, providing,
					       !check_available);
		if (status == LOW_TRANSACTION_UNRESOLVABLE &&
		    requires[i]->name[0] == '/') {
			providing =
				low_repo_set_search_files (trans->repos,
							   requires[i]->name);

			status = select_best_provides (trans, pkg, providing,
						       !check_available);
			if (status != LOW_TRANSACTION_UNRESOLVABLE) {
				continue;
			}

		} else if (status != LOW_TRANSACTION_UNRESOLVABLE) {
			continue;
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
low_transaction_check_removal (LowTransaction *trans,
			       LowTransactionMember *member,
			       gboolean from_update)
{
	LowPackage *pkg = member->pkg;
	LowTransactionStatus status = LOW_TRANSACTION_NO_CHANGE;
	LowPackageDependency **provides;
	LowPackageDependency **update_provides = NULL;
	char **files;
	char **update_files = NULL;
	int i;

	low_debug_pkg ("Checking removal of", pkg);

	provides = low_package_get_provides (pkg);
	files = low_package_get_files (pkg);

	if (from_update) {
		update_provides =
			low_package_get_provides (member->related_pkg);
		update_files = low_package_get_files (member->related_pkg);
	}


	for (i = 0; provides[i] != NULL; i++) {
		LowPackageIter *iter;

		if (from_update &&
		    low_transaction_dep_in_deplist (provides[i],
						    update_provides)) {
		    low_debug ("Provides provided by update %s",
			       provides[i]->name);
		    continue;
		}

		low_debug ("Checking provides %s", provides[i]->name);

		iter = low_repo_rpmdb_search_requires (trans->rpmdb,
						       provides[i]);
		while (iter = low_package_iter_next (iter), iter != NULL) {
			/* It's a self-requires, skip */
			if (pkg == iter->pkg) {
				continue;
			}

			if (low_transaction_is_pkg_in_hash (trans->remove,
							    iter->pkg) ||
			    low_transaction_is_pkg_in_hash (trans->updated,
							    iter->pkg)) {
				low_debug ("Requiring package is being removed");
				continue;
			}


			if (low_transaction_check_package_requires (trans,
								    iter->pkg,
								    FALSE) ==
			    LOW_TRANSACTION_NO_CHANGE) {
				low_debug ("Requiring package satisfied by installed package");
				continue;
			}

			if (from_update) {
				if (low_transaction_add_update (trans, iter->pkg)) {
					low_debug ("found updating package");
					status = LOW_TRANSACTION_PACKAGES_ADDED;
					continue;
				} else {
					return LOW_TRANSACTION_UNRESOLVABLE;
				}
			}

			low_debug_pkg ("Adding for removal", iter->pkg);
			if (low_transaction_add_remove (trans, iter->pkg)) {
				status = LOW_TRANSACTION_PACKAGES_ADDED;
			}
		}
	}

	for (i = 0; files[i] != NULL; i++) {
		LowPackageIter *iter;
		LowPackageDependency *file_dep;

		if (from_update &&
		    low_transaction_dep_in_filelist (files[i], update_files)) {
		    low_debug ("File contained in update");
		    continue;
		}


		file_dep = low_package_dependency_new (files[i],
						       DEPENDENCY_SENSE_NONE,
						       NULL);

		low_debug ("Checking file %s", files[i]);

		iter = low_repo_rpmdb_search_requires (trans->rpmdb,
						       file_dep);
		while (iter = low_package_iter_next (iter), iter != NULL) {
			/* It's a self-requires, skip */
			if (pkg == iter->pkg) {
				continue;
			}

			if (low_transaction_is_pkg_in_hash (trans->remove,
							    iter->pkg) ||
			    low_transaction_is_pkg_in_hash (trans->updated,
							    iter->pkg)) {
				low_debug ("Requiring package is being removed");
				continue;
			}

			if (from_update) {
				if (low_transaction_add_update (trans, iter->pkg)) {
					low_debug ("found updating package");
					status = LOW_TRANSACTION_PACKAGES_ADDED;
					continue;
				} else {
					return LOW_TRANSACTION_UNRESOLVABLE;
				}
			}

			low_debug_pkg ("Adding for removal", iter->pkg);
			if (low_transaction_add_remove (trans, iter->pkg)) {
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
									     pkg,
									     TRUE);

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
			rm_status = low_transaction_check_removal (trans,
								   member,
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
				low_debug_pkg ("Conflicts with installing",
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
//		low_package_dependency_list_free (conflicts);

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
	g_hash_table_destroy (trans->install);
	g_hash_table_destroy (trans->update);
	g_hash_table_destroy (trans->updated);
	g_hash_table_destroy (trans->remove);

	g_hash_table_destroy (trans->unresolved);

	free (trans);
}

/* vim: set ts=8 sw=8 noet: */
