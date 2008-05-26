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

#include <stdlib.h>
#include <string.h>

#include "low-debug.h"
#include "low-transaction.h"
#include "low-repo-rpmdb.h"

LowTransaction *
low_transaction_new (LowRepo *rpmdb, LowRepoSet *repos) {
	LowTransaction *trans = malloc (sizeof (LowTransaction));

	trans->rpmdb = rpmdb;
	trans->repos = repos;

	trans->install = NULL;

	return trans;
}

void
low_transaction_add_install (LowTransaction *trans, LowPackage *to_install)
{
	low_debug_pkg ("Adding for install", to_install);

	trans->install = g_slist_append (trans->install, to_install);
}

/**
 * Check if a requires is in a list of provides */
static gboolean
low_transaction_string_in_list (const char *needle, char **haystack)
{
	int i;

	for (i = 0; haystack[i] != NULL; i++) {
		if (!strcmp (needle, haystack[i])) {
			return TRUE;
		}
	}

	return FALSE;
}

static void
low_transaction_check_requires (LowTransaction *trans)
{
	GSList *cur = trans->install;

	while (cur != NULL) {
		char **requires;
		char **provides;
		int i;

		LowPackage *pkg = (LowPackage *) cur->data;
		low_debug_pkg ("Checking requires for", pkg);

		requires = low_package_get_requires (pkg);
		provides = low_package_get_provides (pkg);

		for (i = 0; requires[i] != NULL; i++) {
			LowPackageIter *providing;

			if (low_transaction_string_in_list (requires[i],
							    provides)) {
			    low_debug ("Self provided requires %s, skipping",
				       requires[i]);
			    continue;
			}
			low_debug ("Checking requires %s", requires[i]);

			providing =
				low_repo_rpmdb_search_provides (trans->rpmdb,
								requires[i]);
			/* XXX memory leak */
			providing = low_package_iter_next (providing);
			if (providing != NULL) {
				low_debug_pkg ("Provided by", providing->pkg);
			} else {
				low_debug ("%s not provided by installed pkg",
					   requires[i]);
			}
		}

		cur = cur->next;
	}
}

void
low_transaction_resolve (LowTransaction *trans G_GNUC_UNUSED)
{
	low_debug ("Resolving transaction");

	while (TRUE) {
		low_transaction_check_requires (trans);
		break;
	}
}

void
low_transaction_free (LowTransaction *trans) {
	free (trans);
}

/* vim: set ts=8 sw=8 noet: */
