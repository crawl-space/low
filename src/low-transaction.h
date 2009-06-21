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

#include "low-repo.h"
#include "low-repo-set.h"

#ifndef _LOW_TRANSACTION_H_
#define _LOW_TRANSACTION_H_

typedef enum _LowTransactionResult {
	LOW_TRANSACTION_OK,
	LOW_TRANSACTION_UNRESOLVED,
	LOW_TRANSACTION_ERROR
} LowTransactionResult;

typedef void (*LowTransactionProgressCallbackFn) (int progress, gpointer data);

typedef struct _LowTransaction {
	LowRepo *rpmdb;
	LowRepoSet *repos;

	GHashTable *install;
	GHashTable *update;
	GHashTable *updated;
	GHashTable *remove;

	/* Should this be on the struct or returned? */
	GHashTable *unresolved;

	LowTransactionProgressCallbackFn callback;
	gpointer callback_data;
} LowTransaction;

typedef struct _LowTransactionMember {
	LowPackage *pkg;
	bool resolved;

	/* Overloaded for either updated or updating */
	LowPackage *related_pkg;
} LowTransactionMember;

LowTransaction *low_transaction_new (LowRepo *rpmdb, LowRepoSet *repos,
				     LowTransactionProgressCallbackFn callback,
				     gpointer callback_data);
void low_transaction_free (LowTransaction *trans);

/*
 * If anything is getting updated or obsoleted, calculate that during these
 * function calls.
 */

bool low_transaction_add_install (LowTransaction *trans, LowPackage *pkg);
bool low_transaction_add_update (LowTransaction *trans, LowPackage *pkg);
bool low_transaction_add_remove (LowTransaction *trans, LowPackage *pkg);

/**
 * Resolve missing dependencies, add them to the transaction as needed.
 */
LowTransactionResult 	low_transaction_resolve	(LowTransaction *trans);

#endif /* _LOW_TRANSACTION_H_ */

/* vim: set ts=8 sw=8 noet: */
