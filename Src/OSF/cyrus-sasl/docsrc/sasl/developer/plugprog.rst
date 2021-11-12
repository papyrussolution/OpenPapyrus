.. _plugprog:

=========================
Plugin Programmer's Guide
=========================

.. note::

    NOTE: This is a work in progress. Any contributions would be
    *very* appreciated.

.. contents::
    :local:

Introduction
============

About this Guide
----------------

This guide gives a brief overview on the things that one
needs to know to write a :ref:`mechanism <authentication_mechanisms>` for the SASLv2 API (and thus
Cyrus SASLv2).  Note that this page is a brief overview
and that the authoritative documentation are the header files
included in the SASL distribution.  If you have any questions, please
feel free to contact the :ref:`Cyrus development team <contribute>`.

Please note that this guide is only intended for developers looking
to write mechanisms for the SASLv2 API, and that application programmers
should be reading the :ref:`Application Programming Guide <programming>` instead.


What is SASL?
-------------

A description of SASL is covered in detail in the
:ref:`programmer's guide <programming>`, which mechanism
developers should probably read first anyway to become familiar
with development using the SASL library.

Common Section
==============

Overview of Plugin Programming
------------------------------

The basic idea behind programming plugins for Cyrus SASL rests in
the ability to dlopen a shared library.  Thus, all plugins should
be shared libraries.  It is recommended that they are libtool
libraries for portability reasons (Cyrus SASL parses .la files to
get the appropriate name to dlopen), but they can have an extention
of .so as well.

All plugins should live in the same directory
(generally /usr/lib/sasl2), which the glue code (that is, the interface
layer that sits between the plugins and the application) scans
when one of the init functions (sasl_server_init or sasl_client_init)
is called.  Cyrus SASL then attempts to open each library and
run an initialization function.  If the initialization function
succeeds, and the versions match, then the glue code determines
that the load was successful and the plugin is available for use.

There are serveral types of plugins (note that a given plugin library
may contain any or all of the following in combination, though
such a plugin would be a beast!):


Mechanism Plugins
    These plugins implement mechanisms
    for authentication, and are the majority of the plugins included
    with Cyrus SASL.  Generally implementing both a client and a server
    side they take care of the authentication process.
User Canonicalization Plugins
    These plugins enable differing
    ways of canonicalizing authentication and authorization IDs.
Auxiliary Property Plugins
    These plugins allow auxilliary
    properties about user accounts to be looked up, such as passwords.
    Cyrus SASL includes a plugin to read sasldb files, for example.


Use of sasl_utils_t
-------------------

Because of the way that shared library plugins are loaded for both
speed and namespace reasons, the symbol tables are not shared across
plugins.  Thus, the only interface that the plugin should assume it
has to the outside world is through the ``sasl_utils_t`` structure (or
through links that it specifically requires).  Likewise, the glue code
has no (and will use no) interface into the plugin other than the
contents of the structures that are passed back to it by the
initialization function.

.. note::

    Do not assume you have access to any
    functions except through links that your library explicitly makes
    or through what is provided via the ``sasl_utils_t`` structure.

Error Reporting
---------------

Error reporting is very important for failed authentication tracking
and helping to debug installations or authentication problems.  For
that reason, in addition to the standard SASL return codes, the
glue code provides an interface to its seterror function (via
``sasl_utils_t``).  This function sets detailed error information for
a given connection.

In order to ensure consistency of this information, it is the
responsibility of the deepest function with access to the sasl_conn_t
make the call to set the errdetail string.

Memory Allocation
-----------------

Memory allocation in SASLv2 follows the simple paradigm that **if you
allocate it, you free it**.  This improves portability, and allows
for a large performance improvement over SASLv1.  To prevent memory
leaks (especially in the mechanism plugins), please ensure that you
follow this paradigm.

Client Send First / Server Send Last
------------------------------------

