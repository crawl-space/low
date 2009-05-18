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

#include <string.h>

#include "config.h"
#include <check.h>

#include "low-package.h"
#include "low-repo-set.h"
#include "low-util.h"

#include "low-repo-sqlite-fake.h"

/*
 * Core test suite
 */
START_TEST (test_core)
{
	fail_unless (1 == 1, "core test suite");
} END_TEST

START_TEST (test_low_package_dependency_new)
{
	LowPackageDependency *dep =
		low_package_dependency_new ("foo", DEPENDENCY_SENSE_EQ,
					    "0.1.3-3");

	fail_unless (!strcmp ("foo", dep->name), "dep name incorrect");
	fail_unless (DEPENDENCY_SENSE_EQ == dep->sense, "dep sense incorrect");
	fail_unless (!strcmp ("0.1.3-3", dep->evr), "dep evr incorrect");

	low_package_dependency_free (dep);
} END_TEST

START_TEST (test_low_package_dependency_new_from_string)
{
	LowPackageDependency *dep =
		low_package_dependency_new_from_string ("foo = 0.1.3-3");

	fail_unless (!strcmp ("foo", dep->name), "dep name incorrect");
	fail_unless (DEPENDENCY_SENSE_EQ == dep->sense, "dep sense incorrect");
	fail_unless (!strcmp ("0.1.3-3", dep->evr), "dep evr incorrect");

	low_package_dependency_free (dep);
} END_TEST

START_TEST (test_low_package_dependency_new_from_string_unversioned)
{
	LowPackageDependency *dep =
		low_package_dependency_new_from_string ("foo");

	fail_unless (!strcmp ("foo", dep->name), "dep name incorrect");
	fail_unless (DEPENDENCY_SENSE_NONE == dep->sense,
		     "dep sense incorrect");
	fail_unless (dep->evr == NULL, "dep evr incorrect");

	low_package_dependency_free (dep);
} END_TEST

START_TEST (test_low_package_dependency_satisfies_unversioned)
{
	LowPackageDependency *dep =
		low_package_dependency_new_from_string ("foo");
	gboolean res = low_package_dependency_satisfies (dep, dep);
	fail_unless (res, "Unversioned dep does not satisfy self");

	low_package_dependency_free (dep);
} END_TEST

START_TEST (test_low_package_dependency_satisfies_unversioned_not_satisfied)
{
	LowPackageDependency *dep1 =
		low_package_dependency_new_from_string ("foo");
	LowPackageDependency *dep2 =
		low_package_dependency_new_from_string ("bar");

	gboolean res = low_package_dependency_satisfies (dep1, dep2);
	fail_unless (!res, "Unversioned dep satisfied");

	low_package_dependency_free (dep1);
	low_package_dependency_free (dep2);
} END_TEST

START_TEST (test_low_package_dependency_satisfies_needs_versioned_satisfies_not)
{
	LowPackageDependency *dep1 =
		low_package_dependency_new_from_string ("foo = 1.2.3");
	LowPackageDependency *dep2 =
		low_package_dependency_new_from_string ("foo");

	gboolean res = low_package_dependency_satisfies (dep1, dep2);
	fail_unless (res, "Versioned dep not satisfied by unversioned");

	low_package_dependency_free (dep1);
	low_package_dependency_free (dep2);
} END_TEST

START_TEST (test_low_package_dependency_satisfies_satisfies_versioned_needs_not)
{
	LowPackageDependency *dep1 =
		low_package_dependency_new_from_string ("foo");
	LowPackageDependency *dep2 =
		low_package_dependency_new_from_string ("foo = 0.0.4");

	gboolean res = low_package_dependency_satisfies (dep1, dep2);
	fail_unless (res, "Unversioned dep not satisfied by versioned");

	low_package_dependency_free (dep1);
	low_package_dependency_free (dep2);
} END_TEST

START_TEST (test_low_package_dependency_satisfies_both_versioned_satisfied)
{
	const char *deps1[4] = { "foo > 0.1", "foo < 0.4", "foo < 1", NULL };
	const char *deps2[4] =
		{ "foo = 3.0.0", "foo = 0.2", "foo > 0.5", NULL };

	int i;
	LowPackageDependency *dep1;
	LowPackageDependency *dep2;

	for (i = 0; deps1[i] != NULL; i++) {
		gboolean res;

		dep1 = low_package_dependency_new_from_string (deps1[i]);
		dep2 = low_package_dependency_new_from_string (deps2[i]);

		res = low_package_dependency_satisfies (dep1, dep2);
		fail_unless (res, "Versioned dep not satisfied: %s - %s",
			     deps1[i], deps2[i]);

		low_package_dependency_free (dep1);
		low_package_dependency_free (dep2);
	}

} END_TEST

