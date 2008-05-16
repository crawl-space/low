#include <stdlib.h>
#include <fcntl.h>
#include <rpm/rpmlib.h>
#include <rpm/rpmdb.h>
#include "low-repo-rpmdb.h"
#include "low-package-rpmdb.h"

typedef struct _LowRepoRpmdb {
	LowRepo super;
	rpmdb db;
} LowRepoRpmdb;

LowRepo *
low_repo_rpmdb_initialize ()
{
	LowRepoRpmdb *repo = malloc (sizeof (LowRepoRpmdb));

	repo->super.id = "rpmdb";
	repo->super.name = "installed";
	repo->super.enabled = TRUE;

	rpmReadConfigFiles (NULL, NULL);
	if (rpmdbOpen ("", &repo->db, O_RDONLY, 0644) != 0) {
		fprintf (stderr, "Cannot open rpm database\n");
		exit (1);
	}

	return (LowRepo *) repo;
}

void
low_repo_rpmdb_shutdown (LowRepo *repo)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	rpmdbClose(repo_rpmdb->db);
	free (repo);
}

LowPackageIter *
low_repo_rpmdb_list_all (LowRepo *repo)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.pkg = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, 0, NULL, 0);
	return (LowPackageIter *) iter;
}

LowPackageIter *
low_repo_rpmdb_list_by_name (LowRepo *repo, const char *name)
{
	LowRepoRpmdb *repo_rpmdb = (LowRepoRpmdb *) repo;
	LowPackageIterRpmdb *iter = malloc (sizeof (LowPackageIterRpmdb));
	iter->super.pkg = NULL;

	iter->rpm_iter = rpmdbInitIterator (repo_rpmdb->db, RPMTAG_NAME, name, 0);
	return (LowPackageIter *) iter;
}
