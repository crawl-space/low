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
#
#  bash completion support for Low.
#
#  To use the completion:
#    1. Copy this file somewhere (e.g. ~/.low-completion.bash).
#    2. Add the following line to your .bashrc:
#        source ~/.low-completion.sh


__low_commandlist="
    clean
    info
    list
    search
    repolist
    whatprovides
    whatrequires
    whatconflicts
    whatobsoletes
    version
    help
    "

__lowcomp ()
{
	local all c s=$'\n' IFS=' '$'\t'$'\n'
	local cur="${COMP_WORDS[COMP_CWORD]}"
	if [ $# -gt 2 ]; then
		cur="$3"
	fi
	for c in $1; do
		case "$c$4" in
		*.)    all="$all$c$4$s" ;;
		*)     all="$all$c$4 $s" ;;
		esac
	done
	IFS=$s
	COMPREPLY=($(compgen -P "$2" -W "$all" -- "$cur"))
	return
}

_low_list ()
{
	local i c=1 command
	while [ $c -lt $COMP_CWORD ]; do
		i="${COMP_WORDS[c]}"
		case "$i" in
            installed|all)
			command="$i"
			break
			;;
		esac
		c=$((++c))
	done

	if [ $c -eq $COMP_CWORD -a -z "$command" ]; then
        __lowcomp "installed all"
    fi
    return
}

_low_repolist ()
{
	local i c=1 command
	while [ $c -lt $COMP_CWORD ]; do
		i="${COMP_WORDS[c]}"
		case "$i" in
            all|enabled|disabled)
			command="$i"
			break
			;;
		esac
		c=$((++c))
	done

	if [ $c -eq $COMP_CWORD -a -z "$command" ]; then
        __lowcomp "
            all
            enabled
            disabled
            "
    fi
    return
}

_low ()
{
	local i c=1 command

    while [ $c -lt $COMP_CWORD ]; do
		i="${COMP_WORDS[c]}"
		case "$i" in
#		--version|--help|--verbose|--nowait|-v|-n|-h|-?) ;;
		*) command="$i"; break ;;
		esac
		c=$((++c))
	done

    if [ $c -eq $COMP_CWORD -a -z "$command" ]; then
		case "${COMP_WORDS[COMP_CWORD]}" in
		--*=*) COMPREPLY=() ;;
		--*)   __pkconcomp "
			"
			;;
        -*) __pkconcomp "
            "
            ;;
		*)     __lowcomp "$__low_commandlist" ;;
		esac
		return
	fi

	case "$command" in
	list)         _low_list ;;
	repolist)     _low_repolist ;;
	*)           COMPREPLY=() ;;
	esac
}

complete -o default -o nospace -F _low low
