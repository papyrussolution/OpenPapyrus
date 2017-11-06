/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_hmac_ext_h_
#define	_hmac_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

void __db_chksum(void *, uint8 *, size_t, uint8 *, uint8 *);
void __db_derive_mac(uint8 *, size_t, uint8 *);
int __db_check_chksum(ENV *, void *, DB_CIPHER *, uint8 *, void *, size_t, int);
void __db_SHA1Transform(uint32 *, unsigned char *);
void __db_SHA1Init(SHA1_CTX *);
void __db_SHA1Update(SHA1_CTX *, unsigned char *, size_t);
void __db_SHA1Final(unsigned char *, SHA1_CTX *);

#if defined(__cplusplus)
}
#endif
#endif /* !_hmac_ext_h_ */
