/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2008 - 2010 James Bowes <jbowes@repl.ca>
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

#ifndef _LOW_DEBUG_H_
#define _LOW_DEBUG_H_

#include <stdarg.h>

#define low_debug(...) low_debug_impl (__FILE__, __func__, __LINE__, \
				       __VA_ARGS__)

#define low_debug_pkg(msg, pkg) low_debug("%s : %s-%s-%s.%s", (msg), \
					  (pkg)->name, (pkg)->version, \
					  (pkg)->release, \
					  low_arch_to_str ((pkg)->arch))

#define low_debug_update(msg, pkg1, pkg2) \
	low_debug("%s : %s-%s-%s.%s to %s-%s-%s.%s", (msg), \
		  (pkg1)->name, \
		  (pkg1)->version, \
		  (pkg1)->release, \
		  low_arch_to_str ((pkg1)->arch), \
		  (pkg2)->name, \
		  (pkg2)->version, \
		  (pkg2)->release, \
		  low_arch_to_str ((pkg2)->arch))

void 	low_debug_impl 	(const char *file, const char *func, const int line,
			 const char *format, ...)
			 __attribute__ ((format (printf, 4, 5)));

void 	low_debug_init 	(void);

#endif /* _LOW_DEBUG_H_ */

/* vim: set ts=8 sw=8 noet: */
