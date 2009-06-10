#!/usr/bin/python
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

import os
import sys

HEADER = \
"""/*
 *  Low: a yum-like package manager
 *
 *  Copyright (C) 2009 James Bowes <jbowes@repl.ca>
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
 */"""

FOOTER = "/* vim: set ts=8 sw=8 noet: */"

def print_dot_h(name):
    doth = open("src/low-%s.h" % name, 'w')
    define = "_LOW_%s_H_" % name.replace('-','_').upper()

    print >> doth, HEADER
    print >> doth, ""
    print >> doth, "#ifndef %s" % define
    print >> doth, "#define %s" % define
    print >> doth, ""
    print >> doth, ""
    print >> doth, ""
    print >> doth, "#endif /* %s */" % define
    print >> doth, ""
    print >> doth, FOOTER

    doth.close()

def print_dot_c(name):
    dotc = open("src/low-%s.c" % name, 'w')
    include = "low-%s.h" % name

    print >> dotc, HEADER
    print >> dotc, ""
    print >> dotc, '#include "%s"' % include
    print >> dotc, ""
    print >> dotc, ""
    print >> dotc, ""
    print >> dotc, FOOTER

    dotc.close()


def main():
    name = sys.argv[1]

    if os.path.exists("src/low-%s.c" % name) or \
        os.path.exists("src/low-%s.h" % name):
        print "Files already exist"
        return

    print_dot_h(name)
    print_dot_c(name)

if __name__ == "__main__":
    main()
