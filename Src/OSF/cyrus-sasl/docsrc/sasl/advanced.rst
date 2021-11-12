.. _advanced:

==============
Advanced Usage
==============

Notes for Advanced Usage of libsasl
===================================

Using Cyrus SASL as a static library
------------------------------------

As of v2.0.2-ALPHA, Cyrus SASL supports the option to compile all of the
supported mechanisms and glue code into a single static library that may
be linked into any application.  In practice, this saves memory by avoiding
the need to have a jump table for each process's reference into the shared
library, and ensures that all the mechanisms are loaded when the application
loads (thus reducing the overhead of loading the DSOs).

However, this is not a recommended procedure to use in general.  It loses
the flexibility of the DSOs that allow one to simply drop in a new mechanism
that even currently-running applications will see for each new connection.
That is, if you choose to use the static version of the library, not only
will you need to recompile the library each time you add a mechanism (provided
the mechanisms even support being compiled staticly), but you will need to
recompile every application that uses Cyrus SASL as well.

However, if you are sure you wish to use a static version of Cyrus SASL,
compile it by giving ``configure`` the ``--enable-static`` option.
This will compile <b>both</b> a dynamic and a static version.  Then, whenever
an application links to libsasl, it will also need to explicitly pull in
any dynamic libraries that may be needed by Cyrus SASL.  Most notably, these
might include the GSSAPI, Kerberos, and Database libraries.  To avoid compiling
the dynamic version, pass ``--disable-shared``.
