#include <stdlib.h>
#include <string.h>
#include <glib.h>
#include "low-config.h"

#define ARRAY_LENGTH 100
#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

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
	
	return joined_config;
}

LowConfig *
low_config_initialize (void)
{
	LowConfig *config = malloc (sizeof (LowConfig));
	gchar *main_config;
	gchar *repo_configs;
	gchar *joined_config;
	GError *error = NULL;

	config->config = g_key_file_new ();

	g_file_get_contents ("/etc/yum.conf", &main_config, NULL, NULL);

	repo_configs = low_config_load_repo_configs ();

	joined_config = g_strjoin ("\n", main_config, repo_configs, NULL);

	g_key_file_load_from_data (config->config, joined_config,
							   strlen (joined_config), G_KEY_FILE_NONE, &error);
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
	return g_key_file_get_groups (config->config, NULL);
}

void
low_config_free (LowConfig *config)
{
	g_key_file_free (config->config);
	free (config);
}
