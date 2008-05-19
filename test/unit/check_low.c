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

#include "config.h"
#include <check.h>

/*
 * Core test suite
 */
START_TEST(test_core)
{
	fail_unless(1==1, "core test suite");
}
END_TEST

Suite *
low_suite(void)
{
	Suite *s = suite_create ("low");
	TCase *tc = tcase_create ("core");
	tcase_add_test(tc, test_core);
	suite_add_tcase(s, tc);
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
