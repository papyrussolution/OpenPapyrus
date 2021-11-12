==============================
Cyrus SASL 2.1.x Release Notes
==============================

New in 2.1.27
-------------

* Added support for OpenSSL 1.1
* Added support for lmdb (from Howard Chu)
* Lots of build fixes (from Ignacio Casal Quinteiro and others)
* Treat SCRAM and DIGEST-MD5 as more secure than PLAIN when selecting client mech
* DIGEST-MD5 plugin:

  - Fixed memory leaks
  - Fixed a segfault when looking for non-existent reauth cache
  - Prevent client from going from step 3 back to step 2
  - Allow cmusaslsecretDIGEST-MD5 property to be disabled

* GSSAPI plugin:

  - Added support for retrieving negotiated SSF
  - Fixed GSS-SPNEGO to use flags negotiated by GSSAPI for SSF
  - Properly compute maxbufsize AFTER security layers have been set

* SCRAM plugin:
  
  - Added support for SCRAM-SHA-256
    
* LOGIN plugin:

  - Don't prompt client for password until requested by server

* NTLM plugin:

  - Fixed crash due to uninitialized HMAC context

* saslauthd:

  - cache.c:

    - Don't use cached credentials if timeout has expired
    - Fixed debug logging output
  
  - ipc_doors.c:

    - Fixed potential DoS attack (from Oracle)

  - ipc_unix.c:

    - Prevent premature closing of socket

  - auth_rimap.c:

    - Added support LOGOUT command
    - Added support for unsolicited CAPABILITY responses in LOGIN reply
    - Properly detect end of responses (don't needlessly wait)
    - Properly handle backslash in passwords
    
  - auth_httpform:

    - Fix off-by-one error in string termination
    - Added support for 204 success response

  - auth_krb5.c:

    - Added krb5_conv_krb4_instance option
    - Added more verbose error logging

New in 2.1.26
-------------

* Modernize SASL malloc/realloc callback prototypes
* Added sasl_config_done() to plug a memory leak when using an application
  specific config file
* Fixed PLAIN/LOGIN authentication failure when using saslauthd
  with no auxprop plugins (bug # 3590).
* unlock the mutex in sasl_dispose if the context was freed by another thread
* MINGW32 compatibility patches
* Fixed broken logic in get_fqhostname() when abort_if_no_fqdn is 0
* Fixed some memory leaks in libsasl
* GSSAPI plugin:

  - Fixed a segfault in gssapi.c introduced in 2.1.25.
  - Code refactoring
  - Added support for GSS-SPNEGO SASL mechanism (Unix only), which is also
    HTTP capable

* GS2 plugin:

  - Updated GS2 plugin not to lose minor GSS-API status codes on errors

* DIGEST-MD5 plugin:

  - Correctly send "stale" directive to prevent clients from (re)promtping
    for password
  - Better handling of HTTP reauthentication cases
  - fixed some memory leaks

* SASLDB plugin:

  - Added support for BerkleyDB 5.X or later

* OTP plugin:

  - Removed calling of EVP_cleanup() on plugin shutdown in order to prevent
    TLS from failing in calling applications

* SRP plugin:

  - Removed calling of EVP_cleanup() on plugin shutdown in order to prevent
    TLS from failing in calling applications

* saslauthd:

  - auth_rimap.c: qstring incorrectly appending the closing double quote,
    which might be causing crashes
  - auth_rimap.c: read the whole IMAP greeting
  - better error reporting from some drivers
  - fixed some memory leaks

New in 2.1.25
-------------

* Make sure that a failed authorization doesn't preclude
  further server-side SASL authentication attempts from working.
* Fixed a crash caused by aborted SASL authentication
  and initiation of another one using the same SASL context.
* (Windows) Fixed the random number generator to actually produce random
  output on each run.
* Be protective against calling sasl_server_step once authentication
  has failed (multiple SASL plugins)
* Fixed several bugs in the mech_avail callback handling
  in the server side code.
* Added support for channel bindings
* Added support for ordering SASL mechanisms by strength (on the client side),
  or using the "client_mech_list" option.
* server_idle needs to obey server's SASL mechanism list from the server
  context.
* Better server plugin API mismatch reporting
* Build:
  - Updated config to the latest GNU snapshot
  - Fixed SASL's libtool MacOS/X 64-bit file magic

* New SASL plugin: SCRAM
* New SASL plugin: GS2
* DIGEST-MD5 plugin:

 -  Allow DIGEST-MD5 plugin to be used for client-side and
    server-side HTTP Digest, including running over non-persistent
    connections (RFC 2617)
 - Use the same username for reauthentication cache lookup and update
 - Minimize the number of auxprop lookups in the server side DIGEST-MD5
   plugin for the most common case when authentication and authorization
   identities are the same.
 - Updated digestmd5_server_mech_step2() to be more defensive against
   empty client input.
 - Fixed some memory leaks on failed plugin initialization.
   Prevent potential race condition when freeding plugin state.
   Set the freed reauthentication cache mutex to NULL, to make errors
   due to mutex access after free more obvious.
 - Test against broken UTF-8 based hashes if calculation using special
   ISO-8859-1 code fails.
 - Fixed an interop problem with some LDAP clients ignoring server
   advertised realm and providing their own.

* GSSAPI plugin:

  - Fix to build GSSAPI with Heimdal
  - Properly set serveroutlen to 0 in one place.
    Don't send empty challenge once server context establishment is done,
    as this is in violation of the RFC 2222 and its successor.
  - Don't send maxbuf, if no security layer can be established.
    Added additional checks for buffer lengths.

* LDAPDB plugin:
  - build fixes

New in 2.1.24
-------------

* Order advertised server-side SASL mechanisms per the specified 'mech_list'
  option or by relative "strength"
* Make sure that sasl_set_alloc() has no effect once sasl_client_init()
  or sasl_server_init() is called
* Fixed sasl_set_mutex() to disallow changing mutex management functions
  once sasl_server_init()/sasl_client_init() is called (bug # 3083)
* Removed unused mutexes in lib/client.c and lib/server.c (bug # 3141)
* Added direct support for hashed password to auxprop API
* Don't treat a constraint violation as an error to store an auxprop property
* Extended libsasl (auxprop) to support user deletion
* Extended SASL auxprop_lookup to return error code
* Updated sasl_user_exists() so that it can handle passwordless accounts (e.g. disabled)
* (Windows) Free handles of shared libraries on Windows that were loaded
  but are not SASL plugins (bug # 2089)
* Prevent freeing of common state on a subsequent call to _sasl_common_init.
  Make sure that the last global callback always wins.
* Implemented sasl_client_done()/sasl_server_done()
* Added automatic hostname canonicalization inside libsasl
* Made sasl_config_init() public
* Strip trailing spaces from server config file option values (bug # 3139, bug # 3041)
* Fixed potential buffer overflow in saslautd_verify_password().
* Fixed segfault in dlclose() on HPUX
* Various bugfixes for 64bit platforms
* Fixed bug # 2895 (passing LF to sasl_decode64) in sample/sample-client.c,
  sample/sample-server.c, utils/smtptest.c
* pluginviewer: Code cleanup, improved human readable messages
* Build:
  - (Windows) Updated makefiles to build with VC 8.0 (VC++ 2005)
  - (Windows) Added Windows64 build
  - Updated to use .plugin extension on MacOS
  - Changed 64bit HP-UX build to use .so for shared libraries

* saslauthd:

  - Fixed bug counting double-quotes in username/password in
    auth_rimap.c. Also fixed bug zeroing password.
  - auth_krb.c: improved diagnostic in the k5support_verify_tgt() function.
  - auth_sasldb.c: pid_file_lock is created with a mask of 644 instead of 0644
  - auth_shadow.c: Define _XOPEN_SOURCE before including unistd.h,
    so that crypt is correctly defined
  - auth_getpwent.c: Fixed Solaris build

* SASLDB plugin:

  - Fixed spurious 'user not found' errors caused by an attempt
    to delete a non-existent property
  - Added direct support for hashed password to auxprop API
  - Sleepycat driver:  Return SASL_NOUSER instead of SASL_FAIL when the database
    file doesn't exist
  - Ignore properties starting with '*' in the auxprop store function

* SQL plugin:
  - Added support for SQLITE3
  - Uninitialized variables can cause crash when the searched user is not found
  - Added direct support for hashed password
  - Ignore properties starting with '*' in the auxprop store function

* LDAPDB plugin:

  - Added code to extend LDAPDB into a canon_user plugin in addition
    to its existing auxprop plugin functionality

* PLAIN plugin:
  - Advertise SASL_SEC_PASS_CREDENTIALS feature

* LOGIN plugin:
  - Advertise SASL_SEC_PASS_CREDENTIALS feature

* DIGEST-MD5 plugin:

  - Fixed a memory leak in the DIGEST-MD5 security layer
  - Fixed memory leaks in client-side reauth and other places
  - More detailed error reporting.
  - Fixed parsing of challenges/responses with extra commas.
  - Allow for multiple qop options from the server and require
    a single qop option from the client.

* GSSAPI plugin:
  - Check that params->serverFQDN is not NULL before using strlen on it
  - Make auxprop lookup calls optional

* EXTERNAL plugin:
  - Make auxprop lookup calls optional

* NTLM plugin:
  - allow a comma separated list of servernames in 'ntlm_server' option
  - Fixed crash in calculating NTv2 reponse

* OTP plugin:
  - Don't use a stack variable for an OTP prompt (bug # 2822)
  - Downgrade the failure to store OTP secret to debug level

* KERBEROS_V4 plugin:
  - Make auxprop lookup calls optional

New in 2.1.23
-------------

* Fixed CERT VU#238019 (make sure sasl_encode64() always NUL
  terminates output or returns SASL_BUFOVER)

New in 2.1.22
-------------

* Added support for spliting big data blocks (bigger than maxbuf)
  into multiple SASL packets in sasl_encodev
* Various sasl_decode64() fixes
* Increase canonicalization buffer size to 1024 bytes
* Call do_authorization() after successful APOP authentication
* Allow for configuration file location to be configurable independently
  of plugin location (bug # 2795)
* Added sasl_set_path function, which provides a more convenient way
  of setting plugin and config paths. Changed the default
  sasl_getpath_t/sasl_getconfpath_t callbacks to calculate
  the value only once and cache it for later use.
* Fixed load_config to search for the config file in all directories
  (bug # 2796). Changed the default search path to be
  /usr/lib/sasl2:/etc/sasl2
* Don't ignore log_level configuration option in default UNIX syslog
  logging callback
* (Windows) Minor IPv6 related changes in Makefiles for Visual Studio 6
* (Windows) Fixed bug of not setting the CODEGEN (code generation option)
  nmake option if STATIC nmake option is set.
* Several fixed to DIGEST-MD5 plugin:

  - Enable RC4 cipher in Windows build of DIGEST-MD5
  - Server side: handle missing realm option as if realm="" was sent
  - Fix DIGEST-MD5 to properly advertise maxssf when both DES and RC4
    are disabled
  - Check that DIGEST-MD5 SASL packet are no shorter than 16 bytes

* Several changes/fixed to SASLDB plugin:

  - Prevent spurious SASL_NOUSER errors
  - Added ability to keep BerkleyDB handle open between operations
    (for performance reason). New behavior can be enabled
    with --enable-keep-db-open.

* Better error checking in SQL (MySQL) auxprop plugin code
* Added support for HTTP POST password validation in saslauthd
* Added new application ("pluginviewer") that helps report information
  about installed plugins
* Allow for building with OpenSSL 0.9.8
* Allow for building with OpenLDAP 2.3+
* Several quoting fixes to configure script
* A large number of other minor bugfixes and cleanups

New in 2.1.21
-------------
* Fixes DIGEST-MD5 server side segfault caused by the client not sending
  any realms
* Minor Other bugfixes

New in 2.1.20
-------------
* Fixes to cram plugin to avoid attempting to canonify uninitialized data.
* NTLM portability fixes.
* Avoid potential attack using SASL_PATH when sasl is used in a setuid
  environment.
* A trivial number of small bugfixes.

New in 2.1.19
-------------
* Fixes to saslauthd to allow better integration with realms (-r flag to
  saslauthd, %R token in LDAP module)
* Support for forwarding of GSSAPI credentials
* SQLite support for the SQL plugin
* A nontrivial number of small bugfixes.

New in 2.1.18
-------------
* saslauthd/LDAP no longer tagged "experimental"
* Add group membership check to saslauthd/LDAP
* Fix Solaris 9 "NI_WITHSCOPEID" issue
* Fix missing "getaddrinfo.c" and other distribution problems
* Significant Windows enhancements
* A large number of other minor bugfixes and cleanups

New in 2.1.17
-------------
* Allow selection of GSSAPI implementation explicitly (--with-gss_impl)
* Other GSSAPI detection improvements
* Now correctly do authorizaton callback in sasl_checkpass()
* Disable KERBEROS_V4 by default
* Continued Win32/Win64 Improvements
* Minor Other bugfixes

New in 2.1.16-BETA
------------------
* Significantly improved Win32 support
* Writable auxprop support
* Expanded SQL support (including postgres)
* Significantly improved documentation
* Improved realm/username handling with saslauthd
* Support for modern automake and autoconf

New in 2.1.15
-------------
* Fix a number of build issues
* Add a doc/components.html that hopefully describes how things
  interact better.

New in 2.1.14
-------------
* OS X 10.2 support
* Support for the Sun SEAM GSSAPI implementation
* Support for MySQL 4
* A number of build fixes
* Other minor bugfixes

New in 2.1.13
-------------
* Add a configure option to allow specification of what /dev/random to use.
* Addition of a saslauthd credential cache feature (-c option).
* Unification of the saslauthd ipc method code.
* Fix a number of autoconf issues.
* A significant number of fixes throughout the library from Sun Microsystems.
* Other minor bugfixes.

New in 2.1.12
-------------
* Distribute in Solaris tar (not GNU tar format)
* Fix a number of build/configure related issues.

New in 2.1.11
-------------
* Add the fastbind auth method to the saslauthd LDAP module.
* Fix a potential memory leak in the doors version of saslauthd.
* NTLM now only requires one of LM or NT, not both.
* Fix a variety of Berkeley DB, LDAP, OpenSSL, and other build issues.
* Win32 support compiles, but no documentation as of yet.

New in 2.1.10
-------------
* Further DIGEST-MD5 DES interoperability fixes.  Now works against Active
  Directory.
* Fix some potential buffer overflows.
* Misc. cleanups in the saslauthd LDAP module
* Fix security properties of NTLM and EXTERNAL

New in 2.1.9
------------
* Include missing lib/staticopen.h file.

New in 2.1.8
------------
* Support for the NTLM mechanism (Ken Murchison <ken@oceana.com>)
* Support libtool --enable-shared and --enable-static
  (Howard Chu <hyc@highlandsun.com>)
* OS/390 Support (Howard Chu <hyc@highlandsun.com>)
* Berkeley DB 4.1 Support (Mika Iisakkila <mika.iisakkila@pingrid.fi>)
* Documentation fixes
* The usual round of assorted other minor bugfixes.

New in 2.1.7
------------
* Add SASL_AUTHUSER as a parameter to sasl_getprop
* Allow applications to require proxy-capable mechanisms (SASL_NEED_PROXY)
* Performance improvements in our treatment of /dev/random
* Removal of buggy DIGEST-MD5 reauth support.
* Documentation fixes
* Assorted other minor bugfixes.

New in 2.1.6
------------
* Security fix for the CRAM-MD5 plugin to check the full length of the
  digest string.
* Return of the Experimental LDAP saslauthd module.
* Addition of Experimental MySQL auxprop plugin.
* Can now select multiple auxprop plugins (and a priority ordering)
* Mechanism selection now includes number of security flags
* Mac OS X 10.1 Fixes
* Misc other minor bugfixes.

New in 2.1.5
------------
* Remove LDAP support due to copyright concerns.
* Minor bugfixes.

New in 2.1.4
------------
* Enhancements and cleanup to the experimental LDAP saslauthd module
  (Igor Brezac <igor@ipass.net>)
* Addition of a new sasl_version() API
* Misc. Bugfixes

New in 2.1.3-BETA
-----------------
* Significant amount of plugin cleanup / standardization.  A good deal of code
  is now shared between them. (mostly due to Ken Murchison <ken@oceana.com>)
* DIGEST-MD5 now supports reauthentication.  Also has a fix for DES
  interoperability.
* saslauthd now supports the Solaris "doors" IPC method
  (--with-ipctype=doors)
* Significant GSSAPI fixes (mostly due to Howard Chu <hyc@highlandsun.com>)
* Auxprop interface now correctly deals with the * prefix indicating
  authid vs. authzid.  (May break some incompatible auxprop plugins).
* We now allow multiple pwcheck_method(s).  Also you can restrict auxprop
  plugins to the use of a single plugin.
* Added an experimental saslauthd LDAP module (Igor Brezac <igor@ipass.net>)
* Removed check for db3/db.h
* Misc. documentation updates.  (Marshall Rose, and others)
* Other misc. bugfixes.

New in 2.1.2
------------
* Mostly a minor-bugfix release
* Improved documentation / cleanup of old references to obsolete
  pwcheck_methods
* Better error reporting for auxprop password verifiers

New in 2.1.1
------------
* Many minor bugfixes throughout.
* Improvements to OTP and SRP mechanisms (now compliant with
  draft-burdis-cat-srp-sasl-06.txt)
* API additions including sasl_global_listmech, and a cleaner handling of
  client-first and server-last semantics (no application level changes)
* Minor documentation improvements

New in 2.1.0
------------
* The Cyrus SASL library is now considered stable.  It is still not backwards
  compatible with applications that require SASLv1.
* Minor API changes occured, namely the canon_user callback interface.
* saslauthd now preforks a number of processes to handle connections
* Many bugfixes through the entire library.