Mechanism plugins used to have to worry about the situation
where they needed clients to send first (or server to send last), yet
the protocol did not support it.  Luckily, this is now handled by
the glue code, provided that the plugin declares the appropriate flags
in the structure returned by its init function.  Thus, the step functions
will not have to worry about these issues and can be implemented
knowing they will be called only when the application actually has
data for them and/or will allow them to send data.  These flags are as
follows:

SASL_FEAT_WANT_CLIENT_FIRST
    The mechanism has the client
    side send first always.  (e.g. PLAIN)
SASL_FEAT_SERVER_FIRST
    The mechanism has the server side
    send first always.  (e.g. CRAM-MD5)


If neither flag is set, the mechanism will handle the client-send
first situation internally, because the client may or may not send
first.  (e.g. DIGEST-MD5).  In this case, the plugin must
intelligently check for the presence (or absence) of clientin/serverin
data.  Note that the optional client send-first is only possible when the
protocol permits an initial response.

The server send last situation is handled by the plugin intelligently
setting \*serverout when the step function returns SASL_OK.

* For mechanisms
  which never send last (e.g. PLAIN), \*serverout must be set to NULL.
* For
  mechanisms which always send last (e.g. DIGEST-MD5), \*serverout must
  point to the success data.
* For mechanisms in which the server may or
  may not send last (e.g. SRP), \*serverout must be set accordingly.

.. _plugprog-client:

Client Plugins
==============

Client-side mechanism plugins are generally included in the same
plugin with their :ref:`server <plugprog-server>` counterpart, though
this is not a requirement.  They take care of the client-side of the
SASL negotiation.  For a simple example, see the ANONYMOUS plugin.
Client plugins must export ``sasl_client_plug_init`` which returns
a ``sasl_client_plug_t`` in order to load.  The structure has
several functional members and a global context (which applies to
all connections using the plugin).  The important ones are described
briefly here.

mech_new
    Called at the beginning of each connection,
    (on a call to sasl_client_start),

    mech_new does mechanism-specific initialization, and if necessary
    allocates a connection context (which the glue code keeps track
    of for it).  mech_new does not actually send any data to the client,
    it simply allocates the context.

mech_step
    Called from ``sasl_client_start`` and
    ``sasl_client_step``, this function does the actual work of
    the client
    side of the authentication.  If authentication is successful, it
    should return SASL_OK, otherwise it should return a valid SASL
    error code (and call seterror).

    This should also set up the
    oparams structure before returning SASL_OK, including any
    security layer information (in the way of callbacks).

    Note
    that as soon as the client has both the authentication and
    authorization IDs, it MUST call the canon_user function provided
    in its params structure (for both the authentication and
    authorization IDs, with SASL_CU_AUTHID and SASL_CU_AUTHZID
    respectively).

mech_dispose
    Called to dispose of a connection context.
    This is only called when the connection will no longer be used
    (e.g. when ``sasl_dispose`` is called)

mech_free
    Called when the sasl library is shutting down
    (by ``sasl_client_done/sasl_server_done/sasl_done``).
    Intended to free any global state of the plugin.

.. _plugprog-server:

Server Plugins
==============

Server-side mechanism plugins are generally included in the same
plugin with their :ref:`client <plugprog-client>` counterpart, though
this is not a requirement.  They take care of the server-side of the
SASL negotiation, and are generally more complicated than their
client-side counterparts.  For a simple example, see the ANONYMOUS
plugin.

Server plugins must export ``sasl_server_plug_init`` which returns
a ``sasl_server_plug_t`` in order to load.  The structure has
several functional members and a global context (which applies to
all connections using the plugin).  The important ones are described
briefly here.

mech_new
    Called at the beginning of each connection,
    (on a call to sasl_client_start),

    mech_new does mechanism-specific initialization, and if necessary
    allocates a connection context (which the glue code keeps track
    of for it).  mech_new does not actually send any data to the client,
    it simply allocates the context.

