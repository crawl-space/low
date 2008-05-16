
#include <stdio.h>
#include <string.h>

#include "low-config.h"
#include "low-package.h"
#include "low-package-rpmdb.h"
#include "low-repo-rpmdb.h"
#include "low-repo-set.h"
#include "low-package-sqlite.h"
#include "low-repo-sqlite.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

static int usage (void);

static void
print_package (const LowPackage *pkg)
{
	printf ("Name        : %s\n", pkg->name);
	printf ("Arch        : %s\n", pkg->arch);
	printf ("Version     : %s\n", pkg->version);
	printf ("Release     : %s\n", pkg->release);
	printf ("Size        : %zd bytes\n", pkg->size);
	printf ("Summary     : %s\n", pkg->summary);
	printf ("URL         : %s\n", pkg->url);
	printf ("License     : %s\n", pkg->license);
	printf ("Description : %s\n", pkg->description);
	printf ("\n");
}

static int
command_info (int argc, const char *argv[])
{
	LowRepo *repo;
	LowPackageIter *iter;

	repo = low_repo_rpmdb_initialize ();
	iter = low_repo_rpmdb_list_by_name (repo, argv[0]);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package (pkg);
	}
	low_repo_rpmdb_shutdown (repo);

	repo = low_repo_sqlite_initialize ("fedora", "fedora", TRUE);
	iter = low_repo_sqlite_list_by_name (repo, argv[0]);
	while (iter = low_sqlite_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		print_package (pkg);
	}
	low_repo_sqlite_shutdown (repo);

	return 0;
}

static int
command_list (int argc, const char *argv[])
{
	LowRepo *repo;
	LowPackageIter *iter;

	if (!strcmp(argv[0], "installed") || !strcmp(argv[0], "all")) {
		repo = low_repo_rpmdb_initialize ();
		iter = low_repo_rpmdb_list_all (repo);
		while (iter = low_package_iter_next (iter), iter != NULL) {
			LowPackage *pkg = iter->pkg;
			printf ("%s.%s  %s-%s\n", pkg->name, pkg->arch,
					pkg->version, pkg->release);
		}
		low_repo_rpmdb_shutdown (repo);
	}
	if (!strcmp(argv[0], "all")) {
		repo = low_repo_sqlite_initialize ("fedora", "fedora", TRUE);
		iter = low_repo_sqlite_list_all (repo);
		while (iter = low_sqlite_package_iter_next (iter), iter != NULL) {
			LowPackage *pkg = iter->pkg;
			printf ("%s.%s  %s-%s\n", pkg->name, pkg->arch,
					pkg->version, pkg->release);
		}
		low_repo_sqlite_shutdown (repo);
	}


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
	LowConfig *config = low_config_initialize ();
	LowRepoSetFilter filter;

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

	rpmdb = low_repo_rpmdb_initialize ();
	print_repo (rpmdb);	
	low_repo_rpmdb_shutdown (rpmdb);

	repos = low_repo_set_initialize_from_config (config);
	low_repo_set_for_each (repos, filter, (LowRepoSetFunc) print_repo, NULL);
	low_repo_set_free (repos);
	low_config_free (config);
	
	return 0;
}
static void
search_provides (LowRepo *repo, gpointer data)
{
	LowPackageIter *iter;
	char *provides = (char *) data;
	
	iter = low_repo_sqlite_search_provides (repo, provides);
	while (iter = low_sqlite_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
			printf ("%s.%s  %s-%s\n", pkg->name, pkg->arch,
					pkg->version, pkg->release);
	}

}

static int
command_whatprovides (int argc, const char *argv[])
{
	LowRepo *rpmdb;
	LowRepoSet *repos;
	LowConfig *config = low_config_initialize ();
	LowPackageIter *iter;
	gchar *provides = g_strdup (argv[0]);

	rpmdb = low_repo_rpmdb_initialize ();
	iter = low_repo_rpmdb_search_provides (rpmdb, provides);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
			printf ("%s.%s  %s-%s\n", pkg->name, pkg->arch,
					pkg->version, pkg->release);
	}
	low_repo_rpmdb_shutdown (rpmdb);

	repos = low_repo_set_initialize_from_config (config);
	low_repo_set_for_each (repos, ENABLED, (LowRepoSetFunc) search_provides,
						   provides);

	low_repo_set_free (repos);
	low_config_free (config);
	g_free (provides);

	return 0;
}

static int
command_version (int argc, const char *argv[])
{
	printf  ("low 0.0.1\n");
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

typedef struct _SubCommand {
	char *name;
	char *summary;
	int (*func) (int argc, const char *argv[]);
} SubCommand;

const SubCommand commands[] = {
	{ "clean", "Remove cached data", NOT_IMPLEMENTED },
	{ "info", "Display package details", command_info },
	{ "list", "Display a group of packages", command_list },
	{ "repolist", "Display configured software repositories",
		command_repolist },
	{ "whatprovides", "Find what package provides the given value",
		command_whatprovides },
	{ "version", "Display version information", command_version },
	{ "help", "Display a helpful usage message", command_help }
};

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
