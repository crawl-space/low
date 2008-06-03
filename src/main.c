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


#include <stdio.h>
#include <string.h>

#include "config.h"

#include "low-debug.h"
#include "low-config.h"
#include "low-package.h"
#include "low-package-rpmdb.h"
#include "low-repo-rpmdb.h"
#include "low-repo-set.h"
#include "low-package-sqlite.h"
#include "low-repo-sqlite.h"
#include "low-transaction.h"
#include "low-util.h"
#include "low-download.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define YUM_REPO "http://download.fedora.redhat.com/pub/fedora/linux/releases" \
                 "/9/Everything/x86_64/os/"
#define LOCAL_CACHE "/var/cache/yum/fedora/packages/"

static void show_help (const char *command);
static int usage (void);

static void
print_size (size_t size)
{
	float tmp_size = size;
	if (tmp_size < 1023) {
		printf ("%.0f bytes\n", tmp_size);
		return;
	}

	tmp_size = tmp_size / 1024;
	if (tmp_size < 1023) {
		printf ("%.1f KB\n", tmp_size);
		return;
	}

	tmp_size = tmp_size / 1024;
	if (tmp_size < 1023) {
		printf ("%.1f MB\n", tmp_size);
		return;
	}

	tmp_size = tmp_size / 1024;
	printf ("%.1f GB\n", tmp_size);
	return;
}

static void
wrap_and_print (const char *text)
{
	int i;
	char **wrapped = low_util_word_wrap (text, 79 - 14);

	if (wrapped[0] != NULL) {
		puts (wrapped[0]);
	}

	for (i = 1; wrapped[i] != NULL; i++) {
		printf ("              %s\n", wrapped[i]);
	}

	g_strfreev (wrapped);
}

static void
print_dependencies (const char *dep_name, char **deps)
{
	int i;

	printf ("%-12s:", dep_name);

	if (deps[0] == NULL) {
		printf ("\n");
		return;
	}

	printf (" %s\n", deps[0]);
	for (i = 1; deps[i] != NULL; i++) {
		printf ("              %s\n", deps[i]);
	}
}

static void
print_files (char **files)
{
	int i;

	printf ("Files       :");

	if (files[0] == NULL) {
		printf ("\n");
		return;
	}

	printf (" %s\n", files[0]);
	for (i = 1; files[i] != NULL; i++) {
		printf ("              %s\n", files[i]);
	}
}

static void
print_package (LowPackage *pkg, gboolean show_all)
{
	printf ("Name        : %s\n", pkg->name);
	printf ("Arch        : %s\n", pkg->arch);
	printf ("Version     : %s\n", pkg->version);
	printf ("Release     : %s\n", pkg->release);

	printf ("Size        : ");
	print_size (pkg->size);

	printf ("Repo        : %s\n", pkg->repo->id);

	printf ("Summary     : ");
	wrap_and_print (pkg->summary);

	printf ("URL         : %s\n", pkg->url ? pkg->url : "");
	printf ("License     : %s\n", pkg->license);

	printf ("Description : ");
	wrap_and_print (pkg->description);

	if (show_all) {
		char **deps;
		char **files;

		deps = low_package_get_provides (pkg);
		print_dependencies ("Provides", deps);
		g_strfreev (deps);

		deps = low_package_get_requires (pkg);
		print_dependencies ("Requires", deps);
		g_strfreev (deps);

		deps = low_package_get_conflicts (pkg);
		print_dependencies ("Conflicts", deps);
		g_strfreev (deps);

		deps = low_package_get_obsoletes (pkg);
		print_dependencies ("Obsoletes", deps);
		g_strfreev (deps);

		files = low_package_get_files (pkg);
		print_files (files);
		g_strfreev (files);

	}

	printf ("\n");
}

static void
print_all_packages (LowPackageIter *iter, gboolean show_all)
{
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package (pkg, show_all);

		low_package_unref (pkg);
	}
}

static int
command_info (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *name;
	gboolean show_all = FALSE;

	/* XXX We might not want to ship with the show_all stuff; its ugly. */
	if (argc == 2) {
		/* XXX do option parsing */
		show_all = TRUE;
		name = g_strdup (argv[1]);
	} else {
		name = g_strdup (argv[0]);
	}

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_list_by_name (rpmdb, name);
	print_all_packages (iter, show_all);

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_list_by_name (repos, name);
	print_all_packages (iter, show_all);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	free (name);

	return EXIT_SUCCESS;
}

static void
print_package_short (LowPackage *pkg)
{
	gchar *name_arch = g_strdup_printf ("%s.%s", pkg->name, pkg->arch);
	gchar *version_release = g_strdup_printf ("%s-%s", pkg->version,
						  pkg->release);

	printf ("%-41.41s%-23.23s%s\n", name_arch, version_release,
		pkg->repo->id);

	free (name_arch);
	free (version_release);
}

