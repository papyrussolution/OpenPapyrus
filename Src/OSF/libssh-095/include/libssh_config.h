//
//
//
#ifndef __LIBSSH_CONFIG_H // {
#define __LIBSSH_CONFIG_H

#define PACKAGE "LIBSSH" /* Name of package */
#define VERSION "0.9.5" /* Version number of package */
#define SYSCONFDIR "${SYSCONFDIR}"
#define BINARYDIR "${BINARYDIR}"
#define SOURCEDIR "${SOURCEDIR}"
#define GLOBAL_BIND_CONFIG "${GLOBAL_BIND_CONFIG}" /* Global bind configuration file path */
#define GLOBAL_CLIENT_CONFIG "${GLOBAL_CLIENT_CONFIG}" /* Global client configuration file path */

/************************** HEADER FILES *************************/

//#define HAVE_ARGP_H 1 /* Define to 1 if you have the <argp.h> header file. */
//#define HAVE_ARPA_INET_H 0 /* Define to 1 if you have the <aprpa/inet.h> header file. */
//#define HAVE_GLOB_H 1 /* Define to 1 if you have the <glob.h> header file. */
//#define HAVE_VALGRIND_VALGRIND_H 1 /* Define to 1 if you have the <valgrind/valgrind.h> header file. */
//#define HAVE_PTY_H 1 /* Define to 1 if you have the <pty.h> header file. */
//#define HAVE_UTMP_H 1 /* Define to 1 if you have the <utmp.h> header file. */
//#define HAVE_UTIL_H 1 /* Define to 1 if you have the <util.h> header file. */
//#define HAVE_LIBUTIL_H 1 /* Define to 1 if you have the <libutil.h> header file. */
#define HAVE_SYS_TIME_H 1 /* Define to 1 if you have the <sys/time.h> header file. */
//#define HAVE_SYS_UTIME_H 1 /* Define to 1 if you have the <sys/utime.h> header file. */
#define HAVE_IO_H 1 /* Define to 1 if you have the <io.h> header file. */
//#define HAVE_TERMIOS_H 1 /* Define to 1 if you have the <termios.h> header file. */
#define HAVE_UNISTD_H 1 /* Define to 1 if you have the <unistd.h> header file. */
#define HAVE_STDINT_H 1 /* Define to 1 if you have the <stdint.h> header file. */
#define _SSIZE_T_DEFINED
#define HAVE_OPENSSL_AES_H 1 /* Define to 1 if you have the <openssl/aes.h> header file. */
//#define HAVE_WSPIAPI_H 1 /* Define to 1 if you have the <wspiapi.h> header file. */
#define HAVE_OPENSSL_BLOWFISH_H 1 /* Define to 1 if you have the <openssl/blowfish.h> header file. */
#define HAVE_OPENSSL_DES_H 1 /* Define to 1 if you have the <openssl/des.h> header file. */
#define HAVE_OPENSSL_ECDH_H 1 /* Define to 1 if you have the <openssl/ecdh.h> header file. */
#define HAVE_OPENSSL_EC_H 1 /* Define to 1 if you have the <openssl/ec.h> header file. */
#define HAVE_OPENSSL_ECDSA_H 1 /* Define to 1 if you have the <openssl/ecdsa.h> header file. */
//#define HAVE_PTHREAD_H 1 /* Define to 1 if you have the <pthread.h> header file. */
#define HAVE_OPENSSL_ECC 1 /* Define to 1 if you have eliptic curve cryptography in openssl */
//#define HAVE_GCRYPT_ECC 1 /* Define to 1 if you have eliptic curve cryptography in gcrypt */
#define HAVE_ECC 1 /* Define to 1 if you have eliptic curve cryptography */
#define HAVE_DSA 1 /* Define to 1 if you have DSA */
#define HAVE_GLOB_GL_FLAGS_MEMBER 1 /* Define to 1 if you have gl_flags as a glob_t sturct member */
#define HAVE_OPENSSL_ED25519 1 // Define to 1 if you have OpenSSL with Ed25519 support 
#define HAVE_OPENSSL_X25519  1 // Define to 1 if you have OpenSSL with X25519 support 

/*************************** FUNCTIONS ***************************/