START_TEST (test_low_package_dependency_satisfies_both_versioned_not_satisfied)
{
	const char *deps1[4] = { "foo = 0.1", "foo < 0.4", "foo < 1", NULL };
	const char *deps2[4] = { "foo = 3.0.0", "foo = 0.6", "foo > 2", NULL };

	int i;
	LowPackageDependency *dep1;
	LowPackageDependency *dep2;

	for (i = 0; deps1[i] != NULL; i++) {
		gboolean res;

		dep1 = low_package_dependency_new_from_string (deps1[i]);
		dep2 = low_package_dependency_new_from_string (deps2[i]);

		res = low_package_dependency_satisfies (dep1, dep2);
		fail_unless (!res, "Versioned dep satisfied: %s - %s",
			     deps1[i], deps2[i]);

		low_package_dependency_free (dep1);
		low_package_dependency_free (dep2);
	}
} END_TEST

START_TEST (test_low_util_word_wrap_no_wrap_needed)
{
	const char *input = "A small string";
	char **output = low_util_word_wrap (input, 79);
	fail_unless (!strcmp (input, output[0]), "unexpected wrapping");
} END_TEST

START_TEST (test_low_util_word_wrap_wrap_one_line_to_two)
{
	const char *input = "A small string";
	char **output = low_util_word_wrap (input, 7);
	fail_unless (!strcmp ("A small", output[0]), "unexpected wrapping");
	fail_unless (!strcmp ("string", output[1]), "unexpected wrapping");
} END_TEST

START_TEST (test_low_util_parse_nevra_empty_string)
{
	const char *input = "";
	gboolean result = low_util_parse_nevra (input, NULL, NULL, NULL, NULL,
						NULL);
	fail_unless (!result, "parse_nevra did not error on an empty string");
} END_TEST

START_TEST (test_low_util_parse_nevra_just_name)
{
	const char *input = "zsh";
	char *name;
	gboolean result = low_util_parse_nevra (input, &name, NULL, NULL, NULL,
						NULL);
	fail_unless (result, "parse_nevra did not errored");
	fail_unless (!strcmp ("zsh", name),
		     "parse_nevra did not parse the name");
} END_TEST

START_TEST (test_low_repo_set_search_no_repos)
{
	int i = 0;
	LowPackageIter *iter;

	/* Should this be part of low_repo_set's interface? */
	LowRepoSet *repo_set = malloc (sizeof (LowRepoSet));
	repo_set->repos = g_hash_table_new (NULL, NULL);

	iter = low_repo_set_list_all (repo_set);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		i++;
	}

	fail_unless (i == 0, "results found for an empty repo");
	low_repo_set_free (repo_set);
} END_TEST

START_TEST (test_low_repo_set_search_single_repo_no_packages)
{
	int i = 0;
	LowPackage **packages = malloc (sizeof (LowPackage *));
	LowRepoSqliteFake *repo;
	LowPackageIter *iter;

	/* Should this be part of low_repo_set's interface? */
	LowRepoSet *repo_set = malloc (sizeof (LowRepoSet));
	repo_set->repos = g_hash_table_new (NULL, NULL);

	repo = (LowRepoSqliteFake *) low_repo_sqlite_initialize ("test",
								 "test repo",
								 "test url",
								 "mirror",
								 TRUE, TRUE);
	packages[0] = NULL;
	repo->packages = packages;
	g_hash_table_insert (repo_set->repos, repo->super.id, repo);

	iter = low_repo_set_list_all (repo_set);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		i++;
	}

	fail_unless (i == 0, "results found for an empty repo");
	low_repo_set_free (repo_set);
} END_TEST

START_TEST (test_low_repo_set_search_two_repos_no_packages)
{
	int i = 0;
	LowPackage **packages = malloc (sizeof (LowPackage *));
	LowRepoSqliteFake *repo;
	LowPackageIter *iter;

	/* Should this be part of low_repo_set's interface? */
	LowRepoSet *repo_set = malloc (sizeof (LowRepoSet));
	repo_set->repos = g_hash_table_new (NULL, NULL);

	packages[0] = NULL;

	repo = (LowRepoSqliteFake *) low_repo_sqlite_initialize ("test1",
								 "test repo",
								 "test url",
								 "mirror",
								 TRUE, TRUE);
	repo->packages = packages;
	g_hash_table_insert (repo_set->repos, repo->super.id, repo);

	repo = (LowRepoSqliteFake *) low_repo_sqlite_initialize ("test2",
								 "test repo",
								 "test url",
								 "mirror",
								 TRUE, TRUE);
	repo->packages = packages;
	g_hash_table_insert (repo_set->repos, repo->super.id, repo);

	iter = low_repo_set_list_all (repo_set);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		i++;
	}

	fail_unless (i == 0, "results found for empty repos");
	low_repo_set_free (repo_set);
} END_TEST

