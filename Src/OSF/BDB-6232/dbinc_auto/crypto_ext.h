/* DO NOT EDIT: automatically built by dist/s_include. */
#ifndef	_crypto_ext_h_
#define	_crypto_ext_h_

#if defined(__cplusplus)
extern "C" {
#endif

int __aes_setup(ENV *, DB_CIPHER *);
u_int __aes_adj_size(size_t);
int __aes_close(ENV *, void *);
int __aes_decrypt(ENV *, void *, void *, uint8 *, size_t);
int __aes_encrypt(ENV *, void *, void *, uint8 *, size_t);
int __aes_init(ENV *, DB_CIPHER *);
int __crypto_env_close(ENV *);
int __crypto_env_refresh(ENV *);
int __crypto_algsetup(ENV *, DB_CIPHER *, uint32, int);
int __crypto_decrypt_meta(ENV *, DB *, uint8 *, int);
int __crypto_set_passwd(ENV *, ENV *);
void __crypto_erase_passwd(ENV*, char **, size_t *);
int __db_generate_iv(ENV *, uint32 *);
int __db_rijndaelKeySetupEnc(u32 *, const u8 *, int);
int __db_rijndaelKeySetupDec(u32 *, const u8 *, int);
void __db_rijndaelEncrypt(u32 *, int, const u8 *, u8 *);
void __db_rijndaelDecrypt(u32 *, int, const u8 *, u8 *);
void __db_rijndaelEncryptRound(const u32 *, int, u8 *, int);
void __db_rijndaelDecryptRound(const u32 *, int, u8 *, int);
int __db_makeKey(keyInstance *, int, int, char *);
int __db_cipherInit(cipherInstance *, int, char *);
int __db_blockEncrypt(cipherInstance *, keyInstance *, uint8 *, size_t, uint8 *);
int __db_padEncrypt(cipherInstance *, keyInstance *, uint8 *, int, uint8 *);
int __db_blockDecrypt(cipherInstance *, keyInstance *, uint8 *, size_t, uint8 *);
int __db_padDecrypt(cipherInstance *, keyInstance *, uint8 *, int, uint8 *);
int __db_cipherUpdateRounds(cipherInstance *, keyInstance *, uint8 *, int, uint8 *, int);

#if defined(__cplusplus)
}
#endif
#endif /* !_crypto_ext_h_ */
