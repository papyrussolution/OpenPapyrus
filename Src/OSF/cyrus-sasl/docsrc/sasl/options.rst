.. _options:

=======
Options
=======

This document contains information on what options are used by the
Cyrus SASL library and bundled mechanisms.  The most commonly used
options (and those that are therefore most commonly misunderstood
are :option:`pwcheck_method` and :option:`auxprop_plugin`.  Please ensure
that you have configured these correctly if things don't seem to
be working right.  Additionally, :option:`mech_list` can be an easy
way to limit what mechanisms a given application will use.

.. contents::
    :depth: 1
    :local:

SASL Library
============

.. option:: authdaemon_path [<path>]

   Path to Courier-IMAP authdaemond's unix socket.

   Default: /dev/null

.. option:: auto_transition [yes|noplain|no]

   When set to 'yes' or 'noplain',
   and when using an auxprop plugin, automatically transition
   users to other mechs when they do a successful plaintext
   authentication.  When set to 'noplain', only non-plaintext secrets
   will be written.  *Note that the only mechanisms (as currently
   implemented) which don't use plaintext secrets are
   OTP, SCRAM and SRP.*

   Default: no

.. option:: canon_user_plugin [<name>]

   Name of canon_user plugin to use

   Default: INTERNAL

.. option:: log_level [<numeric log level>]

   **Numeric** Logging Level (see ``SASL_LOG_*`` in ``sasl.h``
   for values and descriptions)

   Default: 1 (SASL_LOG_ERR)

.. option:: mech_list [<mechanism list>]

   Whitespace separated list of mechanisms to allow (e.g. 'plain
   otp').  Used to restrict the mechanisms to a subset of the installed
   plugins.

   Default: empty (use all available plugins)

.. option:: plugin_list [<path>]

   Location of Plugin list (Unsupported)

   Default: none

.. option:: pwcheck_method [<list of mechanisms>]

   Whitespace separated list of mechanisms used to verify passwords,
   used by sasl_checkpass. Possible values: 'auxprop', 'saslauthd',
   'pwcheck', 'authdaemond' [if compiled with --with-authdaemond])
   and 'alwaystrue' [if compiled with --enable-alwaystrue])

   Default: auxprop

.. option:: saslauthd_path [<path>]

   Path to saslauthd run directory (**including** the "/mux" named pipe)

Auxiliary Property Plugin
=========================

.. option:: auxprop_plugin [<list of plugin names>]

   Name of auxiliary plugin to use, you may specify a space-separated
   list of plugin names, and the plugins will be queried in order.

   Default: empty - queries all plugins.

GSSAPI
======

.. option:: keytab [<path>]

   Location of keytab file

   Default: /etc/krb5.keytab (system dependant)

LDAPDB
======

.. option:: ldapdb_uri [<list of URIs>]

   URI to the LDAP server. You can specify a space-separated list of URIs -
   ldapi:// or ldaps://ldap1/ ldaps://ldap2/

   Default: none

.. option:: ldapdb_id [<auth id>]

   ldap SASL authentication id

   Default: none

.. option:: ldapdb_mech [<mechanism>]

   LDAP SASL mechanism for authentication.

   Default: none

.. option:: ldapdb_pw [<password>]

   LDAP password for SASL authentication id

   Default: none

.. option:: ldapdb_rc [<filename>]

   The filename specified here will be put into the server's LDAPRC
   environment variable, and libldap-specific config options may be set
   in that ldaprc file.

   The main purpose behind this option is to allow
   a client TLS certificate to be configured, so that SASL/EXTERNAL may
   be used between the SASL server and the LDAP server. This is the most
   optimal way to use this plugin when the servers are on separate machines.

   Default: none

.. option:: ldapdb_starttls [try|demand]

   Use StartTLS.  This option may be set to 'try' or 'demand'.
   When set to "try" any failure in StartTLS is ignored.
   When set to "demand" then any failure aborts the connection.

   Default: none

