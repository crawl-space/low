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

#include <sys/utsname.h>
#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "low-config.h"
#include "low-package.h"
#include "low-repo-rpmdb.h"

#define ARRAY_LENGTH 100
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define RELEASE_PKG "redhat-release"

static gchar *
low_config_load_repo_configs (void)
{
	/* FIXME:  need this to grow */
	gchar *config;
	gchar *joined_config;
	gchar *old_joined_config;
	gchar *path;
	const gchar *config_dir_path = "/etc/yum.repos.d";
	GDir *config_dir = g_dir_open (config_dir_path, 0, NULL);
	const char *file_name;

	/* set this up for the free() below */
	joined_config = malloc (1);
	joined_config[0] = '\0';
	while (file_name = g_dir_read_name (config_dir), file_name != NULL) {
		if (g_str_has_suffix (file_name, ".repo")) {
			old_joined_config = joined_config;
			path = g_strjoin ("/", config_dir_path, file_name, NULL);
			g_file_get_contents (path, &config, NULL, NULL);
			joined_config = g_strjoin ("\n", joined_config, config, NULL);
			g_free (path);
			g_free (old_joined_config);
			g_free (config);
		}
	}
	g_dir_close (config_dir);

	return joined_config;
}

LowConfig *
low_config_initialize (LowRepo *rpmdb)
{
	LowConfig *config = malloc (sizeof (LowConfig));
	gchar *main_config;
	gchar *repo_configs;
	gchar *joined_config;
	GError *error = NULL;

	config->rpmdb = rpmdb;
	config->config = g_key_file_new ();

	g_file_get_contents ("/etc/yum.conf", &main_config, NULL, NULL);

	repo_configs = low_config_load_repo_configs ();

	joined_config = g_strjoin ("\n", main_config, repo_configs, NULL);

	g_key_file_load_from_data (config->config, joined_config,
				   strlen (joined_config), G_KEY_FILE_NONE,
				   &error);
	if (error != NULL) {
		g_print ("%s\n", error->message);
		g_error_free (error);
		exit (1);
	}

	g_free (main_config);
	g_free (repo_configs);
	g_free (joined_config);

	return config;
}

char **
low_config_get_repo_names (LowConfig *config)
{
	int i, j;
	gchar **repo_names = g_key_file_get_groups (config->config, NULL);
	gchar **new_repo_names = g_malloc (sizeof (gchar *) *
									   g_strv_length (repo_names) - 1);
	j = 0;
	for (i = 0; i < g_strv_length (repo_names); i++) {
		if (strcmp (repo_names[i], "main")) {
			new_repo_names[j++] = repo_names[i];
		} else {
			free (repo_names[i]);
		}
	}
	new_repo_names[j] = NULL;

	free (repo_names);
	return new_repo_names;
}

static char *
low_config_replace_single_macro (const char *rawstr, const char *key,
				 const char *value)
{
	char **parts;
	char *replaced;

	if (strstr (rawstr, key)) {
		parts = g_strsplit (rawstr, key, -1);
		replaced = g_strjoinv (value, parts);
		g_strfreev (parts);
	} else {
		replaced = g_strdup (rawstr);
	}

	return replaced;
}

static char *
low_config_replace_macros (LowConfig *config, const char *value)
{
	char *replaced;
	char *old_replaced;

	struct utsname uts;
	LowPackageIter *iter;

	uname(&uts);

	/* XXX deal with no providing package, or more than one */
	iter = low_repo_rpmdb_search_provides (config->rpmdb, RELEASE_PKG);
	iter = low_package_iter_next (iter);


	replaced = low_config_replace_single_macro (value, "$releasever",
						    iter->pkg->version);

	/* Do we have to run through it all to free everything? */
	while (iter = low_package_iter_next (iter), iter != NULL) ;

	old_replaced = replaced;
	replaced = low_config_replace_single_macro (old_replaced, "$basearch",
						    uts.machine);
	free (old_replaced);

	return replaced;
}

char *
low_config_get_string (LowConfig *config, const char *group, const char *key)
{
	char *value = g_key_file_get_string (config->config, group, key, NULL);
	char *value_subbed = low_config_replace_macros (config, value);

	free (value);
	return value_subbed;
}

gboolean
low_config_get_boolean (LowConfig *config, const char *group, const char *key)
{
	return g_key_file_get_boolean (config->config, group, key, NULL);
}

void
low_config_free (LowConfig *config)
{
	g_key_file_free (config->config);
	free (config);
}

/* vim: set ts=8 sw=8 noet: */