#define HAVE_OPENSSL_EVP_AES_CTR 1 /* Define to 1 if you have the `EVP_aes128_ctr' function. */
#define HAVE_OPENSSL_EVP_AES_CBC 1 /* Define to 1 if you have the `EVP_aes128_cbc' function. */
#define HAVE_OPENSSL_EVP_AES_GCM 1 /* Define to 1 if you have the `EVP_aes128_gcm' function. */
#define HAVE_OPENSSL_CRYPTO_THREADID_SET_CALLBACK 1 /* Define to 1 if you have the `CRYPTO_THREADID_set_callback' function. */
#define HAVE_OPENSSL_CRYPTO_CTR128_ENCRYPT 1 /* Define to 1 if you have the `CRYPTO_ctr128_encrypt' function. */
#define HAVE_OPENSSL_EVP_CIPHER_CTX_NEW 1 /* Define to 1 if you have the `EVP_CIPHER_CTX_new' function. */
// (since openssl 3.0) #define HAVE_OPENSSL_EVP_KDF_CTX_NEW_ID 1 // Define to 1 if you have the `EVP_KDF_CTX_new_id' function. 
// @v11.6.2 #define HAVE_OPENSSL_FIPS_MODE 1 // Define to 1 if you have the `FIPS_mode' function
#define HAVE_OPENSSL_EVP_DIGESTSIGN 1 /* Define to 1 if you have the `EVP_DigestSign' function. */
#define HAVE_OPENSSL_EVP_DIGESTVERIFY 1 /* Define to 1 if you have the `EVP_DigestVerify' function. */
#define HAVE_OPENSSL_IA32CAP_LOC 1 /* Define to 1 if you have the `OPENSSL_ia32cap_loc' function. */
#define HAVE_SNPRINTF 1 /* Define to 1 if you have the `snprintf' function. */
#define HAVE__SNPRINTF 1 /* Define to 1 if you have the `_snprintf' function. */
#define HAVE__SNPRINTF_S 1 /* Define to 1 if you have the `_snprintf_s' function. */
#define HAVE_VSNPRINTF 1 /* Define to 1 if you have the `vsnprintf' function. */
#define HAVE__VSNPRINTF 1 /* Define to 1 if you have the `_vsnprintf' function. */
#define HAVE__VSNPRINTF_S 1 /* Define to 1 if you have the `_vsnprintf_s' function. */
#define HAVE_ISBLANK 1 /* Define to 1 if you have the `isblank' function. */
#define HAVE_STRNCPY 1 /* Define to 1 if you have the `strncpy' function. */
//#define HAVE_STRNDUP 1 // Define to 1 if you have the `strndup' function. 
#define HAVE_CFMAKERAW 1 /* Define to 1 if you have the `cfmakeraw' function. */
#define HAVE_GETADDRINFO 1 /* Define to 1 if you have the `getaddrinfo' function. */
//#define HAVE_POLL 1 /* Define to 1 if you have the `poll' function. */
#define HAVE_SELECT 1 /* Define to 1 if you have the `select' function. */
//#define HAVE_CLOCK_GETTIME 1 // Define to 1 if you have the `clock_gettime' function. 
//#define HAVE_NTOHLL 1 // Define to 1 if you have the `ntohll' function. 
//#define HAVE_HTONLL 1 // Define to 1 if you have the `htonll' function. 
#define HAVE_STRTOULL 1 /* Define to 1 if you have the `strtoull' function. */
#define HAVE___STRTOULL 1 /* Define to 1 if you have the `__strtoull' function. */
#define HAVE__STRTOUI64 1 /* Define to 1 if you have the `_strtoui64' function. */
//#define HAVE_GLOB 1 // Define to 1 if you have the `glob' function. 
#define HAVE_EXPLICIT_BZERO 1 /* Define to 1 if you have the `memzero' function. */
#define HAVE_MEMSET_S 1 /* Define to 1 if you have the `memset_s' function. */
#define HAVE_SECURE_ZERO_MEMORY 1 /* Define to 1 if you have the `SecureZeroMemory' function. */
//#define HAVE_CMOCKA_SET_TEST_FILTER 1 /* Define to 1 if you have the `cmocka_set_test_filter' function. */

/*************************** LIBRARIES ***************************/

#define HAVE_LIBCRYPTO 1 // Define to 1 if you have the `crypto' library (-lcrypto). 
//#define HAVE_LIBGCRYPT 1 /* Define to 1 if you have the `gcrypt' library (-lgcrypt). */
//#define HAVE_LIBMBEDCRYPTO 1 /* Define to 1 if you have the 'mbedTLS' library (-lmbedtls). */
//#define HAVE_PTHREAD 1 /* Define to 1 if you have the `pthread' library (-lpthread). */
//#define HAVE_CMOCKA 1 /* Define to 1 if you have the `cmocka' library (-lcmocka). */

/**************************** OPTIONS ****************************/

//#define HAVE_GCC_THREAD_LOCAL_STORAGE 1
#define HAVE_MSC_THREAD_LOCAL_STORAGE 1
//#define HAVE_FALLTHROUGH_ATTRIBUTE 1
#define HAVE_UNUSED_ATTRIBUTE 1

//#define HAVE_CONSTRUCTOR_ATTRIBUTE 1
//#define HAVE_DESTRUCTOR_ATTRIBUTE 1

#define HAVE_GCC_VOLATILE_MEMORY_PROTECTION 1

#define HAVE_COMPILER__FUNC__ 1
#define HAVE_COMPILER__FUNCTION__ 1
//#define HAVE_GCC_BOUNDED_ATTRIBUTE 1
#define WITH_GSSAPI 1 /* Define to 1 if you want to enable GSSAPI */
#define WITH_ZLIB 1 /* Define to 1 if you want to enable ZLIB */
#define WITH_SFTP 1 /* Define to 1 if you want to enable SFTP */
#define WITH_SERVER 1 /* Define to 1 if you want to enable server support */
#define WITH_GEX 1 /* Define to 1 if you want to enable DH group exchange algorithms */
#define WITH_BLOWFISH_CIPHER 1 /* Define to 1 if you want to enable blowfish cipher support */
#define DEBUG_CRYPTO 1 /* Define to 1 if you want to enable debug output for crypto functions */
#define DEBUG_PACKET 1 /* Define to 1 if you want to enable debug output for packet functions */
#define WITH_PCAP 1 /* Define to 1 if you want to enable pcap output support (experimental) */
#define DEBUG_CALLTRACE 1 /* Define to 1 if you want to enable calltrace debug output */
//#define WITH_NACL 1 /* Define to 1 if you want to enable NaCl support */

/*************************** ENDIAN *****************************/

/* Define WORDS_BIGENDIAN to 1 if your processor stores words with the most
   significant byte first (like Motorola and SPARC, unlike Intel). */
//#define WORDS_BIGENDIAN 0
#endif // } __LIBSSH_CONFIG_H