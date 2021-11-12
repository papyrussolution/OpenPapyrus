#!/usr/local/bin/perl

# $Id: saslauthd-unix.pl,v 1.2 2003/04/28 20:15:10 rjs3 Exp $

# This is a sample perl script for communicating with saslauthd running
# on a unix socket.  Simply call the function with 4 parameters and you're
# set (userid, password, service, realm or undef)

use Socket;
use strict;

sub saslauthd_verify_password {
	my $SASLAUTHD_PATH = "/var/run/saslauthd/mux";

	my $userid = shift || die "no userid";
	my $passwd = shift || die "no password";
	my $service = shift || die "no service";
	my $realm = shift;

	my $u_len = length $userid;
	my $p_len = length $passwd;
	my $s_len = length $service;
	my $r_len = (defined($realm) ? length $realm : 0);

	if($u_len + $p_len + $s_len + $r_len + 30 > 8192) {
	    warn "request too long in saslauthd_verify_password";
	    return undef;
	}

	my $request = pack "na".$u_len."na".$p_len."na".$s_len."na".$r_len,
		   $u_len, $userid, $p_len, $passwd, $s_len, $service,
		   $r_len, (defined($realm) ? $realm : "");

	socket(SOCK, PF_UNIX, SOCK_STREAM, 0) || die "socket: $!";
	connect(SOCK, sockaddr_un($SASLAUTHD_PATH)) || die "connect: $!";
	
	my $len = length $request;
	my $offset = 0;
	while($len) {
	    my $written = syswrite SOCK, $request, $len, $offset;
	    die "System write error: $!\n" unless defined $written;
	    $len -= $written;
	    $offset += $written;
	}

	# Read back reply
	my $buf;
	$len = sysread SOCK, $buf, 8192;
	die "System read error: $!\n" unless defined $len;

	my $response;
	my $size = unpack("n", $buf);

	die "Bad saslauthd response" unless defined $size;
	($size, $response) = unpack("na".$size, $buf);

	if($response =~ /^OK/) {
	    return 1;
	} else {
	    return 0;
	}	
}

