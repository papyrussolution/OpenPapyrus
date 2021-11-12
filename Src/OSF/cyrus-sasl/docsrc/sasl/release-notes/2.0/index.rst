==============================
Cyrus SASL 2.0.x Release Notes
==============================

New in 2.0.5-BETA
-----------------
* THIS IS A BETA-QUALITY RELEASE THAT IS NOT INTENDED FOR PRODUCTION USE.
  IT *WILL BREAK* ANY APPLICATION EXPECTING THE SASLv1 API.
* Improved performance of security layers in KERBEROS_V4, GSSAPI, and DIGEST.
* This release includes an OTP plugin that requires libopie.
* SRP plugin now in alpha stage.
* Includes many significant bugfixes throughout the library.

New in 2.0.4-BETA
-----------------
* THIS IS A BETA-QUALITY RELEASE THAT IS ONLY INTENDED FOR USE BY
  DEVELOPERS WHOSE APPLICATIONS MAKE USE OF THE CYRUS SASL LIBRARY.
  IT *WILL BREAK* ANY APPLICATION EXPECTING THE SASLv1 API.
* This release now includes Mac OS 9 and Mac OS X support.
* Significant new features include

    * DES and 3DES Encryption should now be working for DIGEST-MD5
    * Improved configuration system
    * Improved documentation (now includes plugin writers guide)
    * Many other bugfixes (see ChangeLog)

New in 2.0.3-BETA
-----------------
* THIS IS A BETA-QUALITY RELEASE THAT IS ONLY INTENDED FOR USE BY
  DEVELOPERS WHOSE APPLICATIONS MAKE USE OF THE CYRUS SASL LIBRARY.
  IT *WILL BREAK* ANY APPLICATION EXPECTING THE SASLv1 API.
* This library should be fairly close to the core features that will be
  released in a final version of Cyrus SASLv2.  It very likely has bugs.
* Major new features included in this release:

    - The glue code now correctly handles client-send-first and server-send-last
      situations based on what the protocol and mechanism each support.
    - The sasldb code has been extracted from the main library and now resides
      in a separate libsasldb.la that is available at build time.
    - SASLdb now supports multiple auxiliary properties, though as distributed
      only userPassword is implemented and used.
    - Much improved configure checking for various items, including
      Berkeley DB, Kerberos, and GSSAPI.
    - Better (more standard) handling of realms in DIGEST-MD5.
    - A new Plugin Programmer's guide.
    - IPv6 support.
    - Error reporting now works in the GSSAPI plugin.

* See the ChangeLog for a more detailed list of changes.

New in 2.0.2-ALPHA
------------------
* THIS IS AN ALPHA-QUALITY RELEASE THAT IS ONLY INTENDED FOR DEVELOPERS
  WHOSE APPLICATIONS MAKE USE OF THE CYRUS SASL LIBRARY.
* This release is intended to show developers that use Cyrus SASL what
  direction we are planning on taking the library so that they can make
  plans to migrate their applications accordingly
* Major new features included in this release:

  - Ability to compile a static library including all mechanisms.  This
    means lower memory usage and faster mechanism loading time, but
    is not for everyone (or even many people). See doc/advanced.html,
    as well as the '--with-staticsasl' configure flag.
  - Man pages should now all be present and are close to being correct.
  - Can now build libsfsasl and the smtptest program (using the --with-sfio
    configure flag)
  - Reverted to the v1 entry points for mechanisms, to allow v1 mechanisms
    to fail loading cleanly.
  - Auxprop and canon_user plugins can now load from DSOs
  - Java code now compiles (but is not well tested, or up to date with the
    current Java API draft)
  - Error handling and use of sasl_errdetail has been fleshed out and
    should now work in most cases.

* Still Coming:

  - Cleanup of the client-send-first and server-send-last situation
  - Error reporting in GSSAPI plugin
  - Move the sasldb code out of the main library and into plugins and
    utilities only.

New in 2.0.0-ALPHA
------------------
* THIS IS AN ALPHA-QUALITY RELEASE THAT IS ONLY INTENDED FOR DEVELOPERS
  WHOSE APPLICATIONS MAKE USE OF THE CYRUS SASL LIBRARY.
* This release is intended to show developers that use Cyrus SASL what
  direction we are planning on taking the library so that they can make
  plans to migrate their applications accordingly
* This release implements the SASLv2 API.
  Some of the major improvements in the API include:

  - Memory management is now sane (whoever allocates the memory is responsible
    for freeing it)
  - Auxiliary Property plugin support (ability to interface with directory
    services as part of authentication)
  - Username canonification plugin support
  - Improved error reporting (not fully implemented in this release)
  - Database support has been simplified.  We now maintain only a single
    store of plaintext passwords that is shared by all supplied plugins
    (using the auxiliary property interface).

  The new API is more fully documented in the header files sasl.h, saslplug.h
  saslutil.h, and prop.h.  The man pages, programmers guide, and system
  administrators guide have also been rewritten to deal with the new API.

* There is still a good amount of work to be done, and as this code is alpha
  quality, it has bugs, known and unknown.  Please either use our bugzilla, or email cyrus-bugs@andrew.cmu.edu with questions, comments, or bug reports.

  - Most notably, the Java bindings have not been converted to work with
    the new API, and thus will not compile successfully.
  - The current development branch with this source is in our
    cvs repository as the "sasl-v2-rjs3" branch of the "sasl" collection.
    (see http://asg.web.cmu.edu/cyrus/download/anoncvs.html for more info)
