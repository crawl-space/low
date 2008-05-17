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

#include "low-repo.h"

#ifndef _LOW_PACKAGE_H_
#define _LOW_PACKAGE_H_

typedef struct _LowPackage {
    const char *name;
    const char *version;
    const char *release;
    const char *arch;
    const char *epoch;

    size_t size;
    const char *repo;
    const char *summary;
    const char *description;
    const char *url;
    const char *license;
} LowPackage;

typedef struct _LowPackageIter {
	LowRepo *repo;
	LowPackage *pkg;
} LowPackageIter;

#endif /* _LOW_PACKAGE_H_ */
