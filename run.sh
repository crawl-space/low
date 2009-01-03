#!/bin/bash

TMP=`mktemp`
echo "set verbose off" >> $TMP
echo "set complaints 0" >> $TMP
echo "run $@" >> $TMP
echo "bt" >> $TMP

libtool --mode=execute gdb src/low -batch -x $TMP

rm $TMP
