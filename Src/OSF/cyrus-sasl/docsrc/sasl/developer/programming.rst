.. _programming:

==============================
Application Programmer's Guide
==============================

.. note::

    NOTE: This is a work in progress. Any contributions would be
    *very* appreciated.

.. contents::
    :local:

Introduction
============

About this Guide
----------------

This guide gives a tutorial on the use of the Cyrus SASL library
for a client or server application. It complies with versions
including and after 2.0.0. The following pages should only be
considered a guide, not the final word on programming with the
Cyrus SASL library. Consult the header files in the distribution in
the case of ambiguities.

What is SASL?
-------------

SASL stands for Simple Authentication Security Layer and is
defined in :rfc:`2222`. That document is very difficult to understand however and
it should be unnecessary to consult it.

Background
==========

How did the world work before SASL?
-----------------------------------

Before SASL, when a new protocol was written which required
authentication (users proving who they are to an entity), the
protocol had to allow explicitly for each individual authentication
mechanism. There had to be a distinct way to say "I want to log in
with Kerberos V4". There had to be another distinct way to say "I
want to log in with CRAM-MD5". There had to be yet a different way
to say "I want to log in anonymously," and so on. This was
not ideal for both the protocol and application writers.

Additionally, many programmers were not very familiar with
security, so the protocol did support many mechanisms, or worse,
they were supported incorrectly. Moreover, when a new
authentication method was invented the protocol needed to be
modified to support that mechanism.

This system also was not ideal for application writer. She had
to have a special case for each mechanism she wished her
application to support. Also, the mechanisms were difficult to
implement. Even with a good library, an understanding of how the
mechanism worked was still necessary. Finally if an application
used more than one protocol (for example a mail client might use
IMAP, POP, and SMTP) then "Kerberos V4 for IMAP", "Kerberos V4 for
POP", "Kerberos V4 for SMTP", "CRAM MD5 for IMAP", "CRAM-MD5 for
POP", etc... would need to be written. This could quickly create a
huge number of different mechanism-protocol pairs to implement.

SASL to the rescue!
-------------------

SASL hopefully solves all these problems. In practice it makes
many of them easier to deal with.

Protocol designers simply have to support SASL (in particular
:rfc:`2222`). Consequently, any mechanism that supports SASL (just
about anything you would want to use does now) is supported by the
protocol. If a new authentication mechanism is invented the
protocol automatically supports it without any modifications.

Application writers, instead of having to support every
mechanism for every protocol, only need to support SASL for every
protocol. Application writers do not need to understand the
authentication mechanisms at all: the SASL library handles all
that. Also with the Cyrus SASL library if a new mechanism is
invented you do not have rewrite your application at all. You may
not even have to restart your application if it is a long running
process. This is because the Cyrus SASL library loads each
mechanism from a shared library. Simply copying a shared library
into a directory will magically make your application support a new
mechanism.

Cyrus SASL version 2 supports a much improved API over version
1, that allows for much smarter and faster memory allocation for
the mechanisms as well as the applications. It is also provides for
several new types of plugins to allow for greater overall
flexibility. Unfortunately, though similar, this new API is
completely incompatible with the old API, and applications will
need to be rewritten.

Briefly
=======

What is the Cyrus SASL library good for?
----------------------------------------

The Cyrus SASL library is good for applications that wish to use
protocols that support SASL authentication. An non-exhaustive list
of these are: IMAP, SMTP, ACAP, and LDAP. Also if you are making a
proprietary system and wish to support authentication it is a good
way of supporting many different authentication types.

What does the Cyrus SASL library do?
------------------------------------

From a client point of view, the Cyrus SASL library, given a
list of mechanisms the server supports it will decide the best
mechanism to use and tell you what to send to the server at each
step of the authentication. From a server perspective, it handles
authentication requests from clients.

What doesn't the Cyrus SASL library do?
---------------------------------------