static void
print_all_packages_short (LowPackageIter *iter)
{
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);

		low_package_unref (pkg);
	}
}

static int
command_list (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowPackageIter *iter;
	LowConfig *config;

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	if (!strcmp(argv[0], "installed") || !strcmp(argv[0], "all")) {
		iter = low_repo_rpmdb_list_all (rpmdb);
		print_all_packages_short (iter);
	}
	if (!strcmp(argv[0], "all")) {
		LowRepoSet *repos =
			low_repo_set_initialize_from_config (config);

		iter = low_repo_set_list_all (repos);
		print_all_packages_short (iter);

		low_repo_set_free (repos);
	} else {
		iter = low_repo_rpmdb_list_by_name (rpmdb, argv[0]);
		print_all_packages_short (iter);

		LowRepoSet *repos =
			low_repo_set_initialize_from_config (config);

		iter = low_repo_set_list_by_name (repos, argv[0]);
		print_all_packages_short (iter);

		low_repo_set_free (repos);
	}


	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);

	return EXIT_SUCCESS;
}

static int
command_search (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *querystr = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_details (rpmdb, querystr);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_details (repos, querystr);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	free (querystr);

	return EXIT_SUCCESS;
}

#define FORMAT_STRING "%-30.30s  %-35.35s  %s\n"

static void
print_repo (LowRepo *repo)
{
	printf (FORMAT_STRING, repo->id, repo->name,
		repo->enabled ? "enabled" : "disabled");
}

static int
command_repolist (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowRepoSetFilter filter = ALL;

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	if (argc == 1) {
		if (!strcmp (argv[0], "all")) {
			filter = ALL;
		} else if (!strcmp (argv[0], "enabled")) {
			filter = ENABLED;
		} else if (!strcmp (argv[0], "disabled")) {
			filter = DISABLED;
		} else {
			printf ("Unknown repo type: %s\n", argv[0]);
			exit (1);
		}
	}

	printf (FORMAT_STRING, "repo id", "repo name", "status");

	print_repo (rpmdb);

	repos = low_repo_set_initialize_from_config (config);
	low_repo_set_for_each (repos, filter, (LowRepoSetFunc) print_repo,
			       NULL);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);

	return EXIT_SUCCESS;
}

static int
command_whatprovides (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *provides = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_provides (rpmdb, provides);
	print_all_packages_short (iter);

	if (provides[0] == '/') {
		iter = low_repo_rpmdb_search_files (rpmdb, provides);
		print_all_packages_short (iter);
	}

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_provides (repos, provides);
	print_all_packages_short (iter);

	if (provides[0] == '/') {
		iter = low_repo_set_search_files (repos, provides);
		print_all_packages_short (iter);
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (provides);

	return EXIT_SUCCESS;
}

static int
command_whatrequires (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *requires = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_requires (rpmdb, requires);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_requires (repos, requires);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (requires);

	return EXIT_SUCCESS;
}

static int
command_whatconflicts (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *conflicts = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_conflicts (rpmdb, conflicts);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_conflicts (repos, conflicts);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (conflicts);

	return EXIT_SUCCESS;
}

static int
command_whatobsoletes (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *obsoletes = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_obsoletes (rpmdb, obsoletes);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_obsoletes (repos, obsoletes);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (obsoletes);

	return EXIT_SUCCESS;
}

static int
command_download (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *name = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_list_by_name (repos, name);
	int found_pkg = 0;
	while (iter = low_package_iter_next (iter), iter != NULL) {
		found_pkg = 1;
		LowPackage *pkg = iter->pkg;

		/* Grab the package name by splitting the location_href on / */
		char **tokens = g_strsplit (pkg->location_href, "/", 20);
		int i = 0;
		char *filename;
		while (1) {
			if (tokens[i + 1] == NULL) {
				filename = tokens[i];
				break;
			}
			i++;
		}

		char *full_url = g_strdup_printf ("%s%s", YUM_REPO, pkg->location_href);
		char *local_file = g_strdup_printf ("%s%s", LOCAL_CACHE, filename);

		printf ("Downloading %s...\n", pkg->name);
		printf ("URL: %s\n", full_url);
		printf ("Saving as: %s\n", local_file);

		low_download_if_missing (full_url, local_file);
		free (full_url);
		free (local_file);

		low_package_unref (pkg);
	}

	if (!found_pkg) {
		printf ("No such package: %s", name);
		return EXIT_FAILURE;
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	free (name);

	return EXIT_SUCCESS;
}

static int
command_install (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	LowTransaction *trans;
	const char *provides = argv[0];
	GSList *install;

	rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (rpmdb);
	repos = low_repo_set_initialize_from_config (config);

	trans = low_transaction_new (rpmdb, repos);

	/* XXX just do the most EVR newest */
	iter = low_repo_set_search_provides (repos, provides);
	iter = low_package_iter_next (iter);
	low_transaction_add_install (trans, iter->pkg);

	/* XXX get rid of this nastiness somehow */
	while (iter = low_package_iter_next (iter), iter != NULL) {
		low_package_unref (iter->pkg);
	}

	if (low_transaction_resolve (trans) != LOW_TRANSACTION_OK) {
		printf ("Error resolving transaction\n");
		return EXIT_FAILURE;
	}

	install = trans->install;
	while (install != NULL) {
		print_package_short (install->data);
		install = install->next;
	}

	low_transaction_free (trans);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);

	return EXIT_SUCCESS;
}

static int
command_remove (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	LowTransaction *trans;
	const char *provides = argv[0];
	GSList *remove;

	rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (rpmdb);
	repos = low_repo_set_initialize_from_config (config);

	trans = low_transaction_new (rpmdb, repos);

	/* XXX just do the most EVR newest */
	iter = low_repo_rpmdb_search_provides (rpmdb, provides);
	iter = low_package_iter_next (iter);

	if (iter == NULL) {
		printf ("No such package to remove\n");
		return EXIT_FAILURE;
	}

	low_transaction_add_remove (trans, iter->pkg);

	while (iter = low_package_iter_next (iter), iter != NULL) ;

	if (low_transaction_resolve (trans) != LOW_TRANSACTION_OK) {
		printf ("Error resolving transaction\n");
		return EXIT_FAILURE;
	}

	remove = trans->remove;
	while (remove != NULL) {
		print_package_short (remove->data);
		remove = remove->next;
	}

	low_transaction_free (trans);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);

	return EXIT_SUCCESS;
}
/**
 * Display the program version as specified in configure.ac
 */
