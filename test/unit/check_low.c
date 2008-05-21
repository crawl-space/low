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

#include <string.h>

#include "config.h"
#include <check.h>

#include "low-package.h"
#include "low-util.h"

/*
 * Core test suite
 */
START_TEST (test_core)
{
	fail_unless (1 == 1, "core test suite");
}
END_TEST

START_TEST (test_low_package_dependency_new)
{
	LowPackageDependency *dep =
		low_package_dependency_new ("foo", DEPENDENCY_SENSE_EQ,
					    "0.1.3-3");

	fail_unless (!strcmp ("foo", dep->name), "dep name incorrect");
	fail_unless (DEPENDENCY_SENSE_EQ == dep->sense, "dep sense incorrect");
	fail_unless (!strcmp ("0.1.3-3", dep->evr), "dep evr incorrect");

	low_package_dependency_free (dep);
}
END_TEST

START_TEST (test_low_package_dependency_new_from_string)
{
	LowPackageDependency *dep =
		low_package_dependency_new_from_string ("foo = 0.1.3-3");

	fail_unless (!strcmp ("foo", dep->name), "dep name incorrect");
	fail_unless (DEPENDENCY_SENSE_EQ == dep->sense, "dep sense incorrect");
	fail_unless (!strcmp ("0.1.3-3", dep->evr), "dep evr incorrect");

	low_package_dependency_free (dep);
}
END_TEST

START_TEST (test_low_package_dependency_new_from_string_unversioned)
{
	LowPackageDependency *dep =
		low_package_dependency_new_from_string ("foo");

	fail_unless (!strcmp ("foo", dep->name), "dep name incorrect");
	fail_unless (DEPENDENCY_SENSE_NONE == dep->sense,
		     "dep sense incorrect");
	fail_unless (dep->evr == NULL, "dep evr incorrect");

	low_package_dependency_free (dep);
}
END_TEST

START_TEST (test_low_util_word_wrap_no_wrap_needed)
{
	const char *input = "A small string";
	char **output = low_util_word_wrap (input, 79);
	fail_unless (!strcmp (input, output[0]), "unexpected wrapping");
}
END_TEST

START_TEST (test_low_util_word_wrap_wrap_one_line_to_two)
{
	const char *input = "A small string";
	char **output = low_util_word_wrap (input, 7);
	fail_unless (!strcmp ("A small", output[0]), "unexpected wrapping");
	fail_unless (!strcmp ("string", output[1]), "unexpected wrapping");
}
END_TEST

Suite *
low_suite(void)
{
	Suite *s = suite_create ("low");

	TCase *tc = tcase_create ("core");
	tcase_add_test (tc, test_core);
	suite_add_tcase (s, tc);

	tc = tcase_create ("low-package");
	tcase_add_test (tc, test_low_package_dependency_new);
	tcase_add_test (tc, test_low_package_dependency_new_from_string);
	tcase_add_test (tc, test_low_package_dependency_new_from_string_unversioned);
	suite_add_tcase (s, tc);

	tc = tcase_create ("low-util");
	tcase_add_test (tc, test_low_util_word_wrap_no_wrap_needed);
	tcase_add_test (tc, test_low_util_word_wrap_wrap_one_line_to_two);
	suite_add_tcase (s, tc);

	return s;
}


int
main () {
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
