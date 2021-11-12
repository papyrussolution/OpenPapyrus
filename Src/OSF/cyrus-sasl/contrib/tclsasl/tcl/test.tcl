#!/bin/sh
# the following restarts using tclsh \
exec tclsh "$0" "$@"

# test the Tcl wrapper for Cyrus SASL


#
# load the library
#

load .libs/libtclsasl.so


if {[llength $argv] > 0} {
    set mechanisms [list [lindex $argv 0]]
    set argv [lrange $argv 1 end]
}
if {[llength $argv] > 0} {
    set pass [lindex $argv 0]
    set argv [lrange $argv 1 end]
}


#
# globals
#

global errorCode


global client server

foreach prop [sasl::info setprops] {
    set client($prop) ""
    set server($prop) ""
}

# edit these as you wish

array set server [list \
    silent              1                               \
    service             example                         \
    fqdn                ""                              \
    auth_external       client@example.com              \
    ssf_external        1                               \
    sec_props           [list min_ssf 0 max_ssf 128]    \
]

array set client [list \
    silent              0                               \
    service             example                         \
    fqdn                example.com                     \
    mechanisms          digest-md5                      \
    user                ""                              \
    language            en-US                           \
    authname            fred                            \
    cnonce              ""                              \
    pass                "secret"                        \
    getrealm            ""                              \
    auth_external       server@example.com              \
    ssf_external        1                               \
    sec_props           [list min_ssf 0 max_ssf 255]    \
]

if {[info exists mechanisms]} {
    set client(mechanisms) $mechanisms
}
if {[info exists pass]} {
    set client(pass) $pass
}


#
# print out available commands and options
#

puts stdout "initializing package...\n"
puts -nonewline stdout [format "%15.15s:" "commands"]
set i 0
foreach p [lsort -dictionary [info commands ::sasl::*]] {
    puts -nonewline stdout [format "    %-15.15s" \
                                   [string range $p 8 end]]
    if {[expr [incr i]%3] == 0} {
        puts -nonewline stdout [format "\n%15.15s " ""]
    }
}
puts stdout "\n"


foreach p [lsort -dictionary [sasl::info]]  {
    puts -nonewline stdout [format "%15.15s:" $p]
    set q [sasl::info $p]
    set i 0
    while {1} {
        puts -nonewline stdout [format "    %-15.15s" [lindex $q 0]]
        if {[llength [set q [lrange $q 1 end]]] == 0} {
            break
        }
        if {[expr [incr i]%3] == 0} {
            puts -nonewline stdout [format "\n%15.15s " ""]
        }
    }
    puts stdout "\n"
}
puts stdout ""


#
# callbacks
#

proc sasl_log {data} {
    array set params $data

    puts stdout "logging"
    foreach {k v} $data {
        puts stdout [format "%11.11s %s" $k $v]
    }
    puts stdout ""
}


#
# server-side callbacks
#

proc server_callback {id data} {
    global server

    array set params $data

    if {!$server(silent)} {
        puts stdout "looking for $id"
        foreach {k v} $data {
            puts stdout [format "%11.11s %s" $k $v]
        }
    }

    switch -- $id {
        getopt {
            if {![info exists params(plugin)]} {
                set params(plugin) ""
            }
            switch -- $params(plugin)/$params(option) {
                /auto_transition
                    -
                /canon_user_plugin
                    -
                /mech_list
                    -
		/sasldb_path
		    -
                OTP/opiekeys {
                }

                default {
                    set value ""
                }
            }
        }

        verifyfile
            -
        proxy
            -
        checkpass {
            set value 0
        }
    }

    if {!$server(silent)} {
        if {[info exists value]} {
            puts stdout [format "%11.11s %s" returning $value]
        }
        puts stdout ""
    }

    return $value
}


#
# timing
#

proc ticks {now then} {
    set elapsed [expr $now-$then]

    if {[set x [string length $elapsed]] <= 6} {
        return $elapsed
    }

    return [format %s.%s [string range $elapsed 0 [expr $x-7]] \
                         [string range $elapsed [expr $x-6] end]]
}


#
# init the server-side code
#

puts stdout "initializing server...\n"
sasl::server_init -callbacks [list [list log sasl_log]]

set callbacks [list [list log sasl_log]]
foreach id [list getopt verifyfile proxy checkpass] {
    lappend callbacks [list $id "server_callback $id"]
}
set args {}
if {[string length $server(fqdn)] > 0} {
    lappend args -serverFQDN $server(fqdn)
}

