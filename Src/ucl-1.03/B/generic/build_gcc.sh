#! /bin/sh
# vi:ts=4:et
set -e
echo "// Using GNU C compiler."
echo "//"

CC="gcc"
CFLAGS="-Wall -O2 -fomit-frame-pointer"

. ./B/generic/build.sh
exit 0
