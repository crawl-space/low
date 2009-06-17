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

#ifndef _LOW_ARCH_H_
#define _LOW_ARCH_H_

#include <stdbool.h>

typedef enum {
	ARCH_NOARCH,
	ARCH_X86_64,
	ARCH_I386,
	ARCH_I586,
	ARCH_I686,
	ARCH_UNKNOWN
} LowArch;

LowArch low_arch_from_str (const char *arch_str);
const char *low_arch_to_str (LowArch arch);

bool low_arch_is_compatible (LowArch target, LowArch arch);
int low_arch_choose_best (LowArch target, LowArch arch1, LowArch arch2);
int low_arch_choose_best_for_system (LowArch pkg1, LowArch pkg2);

#endif /* _LOW_ARCH_H_ */

/* vim: set ts=8 sw=8 noet: */
