.. _install-macos:

=========================================
Building and using Cyrus SASL on Mac OS X
=========================================

The Cyrus SASL v2 distribution now supports Mac OS X, including
applications written to Apple's Carbon and Cocoa interfaces, as well
as the standard Unix-like API. It includes the following
components:

* A port of the Unix SASL library, which lives in ``/usr/local/lib/libsasl2.dylib``
  (or similar) and with plugins in ``/usr/lib/sasl`` (which should be a symlink to ``/usr/local/lib/sasl``).
* A framework which lives in
  ``/Library/Frameworks/SASL2.framework``, and allows the use of the
  ``-framework`` option to Apple's ``ld``, or linking with the
  framework in Project Builder. This framework is in fact a wrapper for a
  symlink to ``/usr/local/lib/libsasl2.dylib`` with the necessary
  information to recognize it as a framework. This is what we expect many
  Cocoa and Carbon Mach-O applications will want to use, and the framework
  is required for CFBundle to work, which is used by the CFM glue library.
* A CFM glue library (``/Library/CFMSupport/SASL2GlueCFM``) which
  can be linked in by Carbon CFM applications, that uses CFBundle to bind
  the framework and thus load the Unix-level library. It automatically loads
  the important functions at ``sasl_client_init`` or
  ``sasl_server_init`` time; it also automatically makes sure memory
  allocation works if you're using the metrowerks malloc; if you're not,
  ``sasl_set_alloc`` works as usual.
* A Carbon port of the existing CFM library for Mac OS 9. Note that
  this could probably be modified fairly easily to work on OS X, but
  there's not much point. The CFM glue layer to the Unix library
  supports many more functions, including the entire server API; also,
  the Unix implementation is mostly independent of Kerberos
  implementation, while the Mac OS 9 Carbon port specifically requires
  MIT Kerberos for Macintosh 3.5 or later in order to get Kerberos
  support. The Mac OS 9 code implements only the client API, but this is
  mostly what is wanted from SASL on OS 9 anyway.

If you are building a Carbon CFM application and intend it to run on
both OS 9 and OS X, you should link against the OS 9 Carbon SASL
library, since it exports fewer APIs (client side only, specifically)
than the OS X CFM glue. Your application should work seamlessly with
both libraries if you do this, despite the different implementations
underneath.

If you need a Carbon CFM application to support server-side SASL
functionality, you need to link against the ``SASL2GlueCFM``
library, but be aware that your application will not run on OS 9.

Compiling and Using the Unix library
====================================

The Unix library is mostly ready to build on Mac OS X, but it does depend
on the ``dlcompat`` package in order to load its plugins.
``dlcompat-20010505`` is a relatively simple version known to work
with SASL; it is provided with the distribution in a tarball. You should
``make`` and ``make install`` the ``dlcompat`` library
(which probably goes into ``/usr/local/lib/libdl.dylib``) before
attempting to ``./configure`` the SASL distribution itself. SASL will
then pretend it's a real Unix ``libdl``, and link against it.

Since there are, at this point, newer and far more complex versions of
dlcompat, you may prefer to use those instead if other software requires
their functionality. The dlcompat homepage is located on the `OpenDarwin <http://www.opendarwin.org/projects/dlcompat>`_
site. Many users may want to install the ``/sw`` tree from `the Fink project <http://fink.sourceforge.net>`_ to get this, as
well as possibly newer autotools and other software.

As of version 2.1.16, SASL uses and requires a recent version of GNU
autotools (autoconf, automake, and libtool) to build its configuration scripts.
If you are building from GIT, you will need to have the autotools installed
on your system. The version included with all releases of the developer tools
for OS X 10.2.x is too old for this; if you aren't using OS X 10.3 or later,
you should upgrade to more recent patchlevels of these tools. The easiest way
to do this is to install the Fink environment and then ``apt-get
install autoconf2.5 automake1.7 libtool14``.

Recent versions of SASL ship with Kerberos v4 disabled by default.
If you need Kerberos v4 for some reason, and you are using MIT Kerberos
for Macintosh 4.0 or later, you should ``./configure`` with
the added options ``"--enable-krb4=/usr --without-openssl
--disable-digest"`` so that it finds the
correct location for the header files, and does not use OpenSSL or
build anything that depends on it (such as the digest-md5 plugin),
since OpenSSL provides its own DES routines which do not work with
Kerberos v4.

.. warning::

    Please read the "Known Problems" section at the end of
    this document for more information on this issue.

You must be root to make install, since ``/usr/local`` is only
modifiable by root. You need not enable the root account using
NetInfo; the recommended (but underdocumented) method is to use
``sudo -s`` from the Terminal window when you are logged into an
administrator's account, and enter the password for that account. When
building on Mac OS X, ``make install`` will automatically add the
framework to ``/Library/Frameworks``.

This does not build the CFM glue library. Building the CFM glue
library requires Metrowerks CodeWarrior Pro 6 or later (tested with
6), and the files necessary to build it are in the
``mac/osx_cfm_glue`` folder.

Changes to the Unix library to make it work on OS X
===================================================

This is provided for reference purposes only. The build system will
automatically take care of all of these issues when building on Darwin
or Mac OS X.

