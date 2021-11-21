/*
 * pki_ed25519 .c - PKI infrastructure using ed25519
 *
 * This file is part of the SSH Library
 *
 * Copyright (c) 2014 by Aris Adamantiadis
 *
 * The SSH Library is free software; you can redistribute it and/or modify
 * it under the terms of the GNU Lesser General Public License as published by
 * the Free Software Foundation; either version 2.1 of the License, or (at your
 * option) any later version.
 *
 * The SSH Library is distributed in the hope that it will be useful, but
 * WITHOUT ANY WARRANTY; without even the implied warranty of MERCHANTABILITY
 * or FITNESS FOR A PARTICULAR PURPOSE.  See the GNU Lesser General Public
 * License for more details.
 *
 * You should have received a copy of the GNU Lesser General Public License
 * along with the SSH Library; see the file COPYING.  If not, write to
 * the Free Software Foundation, Inc., 59 Temple Place - Suite 330, Boston,
 * MA 02111-1307, USA.
 */
#include <libssh-internal.h>
#pragma hdrstop

int pki_key_generate_ed25519(ssh_key key)
{
	int rc;
	key->ed25519_privkey = (uint8 *)SAlloc::M(sizeof(ed25519_privkey));
	if(key->ed25519_privkey == NULL) {
		goto error;
	}
	key->ed25519_pubkey = (uint8 *)SAlloc::M(sizeof(ed25519_pubkey));
	if(key->ed25519_pubkey == NULL) {
		goto error;
	}
	rc = crypto_sign_ed25519_keypair(key->ed25519_pubkey, key->ed25519_privkey);
	if(rc != 0) {
		goto error;
	}
	return SSH_OK;
error:
	ZFREE(key->ed25519_privkey);
	ZFREE(key->ed25519_pubkey);
	return SSH_ERROR;
}

int pki_ed25519_sign(const ssh_key privkey, ssh_signature sig, const uchar * hash, size_t hlen)
{
	int rc;
	uint64_t dlen = 0;
	uint8 * buffer = (uint8 *)SAlloc::M(hlen + ED25519_SIG_LEN);
	if(buffer == NULL) {
		return SSH_ERROR;
	}
	rc = crypto_sign_ed25519(buffer, &dlen, hash, hlen, privkey->ed25519_privkey);
	if(rc != 0) {
		goto error;
	}
	/* This shouldn't happen */
	if(dlen - hlen != ED25519_SIG_LEN) {
		goto error;
	}
	sig->ed25519_sig = (ed25519_signature *)SAlloc::M(ED25519_SIG_LEN);
	if(sig->ed25519_sig == NULL) {
		goto error;
	}
	memcpy(sig->ed25519_sig, buffer, ED25519_SIG_LEN);
	ZFREE(buffer);
	return SSH_OK;
error:
	ZFREE(buffer);
	return SSH_ERROR;
}

int pki_ed25519_verify(const ssh_key pubkey, ssh_signature sig, const uchar * hash, size_t hlen)
{
	uint64_t mlen = 0;
	uint8 * buffer;
	uint8 * buffer2;
	int rc;
	if(pubkey == NULL || sig == NULL || hash == NULL || sig->ed25519_sig == NULL) {
		return SSH_ERROR;
	}
	buffer = (uint8 *)SAlloc::M(hlen + ED25519_SIG_LEN);
	if(buffer == NULL) {
		return SSH_ERROR;
	}
	buffer2 = (uint8 *)SAlloc::M(hlen + ED25519_SIG_LEN);
	if(buffer2 == NULL) {
		goto error;
	}
	memcpy(buffer, sig->ed25519_sig, ED25519_SIG_LEN);
	memcpy(buffer + ED25519_SIG_LEN, hash, hlen);
	rc = crypto_sign_ed25519_open(buffer2, &mlen, buffer, hlen + ED25519_SIG_LEN, pubkey->ed25519_pubkey);
	memzero(buffer, hlen + ED25519_SIG_LEN);
	memzero(buffer2, hlen);
	ZFREE(buffer);
	ZFREE(buffer2);
	if(rc == 0) {
		return SSH_OK;
	}
	else {
		return SSH_ERROR;
	}
error:
	ZFREE(buffer);
	ZFREE(buffer2);
	return SSH_ERROR;
}