START_TEST (test_low_repo_set_search_single_repo_one_package)
{
	int i = 0;
	LowPackage **packages = malloc (sizeof (LowPackage *) * 2);
	LowPackage package;
	LowRepoSqliteFake *repo;
	LowPackageIter *iter;

	/* Should this be part of low_repo_set's interface? */
	LowRepoSet *repo_set = malloc (sizeof (LowRepoSet));
	repo_set->repos = g_hash_table_new (NULL, NULL);

	repo = (LowRepoSqliteFake *) low_repo_sqlite_initialize ("test",
								 "test repo",
								 "test url",
								 "mirror",
								 TRUE, TRUE);
	packages[0] = &package;
	packages[1] = NULL;
	repo->packages = packages;
	g_hash_table_insert (repo_set->repos, repo->super.id, repo);

	iter = low_repo_set_list_all (repo_set);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		i++;
	}

	fail_unless (i == 1, "wrong number of results found");
	low_repo_set_free (repo_set);
} END_TEST

START_TEST (test_low_repo_set_search_two_repos_two_packages)
{
	int i = 0;
	LowPackage **packages = malloc (sizeof (LowPackage *) * 2);
	LowPackage package;
	LowRepoSqliteFake *repo;
	LowPackageIter *iter;

	/* Should this be part of low_repo_set's interface? */
	LowRepoSet *repo_set = malloc (sizeof (LowRepoSet));
	repo_set->repos = g_hash_table_new (NULL, NULL);

	packages[0] = &package;
	packages[1] = NULL;

	repo = (LowRepoSqliteFake *) low_repo_sqlite_initialize ("test1",
								 "test repo",
								 "test url",
								 "mirror",
								 TRUE, TRUE);
	repo->packages = packages;
	g_hash_table_insert (repo_set->repos, repo->super.id, repo);

	repo = (LowRepoSqliteFake *) low_repo_sqlite_initialize ("test2",
								 "test repo",
								 "test url",
								 "mirror",
								 TRUE, TRUE);
	repo->packages = packages;
	g_hash_table_insert (repo_set->repos, repo->super.id, repo);

	iter = low_repo_set_list_all (repo_set);
	while (iter = low_package_iter_next (iter), iter != NULL) {
		i++;
	}

	fail_unless (i == 2, "wrong number of packages found");
	low_repo_set_free (repo_set);
} END_TEST

static Suite *
low_suite (void)
{
	Suite *s = suite_create ("low");

	TCase *tc = tcase_create ("core");
	tcase_add_test (tc, test_core);
	suite_add_tcase (s, tc);

	tc = tcase_create ("low-package");
	tcase_add_test (tc, test_low_package_dependency_new);
	tcase_add_test (tc, test_low_package_dependency_new_from_string);
	tcase_add_test (tc,
			test_low_package_dependency_new_from_string_unversioned);
	tcase_add_test (tc, test_low_package_dependency_satisfies_unversioned);
	tcase_add_test (tc,
			test_low_package_dependency_satisfies_unversioned_not_satisfied);
	tcase_add_test (tc,
			test_low_package_dependency_satisfies_needs_versioned_satisfies_not);
	tcase_add_test (tc,
			test_low_package_dependency_satisfies_satisfies_versioned_needs_not);
	tcase_add_test (tc,
			test_low_package_dependency_satisfies_both_versioned_satisfied);
	tcase_add_test (tc,
			test_low_package_dependency_satisfies_both_versioned_not_satisfied);
	suite_add_tcase (s, tc);

	tc = tcase_create ("low-util");
	tcase_add_test (tc, test_low_util_word_wrap_no_wrap_needed);
	tcase_add_test (tc, test_low_util_word_wrap_wrap_one_line_to_two);
	tcase_add_test (tc, test_low_util_parse_nevra_empty_string);
	tcase_add_test (tc, test_low_util_parse_nevra_just_name);
	suite_add_tcase (s, tc);

	tc = tcase_create ("low-repo-set");
	tcase_add_test (tc, test_low_repo_set_search_no_repos);
	tcase_add_test (tc, test_low_repo_set_search_single_repo_no_packages);
	tcase_add_test (tc, test_low_repo_set_search_two_repos_no_packages);
	tcase_add_test (tc, test_low_repo_set_search_single_repo_one_package);
	tcase_add_test (tc, test_low_repo_set_search_two_repos_two_packages);
	suite_add_tcase (s, tc);

	return s;
}

int
main (void)
{
	int nf;
	Suite *s = low_suite ();
	SRunner *sr = srunner_create (s);
	srunner_set_log (sr, "check_low.log");
	srunner_run_all (sr, CK_NORMAL);
	nf = srunner_ntests_failed (sr);
	srunner_free (sr);
	return (nf == 0) ? 0 : 1;
}

/* vim: set ts=8 sw=8 noet: */