set server(token) \
    [eval [list sasl::server_new -service    $server(service) \
                                 -callbacks  $callbacks       \
                                 -flags      [list success_data]] $args]

puts -nonewline stdout [format "%15.15s:" "operations"]
set i 0
foreach p [lsort -dictionary [$server(token) -operation info]] {
    puts -nonewline stdout [format "    %-15.15s" $p]
    if {[expr [incr i]%3] == 0} {
        puts -nonewline stdout [format "\n%15.15s " ""]
    }
}
puts stdout "\n\n"


set hitP 0
foreach prop [lsort -dictionary [sasl::info setprops]] {
    if {[string length [set value $server($prop)]] > 0} {
        if {!$hitP} {
            puts stdout "setting server properties...\n"
        }
        puts stdout "    set $prop to $value"
        $server(token) -operation setprop \
                       -property  $prop   \
                       -value     $value
        set hitP 1
    }
}
if {$hitP} {
    puts stdout "\n"
}


puts -nonewline stdout [format "%15.15s:" "mechanisms"]
set i 0
foreach p [lsort -dictionary [set mechlist [$server(token) -operation list]]] {
    puts -nonewline stdout [format "    %-15.15s" $p]
    if {[expr [incr i]%3] == 0} {
        puts -nonewline stdout [format "\n%15.15s " ""]
    }
}
puts stdout "\n\n"

set server(silent) 0


#
# client-side callbacks
#

proc client_callback {data} {
    global client

    array set params $data

    if {!$client(silent)} {
        puts stdout "looking for $params(id)"
        foreach {k v} $data {
            puts stdout [format "%11.11s %s" $k $v]
        }
    }

    switch -- $params(id) {
        pass {
            set value $client(pass)
        }

        getrealm {
            if {(![info exists params(available)]) \
                    || ([lsearch -exact $params(available) \
                                 $client(getrealm)] >= 0)} {
                set value $client(getrealm)
            }
        }
    }

    if {!$client(silent)} {
        if {[info exists value]} {
            puts stdout [format "%11.11s %s" returning $value]
        }
        puts stdout ""
    }

    return $value
}

proc client_interact {data} {
    global client

    array set params $data
    set id $params(id)

    catch { set value $params(default) }

    puts stdout "asking for  $id"
    foreach {k v} $data {
        if {![string compare $k id]} {
            continue
        }
        puts stdout [format "%11.11s %s" $k $v]
    }

    if {[string length $client($id)] > 0} {
        set value $client($id)
    }

    if {[info exists value]} {
        puts stdout [format "%11.11s %s" "returning" $value]
    } else {
        puts stdout [format "%11.11s %s" "nothing" "to return"]
    }
    puts stdout ""

# if value isn't defined, that's ok...
    return $value
}


# init the client-side code

puts stdout "initializing client...\n"
sasl::client_init -callbacks [list [list log sasl_log]]

set callbacks [list [list log sasl_log]]
foreach id [list user language authname cnonce] {
    if {[string length $client($id)] > 0} {
        lappend callbacks $id
    }
}
if {[string length $client(pass)] > 0} {
    foreach id [list pass echoprompt noechoprompt] {
        lappend callbacks [list $id client_callback]
    }
}
set id getrealm
if {[string length $client($id)] > 0} {
    lappend callbacks [list $id client_callback]
}

set client(token) [sasl::client_new -service    $client(service) \
                                    -serverFQDN $client(fqdn)    \
                                    -callbacks  $callbacks       \
                                    -flags      [list success_data]]

puts -nonewline stdout [format "%15.15s:" "operations"]
set i 0
foreach p [lsort -dictionary [$client(token) -operation info]] {
    puts -nonewline stdout [format "    %-15.15s" $p]
    if {[expr [incr i]%3] == 0} {
        puts -nonewline stdout [format "\n%15.15s " ""]
    }
}
puts stdout "\n\n"


set hitP 0
foreach prop [lsort -dictionary [sasl::info setprops]] {
    if {[string length [set value $client($prop)]] > 0} {
        if {!$hitP} {
            puts stdout "setting client properties...\n"
        }
        puts stdout "    set $prop to $value"
        $client(token) -operation setprop \
                       -property  $prop   \
                       -value     $value
        set hitP 1
    }
}
if {$hitP} {
    puts stdout "\n"
}


