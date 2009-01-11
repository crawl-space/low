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
#include <fcntl.h>
#include <errno.h>
#include <time.h>

#include <bzlib.h>
#include <glib.h>
#include <rpm/rpmlog.h>
#include <rpm/rpmcli.h>
#include <rpm/rpmdb.h>

#include "config.h"

#include "low-debug.h"
#include "low-config.h"
#include "low-package.h"
#include "low-repo-rpmdb.h"
#include "low-repo-set.h"
#include "low-repo-sqlite.h"
#include "low-repomd-parser.h"
#include "low-transaction.h"
#include "low-util.h"
#include "low-download.h"

#define ARRAY_SIZE(a) (sizeof(a) / sizeof((a)[0]))

#define LOCAL_CACHE "/var/cache/yum"

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
print_dependency (const LowPackageDependency * dep)
{
	fputs (dep->name, stdout);
	if (dep->sense != DEPENDENCY_SENSE_NONE) {
		switch (dep->sense) {
			case DEPENDENCY_SENSE_EQ:
				fputs (" = ", stdout);
				break;
			case DEPENDENCY_SENSE_LT:
				fputs (" < ", stdout);
				break;
			case DEPENDENCY_SENSE_LE:
				fputs (" <= ", stdout);
				break;
			case DEPENDENCY_SENSE_GT:
				fputs (" > ", stdout);
				break;
			case DEPENDENCY_SENSE_GE:
				fputs (" >= ", stdout);
				break;
			case DEPENDENCY_SENSE_NONE:
			default:
				break;
		}

		fputs (dep->evr, stdout);
	}
	putchar ('\n');
}

