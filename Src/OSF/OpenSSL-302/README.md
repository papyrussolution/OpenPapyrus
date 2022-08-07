# openssl3-win-build

openssl-3 Windows build with Visual Studio.

This version is openssl-3.0.2.

To build, simply open the required solution file, and
you know how to use Visual Studio, right?
(or perhaps this is the wrong place for you.)

Depends on zlib-win-build. There are hard references assuming
zlib-win-build sits next to openssl3-win-build.

Basically, in a command prompt:

> \> cd {somewhere}\\  
> \> git clone https://github.com/kiyolee/zlib-win-build.git  
> \> git clone https://github.com/kiyolee/openssl3-win-build.git

Build zlib first and then openssl3, with the same corresponding Visual Studio solution of course.

Note that only zlib static libraries are used.

This repository tracks the openssl 3 series.\
For openssl 1.0, check repository https://github.com/kiyolee/openssl1_0-win-build.git. \
For openssl 1.1, check repository https://github.com/kiyolee/openssl1_1-win-build.git.

### Highlights:

1. Multiple Visual Studio versions build happily sharing the same build directory.
2. Build both 32-bit (x86) and 64-bit (x64) binaries in one solution.
3. Assembly sources are generated from original perlasm scripts. No pre-generated assembly source are checked in.
4. Keyboard is optional. (Once this repository is cloned.)

### Build Requirements:

The following third party tools are required:

1. Perl

   Any reasonably recent version should be fine.\
   Strawberry Perl is used to develop this project.\
   Download Strawberry Perl from https://strawberryperl.com/.

2. NASM (Assembler)

   Download NASM from https://www.nasm.us.

Make sure both perl and nasm can be found through command path.

### Testing:

To test one set of output binaries that use DLLs:

> \> cd {somewhere}\openssl3-win-build\
> \> test\test_one.cmd build-{vsver}\\{outdir}

where build-{vsver} is one of the VS build directories and {outdir} is one of the following:
* Release (32-bit release build)
* Debug (32-bit debug build)
* x64\Release (64-bit release build)
* x64\Debug (64-bit debug build)

To test one set of output binaries that use static libraries:

> \> cd {somewhere}\openssl3-win-build\
> \> test\test_one.cmd build-{vsver}\\{outdir} -static

To test all binaries that have been built:

> \> cd {somewhere}\openssl3-win-build\
> \> test\test_all.cmd

### Using The Build:

It is not necessary to install OpenSSL to develop applications linking to
OpenSSL libraries built with this repository.

Every \$(OutDir) for different combinations of Platform and Configuration
from a solution has a copy of "include\openssl" which is the same as what
get installed like the official package. Note that the include directories
are exactly the same between different Platforms and/or Configurations. The
duplication is required to facilitate batch/parallel builds.

For a project to use OpenSSL from this repository, simply add "\$(OutDir)\include"
to the include path and link to libraries from \$(OutDir). Note that \$(OutDir) is
just a reference here and may need specific value for the project depending on
the combination of Visual Studio version, Platform and Configuration required.

If static libraries (libcrypto-3-static.lib and libssl-3-static.lib) are used,
libz-static.lib (from zlib-win-build) is needed as well.
