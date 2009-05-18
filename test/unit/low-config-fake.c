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

#include <stdlib.h>
#include "low-config-fake.h"

/**
 * Stub in-memory replacement for the config
 */

LowConfig *
low_config_initialize (LowRepo *rpmdb)
{
	LowConfig *config = malloc (sizeof (LowConfig));

	config->rpmdb = rpmdb;
	config->config = g_key_file_new ();

	return config;
}

char **
low_config_get_repo_names (LowConfig *config G_GNUC_UNUSED)
{
	static char *names[3];
	names[0] = g_strdup ("test1");
	names[1] = g_strdup ("test2");
	names[2] = NULL;

	return names;
}

char *
low_config_get_string (LowConfig *config G_GNUC_UNUSED,
		       const char *group G_GNUC_UNUSED,
		       const char *key G_GNUC_UNUSED)
{
	return g_strdup ("test string");
}

gboolean
low_config_get_boolean (LowConfig *config G_GNUC_UNUSED,
			const char *group G_GNUC_UNUSED,
			const char *key G_GNUC_UNUSED)
{
	return TRUE;
}

void
low_config_free (LowConfig *config)
{
	g_key_file_free (config->config);
	free (config);
}

/* vim: set ts=8 sw=8 noet: */