static int
command_version (int argc G_GNUC_UNUSED, const char *argv[] G_GNUC_UNUSED)
{
	printf  (PACKAGE_STRING"\n");
	return EXIT_SUCCESS;
}



static int
command_help (int argc, const char *argv[])
{
	if (argc == 0) {
		usage ();
	} else if (argc == 1) {
		show_help (argv[0]);
	} else {
		show_help ("help");
	}

	return EXIT_SUCCESS;
}

static int
NOT_IMPLEMENTED (int argc G_GNUC_UNUSED, const char *argv[] G_GNUC_UNUSED)
{
	printf ("This function is not yet implemented\n");

	return EXIT_FAILURE;
}

typedef struct _SubCommand {
	char *name;
	char *usage;
	char *summary;
	int (*func) (int argc, const char *argv[]);
} SubCommand;

const SubCommand commands[] = {
	{ "install", "PACKAGE", "Install a package", command_install },
	{ "update", "[PACKAGE]", "Update or install a package",
	  NOT_IMPLEMENTED },
	{ "remove", "PACKAGE", "Remove a package", command_remove },
	{ "clean", NULL, "Remove cached data", NOT_IMPLEMENTED },
	{ "info", "PACKAGE", "Display package details", command_info },
	{ "list", "[all|installed|PACKAGE]", "Display a group of packages",
	  command_list },
	{ "download", NULL, "Download (but don't install) a list of packages",
	  command_download},
	{ "search", "PATTERN",
	  "Search package information for the given string", command_search },
	{ "repolist", "[all|enabled|disabled]",
	  "Display configured software repositories", command_repolist },
	{ "whatprovides", "PATTERN",
	  "Find what package provides the given value", command_whatprovides },
	{ "whatrequires", "PATTERN",
	  "Find what package requires the given value", command_whatrequires },
	{ "whatconflicts", "PATTERN",
	  "Find what package conflicts the given value",
	  command_whatconflicts },
	{ "whatobsoletes", "PATTERN",
	  "Find what package obsoletes the given value",
	  command_whatobsoletes },
	{ "version", NULL, "Display version information", command_version },
	{ "help", "COMMAND", "Display a helpful usage message", command_help }
};

static void
show_help (const char *command)
{
	unsigned int i;

	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		if (!strcmp (command, commands[i].name)) {
			printf ("Usage: %s %s\n", commands[i].name,
				commands[i].usage ? commands[i].usage : "");
			printf("\n%s\n", commands[i].summary);
		}
	}
}

/**
 * Display the default help message.
 *
 * @return A return code to exit the program with.
 */
static int
usage (void)
{
	unsigned int i;

	printf ("low: a yum-like package manager\n");
	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		printf ("  %-20s%s\n", commands[i].name, commands[i].summary);
	}

	return EXIT_FAILURE;
}

int
main (int argc, const char *argv[])
{
	unsigned int i;

	if (argc < 2) {
		return usage ();
	}

	low_debug_init ();

	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		if (!strcmp (argv[1], commands[i].name)) {
			return commands[i].func (argc - 2, argv + 2);
		}
	}
	printf ("Unknown command: %s\n", argv[1]);

	return usage ();
}

/* vim: set ts=8 sw=8 noet: */
