/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2008, 2009 James Bowes <jbowes@repl.ca>
 *  Copyright (C) 2008 Devan Goodwin <dgoodwin@dangerouslyinc.com>
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

#include "low-mirror-list.h"
#include "low-util.h"

#ifndef _LOW_DOWNLOAD_H_
#define _LOW_DOWNLOAD_H_

typedef int (*LowDownloadCallback) (void *clientp, double dltotal,
				    double dlnow, double ultotal,
				    double ulnow);

int      low_download 		     (const char *url,
				      const char *file,
				      const char *basename,
				      LowDownloadCallback callback);

int      low_download_from_mirror    (LowMirrorList *mirrors,
				      const char *relative_path,
				      const char *file,
				      const char *basename,
				      LowDownloadCallback callback);

int      low_download_if_missing     (LowMirrorList *mirrors,
				      const char *relative_path,
				      const char *file,
				      const char *basename,
				      const char *digest,
				      LowDigestType digest_type,
				      off_t size,
				      LowDownloadCallback callback);

#endif /* _LOW_DOWNLOAD_H_ */

/* vim: set ts=8 sw=8 noet: */
