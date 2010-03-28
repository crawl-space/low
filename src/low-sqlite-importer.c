/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2010 James Bowes <jbowes@repl.ca>
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

#include <glib.h>

#include "low-debug.h"

#include "low-sqlite-importer.h"

void
low_sqlite_importer_begin_package (LowSqliteImporter *importer G_GNUC_UNUSED,
				   const char *name, const char *arch,
				   const char *epoch G_GNUC_UNUSED,
				   const char *version,
				   const char *release)
{
	low_debug ("begin package: %s-%s-%s.%s", name, version, release, arch);
}

void
low_sqlite_importer_add_details (LowSqliteImporter *importer G_GNUC_UNUSED,
				 const char *summary G_GNUC_UNUSED,
				 const char *description G_GNUC_UNUSED,
				 const char *url G_GNUC_UNUSED,
				 const char *license G_GNUC_UNUSED)
{
	low_debug ("add details");
}

void
low_sqlite_importer_add_dependency (LowSqliteImporter *importer G_GNUC_UNUSED,
				    const char *name,
				    const char *sense G_GNUC_UNUSED,
				    LowSqliteImporterDepType type G_GNUC_UNUSED,
				    bool is_pre G_GNUC_UNUSED,
				    const char *epoch G_GNUC_UNUSED,
				    const char *version G_GNUC_UNUSED,
				    const char *release G_GNUC_UNUSED)
{
	low_debug ("dep - %s", name);
}

void
low_sqlite_importer_add_file (LowSqliteImporter *importer G_GNUC_UNUSED, const char *name)
{
	low_debug ("file - %s", name);
}

void
low_sqlite_importer_finish_package (LowSqliteImporter *importer G_GNUC_UNUSED)
{

}

/* vim: set ts=8 sw=8 noet: */