puts -nonewline stdout [format "%15.15s:" "mechanisms"]
set i 0
foreach p [lsort -dictionary [sasl::mechanisms]] {
    puts -nonewline stdout [format "    %-15.15s" $p]
    if {[expr [incr i]%3] == 0} {
        puts -nonewline stdout [format "\n%15.15s " ""]
    }
}
puts stdout "\n\n"


puts stdout "starting client...\n"
flush stdout

if {[string length $client(mechanisms)] > 0} {
    set mechlist $client(mechanisms)
}

set clicks [clock clicks]
set client(code) [catch {
    $client(token) -operation  start     \
                   -mechanisms $mechlist \
                   -interact   client_interact
} result]
puts stdout "     elapsed: [ticks [clock clicks] $clicks] msec"

switch -- $client(code) {
    0 {
        puts stdout "          OK: $result"
    }

    4 {
        puts stdout "    CONTINUE: $result"
    }

    default {
        puts stdout "        code: $client(code), $errorCode"
        return
    }
}
puts stdout "\n"
flush stdout


puts stdout "starting server...\n"
flush stdout

array set params $result

set clicks [clock clicks]
set server(code) [catch {
    $server(token) -operation start              \
                   -mechanism $params(mechanism) \
                   -input     $params(output)
} result]
puts stdout "     elapsed: [ticks [clock clicks] $clicks] msec"

switch -- $server(code) {
    0 {
        puts stdout "         OK: $result"
    }

    4 {
        puts stdout "    CONTINUE: $result"
    }

    default {
        puts stdout "        code: $server(code), $errorCode"
    }
}
puts stdout "\n"
flush stdout


while {($server(code) == 4) || ($client(code) == 4)} {
    puts stdout "stepping client...\n"
    flush stdout

    set clicks [clock clicks]
    set client(code) [catch {
        $client(token) -operation step    \
                       -input     $result \
                       -interact  client_interact
    } result]
    puts stdout "     elapsed: [ticks [clock clicks] $clicks] msec"
    
    switch -- $client(code) {
        0 {
            puts stdout "          OK: $result"
        }
    
        4 {
            puts stdout "    CONTINUE: $result"
        }
    
        default {
            puts stdout "        code: $client(code), $errorCode"
            return
        }
    }
    puts stdout "\n\n"
    flush stdout

    if {($client(code) == 0) && ([string length $result] == 0)} {
	break
    }

    puts stdout "stepping server...\n"
    flush stdout

    set clicks [clock clicks]
    set server(code) [catch {
        $server(token) -operation step    \
                       -input     $result
    } result]
    puts stdout "     elapsed: [ticks [clock clicks] $clicks] msec"

    switch -- $server(code) {
        0 {
            puts stdout "          OK: $result"
        }
    
        4 {
            puts stdout "    CONTINUE: $result"
        }
    
        default {
            puts stdout "        code: $server(code), $errorCode"
        }
    }
    puts stdout "\n\n"
    flush stdout
}


switch -- $client(code)/$server(code) {
    4/0 {
	puts stderr "server done, but client isn't...\n"
	return
    }

    0/4 {
	puts stderr "client done, but server isn't...\n"
	return
    }
}


rename sasl_log ""


puts stdout "getting client properties...\n"
foreach prop [lsort -dictionary [sasl::info getprops]] {
    if {[set code [catch { $client(token) -operation getprop \
                                          -property  $prop } result]]} {
        set result "ERROR: $result"
    }
    puts stdout [format "%14.14s: %s" $prop $result]
}
puts stdout ""

if {[set code [catch { $client(token) -operation errdetail } result]]} {
        set result "ERROR: $result"
}
puts stdout [format "%14.14s: %s" errdetail $result]
puts stdout "\n"


puts stdout "getting server properties...\n"
foreach prop [lsort -dictionary [sasl::info getprops]] {
    if {[set code [catch { $server(token) -operation getprop \
                                          -property  $prop } result]]} {
        set result "ERROR: $result"
    }
    puts stdout [format "%14.14s: %s" $prop $result]
}
puts stdout ""

if {[set code [catch { $server(token) -operation errdetail } result]]} {
        set result "ERROR: $result"
}
puts stdout [format "%14.14s: %s" errdetail $result]
puts stdout "\n"


puts stdout "shutting down"
sasl::done

puts stdout "\n"


puts stdout "if you see this, the test was passed..."
