.. _authentication_mechanisms:

=========================
Authentication Mechanisms
=========================

Mechanisms
==========

ANONYMOUS
---------

.. todo::
   Content needed here

CRAM-MD5
--------

.. todo::
   Content needed here


DIGEST-MD5
----------

.. todo::
   Content needed here

EXTERNAL
--------

.. todo::
   Content needed here


G2
-----

.. todo::
   Content needed here


GSSAPI
------

Not sure how to get GSSAPI going? Check out our :ref:`GSSAPI configuration guide <gssapi>`.

.. todo::
   Content needed here


GSS-SPEGNO
----------

.. todo::
   Content needed here

KERBEROS_V4
-----------

.. todo::
   Content needed here

LOGIN
-----

.. todo::
   Content needed here

NTLM
----

.. todo::
   Content needed here

OTP
---

  * OTP-MD4
  * OTP-MD5
  * OTP-SHA1

.. todo::
   Content needed here

PASSDSS
-------

  * PASSDSS-3DES-1

.. todo::
   Content needed here

PLAIN
-----

.. todo::
   Content needed here

SCRAM
-----

  * SCRAM-SHA-1(-PLUS)
  * SCRAM-SHA-224(-PLUS)
  * SCRAM-SHA-256(-PLUS)
  * SCRAM-SHA-384(-PLUS)
  * SCRAM-SHA-512(-PLUS)

.. todo::
   Content needed here

SRP
---

  * mda=sha1,rmd160,md5
  * confidentiality=des-ofb,des-ede-ofb,aes-128-ofb,bf-ofb,cast5-ofb,idea-ofb

.. todo::
   Content needed here

Non-SASL Authentication
-----------------------

.. todo::
   Content needed here

----

Summary
=======

This table shows what security flags and features are supported by each
of the mechanisms provided by the Cyrus SASL Library.

+-------------+---------+----------------------------------------------------------------+-----------------------------------------------------------+
|             | MAX SSF | SECURITY PROPERTIES                                            | FEATURES                                                  |
+-------------+         +---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
|             |         | NOPLAIN | NOACTIVE | NODICT | FORWARD | NOANON | CRED | MUTUAL | CLT FIRST | SRV FIRST    | SRV LAST | PROXY | BIND | HTTP |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| ANONYMOUS   | 0       | X       |          |        |         |        |      |        | X         |              |          |       |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| CRAM-MD5    | 0       | X       |          |        |         | X      |      |        |           | X            |          |       |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| DIGEST-MD5  | 128     | X       |          |        |         | X      |      | X      | reauth    | initial auth | X        | X     |      | X    |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| EXTERNAL    | 0       | X       |          | X      |         | X      |      |        | X         |              |          | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| G2          | 56      | X       | X        |        |         | X      |      | X      | X         |              | X        | X     | X    |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| GSSAPI      | 56      | X       | X        |        |         | X      | X    | X      | X         |              |          | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| GSS-SPNEGO  | 56      | X       | X        |        |         | X      | X    | X      | X         |              |          | X     |      | X    |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| KERBEROS_V4 | 56      | X       | X        |        |         | X      |      | X      |           | X            |          | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| LOGIN       | 0       |         |          |        |         | X      | X    |        |           | X            |          |       |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| NTLM        | 0       | X       |          |        |         | X      |      |        | X         |              |          |       |      | X    |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| OTP         | 0       | X       |          |        | X       | X      |      |        | X         |              |          | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| PASSDSS     | 112     | X       | X        | X      | X       | X      | X    | X      | X         |              |          | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| PLAIN       | 0       |         |          |        |         | X      | X    |        | X         |              |          | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| SCRAM       | 0       | X       | X        |        |         | X      |      | X      | X         |              | X        | X     | X    | ?    |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+
| SRP         | 128     | X       | X        | X      | X       | X      |      | X      | X         |              | X        | X     |      |      |
+-------------+---------+---------+----------+--------+---------+--------+------+--------+-----------+--------------+----------+-------+------+------+

..  Helpfully generated  from http://www.tablesgenerator.com/text_tables#

Understanding this table:

Security Properties:

* **MAX SSF** - The maximum Security Strength Factor supported by the mechanism (roughly the number of bits of encryption provided, but may have other meanings, for example an SSF of 1 indicates integrity protection only, no encryption).
* **NOPLAIN** - Mechanism is not susceptable to simple passive (eavesdropping) attack.
* **NOACTIVE** - Protection from active (non-dictionary) attacks during authentication exchange. (Implies MUTUAL).
* **NODICT** - Not susceptable to passive dictionary attack.
* **NOFORWARD** - Breaking one session won't help break the next.
* **NOANON** - Don't permit anonymous logins.
* **CRED** - Mechanism can pass client credentials.
* **MUTUAL** - Supports mutual authentication (authenticates the server to the client)

Features:

* **CLTFIRST** - The client should send first in this mechanism.
* **SRVFIRST** - The server must send first in this mechanism.
* **SRVLAST** - This mechanism supports server-send-last configurations.
* **PROXY** - This mechanism supports proxy authentication.
* **BIND** - This mechanism supports channel binding.
* **HTTP** - This mechanism has a profile for HTTP.

.. toctree::
    :hidden:

    gssapi