mech_step
    Called from ``sasl_server_start`` and
    ``sasl_server_step``, this function does the actual work of
    the server
    side of the authentication.

    If authentication is successful, it
    should return SASL_OK, otherwise it should return a valid SASL
    error code (and call seterror).  This should also set up the
    oparams structure before returning SASL_OK, including any
    security layer information (in the way of callbacks and SSF
    information).

    Also, as soon
    as the mechanism has computed both the authentication and the
    authorization IDs, it MUST call the canon_user function provided
    in the server params structure (for both the authentication and
    authorization IDs, with SASL_CU_AUTHID and SASL_CU_AUTHZID
    respectively).  This action will also fill in its
    propctx, so any auxiliary property *requests*
    (for example, to lookup
    the password) should be done before the request to canonicalize
    the authentication id.  Authorization ID lookups do not occur until
    after the plugin returns success to the SASL library.

    Before returning SASL_OK, ``mech_step`` must fill in the
    oparams fields for which it is responsible, that is, ``doneflag``
    (set to 1 to indicate a complete exchange), ``maxoutbuf``, or
    the maximum output size it can do at once for a security layer,
    ``mech_ssf`` or the supplied SSF of the security layer,
    and ``encode``, ``decode``, ``encode_context``,
    and ``decode_context``,
    which are what the glue code will call on calls to ``sasl_encode``,
    ``sasl_encodev``, and ``sasl_decode``.

mech_dispose
    Called to dispose of a connection context.
    This is only called when the connection will no longer be used
    (e.g. when ``sasl_dispose`` is called)

mech_free
    Called when the sasl library is shutting down
    (by ``sasl_client_done/sasl_server_done/sasl_done``).
    Intended to free any global state of the plugin.

setpass
    Called to set a user's password.  This allows
    mechanisms to support their own internal password or secret
    database.

mech_avail
    Called by the first call to
    ``sasl_listmech``,
    it checks to see if the mechanism is available for the given
    user, and MAY allocate a connection context (thus avoiding
    a call to ``mech_new``).  However it should not do this
    without significant performance benefit as it forces the glue
    code to keep track of extra contexts that may not be used.

User Canonicalization (canon_user) Plugins
==========================================

User Canonicalization plugins allow for nonstandard ways of
canonicalizing the username.  They are subject to the following
requirements:

* They must copy their output into the provided output buffers.
* The output buffers may be the same as the input buffers.
* They must function for the case which is only an authentication
  ID (flags == SASL_CU_AUTHID) or only an authorization ID
  (flags == SASL_CU_AUTHZID) or both
  (flags == SASL_CU_AUTHID | SASL_CU_AUTHZID)

User canonicalization plugins must export a ``sasl_canonuser_init``
function which returns a ``sasl_canonuser_plug_t`` in order
to load successfully.  They must implement at least one of
the ``canon_user_client`` or ``canon_user_server`` members
of the ``sasl_canonuser_plug_t``.  The INTERNAL canon_user plugin
that is inside of the glue code implements both in the same way.

Auxiliary Property (auxprop) Plugins
====================================

Perhaps the most exciting addition in SASLv2, Auxprop plugins
allow for an easy way to perform password and secret lookups (as well
as other information needed for authentication and authorization)
from directory services, and in the same request allow the application
to receive properties that it needs to provide the service.

Auxprop plugins need to export the ``sasl_auxprop_init`` function
and pass back a ``sasl_auxprop_plug_t`` in order to load
successfully.  The sasldb plugin included with the Cyrus SASL
distribution would be a good place to start.

Interfacing with property contexts is extremely well documented in
``prop.h`` and so that is omitted here.  The only important
note is to be sure that you are using the interfaces provided
through the ``sasl_utils_t`` structure and not calling
the functions directly.

To successfully implement an auxprop plugin there is only one
required function to implement, that is the ``auxprop_lookup``
member of the ``sasl_auxprop_plug_t``.  This is called
just after canonicalization of the username, with the canonicalized
username.  It can then do whatever lookups are necessary for any
of the requested auxiliary properties.
