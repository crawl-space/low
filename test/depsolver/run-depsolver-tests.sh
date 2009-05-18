#!/bin/bash
#
#  Low: a yum-like package manager
#
#  Copyright (C) 2008, 2009 James Bowes <jbowes@repl.ca>
#
#  This program is free software; you can redistribute it and/or modify
#  it under the terms of the GNU General Public License as published by
#  the Free Software Foundation; either version 2 of the License, or
#  (at your option) any later version.
#
#  This program is distributed in the hope that it will be useful,
#  but WITHOUT ANY WARRANTY; without even the implied warranty of
#  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
#  GNU General Public License for more details.
#
#  You should have received a copy of the GNU General Public License
#  along with this program; if not, write to the Free Software
#  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA
#  02110-1301  USA
#
#  Collect results from the YAML driven depsolver test cases


PASSED=1

TOTAL=0
NUM_PASSED=0

function run_test {
    let TOTAL=$TOTAL+1

    printf "Testing '$1'... "
    `./test_depsolver yaml/$1 > /dev/null`
    if (($?)); then
        printf "\E[31mFAIL\n"
        PASSED=0
    else
        printf "\E[32mOK\n"
        let NUM_PASSED=$NUM_PASSED+1
    fi
    tput sgr0
}

for test_file in $( ls yaml ); do
    run_test $test_file
done

echo "$TOTAL tests run, $[ $TOTAL - $NUM_PASSED ] failures"

if (($PASSED)); then
    echo "Depsolver tests passed"
    exit 0
else
    echo "*** DEPSOLVER TESTS FAILED ***"
    exit 1
fi
