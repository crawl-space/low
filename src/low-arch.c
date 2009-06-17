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

#include <string.h>

#include "low-arch.h"

bool
low_arch_is_compatible (char *target, char *arch)
{
	if (target[0] == 'i' && arch[0] == 'i') {
		return true;
	}

	if (strcmp (target, arch) == 0) {
		return true;
	}

	if (strcmp (target, "noarch") == 0) {
		return true;
	}
	if (strcmp (arch, "noarch") == 0) {
		return true;
	}

	return false;
}

static const char *
low_arch_cmp_for_x86 (char *arch1, char *arch2)
{
	if (arch1[0] == 'i' && arch2[0] == 'i') {
		return arch1[1] >= arch2[1] ? arch1 : arch2;
	} else if (arch1[0] == 'i') {
		return arch1;
	} else if (arch2[0] == 'i') {
		return arch2;
	} else {
		return low_arch_choose_best_for_system (arch1, arch2);
	}
}

const char *
low_arch_choose_best (char *target, char *arch1, char *arch2)
{
	if (target[0] == 'i') {
		return low_arch_cmp_for_x86 (arch1, arch2);
	}

	if (strcmp (target, arch1) == 0) {
		return arch1;
	}
	if (strcmp (target, arch2) == 0) {
		return arch2;
	}

	if (strcmp (arch1, "noarch") == 0) {
		return arch1;
	}
	if (strcmp (arch2, "noarch") == 0) {
		return arch2;
	}

	return low_arch_choose_best_for_system (arch1, arch2);
}

const char *
low_arch_choose_best_for_system (char *arch1, char *arch2)
{
	/* XXX need to generalize */
	if (strcmp (arch2, "x86_64") == 0 && arch1[0] == 'i') {
		return arch2;
	}

	if (arch1[0] == 'i' && arch2[0] == 'i') {
		return arch1[1] > arch2[1] ? arch1 : arch2;
	}

	return arch1;
}

/* vim: set ts=8 sw=8 noet: */