* The random code supports the preferred way to generate random
  numbers in Darwin. (In SASL v2, it does this on all unix-like
  platforms that lack jrand48). *Note that Mac OS X "Jaguar", version
  10.2,
  now has the standard jrand48 function, and that SASL will use this
  instead
  of the previous workaround.*
* Symbols which are dlopened have an underscore prefixed. (This
  behavior is detected by configure in SASL v2.)
* Plugins are linked with the ``-module`` option to ``libtool``,
  which causes the ``-bundle`` option to be
  supplied to Apple's ``ld``. (This is done on all platforms in
  SASL v2.)
* The MD5 symbols are renamed to avoid library conflicts. This
  allows proper compilations against Heimdal and MIT's unix kerberos
  distribution, and prevents crashes when linked against MIT Kerberos
  for Macintosh (which also duplicates the symbols, but in a different
  way). Note that the MD5 symbols have local names on all platforms with
  SASL v2; this was only different in SASL v1.
* MIT Kerberos for Macintosh 4.0 and later are fully supported. This
  was accomplished by using ``krb_get_err_text`` if available and
  checking for additional names for the krb4 libraries.

Changes to the Mac OS 9 projects to support Carbon
==================================================

.. warning::

    Please read these notes before you attempt to build SASL for OS 9 Carbon!

* **Important!** You must make sure that all files have their
  correct HFS filetype before starting to build this code! In
  particular, all source and text files must be of type ``'TEXT'``,
  which is not the default if you use the Mac OS X GIT client to check
  out the projects. If you run into this problem, you may want to use a
  utility such as FileTyper to recursively change the type on all
  files. CodeWarrior is less picky about the projects' filetypes, but
  setting them to filetype ``'MMPr'``, creator code ``'CWIE'``
  may be helpful in opening the projects from the Finder. Users on Mac OS
  X familiar with the Unix ``find``
  command should be able to rig ``/Developer/Tools/SetFile``
  to do this job as well.
* Many of the important projects (for ``libdes``, ``libsasl``,
  ``build_plugins``, and the sample client ``sc_shlb``)
  have Carbon versions.
* Plugins are loaded from a ``Carbon`` subfolder of the ``SASL
  v2`` folder in the Extensions folder. Plugins directly
  in the ``SASL v2`` folder are considered to be for the Classic
  libraries.
* Note that when using the ``build_plugins`` project, you must
  generate the plugin init files using the ``makeinit.sh`` script in
  the ``plugins`` directory. The easiest way to do this is to run the
  script from a Unix shell, such as Mac OS X. You must then fix the
  filetypes of the generated source files (see above).
* There is a new folder in ``CommonKClient`` called ``mac_kclient3``
  which contains code compatible with MIT's new `KClient
  3.0 <http://web.mit.edu/macdev/Development/MITKerberos/MITKerberosLib/KClient/Documentation/index.html>`_
  API. This folder must be in your CodeWarrior access paths, the
  old ``mac_kclient`` folder must not, and it must precede the
  project's main folder.
* The kerberos4 plugin uses this new code. The kerberos4 plugin
  also
  statically links the Carbon ``libdes``, and no other part of
  Carbon SASL uses ``libdes`` directly. *Your application should
  **not** link against* ``libdes.shlb`` *under Carbon!*
  (It causes problems due to DES symbols also existing in the MIT
  Kerberos library, which loads first.)
* To build the projects, you should have the MIT Kerberos for
  Macintosh 3.5 installation disk images mounted, since the access paths
  include the absolute paths to the library directories from that
  image. It's easier than you having to find the paths yourself, and
  smaller than having to distribute the libraries with SASL.

Known Problems
==============

* The Kerberos v4 headers bundled with Mac OS X (and Kerberos for
  Macintosh) are not compatible with OS X's OpenSSL headers. (Kerberos v4
  support is disabled by default.) If you actually need krb4 support, the
  easiest solution is to build without using OpenSSL's
  ``libcrypto``. To do this, specify the ``--without-openssl``
  option to ``configure``. As of version 2.1.18, this automatically
  disables using ``libcrypto`` for DES as well. You will probably
  also need to specify ``--disable-digest`` since the digestmd5 plugin
  does not build against Kerberos v4's DES headers or library. Note that
  this disables several features (DIGEST-MD5, NTLM, OTP, PASSDSS, SCRAM, SRP)
  which require OpenSSL. If both Kerberos v4 and functionality that requires
  OpenSSL are needed, it is possible to build the Kerberos v4 plugin against
  the correct K4 DES libraries, and everything else against OpenSSL;
  however, we do not support that configuration.
* Versions of Cyrus SASL prior to 2.1.14 with support for Carbon
  CFM applications on Mac OS X have a known bug involving the CFM glue
  code (in ``mac/osx_cfm_glue``). If ``sasl_done`` is called
  to unload the SASL library, and then one of the initialization
  functions (such as ``sasl_client_init``) is called to
  reinitialize it from the same process, the application will crash. A
  fix for one obvious cause of this problem is included in 2.1.14;
  however, as of this writing, it has not been tested. It is possible
  that other bugs in Cyrus SASL, or deficiencies in Apple's libraries,
  will make this fix insufficient to resolve this issue.
