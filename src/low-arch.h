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

#include "low-package.h"

bool low_arch_is_compatible (LowPackage *target, LowPackage *pkg);
const LowPackage *low_arch_choose_best (LowPackage *target, LowPackage *pkg1, LowPackage *pkg2);
const LowPackage *low_arch_choose_best_for_system (LowPackage *pkg1, LowPackage *pkg2);

#endif /* _LOW_ARCH_H_ */

/* vim: set ts=8 sw=8 noet: */
