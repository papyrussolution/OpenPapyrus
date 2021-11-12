#!/usr/bin/env python
#
# This script changes a SASLDB2 realm to another one.
#
# Written by Sander Steffann <sander@steffann.nl>
# No rights reserved: do with this script as you please.
#
# Usage: change-sasldb2-realm.py <orig-realm> <new-realm>
# where <orig-realm> and <new-realm> are case-sensitive.
#
# !WARNING!  This script opens /etc/sasldb2 directly, without going through
#            the official API. If the file format changes this script breaks.
#
# The following file-format for sasldb2 is assumed: a BSD-DB hash-file with a
# key in the format "<data> \x00 <realm> \x00 <data>". The <data> parts are
# copied without modification. The values corresponding to the keys are copied
# without modification too.
#
# To be safe, this script opens /etc/sasldb2 read-only and writes the result
# to a new file: /etc/sasldb2.new. If this file exists this script will
# overwrite it. Don't overwrite your /etc/sasldb2 file until you are sure the
# results in /etc/sasldb2.new are what you want.
#
# This script uses the bsddb module provided with python. It is assumed that
# the DB library used by python is the same as the one used by SASL2. If this
# is not the case the script will abort with an error when opening sasldb2.
#

import bsddb
import sys

# Check command-line arguments
if len(sys.argv) != 3:
	print "Usage: %s <orig-realm> <new-realm>" % (sys.argv[0],)
	sys.exit(1)

# Extract the command-line arguments into properly named variables
orig_realm = sys.argv[1]
new_realm = sys.argv[2]

# Open the BSD-DB files
orig_db = bsddb.hashopen('/etc/sasldb2', 'r')
new_db = bsddb.hashopen('/etc/sasldb2.new', 'n')

# Loop over all the keys in the original sasldb2
for orig_key in orig_db.keys():
	# Split the key into the three components
	elements = orig_key.split('\x00')
	if len(elements) != 3:
		raise ValueError, "The structure of /etc/sasldb2 is not as expected!"
	
	# Compare the current realm with the realm we want to replace
	if elements[1] == orig_realm:
		# Replace the current realm with the new one
		new_key = '\x00'.join([elements[0], new_realm, elements[2]])
	else:
		# Wrong realm: Don't touch the key
		new_key = orig_key

	# Write the key with the corresponding value in the new DB
	new_db[new_key] = orig_db[orig_key]

# Close the DB files
orig_db.close()
new_db.close()