static void
print_dependencies (const char *dep_name, LowPackageDependency **deps)
{
	int i;

	printf ("%-12s:", dep_name);

	if (deps[0] == NULL) {
		putchar ('\n');
		return;
	}

	putchar (' ');
	print_dependency (deps[0]);
	for (i = 1; deps[i] != NULL; i++) {
		fputs ("              ", stdout);
		print_dependency (deps[i]);
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
	LowPackageDetails *details = low_package_get_details (pkg);

	printf ("Name        : %s\n", pkg->name);
	printf ("Arch        : %s\n", pkg->arch);
	printf ("Version     : %s\n", pkg->version);
	printf ("Release     : %s\n", pkg->release);

	printf ("Size        : ");
	print_size (pkg->size);

	printf ("Repo        : %s\n", pkg->repo->id);

	printf ("Summary     : ");
	wrap_and_print (details->summary);

	printf ("URL         : %s\n", details->url ? details->url : "");
	printf ("License     : %s\n", details->license);

	printf ("Description : ");
	wrap_and_print (details->description);

	low_package_details_free (details);

	if (show_all) {
		LowPackageDependency **deps;
		char **files;

		deps = low_package_get_provides (pkg);
		print_dependencies ("Provides", deps);

		deps = low_package_get_requires (pkg);
		print_dependencies ("Requires", deps);

		deps = low_package_get_conflicts (pkg);
		print_dependencies ("Conflicts", deps);
		low_package_dependency_list_free (deps);

		deps = low_package_get_obsoletes (pkg);
		print_dependencies ("Obsoletes", deps);
		low_package_dependency_list_free (deps);

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
	LowRepo *repo_rpmdb;
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

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	iter = low_repo_rpmdb_list_by_name (repo_rpmdb, name);
	print_all_packages (iter, show_all);

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_list_by_name (repos, name);
	print_all_packages (iter, show_all);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
	free (name);

	return EXIT_SUCCESS;
}

static void
print_package_short (LowPackage *pkg)
{
	gchar *name_arch = g_strdup_printf ("%s.%s", pkg->name, pkg->arch);
	gchar *version_release = g_strdup_printf ("%s-%s", pkg->version,
						  pkg->release);

	printf ("%-41.41s %-23.23s %s\n", name_arch, version_release,
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

static void
print_transaction_part (GHashTable *hash)
{
	GList *list = g_hash_table_get_values (hash);
	while (list != NULL) {
		LowTransactionMember *member = list->data;
		print_package_short (member->pkg);
		list = list->next;
	}
}

static int
print_updates (void)
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	LowTransaction *trans;

	repo_rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (repo_rpmdb);
	repos = low_repo_set_initialize_from_config (config, TRUE);

	trans = low_transaction_new (repo_rpmdb, repos);

	iter = low_repo_rpmdb_list_all (repo_rpmdb);

	while (iter = low_package_iter_next (iter), iter != NULL) {
		low_transaction_add_update (trans, iter->pkg);
	}

	print_transaction_part (trans->update);

	low_transaction_free (trans);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

	return EXIT_SUCCESS;
}

static int
command_list (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowPackageIter *iter;
	LowConfig *config;

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	if (argc == 1 && strcmp (argv[0], "updates") == 0) {
		return print_updates ();
	}

	if (argc == 0 || !strcmp(argv[0], "installed") ||
	    !strcmp(argv[0], "all")) {
		iter = low_repo_rpmdb_list_all (repo_rpmdb);
		print_all_packages_short (iter);
	}
	if (argc == 0 || !strcmp(argv[0], "all") ||
	    !strcmp (argv[0], "available")) {
		LowRepoSet *repos =
			low_repo_set_initialize_from_config (config, TRUE);

		iter = low_repo_set_list_all (repos);
		print_all_packages_short (iter);

		low_repo_set_free (repos);
	} else {
		LowRepoSet *repos;

		iter = low_repo_rpmdb_list_by_name (repo_rpmdb, argv[0]);
		print_all_packages_short (iter);

		repos = low_repo_set_initialize_from_config (config, TRUE);

		iter = low_repo_set_list_by_name (repos, argv[0]);
		print_all_packages_short (iter);

		low_repo_set_free (repos);
	}


	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

	return EXIT_SUCCESS;
}

static int
command_search (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *querystr = g_strdup (argv[0]);

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	iter = low_repo_rpmdb_search_details (repo_rpmdb, querystr);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_search_details (repos, querystr);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
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
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowRepoSetFilter filter = ALL;

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

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

	print_repo (repo_rpmdb);

	repos = low_repo_set_initialize_from_config (config, TRUE);
	low_repo_set_for_each (repos, filter, (LowRepoSetFunc) print_repo,
			       NULL);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

	return EXIT_SUCCESS;
}

static int
command_whatprovides (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	LowPackageDependency *provides =
		low_package_dependency_new_from_string (argv[0]);

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	iter = low_repo_rpmdb_search_provides (repo_rpmdb, provides);
	print_all_packages_short (iter);

	if (provides->name[0] == '/') {
		iter = low_repo_rpmdb_search_files (repo_rpmdb, provides->name);
		print_all_packages_short (iter);
	}

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_search_provides (repos, provides);
	print_all_packages_short (iter);

	if (provides->name[0] == '/') {
		iter = low_repo_set_search_files (repos, provides->name);
		print_all_packages_short (iter);
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
	low_package_dependency_free (provides);

	return EXIT_SUCCESS;
}

static int
command_whatrequires (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	LowPackageDependency *requires =
		low_package_dependency_new_from_string (argv[0]);

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	iter = low_repo_rpmdb_search_requires (repo_rpmdb, requires);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_search_requires (repos, requires);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
	low_package_dependency_free (requires);

	return EXIT_SUCCESS;
}

static int
command_whatconflicts (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	LowPackageDependency *conflicts =
		low_package_dependency_new_from_string (argv[0]);

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	iter = low_repo_rpmdb_search_conflicts (repo_rpmdb, conflicts);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_search_conflicts (repos, conflicts);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
	low_package_dependency_free (conflicts);

	return EXIT_SUCCESS;
}

static int
command_whatobsoletes (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowPackageIter *iter;
	LowPackageDependency *obsoletes =
		low_package_dependency_new_from_string (argv[0]);

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	iter = low_repo_rpmdb_search_obsoletes (repo_rpmdb, obsoletes);
	print_all_packages_short (iter);

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_search_obsoletes (repos, obsoletes);
	print_all_packages_short (iter);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
	low_package_dependency_free (obsoletes);

	return EXIT_SUCCESS;
}

static const char *
get_file_basename (const char *location_href)
{
	const char *filename = rindex (location_href, '/');
	if (filename == NULL) {
		filename = location_href;
	} else {
		filename++;
	}

	return filename;
}

static char *
create_package_filepath (LowPackage *pkg)
{
	const char *filename = get_file_basename (pkg->location_href);
	char *local_file = g_strdup_printf ("%s/%s/packages/%s",
					    LOCAL_CACHE, pkg->repo->id,
					    filename);

	return local_file;
}

/* Generate a random integer between 0 and upper bound. (inclusive) */
static int
random_int(int upper)
{
	/* Not the most random thing in the world but do we care? */
	unsigned int iseed = (unsigned int)time (NULL);
	srand (iseed);
	return rand () % (upper + 1);
}

static void
free_g_list_node(gpointer data_ptr, gpointer ignored G_GNUC_UNUSED)
{
	g_string_free ((GString *) data_ptr, TRUE);
}

static char *
lookup_random_mirror (char *repo_id)
{
	char *mirrors_file =
		g_strdup_printf ("/var/cache/yum/%s/mirrorlist.txt",
				 repo_id);

	/* List of all mirrors as GString's. */
	GList *all_mirrors = NULL;
	gchar x;
	GString *url;
	int choice;
	GString *random_url;
	gchar *return_val;

	FILE *file = fopen(mirrors_file, "r");
	if (file == 0)
	{
		printf("Error opening file: %s\n", mirrors_file);
		return NULL;
	}

	url = g_string_new("");
	while ((x = fgetc (file)) != EOF)
	{
		if (x == '\n')
		{
			/* g_print ("Final string: %s\n", url->str); */
			/* Ignore lines commented out: */
			if (url->str[0] != '#')
			{
				all_mirrors = g_list_append (all_mirrors,
							     (gpointer) url);
			}
			url = g_string_new("");
			continue;
		}
		url = g_string_append_c (url, x);
	}
	fclose (file);
	choice = random_int (g_list_length (all_mirrors) - 1);
	random_url = (GString *) g_list_nth_data(all_mirrors, choice);
	/* Copy so we can free the entire list: */
	return_val = g_strdup (random_url->str);

	g_list_foreach(all_mirrors, free_g_list_node, NULL);
	g_list_free (all_mirrors);
	free (mirrors_file);

	return return_val;
}

static char *
select_mirror_url (LowRepo *repo, GHashTable *repo_mirrors)
{
	char *baseurl = repo->baseurl;
	/* baseurl will be NULL if repo is configured to use a mirror list. */
	if (!baseurl)
	{
		if (repo_mirrors)
		{
			baseurl = (char *) g_hash_table_lookup (repo_mirrors,
								repo->id);
		}

		/* Have we already looked up a mirror for this repo? */
		if (!baseurl)
		{
			baseurl = lookup_random_mirror (repo->id);
			low_debug ("Using %s mirror: %s\n", repo->id,
				   baseurl);
			if (repo_mirrors)
			{
				g_hash_table_replace(repo_mirrors,
						     repo->id,
						     baseurl);
			}
		}
	}

	return baseurl;
}

static char *
create_file_url (const char *baseurl, const char *relative_file)
{
	char *full_url;

	if (baseurl[strlen (baseurl) - 1] != '/')
	{
		full_url = g_strdup_printf ("%s/%s", baseurl, relative_file);
	} else {
		full_url = g_strdup_printf ("%s%s", baseurl, relative_file);
	}

	return full_url;
}

/* Repo mirrors hash is optional. */
static void
download_package (LowPackage *pkg, GHashTable *repo_mirrors)
{
	char *baseurl = select_mirror_url (pkg->repo, repo_mirrors);

	char *full_url = create_file_url (baseurl, pkg->location_href);
	char *local_file = create_package_filepath (pkg);
	const char *filename = get_file_basename (pkg->location_href);

	low_download_if_missing (full_url, local_file, filename);
	free (full_url);
	free (local_file);
}

static int
command_download (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	gchar *name = g_strdup (argv[0]);
	int found_pkg;

	repo_rpmdb = low_repo_rpmdb_initialize ();
	config = low_config_initialize (repo_rpmdb);

	repos = low_repo_set_initialize_from_config (config, TRUE);

	iter = low_repo_set_list_by_name (repos, name);
	found_pkg = 0;
	while (iter = low_package_iter_next (iter), iter != NULL) {
		LowPackage *pkg = iter->pkg;
		found_pkg = 1;
		download_package (pkg, NULL);
		low_package_unref (pkg);
	}

	if (!found_pkg) {
		printf ("No such package: %s", name);
		return EXIT_FAILURE;
	}

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);
	free (name);

	return EXIT_SUCCESS;
}

static void
print_transaction (LowTransaction *trans)
{
	printf ("Update:\n");
	print_transaction_part (trans->update);

	printf ("\nInstall:\n");
	print_transaction_part (trans->install);

	printf ("\nRemove:\n");
	print_transaction_part (trans->remove);
}

static gboolean
prompt_confirmed (void)
{
	char input;

	printf("\nRun transaction? [y/N] ");

	input = getchar();

	if (input == 'y' || input == 'Y') {
		return TRUE;
	}

	return FALSE;
}

static void
download_required_packages (LowTransaction *trans)
{
	/* Maintain a running list of mirror URLs to use for each
	 * repository encountered. */
	GHashTable *repo_mirrors;
	GList *list;

	repo_mirrors = g_hash_table_new(g_str_hash, g_str_equal);

	list = g_hash_table_get_values (trans->install);
	while (list != NULL) {
		LowTransactionMember *member = list->data;
		download_package (member->pkg, repo_mirrors);
		list = list->next;
	}

	list = g_hash_table_get_values (trans->update);
	while (list != NULL) {
		LowTransactionMember *member = list->data;
		download_package (member->pkg, repo_mirrors);
		list = list->next;
	}
}

static void
add_installs_to_transaction (GHashTable *hash, rpmts ts)
{
	GList *list = g_hash_table_get_values (hash);
	while (list != NULL) {
		LowTransactionMember *member = list->data;

		/* XXX RPM needs this around during the transaction */
		char *filepath = create_package_filepath (member->pkg);
		FD_t fd = Fopen (filepath, "r.ufdio");
		Header hdr;
		int res = rpmReadPackageFile (ts, fd, NULL, &hdr);
		Fclose(fd);
		if (res != RPMRC_OK) {
			/* XXX do something better here */
			printf ("Unable to read %s, skipping\n", filepath);
		} else {
			rpmtsAddInstallElement (ts, hdr, (fnpyKey) filepath,
						0, 0);
		}
		list = list->next;
	}
}

static void
add_removes_to_transaction (GHashTable *hash, rpmts ts)
{
	GList *list = g_hash_table_get_values (hash);
	while (list != NULL) {
		LowTransactionMember *member = list->data;
		LowPackage *pkg = member->pkg;

		rpmdbMatchIterator iter;
		Header hdr;
		unsigned int offset;

		iter = rpmdbInitIterator (low_repo_rpmdb_get_db (pkg->repo),
					  RPMTAG_PKGID,
					  pkg->id, 16);
		hdr = rpmdbNextIterator (iter);
		offset = rpmdbGetIteratorOffset (iter);

		rpmtsAddEraseElement (ts, hdr, offset);
		list = list->next;

		rpmdbFreeIterator (iter);
	}
}

typedef enum _CallbackState {
	CALLBACK_PREPARE,
	CALLBACK_INSTALL,
	CALLBACK_REMOVE
} CallbackState;

typedef struct _CallbackData {
	gboolean verbose;
	char *name;
	int total_rpms;
	int current_rpm;
	CallbackState state;
} CallbackData;

static void
printHash (int part, int total, CallbackData *data)
{
	if (data->state == CALLBACK_INSTALL) {
		printf ("\r(%d/%d) Installing ", data->current_rpm,
			data->total_rpms);
	} else if (data->state == CALLBACK_REMOVE) {
		printf ("\r(%d/%d) Removing ", data->current_rpm,
			data->total_rpms);
	} else {
		printf ("\r");
	}
	printf ("%s %d%%", data->name, part * 100 / total);
	if (part == total)
		printf ("\n");

	(void) fflush(stdout);
}

static void *
low_show_rpm_progress (const void * arg, const rpmCallbackType what,
		       const rpm_loff_t amount, const rpm_loff_t total,
		       fnpyKey key, void * data)
{
	Header h = (Header) arg;
	void * rc = NULL;
	const char * filename = (const char *)key;
	static FD_t fd = NULL;
	int xx;

	CallbackData *callback = (CallbackData *) data;
	gboolean verbose = callback->verbose;

	switch (what) {
	case RPMCALLBACK_INST_OPEN_FILE:
		if (filename == NULL || filename[0] == '\0')
			return NULL;
		fd = Fopen(filename, "r.ufdio");
		/* FIX: still necessary? */
		if (fd == NULL || Ferror(fd)) {
			rpmlog(RPMLOG_ERR, "open of %s failed: %s\n", filename,
			       Fstrerror(fd));
			if (fd != NULL) {
				xx = Fclose(fd);
				fd = NULL;
			}
		} else
			fd = fdLink(fd, "persist (showProgress)");
		return (void *)fd;
		break;

	case RPMCALLBACK_INST_CLOSE_FILE:
		/* FIX: still necessary? */
		fd = fdFree(fd, "persist (showProgress)");
		if (fd != NULL) {
			xx = Fclose(fd);
			fd = NULL;
		}
		break;

	case RPMCALLBACK_INST_START:
	case RPMCALLBACK_UNINST_START:
		rpmcliHashesCurrent = 0;
		if (h == NULL)
			break;
		/* @todo Remove headerFormat() on a progress callback. */
		if (verbose) {
			callback->current_rpm++;
			callback->state = (what == RPMCALLBACK_INST_START) ?
				CALLBACK_INSTALL : CALLBACK_REMOVE;
			callback->name = headerFormat(h, "%{NAME}-%{VERSION}-%{RELEASE}.%{ARCH}", NULL);
			printHash (0, 1, callback);
		}
		break;

	case RPMCALLBACK_TRANS_PROGRESS:
	case RPMCALLBACK_INST_PROGRESS:
	case RPMCALLBACK_UNINST_PROGRESS:
		if (verbose)
			printHash(amount, total, callback);

		if (amount == total) {
			free (callback->name);
			callback->name = NULL;
		}
		break;

	case RPMCALLBACK_TRANS_START:
		if (verbose) {
			callback->state = CALLBACK_PREPARE;
			callback->name = g_strdup ("Preparing...");
			callback->total_rpms = total;
			callback->current_rpm = 0;
		}
		break;

	case RPMCALLBACK_TRANS_STOP:
	case RPMCALLBACK_UNINST_STOP:
		if (verbose)
			printHash(1, 1, callback);
		rpmcliProgressTotal = rpmcliPackagesTotal;
		rpmcliProgressCurrent = 0;

		free (callback->name);
		callback->name = NULL;

		break;

	case RPMCALLBACK_UNPACK_ERROR:
	case RPMCALLBACK_CPIO_ERROR:
	case RPMCALLBACK_SCRIPT_ERROR:
	case RPMCALLBACK_UNKNOWN:
	case RPMCALLBACK_REPACKAGE_PROGRESS:
	case RPMCALLBACK_REPACKAGE_START:
	case RPMCALLBACK_REPACKAGE_STOP:
	default:
		break;
	}

	return rc;
}

static rpmts
low_transaction_to_rpmts (LowTransaction *trans, CallbackData *data)
{
	rpmts ts = rpmtsCreate();
	rpmtsSetRootDir(ts, "/");
	rpmtsSetNotifyCallback(ts, low_show_rpm_progress, data);

	add_installs_to_transaction (trans->install, ts);
	add_installs_to_transaction (trans->update, ts);

	add_removes_to_transaction (trans->remove, ts);
	add_removes_to_transaction (trans->updated, ts);

	return ts;
}

static void
run_transaction (LowTransaction *trans)
{
	if (!(g_hash_table_size (trans->install) ||
	      g_hash_table_size (trans->update) ||
	      g_hash_table_size (trans->remove) ||
	      g_hash_table_size (trans->updated))) {
		printf ("Nothing to do.\n");
		return;
	}

	print_transaction (trans);

	if (prompt_confirmed()) {
		rpmts ts;
		int rc;
		CallbackData data;
		data.name = NULL;
		data.verbose = TRUE;

		printf ("Running\n");
		download_required_packages(trans);

//		rpmSetVerbosity(RPMLOG_DEBUG);
		ts = low_transaction_to_rpmts (trans, &data);
		rpmtsSetFlags(ts, RPMTRANS_FLAG_NONE);

		rc = rpmtsRun(ts, NULL, RPMPROB_FILTER_NONE);
		if (rc != 0) {
			printf ("Error running transaction\n");
		}
	}
}

static LowPackage *
select_package_for_install (LowPackageIter *iter)
{
	LowPackage *best = NULL;
	char *best_evr = g_strdup ("0");

	while (iter = low_package_iter_next (iter), iter != NULL) {
		char *new_evr = low_package_evr_as_string (iter->pkg);

		if (rpmvercmp (new_evr, best_evr) > 0 &&
		    iter->pkg->arch[0] != 'i') {
			if (best) {
				low_package_unref (best);
			}

			g_free (best_evr);
			best = iter->pkg;
			best_evr = new_evr;
		} else {
			low_package_unref (iter->pkg);
			g_free (new_evr);
		}

	}

	g_free (best_evr);

	return best;
}

static int
command_install (int argc, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	LowTransaction *trans;
	int i;
	int res;

	repo_rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (repo_rpmdb);
	repos = low_repo_set_initialize_from_config (config, TRUE);

	trans = low_transaction_new (repo_rpmdb, repos);

	for (i = 0; i < argc; i++) {
		LowPackage *pkg;
		LowPackageDependency *provides =
			low_package_dependency_new_from_string (argv[i]);

		iter = low_repo_set_search_provides (repos, provides);
		pkg = select_package_for_install (iter);
		if (pkg) {
			low_transaction_add_install (trans, pkg);
		}

		low_package_dependency_free (provides);
	}

	if (low_transaction_resolve (trans) != LOW_TRANSACTION_OK) {
		printf ("Error resolving transaction\n");
		res = EXIT_FAILURE;
	} else {
		run_transaction (trans);
		res = EXIT_SUCCESS;
	}

	low_transaction_free (trans);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

	return res;
}

static int
command_update (int argc G_GNUC_UNUSED, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	LowTransaction *trans;
	int i;
	int res;

	repo_rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (repo_rpmdb);
	repos = low_repo_set_initialize_from_config (config, TRUE);

	trans = low_transaction_new (repo_rpmdb, repos);

	for (i = 0; i < argc; i++) {
		LowPackageDependency *provides =
			low_package_dependency_new_from_string (argv[i]);
		/* XXX just do the most EVR newest */
		iter = low_repo_rpmdb_search_provides (repo_rpmdb, provides);

		/* XXX get rid of this nastiness somehow */
		while (iter = low_package_iter_next (iter), iter != NULL) {
			low_transaction_add_update (trans, iter->pkg);
		}

		low_package_dependency_free (provides);
	}

	if (argc == 0) {
		iter = low_repo_rpmdb_list_all (repo_rpmdb);

		while (iter = low_package_iter_next (iter), iter != NULL) {
			low_transaction_add_update (trans, iter->pkg);
		}
	}

	if (low_transaction_resolve (trans) != LOW_TRANSACTION_OK) {
		printf ("Error resolving transaction\n");
		res = EXIT_FAILURE;
	} else {
		run_transaction (trans);
		res = EXIT_SUCCESS;
	}

	low_transaction_free (trans);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

	return res;
}

static int
command_remove (int argc, const char *argv[])
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowPackageIter *iter;
	LowConfig *config;
	LowTransaction *trans;
	int i;
	int res;

	repo_rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (repo_rpmdb);
	repos = low_repo_set_initialize_from_config (config, TRUE);

	trans = low_transaction_new (repo_rpmdb, repos);

	for (i = 0; i < argc; i++) {
		LowPackageDependency *provides =
			low_package_dependency_new_from_string (argv[i]);

		/* XXX just do the most EVR newest */
		iter = low_repo_rpmdb_search_provides (repo_rpmdb, provides);
		iter = low_package_iter_next (iter);

		if (iter == NULL) {
			printf ("No such package to remove\n");
			return EXIT_FAILURE;
		}

		low_transaction_add_remove (trans, iter->pkg);

		while (iter = low_package_iter_next (iter), iter != NULL) {
			low_package_unref (iter->pkg);
		}

		low_package_dependency_free (provides);
	}

	if (low_transaction_resolve (trans) != LOW_TRANSACTION_OK) {
		printf ("Error resolving transaction\n");
		res = EXIT_FAILURE;
	} else {
		run_transaction (trans);
		res = EXIT_SUCCESS;
	}

	low_transaction_free (trans);
	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

	return res;
}

static char *
download_repodata_file (LowRepo *repo, const char *relative_name)
{
	char *baseurl = select_mirror_url (repo, NULL);

	const char *basename = get_file_basename (relative_name);
	char *full_url = create_file_url (baseurl, relative_name);
	char *local_file = g_strdup_printf ("%s/%s/%s.tmp",
					    LOCAL_CACHE, repo->id,
					    basename);

	/* Just something nice to display */
	char *displayed_basename;
	if (strlen (basename) > 24) {
		int offset = strlen(basename) - 24;
		displayed_basename =
			g_strdup_printf ("%s - ...%s", repo->id,
					 basename + offset);
	} else {
		displayed_basename = g_strdup_printf ("%s - %s", repo->id,
						      basename);
	}

	low_download (full_url, local_file, displayed_basename);

	g_free (full_url);
	g_free (displayed_basename);

	return local_file;
}

#define BUF_SIZE 1024

/* XXX should do this while downloading */
static void
uncompress_file (const char *filename)
{
	int error;
	int size_read;
	FILE *file;
	BZFILE *compressed;
	int fd;
	char buf[BUF_SIZE];
	char *uncompressed_name = malloc (strlen (filename) - 3);
	strncpy (uncompressed_name, filename, strlen (filename) - 4);
	uncompressed_name[strlen (filename) - 4] = '\0';

	file = fopen (filename, "r");
	compressed = BZ2_bzReadOpen (&error, file, 0, 0, NULL, 0);

	fd = open (uncompressed_name, O_WRONLY | O_CREAT | O_TRUNC, 0644);

	printf ("Uncompressing...\n");
	while (size_read = BZ2_bzRead (&error, compressed, &buf, BUF_SIZE),
	       size_read != 0) {
		if (write (fd, buf, size_read) == -1) {
			/* XXX do something better here on failure */
			exit (EXIT_FAILURE);
		}
	}

	close (fd);
}

static char *
create_repodata_filename (LowRepo *repo, const char *relative_name)
{
	const char *basename = get_file_basename (relative_name);
	return g_strdup_printf ("%s/%s/%s", LOCAL_CACHE, repo->id, basename);

}

static void
refresh_repo (LowRepo *repo)
{
	char *local_file;
	char *tmp_file;
	LowRepomd *old_repomd;
	LowRepomd *new_repomd;

	if (repo->mirror_list) {
		local_file = create_repodata_filename (repo,
						       "mirrorlist.txt");
		low_download (repo->mirror_list, local_file,
			      "mirrorlist.txt");

		g_free (local_file);
	}

	local_file = create_repodata_filename (repo, "repodata/repomd.xml");
	old_repomd = low_repomd_parse (local_file);

	tmp_file = download_repodata_file (repo, "repodata/repomd.xml");
	new_repomd = low_repomd_parse (tmp_file);

	if (old_repomd != NULL &&
	    old_repomd->primary_db_time >= new_repomd->primary_db_time &&
	    old_repomd->filelists_db_time >= new_repomd->filelists_db_time) {
		g_free (local_file);
		g_free (tmp_file);

		return;
	}

	rename (tmp_file, local_file);
	g_free (local_file);
	g_free (tmp_file);

	tmp_file = download_repodata_file (repo, new_repomd->primary_db);
	local_file = create_repodata_filename (repo, new_repomd->primary_db);
	rename (tmp_file, local_file);
	uncompress_file (local_file);

	g_free (local_file);
	g_free (tmp_file);

	tmp_file = download_repodata_file (repo, new_repomd->filelists_db);
	local_file = create_repodata_filename (repo,
					       new_repomd->filelists_db);
	rename (tmp_file, local_file);
	uncompress_file (local_file);

	g_free (local_file);
	g_free (tmp_file);
}

static int
command_refresh (int argc G_GNUC_UNUSED, const char *argv[] G_GNUC_UNUSED)
{
	LowRepo *repo_rpmdb;
	LowRepoSet *repos;
	LowConfig *config;
	LowRepoSetFilter filter = ENABLED;

	repo_rpmdb = low_repo_rpmdb_initialize ();

	config = low_config_initialize (repo_rpmdb);
	repos = low_repo_set_initialize_from_config (config, FALSE);

	low_repo_set_for_each (repos, filter, (LowRepoSetFunc) refresh_repo,
			       NULL);

	low_repo_set_free (repos);
	low_config_free (config);
	low_repo_rpmdb_shutdown (repo_rpmdb);

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
	const char *name;
	const char *usage;
	const char *summary;
	int (*func) (int argc, const char *argv[]);
} SubCommand;

const SubCommand commands[] = {
	{ "refresh", NULL, "Download new metadata", command_refresh },
	{ "install", "PACKAGE", "Install a package", command_install },
	{ "update", "[PACKAGE]", "Update or install a package",
	  command_update },
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