.. option:: ldapdb_canon_attr [<user's canonical name>]

   Use the value of the specified attribute as the user's
   canonical name. The attribute will be looked up in the user's LDAP
   entry. This setting must be configured in order to use LDAPDB as
   a canonuser plugin.

   Default: none

Notes on LDAPDB
---------------

Unlike other LDAP-enabled plugins for other services that are common
on the web, this plugin does not require you to configure DN search
patterns to map usernames to LDAP DNs. This plugin requires SASL name
mapping to be configured on the target slapd. This approach keeps the
LDAP-specific configuration details in one place, the slapd.conf, and
makes the configuration of remote services much simpler.

This plugin is not for use with slapd itself. When OpenLDAP is
built with SASL support, slapd uses its own internal auxprop and
canonuser module.

By default, without configuring anything else, slapd will fail to load
the ldapdb module when it's present. This is as it should be. If you
don't like the "auxpropfunc: error -7" message that is sent to syslog
by slapd, you can stop it by creating /usr/lib/sasl2/slapd.conf with::

   auxprop_plugin: slapd

which will force the SASL library to ignore all other auxprop modules.

Examples
--------

::

   ldapdb_uri: ldap://ldap.example.com
   ldapdb_id: root
   ldapdb_pw: secret
   ldapdb_mech: DIGEST-MD5
   ldapdb_canon_attr: uid

The LDAP server must be configured to map the SASL authcId "root" into a DN
that has proxy authorization privileges to every account that is allowed to
login to this server. (See the OpenLDAP Admin Guide section 10 for
details.)

::

   ldapdb_uri: ldapi://
   ldapdb_mech: EXTERNAL

This configuration assumes an LDAP server is on the same server that is
using SASL and the underlying OS is \*NIX based (ldapi:// requires UNIX domain
sockets).  This is fast and secure, and needs no username or password to be
stored.  The slapd.conf will need to map these usernames to LDAP DNs:

::

   sasl-regexp uidNumber=(.*)\\+gidNumber=(.*),cn=peercred,cn=external,cn=auth
       ldap:///dc=example,dc=com??sub?(&(uidNumber=$1)(gidNumber=$2))


   sasl-regexp uid=(.*),cn=external,cn=auth
       ldap:///dc=example,dc=com??sub?(uid=$1)

NTLM
====

.. option:: ntlm_server [<list of server names>]

   Comma separated list of servernames (WinNT, Win2K, Samba, etc) to
   which authentication will be proxied.

   Default: empty - perform authentication internally

.. option:: ntlm_v2 [yes|no]

   (Client) Send NTLMv2 responses to the server.

   Default: no (send NTLMv1)

OTP
===

.. option:: opiekeys [<path>]

   Location of the opiekeys file

   Default: /etc/opiekeys

.. option:: otp_mda [md4 | md5 | sha1]

   (Without opie) Message digest algorithm for one-time passwords, used by sasl_setpass

   Default: md5

Digest-md5
==========

.. option:: reauth_timeout [<minutes>]

   Length in time (in minutes) that authentication info will be
   cached for a fast reauth.  A value of 0 will disable reauth.

   Default: 0 - reauth disabled.

SASLDB
======

.. option:: sasldb_path [<path to sasldb file>]

   Path to sasldb file

   Default: /etc/sasldb2 (system dependant)

.. option:: sasldb_mapsize [<size in bytes>]

   For sasldb with LMDB. Size of the memory map used by the DB. This is also the maximum possible
   size of the database, so it must be set to a value large enough to contain
   all the desired user records.

   Default: 1048576 bytes

.. option:: sasldb_maxreaders [<max threads>]

   For sasldb with LMDB. Maximum number of threads (or processes) that may concurrently read the
   database.

   Default: 126

Notes on sasldb with LMDB
-------------------------

The OpenLDAP LMDB library is an extremely compact, extremely high performance
B+tree database. The code for it is available in the regular OpenLDAP source
distributions and it is distributed under the terms of the OpenLDAP Public License.

Full documentation, plus papers and presentations are available on
`the LMDB page <symas.com/lmdb/>`_.

SQL Plugin
==========

.. option:: sql_engine [<name>]

   Name of SQL engine to use (possible values: 'mysql', 'pgsql', 'sqlite', 'sqlite3').

   Default: mysql

.. option:: sql_hostnames [<list of SQL servers>]

   Comma separated list of SQL servers (in host[:port] format).

.. option:: sql_user <username>

   Username to use for authentication to the SQL server.

.. option:: sql_passwd <password>

   Password to use for authentication to the SQL server.

.. option:: sql_database <database name>

   Name of the database which contains the auxiliary properties.

.. option:: sql_select <statement>

   SELECT statement to use for fetching properties.  This option is
   **required** in order to use the SQL plugin.

.. option:: sql_insert <statement>

   INSERT statement to use for creating properties for new users.

.. option:: sql_update <statement>

   UPDATE statement to use for modifying properties.

.. option: sql_usessl [yes | no]

   When set to 'yes', 'on', '1' or 'true', a secure connection will
   be made to the SQL server.

   Default: no

Notes on SQL
------------

The sql_insert and sql_update options are
optional and are only needed if you wish to allow the SASL library
(e.g., saslpasswd2) and plugins (e.g., OTP) to write properties to the
SQL server.  If used, both statements MUST be provided so that
properties can be added, changed and deleted.

NOTE: The columns for writable properites MUST accept NULL values.

The SQL statements provided in the sql_select,
sql_insert and sql_update options can contain
arguments which will be substituted with the appropriate values.  The
valid arguments are:

%u
  Username whose properties are being fetched/stored.
%p
  Name of the property being fetched/stored.  This could
  technically be anything, but SASL authentication will try
  userPassword and cmusaslsecretMECHNAME (where MECHNAME is the
  name of a SASL mechanism).
%r
  Realm to which the user belongs.  This could be the
  kerberos realm, the FQDN of the computer the SASL application is
  running on or whatever is after the @ on a username.  (read the
  realm documentation).
%v
  Value of the property being stored (INSERT or
  UPDATE only!). This could technically be anything depending on
  the property itself, but is generally a userPassword.

Note: DO NOT put quotes around the entire SQL
statement, but each individual %u, %r and %v argument MUST be
quoted.


Examples
--------


   ``sql_select: SELECT %p FROM user_table WHERE username = '%u' and realm = '%r'``

would send the following statement to SQL for user "bovik" and
the default realm for the machine "madoka.surf.org.uk"::

   SELECT userPassword FROM user_table WHERE username = 'bovik' and
   realm = 'madoka.surf.org.uk'

::

  sql_insert: INSERT INTO user_table (username, realm, %p) VALUES ('%u', '%r', '%v')

would generate the following statement to SQL for user "bovik" in
realm "madoka.surf.org.uk" with userPassword "wert"::

   INSERT INTO user_table (username, realm, userPassword) VALUES
   ('bovik', 'madoka.surf.org.uk', 'wert');


Note that all substitutions do not have to be used. For instance,

::

   SELECT password FROM auth WHERE username = '%u'

is a valid value for sql_select.



SRP
===

.. option:: srp_mda [md5 | sha1 | rmd160]

   Message digest algorithm for SRP calculations

   Default: sha1

Kerberos V4
===========

.. option:: srvtab [<path>]

   Location of the srvtab file

   Default: /etc/srvtab
