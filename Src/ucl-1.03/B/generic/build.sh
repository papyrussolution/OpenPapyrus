#! /bin/sh
# vi:ts=4:et
set -e
echo "// Copyright (C) 1996-2004 Markus F.X.J. Oberhumer"
echo "//"
echo "//   Generic Posix/Unix system"
echo "//   Generic C compiler"


CFI="-Iinclude -I."
CFASM=-DUCL_USE_ASM
BNAME=ucl
BLIB=libucl.a

test "X${CC+set}" = Xset || CC="cc"
test "X${CFLAGS+set}" = Xset || CFLAGS="-O"
CF="$CFLAGS $CFI"
LF="$LDFLAGS $BLIB"

rm -f *.o "$BLIB" simple uclpack
for f in src/*.c; do
    echo $CC $CF -c $f
         $CC $CF -c $f
done
echo ar rcs "$BLIB" *.o
     ar rcs "$BLIB" *.o

echo $CC $CF -o simple  examples/simple.c $LF
     $CC $CF -o simple  examples/simple.c $LF
echo $CC $CF -o uclpack examples/uclpack.c $LF
     $CC $CF -o uclpack examples/uclpack.c $LF


echo "//"
echo "// Building UCL was successful. All done."
exit 0
