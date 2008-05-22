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
low_config_get_repo_names (LowConfig *config)
{
	static char *names[] = {"test1", "test2", NULL};

	return names;
}

char *
low_config_get_string (LowConfig *config, const char *group, const char *key)
{
	return "test string";
}

gboolean
low_config_get_boolean (LowConfig *config, const char *group, const char *key)
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
