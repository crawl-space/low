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

#include "low-config.h"
#include "low-package.h"
#include "low-package-rpmdb.h"
#include "low-repo-rpmdb.h"
#include "low-repo-set.h"
#include "low-package-sqlite.h"
#include "low-repo-sqlite.h"
#include "low-util.h"
#include "low-download.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define YUM_REPO "http://download.fedora.redhat.com/pub/fedora/linux/releases" \
                 "/9/Everything/x86_64/os/"
#define LOCAL_CACHE "/var/cache/yum/fedora"

static int usage (void);

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
print_package (const LowPackage *pkg)
{
	printf ("Name        : %s\n", pkg->name);
	printf ("Arch        : %s\n", pkg->arch);
	printf ("Version     : %s\n", pkg->version);
	printf ("Release     : %s\n", pkg->release);
	printf ("Size        : %zd bytes\n", pkg->size);
	printf ("Repo        : %s\n", pkg->repo);

	printf ("Summary     : ");
	wrap_and_print (pkg->summary);

	printf ("URL         : %s\n", pkg->url ? pkg->url : "");
	printf ("License     : %s\n", pkg->license);

	printf ("Description : ");
	wrap_and_print (pkg->description);
	printf ("\n");
}

static void
info (LowRepo *repo, gpointer data)
{
	LowPackageIter *iter;
	char *name = (char *) data;

	iter = low_repo_sqlite_list_by_name (repo, name);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package (pkg);
	}
}

static int
command_info (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *name = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_list_by_name (rpmdb, name);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package (pkg);
	}

	repos = low_repo_set_initialize_from_config (config);
	low_repo_set_for_each (repos, ENABLED, (LowRepoSetFunc) info, name);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	free (name);

	return 0;
}

static void
print_package_short (LowPackage *pkg)
{
	gchar *name_arch = g_strdup_printf ("%s.%s", pkg->name, pkg->arch);
	gchar *version_release = g_strdup_printf ("%s-%s", pkg->version,
						  pkg->release);

	printf ("%-41.41s%-23.23s%s\n", name_arch, version_release, pkg->repo);

	free (name_arch);
	free (version_release);
}

static void
list (LowRepo *repo, gpointer data)
{
	LowPackageIter *iter;

	iter = low_repo_sqlite_list_all (repo);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}
}

static int
command_list (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowPackageIter *iter;
	LowConfig *config;

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	if (!strcmp(argv[0], "installed") || !strcmp(argv[0], "all")) {
		iter = low_repo_rpmdb_list_all (rpmdb);
		while (iter = low_package_iter_next (iter), iter != NULL) {
			LowPackage *pkg = iter->pkg;
			print_package_short (pkg);
		}
	}
	if (!strcmp(argv[0], "all")) {
		LowRepoSet *repos =
			low_repo_set_initialize_from_config (config);
		low_repo_set_for_each (repos, ENABLED, (LowRepoSetFunc) list,
				       NULL);
		low_repo_set_free (repos);
	}

	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);

	return 0;
}

static void
search (LowRepo *repo, gpointer data)
{
	LowPackageIter *iter;
	const char *querystr = (const char *) data;

	iter = low_repo_sqlite_generic_search (repo, querystr);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}
}

static int
command_search (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *querystr = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_generic_search (rpmdb, querystr);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	repos = low_repo_set_initialize_from_config (config);
	low_repo_set_for_each (repos, ENABLED, (LowRepoSetFunc) search,
			       querystr);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	free (querystr);

	return 0;
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

	return 0;
}

static int
command_whatprovides (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *provides = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);
	iter = low_repo_rpmdb_search_provides (rpmdb, provides);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	iter = low_repo_rpmdb_search_files (rpmdb, provides);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_provides (repos, provides);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	iter = low_repo_set_search_files (repos, provides);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (provides);

	return 0;
}

static int
command_whatrequires (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *requires = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_requires (rpmdb, requires);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_requires (repos, requires);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (requires);

	return 0;
}

static int
command_whatconflicts (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *conflicts = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_conflicts (rpmdb, conflicts);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_conflicts (repos, conflicts);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}


	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (conflicts);

	return 0;
}

static int
command_whatobsoletes (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	gchar *obsoletes = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (rpmdb);

	iter = low_repo_rpmdb_search_obsoletes (rpmdb, obsoletes);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	repos = low_repo_set_initialize_from_config (config);

	iter = low_repo_set_search_obsoletes (repos, obsoletes);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package_short (pkg);
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (rpmdb);
	g_free (obsoletes);

	return 0;
}

/**
 * Display the program version as specified in configure.ac
 */
static int
command_version (int argc, const char *argv[])
{
	printf  (PACKAGE_STRING"\n");
	return 0;
}

static int
command_help (int argc, const char *argv[])
{
	usage ();
	return 0;
}

static int
NOT_IMPLEMENTED (int argc, const char *argv[])
{
	printf ("This function is not yet implemented\n");

	return 1;
}

static int
command_download (int argc, const char *argv[])
{
	char *url = YUM_REPO "fedora8.png";
	char *file = "/tmp/fedora8.png";
	low_download_if_missing(url, file);
	return 0;
}

typedef struct _SubCommand {
	char *name;
	char *summary;
	int (*func) (int argc, const char *argv[]);
} SubCommand;

const SubCommand commands[] = {
	{ "install", "Install a package", NOT_IMPLEMENTED },
	{ "update", "Update or install a package", NOT_IMPLEMENTED },
	{ "remove", "Remove a package", NOT_IMPLEMENTED },
	{ "clean", "Remove cached data", NOT_IMPLEMENTED },
	{ "info", "Display package details", command_info },
	{ "list", "Display a group of packages", command_list },
	{ "download", "Download (but don't install) a list of packages",
		command_download},
	{ "search", "Search package information for the given string",
	  command_search },
	{ "repolist", "Display configured software repositories",
	  command_repolist },
	{ "whatprovides", "Find what package provides the given value",
	  command_whatprovides },
	{ "whatrequires", "Find what package requires the given value",
	  command_whatrequires },
	{ "whatconflicts", "Find what package conflicts the given value",
	  command_whatconflicts },
	{ "whatobsoletes", "Find what package obsoletes the given value",
	  command_whatobsoletes },
	{ "version", "Display version information", command_version },
	{ "help", "Display a helpful usage message", command_help }
};

/**
 * Display the default help message.
 *
 * @return A return code to exit the program with.
 */
static int
usage (void)
{
	int i;

	printf ("low: a yum-like package manager\n");
	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		printf ("  %-20s%s\n", commands[i].name, commands[i].summary);
	}

	return 1;
}

int
main (int argc, const char *argv[])
{
	int i;

	if (argc < 2) {
		return usage ();
	}

	for (i = 0; i < ARRAY_SIZE (commands); i++) {
		if (!strcmp (argv[1], commands[i].name)) {
			return commands[i].func (argc - 2, argv + 2);
		}
	}
	printf ("Unknown command: %s\n", argv[1]);

	return usage ();
}

/* vim: set ts=8 sw=8 noet: */
