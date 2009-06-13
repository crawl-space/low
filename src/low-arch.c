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
low_arch_is_compatible (LowPackage *target, LowPackage *pkg)
{
	if (target->arch[0] == 'i' && pkg->arch[0] == 'i') {
		return true;
	}

	if (strcmp (target->arch, pkg->arch) == 0) {
		return true;
	}

	if (strcmp (target->arch, "noarch") == 0) {
		return true;
	}
	if (strcmp (pkg->arch, "noarch") == 0) {
		return true;
	}

	return false;
}

static const LowPackage *
low_arch_cmp_for_x86 (LowPackage *pkg1, LowPackage *pkg2)
{
	if (pkg1->arch[0] == 'i' && pkg2->arch[0] == 'i') {
		return pkg1->arch[1] >= pkg2->arch[1] ? pkg1 : pkg2;
	} else if (pkg1->arch[0] == 'i') {
		return pkg1;
	} else if (pkg2->arch[0] == 'i') {
		return pkg2;
	} else {
		return low_arch_choose_best_for_system (pkg1, pkg2);
	}
}

const LowPackage *
low_arch_choose_best (LowPackage *target, LowPackage *pkg1, LowPackage *pkg2)
{
	if (target->arch[0] == 'i') {
		return low_arch_cmp_for_x86 (pkg1, pkg2);
	}

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

	return low_arch_choose_best_for_system (pkg1, pkg2);
}

const LowPackage *
low_arch_choose_best_for_system (LowPackage *pkg1, LowPackage *pkg2)
{
	/* XXX need to generalize */
	if (strcmp (pkg2->arch, "x86_64") == 0 && pkg1->arch[0] == 'i') {
		return pkg2;
	}

	if (pkg1->arch[0] == 'i' && pkg2->arch[0] == 'i') {
		return pkg1->arch[1] > pkg2->arch[1] ? pkg1 : pkg2;
	}

	return pkg1;
}

/* vim: set ts=8 sw=8 noet: */