The Cyrus SASL library is neither network nor protocol aware. It
is up to the application to send the data over the wire as well as
to send the data in the protocol specific manner. With IMAP this
means putting it in the form: ``+ [base64'ed data]\\r\\n``. LDAP
just sends data in binary via bind requests. The Cyrus SASL library
has utility base64 encode and decode routines to help with
this.

Client-only Section
===================

A typical interaction from the client's perspective
---------------------------------------------------


* A client makes a few calls to initialize SASL.

* Every time the client application makes a new connection it
  should make a new context that is kept for the life of the
  connection.

* Ask the server for the list of supported mechanisms

* Feed this list to the library

* Start the authentication with the mechanism the library
  chose

* The server will return some bytes

* Give these to the library

* The library returns some bytes to the application

* Application sends these bytes over the network

* Repeat the last 4 steps until the server tells you that the
  authentication is completed


How does this look in code
--------------------------

1. Initialize the library
#########################

This is done once, using :saslman:`sasl_client_init(3)`.

.. code-block:: C

        int result;

        /* attempt to start sasl
         * See the section on Callbacks and Interactions for an
         * explanation of the variable callbacks
         */

        result=sasl_client_init(callbacks);

            /* check to see if that worked */
            if (result!=SASL_OK) /* [failure] */

2. Make a new SASL connection
#############################

For every network connection, make a new SASL connection, using
:saslman:`sasl_client_new(3)`:

.. code-block:: C

            /* The SASL context kept for the life of the connection */
            sasl_conn_t *conn;


            /* client new connection */
            result=sasl_client_new("imap",     /* The service we are using */
                       serverFQDN, /* The fully qualified domain
                                                  name of the server we're
                                                  connecting to */
                       NULL, NULL, /* Local and remote IP
                                                  address strings
                                                  (NULL disables mechanisms
                                                   which require this info)*/
                                   NULL,       /* connection-specific
                                                  callbacks */
                       0,          /* security flags */
                       &conn);     /* allocated on success */

            /* check to see if that worked */
            if (result!=SASL_OK) /* [failure] */


3. Get the mechanism list
#########################

Next get the list of SASL mechanisms the server supports. This is
usually done through a capability command. Format the list as a
single string separated by spaces. Feed this string into SASL to
begin the authentication process.

.. code-block:: C

            sasl_interact_t *client_interact=NULL;
            const char *out, *mechusing;
            unsigned outlen;

            do {

              result=sasl_client_start(conn,      /* the same context from
                                                     above */
                       mechlist,  /* the list of mechanisms
                                     from the server */
                       &client_interact, /* filled in if an
                                            interaction is needed */
                       &out,      /* filled in on success */
                       &outlen,   /* filled in on success */
                       &mechusing);

              if (result==SASL_INTERACT)
              {
                 /* [deal with the interactions. See interactions section below] */
              }


           } while (result==SASL_INTERACT); /* the mechanism may ask us to fill
                                               in things many times. result is
                                               SASL_CONTINUE on success */
           if (result!=SASL_CONTINUE) /* [failure] */


Note that you do not need to worry about the allocation and freeing
of the output buffer `out`. This is all handled inside the
mechanism. It is important to note, however, that the output buffer
is not valid after the next call to :saslman:`sasl_client_start(3)` or
:saslman:`sasl_client_step(3)`.

If this is successful send the protocol specific command to
start the authentication process. This may or may not allow for
initial data to be sent (see the documentation of the protocol to
see).

4. Start authentication
#######################

* For IMAP this might look like::

          {tag} "AUTHENTICATE" {mechusing}\r\n
          A01 AUTHENTICATE KERBEROS_V4\r\n

* SMTP looks like::

         "AUTH" {mechusing}[ {out base64 encoded}]
         AUTH DIGEST-MD5 GHGJJGDDFDKHGHJG=


.. _client_authentication_step:

5. Check Results
################

Read what the server sent back. It can be one of three
things:

1. Authentication failure. Authentication process is halted. This
   might look like ``A01 NO Authentication failure`` in IMAP or
   ``501 Failed`` in SMTP. Either retry the authentication or
   abort.

2. Authentication success. We're now successfully authenticated.
   This might look like ``A01 OK Authenticated successful`` in
   IMAP or ``235 Authentication successful`` in SMTP.
   Go :ref:`here <client_authentication_success>`.

3. Another step in the authentication process is necessary. This
   might look like ``+ HGHDS1HAFJ=`` in IMAP or ``334
   PENCeUxFREJoU0NnbmhNWitOMjNGNndAZWx3b29kLmlubm9zb2Z0LmNvbT4=``
   in SMTP. Note it could be an empty string such as ``+ \r\n``
   in IMAP.


Convert the continuation data to binary format (for example, this
may include base64 decoding it). Perform another step in the
authentication using :saslman:`sasl_client_step(3)`:

.. code-block:: C

              do {
                result=sasl_client_step(conn,  /* our context */
                        in,    /* the data from the server */
                        inlen, /* its length */
                        &client_interact,  /* this should be
                                              unallocated and NULL */
                        &out,     /* filled in on success */
                        &outlen); /* filled in on success */

                if (result==SASL_INTERACT)
                {
                   /* [deal with the interactions. See below] */
                }


              } while (result==SASL_INTERACT || result == SASL_CONTINUE);

              if (result!=SASL_OK) /* [failure] */


Format the output (variable out of length outlen) in the protocol
specific manner and send it across the network to the server.

Go :ref:`back to check results <client_authentication_step>` (this process
repeats until authentication either succeeds or fails.

.. _client_authentication_success:

6. Authentication Successful
############################

Before we're done we need to call :saslman:`sasl_client_step(3)` one more
time to make sure the server isn't trying to fool us. Some
protocols include data along with the last step. If so this data
should be used here. If not use a length of zero.

.. code-block:: C

                result=sasl_client_step(conn,  /* our context */
                        in,    /* the data from the server */
                        inlen, /* it's length */
                        &client_interact,  /* this should be unallocated and NULL */
                        &out,     /* filled in on success */
                        &outlen); /* filled in on success */

                if (result!=SASL_OK) /* [failure] */

Congratulations. You have successfully authenticated to the
server.

Don't throw away the SASL connection object (sasl_conn_t \*) yet
though. If a security layer was negotiated you will need it to
encode and decode the data sent over the network.

7. Cleaning up
##############

When you are finally done with connection to server, dispose of
SASL connection using :saslman:`sasl_dispose(3)`:

.. code-block:: C

               sasl_dispose(&conn);


If you are done with SASL forever (application quitting for
example), use :saslman:`sasl_client_done(3)`:

.. code-block:: C

                sasl_client_done();

Or if your application is both a SASL client and a SASL server,
use :saslman:`sasl_done(3)`:

.. code-block:: C

                sasl_done();

But note that applications should be using
:saslman:`sasl_client_done(3)`/:saslman:`sasl_server_done(3)` whenever possible.

Server-only Section
===================

A typical interaction from the server's perspective
---------------------------------------------------

The server makes a few Cyrus SASL calls for initialization. When it
gets a new connection it should make a new context for that
connection immediately. The client may then request a list of
mechanisms the server supports. The client also may request to
authenticate at some point. The client will specify the mechanism
it wishes to use. The server should negotiate this authentication
and keep around the context afterwards for encoding and decoding
the layers.

How does this look in code?
---------------------------

Initialization
##############

This is done once, using :saslman:`sasl_server_init(3)`.
The application name is used for reading configuration information.

.. code-block:: C

    int result;

    /* Initialize SASL */
    result=sasl_server_init(callbacks,      /* Callbacks supported */
                            "TestServer");  /* Name of the application */

:saslman:`sasl_server_new(3)` should be called for each new connection. It should
be called when the socket is accepted.

.. code-block:: C

    sasl_conn_t *conn;
    int result;

    /* Make a new context for this connection */
    result=sasl_server_new("smtp", /* Registered name of service */
            NULL, /* my fully qualified domain name;
                    NULL says use gethostname() */
            NULL, /* The user realm used for password
                    lookups; NULL means default to serverFQDN
                    Note: This does not affect Kerberos */
            NULL, NULL, /* IP Address information strings */
            NULL,   /* Callbacks supported only for this connection */
            0,  /* security flags (security layers are enabled
                   using security properties, separately) */
            &conn);


When a client requests the list of mechanisms supported by the
server, use :saslman:`sasl_listmech(3)`. This particular call might
produce the string: ``{PLAIN, KERBEROS_V4, CRAM-MD5, DIGEST-MD5}``

.. code-block:: C

    result=sasl_listmech(conn,  /* The context for this connection */
             NULL,  /* not supported */
             "{",   /* What to prepend the string with */
             ", ",  /* What to separate mechanisms with */
             "}",   /* What to append to the string */
             &result_string, /* The produced string. */
                         &string_length, /* length of the string */
                         &number_of_mechanisms); /* Number of mechanisms in
                                                the string */


When a client requests to authenticate, use :saslman:`sasl_server_start(3)`:

.. code-block:: C

    int result;
    const char *out;
    unsigned outlen;

    result = sasl_server_start(conn, /* context */
                 mechanism_client_chose,
                 clientin,    /* the optional string the client gave us */
                 clientinlen, /* and it's length */
                 &out, /* The output of the library.
                          Might not be NULL terminated */
                 &outlen);

    if ((result!=SASL_OK) && (result!=SASL_CONTINUE))
      /* failure. Send protocol specific message that says authentication failed */
    else if (result==SASL_OK)
      /* authentication succeeded. Send client the protocol specific message
       to say that authentication is complete */
    else
      /* send data 'out' with length 'outlen' over the network in protocol
       specific format */

When a response is returned by the client. ``clientin`` is the
data from the client decoded from protocol specific format to a
string of bytes of length ``clientinlen``. This step may occur
zero or more times. An application must be able to deal with it
occurring an arbitrary number of times, using :saslman:`sasl_server_step(3)`:

.. code-block:: C

    int result;

    result=sasl_server_step(conn,
                            clientin,      /* what the client gave */
                            clientinlen,   /* it's length */
                            &out,          /* allocated by library on success.
                                              Might not be NULL terminated */
                            &outlen);

    if ((result!=SASL_OK) && (result!=SASL_CONTINUE))
        /* failure. Send protocol specific message that says authentication failed */
    else if (result==SASL_OK)
        /* authentication succeeded. Send client the protocol specific message
           to say that authentication is complete */
    else
        /* send data 'out' with length 'outlen' over the network in protocol
            specific format */


This continues until authentication succeeds. When the connection
is concluded, make a call to :saslman:`sasl_dispose(3)`, as with the
client connection.

Common Section
==============

.. _callbacks:

Callbacks and Interactions
--------------------------

When the application starts and calls :saslman:`sasl_client_init(3)` you must
specify for what data you support callbacks and/or interactions.

These are for the library getting information needed for
authentication from the application. This is needed for things like
authentication name and password. If you do not declare supporting
a callback you will not be able to use mechanisms that need that
data.

A *callback* is for when you have the information before you
start the authentication. The SASL library calls a function you
specify and your function fills in the requested information. For
example if you had the userid of the user already for some reason.

An *interaction* is usually for things you support but will need to ask the user
for (e.g. password). :saslman:`sasl_client_start(3)` or
:saslman:`sasl_client_step(3)` will return :c:macro:`SASL_INTERACT`.  This will
be a list of sasl_interact_t's which contain a human readable string you can
prompt the user with, a possible computer readable string, and a default result.
The nice thing about interactions is you get them all at once so if you had a
GUI application you could bring up a dialog box asking for authentication name
and password together instead of one at a time.

Any memory that is given to the SASL library for the purposes of
callbacks and interactions must persist until the exchange
completes in either success or failure. That is, the data must
persist until :saslman:`sasl_client_start(3)` or
:saslman:`sasl_client_step(3)` returns something other than
:c:macro:`SASL_INTERACT` or :c:macro:`SASL_CONTINUE`.

Memory management
    As in the rest of the SASLv2 API,
    whoever allocates the memory is responsible for freeing it. In
    almost all cases this should be fairly easy to manage, however a
    slight exception where the interaction sasl_interact_t structure is
    allocated and freed by the library, while the results are allocated
    and freed by the application. As noted above, however, the
    results may not be freed until after the exchange completes, in
    either success or failure.

For a detailed description of what each of the callback types
are see the sasl.h file. Here are some brief explanations:

SASL_CB_AUTHNAME
    the name of the user authenticating
SASL_CB_USER
    the name of the user acting for. (for example
    postman delivering mail for tmartin might have an AUTHNAME of
    postman and a USER of tmartin)
SASL_CB_PASS
    password for AUTHNAME
SASL_CB_GETREALM
    Realm of the server

An example of a way to handle callbacks:

.. code-block:: C

   /* callbacks we support. This is a global variable at the
       top of the program */
    static sasl_callback_t callbacks[] = {
    {
      SASL_CB_GETREALM, NULL, NULL  /* we'll just use an interaction if this comes up */
    }, {
      SASL_CB_USER, NULL, NULL      /* we'll just use an interaction if this comes up */
    }, {
      SASL_CB_AUTHNAME, &getauthname_func, NULL /* A mechanism should call getauthname_func
                                                   if it needs the authentication name */
    }, {
      SASL_CB_PASS, &getsecret_func, NULL      /* Call getsecret_func if need secret */
    }, {
      SASL_CB_LIST_END, NULL, NULL
    }
    };


    static int getsecret_func(sasl_conn_t *conn,
      void *context __attribute__((unused)),
      int id,
      sasl_secret_t **psecret)
    {
       /* [ask the user for their secret] */

       /* [allocate psecret and insert the secret] */

      return SASL_OK;
    }

    static int getauthname_func(void *context,
                                int id,
                                const char **result,
                                unsigned *len)
    {
       if (id!=SASL_CB_AUTHNAME) return SASL_FAIL;

       /* [fill in result and len] */

       return SASL_OK;
     }


in the main program somewhere

.. code-block:: C

   sasl_client_init(callbacks);


Security layers
---------------

All is well and good to securely authenticate, but if you don't
have some sort of integrity or privacy layer, anyone can hijack
your TCP session after authentication. If your application has
indicated that it can support a security layer, one might be
negotiated.

To set that you support a security layer, set a security
property structure with ``max_ssf`` set to a non-zero
number:

.. code-block:: C

   sasl_security_properties_t secprops;

   secprops.min_ssf = 0;
   secprops.max_ssf = 256;
   secprops.maxbufsize = /* SEE BELOW */;

   secprops.property_names = NULL;
   secprops.property_values = NULL;
   secprops.security_flags = SASL_SEC_NOANONYMOUS; /* as appropriate */

   sasl_setprop(conn, SASL_SEC_PROPS, &secprops);

The ``secprops`` variable will be copied during the call to
``sasl_setprop``, so you may free its memory immediately. The
SSF stands for "security strength factor" and is a
rough indication of how secure the connection is. A connection
supplying only integrity with no privacy would have an SSF of 1. A
connection secured by 56-bit DES would have an SSF of 56.

To require a security layer, set ``min_ssf`` to the minimum
acceptable security layer strength.

After authentication is successful, you can determine whether or
not a security layer has been negotiated by looking at the SASL_SSF
property:

.. code-block:: C

   const int *ssfp;

   result = sasl_getprop(conn, SASL_SSF, (const **) &ssfp);
   if (result != SASL_OK) {
       /* ??? */
   }
   if (*ssfp &gt; 0) {
       /* yay, we have a security layer! */
   }

If a security layer has been negotiated, your application must
make use of the :saslman:`sasl_encode(3)` and :saslman:`sasl_decode(3)`
calls. All output must be passed through :saslman:`sasl_encode(3)`
before being written to the wire; all input must be passed through
:saslman:`sasl_decode(3)` before being looked at by the application.
Your application must also be prepared to deal with
:saslman:`sasl_decode(3)` not returning any data in the rare case that
the peer application did something strange (by splitting a single
SASL blob into two seperate TCP packets).

The only subtlety dealing with security layers is the maximum size
of data that can be passed through :saslman:`sasl_encode(3)` or
:saslman:`sasl_decode(3)`. This must be limited to make sure that only
a finite amount of data needs to be buffered. The simple rules to
follow:

* Before starting authentication, set ``maxbufsize`` in your
  security properties to be the buffer size that you pass to the
  ``read()`` system call&mdash;that is, the amount of data
  you're prepared to read at any one time.

* After authentication finishes, use :saslman:`sasl_getprop(3)` to
  retrieve the ``SASL_MAXOUTBUF`` value, and call
  :saslman:`sasl_encode(3)` with chunks of data of that size or less.
  :saslman:`sasl_encode(3)` will throw an error if you call it with a
  larger chunk of data, so be careful!

Memory management
    As usual, whoever allocates the memory
    must free it. The SASL library will keep the data returned from
    :saslman:`sasl_encode(3)` until the next call to :saslman:`sasl_encode(3)`
    on that connection. (:saslman:`sasl_decode(3)` results persist until the
    next call to :saslman:`sasl_decode(3)` on that connection.) The
    application must not attempt to free the memory returned from either
    function.

Internally
    * your application sets SASL_SEC_PROPS with the buffer size X of
      the amount of data it will be using to read() from the socket.
    * libsasl passes this number to the mechanism.
    * the mechanism passes this number to the other side. the other
      side gives the corresponding read() size to our side.
    * the mechanism subtracts the overhead of the layers from the
      size retrieved from the other side and returns it to the
      libsasl.
    * libsasl then returns (via SASL_MAXOUTBUF) this number as the
      maximum amount of plaintext material that can be encoded at any one
      time, Y.
    * sasl_encode() enforces the restriction of the length Y.

Example applications that come with the Cyrus SASL library
==========================================================

`sample-client` and `sample-server`
---------------------------------------

The sample client and server included with this distribution were
initially written to help debug mechanisms. They base64 encode all
the data and print it out on standard output.

Make sure that you set the IP addresses, the username, the
authenticate name, and anything else on the command line (some
mechanisms depend on these being present).

Also, sometimes you will receive a ``realm: Information
not available`` message, or similar; this is due to the fact
that some mechanisms do not support realms and therefore never set
it.

Cyrus imapd v2.1.0 or later
---------------------------

The Cyrus IMAP server now incorporates SASLv2 for all its
authentication needs. It is a good example of a fairly large server
application. Also of interest is the prot layer, included in
libcyrus. This is a stdio-like interface that automatically takes
care of layers using a simple ``prot_setsasl()`` call.

Cyrus imapd also sets a ``SASL_CB_PROXY_POLICY`` callback,
which should be of interest to many applications.

`imtest`, from Cyrus 2.1.0 or later
-------------------------------------

:ref:`imtest(3) <cyrusimap:imap-reference-manpages-usercommands-imtest>`
is an application included with Cyrus IMAPd. It is
a very simple IMAP client, but should be of interest to those
writing applications. It also uses the prot layer, but it is easy
to incorporate similar support without using the prot layer.
Likewise, there are other sample client applications that you can
look at including
:ref:`smtptest(3) <cyrusimap:imap-reference-manpages-usercommands-smtptest>`
and :ref:`pop3test(3) <cyrusimap:imap-reference-manpages-usercommands-pop3test>`
in the Cyrus IMAPd distribution, respectively.

Miscellaneous Information
=========================

Empty exchanges
---------------

Some SASL mechanisms intentionally send no data; an application
should be prepared to either send or receive an empty exchange. The
SASL profile for the protocol should define how to send an empty
string; make sure to send an empty string when requested, and when
receiving an empty string make sure that the ``inlength``
passed in is 0.

Note especially that the distinction between the empty string ""
and the lack of a string (NULL) is extremely important in many
cases (most notably, the client-send first scenario), and the
application must ensure that it is passing the correct values to
the SASL library at all times.

Idle
----

While the implementation and the plugins correctly implement the
idle calls, none of them currently do anything.
