#!/bin/bash
#
#  Low: a yum-like package manager
#
#  Copyright (C) 2008 James Bowes <jbowes@dangerouslyinc.com>
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

LOW=../src/low

function run_or_die {
    echo "Running $LOW $1 > /dev/null"
    $LOW $1 > /dev/null
    if (($?)); then
        echo "*** SMOKE TESTS FAILED ***"
        exit 1
    fi
}

run_or_die "version"
run_or_die "help"

run_or_die "repolist"
run_or_die "list installed"
run_or_die "list all"

run_or_die "list search zsh"

run_or_die "whatprovides zsh"
run_or_die "whatrequires git"
run_or_die "whatconflicts bash"
run_or_die "whatobsoletes git-core"

echo "Smoke tests passed"
