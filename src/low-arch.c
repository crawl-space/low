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

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static struct {
	LowArch arch;
	const char *str;
} arch_map[] = {
	{ ARCH_NOARCH, "noarch" },
	{ ARCH_X86_64, "x86_64" },
	{ ARCH_I386, "i386" },
	{ ARCH_I586, "i586" },
	{ ARCH_I686, "i686" }
};

LowArch
low_arch_from_str (const char *arch_str)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE (arch_map); i++) {
		if (strcmp (arch_map[i].str, arch_str) == 0) {
			return arch_map[i].arch;
		}
	}

	return ARCH_UNKNOWN;
}

const char *
low_arch_to_str (LowArch arch)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE (arch_map); i++) {
		if (arch_map[i].arch == arch) {
			return arch_map[i].str;
		}
	}

	return "unknown";
}

static bool
low_arch_is_ix86 (LowArch arch)
{
	return arch == ARCH_I386 || arch == ARCH_I586 || arch == ARCH_I686;
}

bool
low_arch_is_compatible (LowArch target, LowArch arch)
{
	if (low_arch_is_ix86 (target) && low_arch_is_ix86 (arch)) {
		return true;
	}

	if (target == arch) {
		return true;
	}

	if (target == ARCH_NOARCH) {
		return true;
	}
	if (arch == ARCH_NOARCH) {
		return true;
	}

	return false;
}

static LowArch
low_arch_cmp_for_x86 (LowArch arch1, LowArch arch2)
{
	if (low_arch_is_ix86 (arch1) && low_arch_is_ix86 (arch2)) {
		return arch1 >= arch2 ? 1 : -1;
	} else if (low_arch_is_ix86 (arch1)) {
		return 1;
	} else if (low_arch_is_ix86 (arch2)) {
		return -1;
	} else {
		return low_arch_choose_best_for_system (arch1, arch2);
	}
}

int
low_arch_choose_best (LowArch target, LowArch arch1, LowArch arch2)
{
	if (arch1 == arch2) {
		return 0;
	}

	if (low_arch_is_ix86 (target)) {
		return low_arch_cmp_for_x86 (arch1, arch2);
	}

	if (target == arch1) {
		return 1;
	}
	if (target == arch2) {
		return -1;
	}

	if (arch1 == ARCH_NOARCH) {
		return 1;
	}
	if (arch2 == ARCH_NOARCH) {
		return -1;
	}

	return low_arch_choose_best_for_system (arch1, arch2);
}

int
low_arch_choose_best_for_system (LowArch arch1, LowArch arch2)
{
	if (arch1 == arch2) {
		return 0;
	}

	/* XXX need to generalize */
	if (arch2 == ARCH_X86_64 && low_arch_is_ix86 (arch1)) {
		return -1;
	}

	if (low_arch_is_ix86 (arch1) && low_arch_is_ix86 (arch2)) {
		return arch1 > arch2 ? 1 : -1;
	}

	return 1;
}

/* vim: set ts=8 sw=8 noet: */
