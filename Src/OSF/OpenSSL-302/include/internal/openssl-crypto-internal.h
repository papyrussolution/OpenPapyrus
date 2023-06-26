// OPENSSL-CRYPTO-INTERNAL.H
//
#ifndef __OPENSSL_CRYPTO_INTERNAL_H
#define __OPENSSL_CRYPTO_INTERNAL_H

#include <slib.h>
//#include <malloc.h>
#define STATIC_LEGACY // @sobolev
// @v11.7.7 Следующие макро-определения перенесены из файлов *.props {
#define ZLIB
#define L_ENDIAN
#define _CRT_SECURE_NO_DEPRECATE
#define _WINSOCK_DEPRECATED_NO_WARNINGS
#define OPENSSL_BUILDING_OPENSSL
#define OPENSSL_SYS_WIN32
#define OPENSSL_PIC
#define OPENSSL_CPUID_OBJ
#define OPENSSL_IA32_SSE2
#define OPENSSL_BN_ASM_GF2m
#define OPENSSL_BN_ASM_MONT
#define OPENSSL_BN_ASM_PART_WORDS
#define DES_ASM
#define RC4_ASM
#define SHA1_ASM
#define SHA256_ASM
#define SHA512_ASM
#define MD5_ASM
#define RMD160_ASM
#define AES_ASM
#define VPAES_ASM
#define WHIRLPOOL_ASM
#define CMLL_ASM
#define GHASH_ASM
#define ECP_NISTZ256_ASM
#define POLY1305_ASM
// } @v11.7.7 
#include "internal/deprecated.h" // Low level APIs are deprecated for public use, but still ok for internal use.
#include "internal/crypto/bio_local.h"
#include "internal/cryptlib.h"

#endif // !__OPENSSL_CRYPTO_INTERNAL_H
