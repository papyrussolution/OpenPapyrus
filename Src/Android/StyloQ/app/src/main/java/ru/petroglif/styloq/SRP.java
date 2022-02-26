// SRP.java
// Copyright (c) A.Sobolev 2021, 2022
//
package ru.petroglif.styloq;

import java.io.ByteArrayOutputStream;
import java.io.IOException;
import java.math.BigInteger;
import java.security.MessageDigest;
import java.security.NoSuchAlgorithmException;
import java.util.Arrays;

public class SRP {
	private final static boolean SRP_NETWORK_DEBUG = false; // @debug
	enum HashAlgorithm {
		SRP_SHA1,
		SRP_SHA224,
		SRP_SHA256,
		SRP_SHA384,
		SRP_SHA512
	}
	enum NGType {
		SRP_NG_1024,
		SRP_NG_2048,
		SRP_NG_4096,
		SRP_NG_8192,
		SRP_NG_CUSTOM
	}
	static class NGConstant {
		NGConstant(NGType ngType, String n_hex, String g_hex)
		{
			switch(ngType) {
				case SRP_NG_CUSTOM:
					break;
				case SRP_NG_1024:
					n_hex = "EEAF0AB9ADB38DD69C33F80AFA8FC5E86072618775FF3C0B9EA2314C9C256576D674DF7496" +
							"EA81D3383B4813D692C6E0E0D5D8E250B98BE48E495C1D6089DAD15DC7D7B46154D6B6CE8E" +
							"F4AD69B15D4982559B297BCF1885C529F566660E57EC68EDBC3C05726CC02FD4CBF4976EAA" +
							"9AFD5138FE8376435B9FC61D2FC0EB06E3";
					g_hex = "2";
					break;
				case SRP_NG_2048:
					n_hex = "AC6BDB41324A9A9BF166DE5E1389582FAF72B6651987EE07FC3192943DB56050A37329CBB4" +
							"A099ED8193E0757767A13DD52312AB4B03310DCD7F48A9DA04FD50E8083969EDB767B0CF60" +
							"95179A163AB3661A05FBD5FAAAE82918A9962F0B93B855F97993EC975EEAA80D740ADBF4FF" +
							"747359D041D5C33EA71D281E446B14773BCA97B43A23FB801676BD207A436C6481F1D2B907" +
							"8717461A5B9D32E688F87748544523B524B0D57D5EA77A2775D2ECFA032CFBDBF52FB37861" +
							"60279004E57AE6AF874E7303CE53299CCC041C7BC308D82A5698F3A8D0C38271AE35F8E9DB" +
							"FBB694B5C803D89F7AE435DE236D525F54759B65E372FCD68EF20FA7111F9E4AFF73";
					g_hex = "2";
					break;
				case SRP_NG_4096:
					n_hex = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"+
							"8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"+
							"302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"+
							"A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE6"+
							"49286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8"+
							"FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D"+
							"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C"+
							"180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"+
							"3995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D"+
							"04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7D"+
							"B3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D226"+
							"1AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200C"+
							"BBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFC"+
							"E0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B26"+
							"99C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB"+
							"04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2"+
							"233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127"+
							"D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C934063199"+
							"FFFFFFFFFFFFFFFF";
					g_hex = "5";
					break;
				case SRP_NG_8192:
					n_hex = "FFFFFFFFFFFFFFFFC90FDAA22168C234C4C6628B80DC1CD129024E08"+
							"8A67CC74020BBEA63B139B22514A08798E3404DDEF9519B3CD3A431B"+
							"302B0A6DF25F14374FE1356D6D51C245E485B576625E7EC6F44C42E9"+
							"A637ED6B0BFF5CB6F406B7EDEE386BFB5A899FA5AE9F24117C4B1FE6"+
							"49286651ECE45B3DC2007CB8A163BF0598DA48361C55D39A69163FA8"+
							"FD24CF5F83655D23DCA3AD961C62F356208552BB9ED529077096966D"+
							"670C354E4ABC9804F1746C08CA18217C32905E462E36CE3BE39E772C"+
							"180E86039B2783A2EC07A28FB5C55DF06F4C52C9DE2BCBF695581718"+
							"3995497CEA956AE515D2261898FA051015728E5A8AAAC42DAD33170D"+
							"04507A33A85521ABDF1CBA64ECFB850458DBEF0A8AEA71575D060C7D"+
							"B3970F85A6E1E4C7ABF5AE8CDB0933D71E8C94E04A25619DCEE3D226"+
							"1AD2EE6BF12FFA06D98A0864D87602733EC86A64521F2B18177B200C"+
							"BBE117577A615D6C770988C0BAD946E208E24FA074E5AB3143DB5BFC"+
							"E0FD108E4B82D120A92108011A723C12A787E6D788719A10BDBA5B26"+
							"99C327186AF4E23C1A946834B6150BDA2583E9CA2AD44CE8DBBBC2DB"+
							"04DE8EF92E8EFC141FBECAA6287C59474E6BC05D99B2964FA090C3A2"+
							"233BA186515BE7ED1F612970CEE2D7AFB81BDD762170481CD0069127"+
							"D5B05AA993B4EA988D8FDDC186FFB7DC90A6C08F4DF435C934028492"+
							"36C3FAB4D27C7026C1D4DCB2602646DEC9751E763DBA37BDF8FF9406"+
							"AD9E530EE5DB382F413001AEB06A53ED9027D831179727B0865A8918"+
							"DA3EDBEBCF9B14ED44CE6CBACED4BB1BDB7F1447E6CC254B33205151"+
							"2BD7AF426FB8F401378CD2BF5983CA01C64B92ECF032EA15D1721D03"+
							"F482D7CE6E74FEF6D55E702F46980C82B5A84031900B1C9E59E7C97F"+
							"BEC7E8F323A97A7E36CC88BE0F1D45B7FF585AC54BD407B22B4154AA"+
							"CC8F6D7EBF48E1D814CC5ED20F8037E0A79715EEF29BE32806A1D58B"+
							"B7C5DA76F550AA3D8A1FBFF0EB19CCB1A313D55CDA56C9EC2EF29632"+
							"387FE8D76E3C0468043E8F663F4860EE12BF2D5B0B7474D6E694F91E"+
							"6DBE115974A3926F12FEE5E438777CB6A932DF8CD8BEC4D073B931BA"+
							"3BC832B68D9DD300741FA7BF8AFC47ED2576F6936BA424663AAB639C"+
							"5AE4F5683423B4742BF1C978238F16CBE39D652DE3FDB8BEFC848AD9"+
							"22222E04A4037C0713EB57A81A23F0C73473FC646CEA306B4BCBC886"+
							"2F8385DDFA9D4B7FA2C087E879683303ED5BDD3A062B3CF5B3A278A6"+
							"6D2A13F83F44F82DDF310EE074AB6A364597E899A0255DC164F31CC5"+
							"0846851DF9AB48195DED7EA1B1D510BD7EE74D73FAF36BC31ECFA268"+
							"359046F4EB879F924009438B481C6CD7889A002ED5EE382BC9190DA6"+
							"FC026E479558E4475677E9AA9E3050E2765694DFC81F56E880B96E71"+
							"60C980DD98EDD3DFFFFFFFFFFFFFFFFF";
					g_hex = "13";
					break;
			}
			N = new BigInteger(n_hex, 16);
			g = new BigInteger(g_hex, 16);
			/*
			NGConstant * ng = (NGConstant *)SAlloc::M(sizeof(NGConstant) );
			ng->N = BN_new();
			ng->g = BN_new();
			if(!ng || !ng->N || !ng->g)
				return 0;
			if(ng_type != SlSRP::SRP_NG_CUSTOM) {
				n_hex = global_Ng_constants[ng_type].n_hex;
				g_hex = global_Ng_constants[ng_type].g_hex;
			}
			BN_hex2bn(&ng->N, n_hex);
			BN_hex2bn(&ng->g, g_hex);
			return ng;
			 */
		}
		public BigInteger N;
		public BigInteger g;
	}
	static class VerifierBase {
		VerifierBase(HashAlgorithm alg, NGType ngType, String n_hex, String g_hex)
		{
			Alg = alg;
			NgC = new NGConstant(ngType, n_hex, g_hex);
		}
		int IsAuthenticated()
		{
			return Authenticated;
		}
		HashAlgorithm Alg;
		int    Authenticated;
		NGConstant NgC;
		byte [] M;
		byte [] H_AMK;
		byte [] SessionKey;
	}
	static BigInteger H_nn(HashAlgorithm alg, final BigInteger n1, final BigInteger n2) throws StyloQException
	{
		BigInteger result = null;
		try {
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			baos.write(SLib.BigNumberToBytesWithoutLZ(n1));
			baos.write(SLib.BigNumberToBytesWithoutLZ(n2));
			result = new BigInteger(+1, SRP.Hash(alg, baos.toByteArray()));
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return result;
		/*
		uchar buff[SHA512_DIGEST_LENGTH];
		int len_n1 = BN_num_bytes(n1);
		int len_n2 = BN_num_bytes(n2);
		int nbytes = len_n1 + len_n2;
		uchar * bin = (uchar *)SAlloc::M(nbytes);
		if(!bin)
			return 0;
		BN_bn2bin(n1, bin);
		BN_bn2bin(n2, bin + len_n1);
		hash(alg, bin, nbytes, buff);
		SAlloc::F(bin);
		return BN_bin2bn(buff, hash_length(alg), NULL);
		 */
	}
	static byte [] HashNum(HashAlgorithm alg, BigInteger n/*, uchar * dest*/) throws StyloQException
	{
		return SRP.Hash(alg, SLib.BigNumberToBytesWithoutLZ(n));
		/*int nbytes = BN_num_bytes(n);
		uchar * bin    = (uchar *)SAlloc::M(nbytes);
		if(bin) {
			BN_bn2bin(n, bin);
			hash(alg, bin, nbytes, dest);
			SAlloc::F(bin);
		}*/
	}
	static byte [] calculate_M(HashAlgorithm alg, final NGConstant ngc,
	   	final byte [] I, BigInteger s, BigInteger A, BigInteger B, byte [] K) throws StyloQException
	{
		byte [] result = null;
		byte [] hash_ngc_n = HashNum(alg, ngc.N);
		byte [] hash_ngc_g = HashNum(alg, ngc.g);
		byte [] hash_I = Hash(alg, I);
		assert(hash_ngc_n.length == hash_ngc_g.length);
		byte [] hash_xor = new byte[hash_ngc_g.length];
		for(int i = 0; i < hash_ngc_n.length; i++) {
			hash_xor[i] = (byte)(hash_ngc_n[i] ^ hash_ngc_g[i]);
		}
		try {
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			baos.write(hash_xor);
			baos.write(hash_I);
			baos.write(SLib.BigNumberToBytesWithoutLZ(s));
			baos.write(SLib.BigNumberToBytesWithoutLZ(A));
			baos.write(SLib.BigNumberToBytesWithoutLZ(B));
			baos.write(K);
			result = SRP.Hash(alg, baos.toByteArray());
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return result;
		/*
		uchar H_N[SHA512_DIGEST_LENGTH];
		uchar H_g[SHA512_DIGEST_LENGTH];
		uchar H_I[SHA512_DIGEST_LENGTH];
		uchar H_xor[SHA512_DIGEST_LENGTH];
		HashCTX ctx;
		int i = 0;
		int hash_len = hash_length(alg);
		hash_num(alg, ng->N, H_N);
		hash_num(alg, ng->g, H_g);
		hash(alg, (const uchar *)I, strlen(I), H_I);
		for(i = 0; i < hash_len; i++)
			H_xor[i] = H_N[i] ^ H_g[i];
		hash_init(alg, &ctx);
		hash_update(alg, &ctx, H_xor, hash_len);
		hash_update(alg, &ctx, H_I,   hash_len);
		update_hash_n(alg, &ctx, s);
		update_hash_n(alg, &ctx, A);
		update_hash_n(alg, &ctx, B);
		hash_update(alg, &ctx, K, hash_len);
		hash_final(alg, &ctx, dest);
		 */
	}
	static byte [] calculate_H_AMK(HashAlgorithm alg, BigInteger A, byte [] M, byte [] K) throws StyloQException
	{
		byte [] result = null;
		try {
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			baos.write(SLib.BigNumberToBytesWithoutLZ(A));
			baos.write(M);
			baos.write(K);
			result = SRP.Hash(alg, baos.toByteArray());
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return result;
		/*
		HashCTX ctx;
		hash_init(alg, &ctx);
		update_hash_n(alg, &ctx, A);
		hash_update(alg, &ctx, M, hash_length(alg) );
		hash_update(alg, &ctx, K, hash_length(alg) );
		hash_final(alg, &ctx, dest);
		 */
	}
	static class Verifier extends VerifierBase {
		//Verifier(HashAlgorithm alg, NGType ngType, const char * pUserName,
			//const SBinaryChunk & rS, const SBinaryChunk & rV, const SBinaryChunk & rA, SBinaryChunk & rB, const char * pNHex, const char * pGHex);
		byte [] GetBytesB() { return BytesB; }
		Verifier(HashAlgorithm alg, NGType ngType, String n_hex, String g_hex,
			final String userName, final byte [] S, final byte [] V, final byte [] A) throws StyloQException
		{
			super(alg, ngType, n_hex, g_hex);
			UserName = userName;
			// SRP-6a safety check
			BigInteger _A = new BigInteger(+1, A);
			//BigInteger _S = new BigInteger(S);
			BigInteger tmp1 = _A.mod(NgC.N);
			if(tmp1.compareTo(BigInteger.ZERO) != 0) {
				BigInteger __b = null;
				if(SRP_NETWORK_DEBUG) {
					__b = new BigInteger("263f95a17dae6f22c28d847a9f87d6ab3f6147e2734816697e119a92c2c59ee9", 16);
				}
				else {
					__b = SLib.GenerateRandomBigNumber(256);
				}
				BigInteger __k = H_nn(alg, NgC.N, NgC.g);
				// B = kv + g^b
				tmp1 = __k.multiply(new BigInteger(+1, V));
				BigInteger tmp2 = NgC.g.modPow(__b, NgC.N); //BN_mod_exp(tmp2, ng->g, p_b, ng->N, ctx);
				BigInteger _B = tmp1.add(tmp2).mod(NgC.N); //BN_mod_add(p_B, tmp1, tmp2, ng->N, ctx);
				//p_u = H_nn(alg, p_A, p_B);
				BigInteger __u = H_nn(alg, _A, _B);
				// S = (A *(v^u)) ^ b
				BigInteger _V = new BigInteger(+1, V);
				tmp1 = _V.modPow(__u, NgC.N); //BN_mod_exp(tmp1, p_v, p_u, ng->N, ctx);
				tmp2 = _A.multiply(tmp1); //BN_mul(tmp2, p_A, tmp1, ctx);
				BigInteger _S = tmp2.modPow(__b, NgC.N); //BN_mod_exp(p_S, tmp2, p_b, ng->N, ctx);
				SessionKey = HashNum(alg, _S);
				//calculate_M(alg, ng, M, UserName.cptr(), p_s, p_A, p_B, SessionKey);
				byte [] user_name_bytes = UserName.getBytes();
				BigInteger __s = new BigInteger(+1, S);
				M = calculate_M(alg, NgC, user_name_bytes, __s, _A, _B, SessionKey);
				//calculate_H_AMK(alg, H_AMK, p_A, M, SessionKey);
				H_AMK = calculate_H_AMK(alg, _A, M, SessionKey);
				BytesB = SLib.BigNumberToBytesWithoutLZ(_B);
			}
		}
		byte [] VerifySession(byte [] userM) // returns bytesHAMK
		{
			if(Arrays.equals(M, userM)) {
				Authenticated = 1;
				return H_AMK;
			}
			else {
				Authenticated = 0;
				return null;
			}
			/*
			if(memcmp(M, pUserM, hash_length(HashAlg)) == 0) {
				Authenticated = 1;
				*ppBytesHAMK = H_AMK;
			}
			else
				*ppBytesHAMK = NULL;
			 */
		}
		String UserName;
		byte [] BytesB;
	}
	static class User extends VerifierBase {
		User(HashAlgorithm alg, NGType ngType, String userName, byte [] password, String n_hex, String g_hex)
		{
			super(alg, ngType, n_hex, g_hex);
			UserName = userName;
			Password = password;
		}
		final String GetUserName() { return UserName; }
		byte [] StartAuthentication() // returns copy of bytesA
		{
			if(SRP_NETWORK_DEBUG) {
				a = new BigInteger("17e3d491cde2cfeb1922d52ed1790f11a372fe6532f63e9e91fb5587de80c353", 16);
			}
			else {
				a = SLib.GenerateRandomBigNumber(256);
			}
			A = NgC.g.modPow(a, NgC.N);
			BytesA = SLib.BigNumberToBytesWithoutLZ(A);
			return BytesA;
		}
		byte [] ProcessChallenge(byte [] s, byte [] b) throws StyloQException
		{
			byte [] M = null;
			BigInteger _B = new BigInteger(+1, b);
			BigInteger bn_salt = new BigInteger(+1, s);
			BigInteger u = H_nn(Alg, A, _B); // u = H_nn(HashAlg, static_cast<BIGNUM *>(P_A), B);
			BigInteger x = calculate_x(Alg, bn_salt, UserName, Password); //x = calculate_x(HashAlg, s, P_UserName, P_Password, PasswordLen);
			BigInteger k = H_nn(Alg, NgC.N, NgC.g); //k = H_nn(HashAlg, P_Ng->N, P_Ng->g);
			if(!_B.equals(BigInteger.ZERO) && !u.equals(BigInteger.ZERO)) { // SRP-6a safety check
				BigInteger v = NgC.g.modPow(x, NgC.N); // BN_mod_exp(v, P_Ng->g, x, P_Ng->N, ctx);
				// S = (B - k*(g^x)) ^ (a + ux)
				BigInteger tmp1 = u.multiply(x); //BN_mul(tmp1, u, x, ctx);
				BigInteger tmp2 = a.add(tmp1); // BN_add(tmp2, static_cast<BIGNUM *>(P_a), tmp1); // tmp2 = (a + ux)
				tmp1 = NgC.g.modPow(x, NgC.N); //BN_mod_exp(tmp1, P_Ng->g, x, P_Ng->N, ctx);
				BigInteger tmp3 = k.multiply(tmp1); //BN_mul(tmp3, k, tmp1, ctx); // tmp3 = k*(g^x)
				tmp1 = _B.subtract(tmp3); //BN_sub(tmp1, B, tmp3); // tmp1 = (B - K*(g^x))
				S = tmp1.modPow(tmp2, NgC.N); //BN_mod_exp(static_cast<BIGNUM *>(P_S), tmp1, tmp2, P_Ng->N, ctx);
				SessionKey = HashNum(Alg, S); //hash_num(HashAlg, static_cast<BIGNUM *>(P_S), SessionKey);
				//calculate_M(HashAlg, P_Ng, M, P_UserName, s, static_cast<BIGNUM *>(P_A), B, SessionKey);
				M = calculate_M(Alg, NgC, UserName.getBytes(), bn_salt, A, _B, SessionKey);
				//calculate_H_AMK(HashAlg, H_AMK, static_cast<BIGNUM *>(P_A), M, SessionKey);
				H_AMK = calculate_H_AMK(Alg, A, M, SessionKey);
			}
			else {
				//M = new byte[1];
			}
			return M;
			/*
			BIGNUM * s = BN_bin2bn(static_cast<const uchar *>(rS.PtrC()), rS.Len(), NULL);
			BIGNUM * B = BN_bin2bn(static_cast<const uchar *>(rB.PtrC()), rB.Len(), NULL);
			BIGNUM * u = 0;
			BIGNUM * x = 0;
			BIGNUM * k = 0;
			BIGNUM * v = BN_new();
			BIGNUM * tmp1 = BN_new();
			BIGNUM * tmp2 = BN_new();
			BIGNUM * tmp3 = BN_new();
			BN_CTX * ctx  = BN_CTX_new();
			//*pLenM = 0;
			//*ppBytesM = 0;
			rM.Z();
			THROW(s && B && v && tmp1 && tmp2 && tmp3 && ctx);
			u = H_nn(HashAlg, static_cast<BIGNUM *>(P_A), B);
			THROW(u);
			x = calculate_x(HashAlg, s, P_UserName, P_Password, PasswordLen);
			THROW(x);
			k = H_nn(HashAlg, P_Ng->N, P_Ng->g);
			THROW(k);
			// SRP-6a safety check
			if(!BN_is_zero(B) && !BN_is_zero(u) ) {
				BN_mod_exp(v, P_Ng->g, x, P_Ng->N, ctx);
				// S = (B - k*(g^x)) ^ (a + ux)
				BN_mul(tmp1, u, x, ctx);
				BN_add(tmp2, static_cast<BIGNUM *>(P_a), tmp1); // tmp2 = (a + ux)
				BN_mod_exp(tmp1, P_Ng->g, x, P_Ng->N, ctx);
				BN_mul(tmp3, k, tmp1, ctx); // tmp3 = k*(g^x)
				BN_sub(tmp1, B, tmp3); // tmp1 = (B - K*(g^x))
				BN_mod_exp(static_cast<BIGNUM *>(P_S), tmp1, tmp2, P_Ng->N, ctx);
				hash_num(HashAlg, static_cast<BIGNUM *>(P_S), SessionKey);
				calculate_M(HashAlg, P_Ng, M, P_UserName, s, static_cast<BIGNUM *>(P_A), B, SessionKey);
				calculate_H_AMK(HashAlg, H_AMK, static_cast<BIGNUM *>(P_A), M, SessionKey);
				rM.Put(M, hash_length(HashAlg));
				//*ppBytesM = M;
				//ASSIGN_PTR(pLenM, hash_length(HashAlg));
			}
			else {
				rM.Z();
				//*ppBytesM = NULL;
				//ASSIGN_PTR(pLenM, 0);
			}
//cleanup_and_exit:
			CATCH
			;
			ENDCATCH
			BN_free(s);
			BN_free(B);
			BN_free(u);
			BN_free(x);
			BN_free(k);
			BN_free(v);
			BN_free(tmp1);
			BN_free(tmp2);
			BN_free(tmp3);
			BN_CTX_free(ctx);
			 */
		}
		boolean VerifySession(byte [] hamk)
		{
			if(Arrays.equals(hamk, H_AMK)) {
				Authenticated = 1;
				return true;
			}
			else {
				Authenticated = 0;
				return false;
			}
		}
		//NGConstant * P_ng;
		BigInteger a;
		BigInteger A;
		BigInteger S;
		byte [] BytesA;
		String UserName;
		byte [] Password;
	}
	static byte [] Hash(HashAlgorithm alg, final byte [] d) throws StyloQException
	{
		byte [] result = null;
		try {
			String digalg = null;
			switch(alg) {
				case SRP_SHA1:
					digalg = "SHA-1";
					break;
				case SRP_SHA224:
					digalg = "SHA-224";
					break;
				case SRP_SHA256:
					digalg = "SHA-256";
					break;
				case SRP_SHA384:
					digalg = "SHA-384";
					break;
				case SRP_SHA512:
					digalg = "SHA-512";
					break;
			}
			MessageDigest digest = MessageDigest.getInstance(digalg);
			digest.update(d);
			result = digest.digest();
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		}
		return result;
	}
	static BigInteger H_ns(HashAlgorithm alg, BigInteger n, final byte [] bytes) throws StyloQException
	{
		BigInteger result = null;
		try {
			//ByteBuffer bin_buffer = ByteBuffer.allocate(n.bitLength() / 8 + bytes.length);
			ByteArrayOutputStream baos = new ByteArrayOutputStream();
			baos.write(SLib.BigNumberToBytesWithoutLZ(n));
			baos.write(bytes);
			byte[] _hash = Hash(alg, baos.toByteArray());
			//BigInteger result = new BigInteger(bin);
			result = new BigInteger(+1, _hash);
		} catch(IOException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_IO, exn.getMessage());
		}
		return result;
		/*
		uchar buff[SHA512_DIGEST_LENGTH];
		int len_n  = BN_num_bytes(n);
		int nbytes = len_n + len_bytes;
		uchar * bin = (uchar *)SAlloc::M(nbytes);
		if(!bin)
			return 0;
		BN_bn2bin(n, bin);
		memcpy(bin + len_n, bytes, len_bytes);
		hash(alg, bin, nbytes, buff);
		SAlloc::F(bin);
		return BN_bin2bn(buff, hash_length(alg), NULL);
		*/
	}
	static BigInteger calculate_x(HashAlgorithm alg, BigInteger salt, String username, final byte [] password) throws StyloQException
	{
		BigInteger result = null;
		try {
			String digalg = null;
			switch(alg) {
				case SRP_SHA1:
					digalg = "SHA-1";
					break;
				case SRP_SHA224:
					digalg = "SHA-224";
					break;
				case SRP_SHA256:
					digalg = "SHA-256";
					break;
				case SRP_SHA384:
					digalg = "SHA-384";
					break;
				case SRP_SHA512:
					digalg = "SHA-512";
					break;
			}
			MessageDigest digest = MessageDigest.getInstance(digalg);
			byte[] user_name_bytes = username.getBytes();
			byte[] symb_colon_bytes = ":".getBytes();
			digest.update(user_name_bytes);
			digest.update(symb_colon_bytes);
			digest.update(password);
			byte[] ucp_hash = digest.digest();
			result = H_ns(alg, salt, ucp_hash);
		} catch(NoSuchAlgorithmException exn) {
			new StyloQException(ppstr2.PPERR_JEXN_NOSUCHALG, exn.getMessage());
		}
		return result;
	}
	static class SaltedVerificationKey {
		SaltedVerificationKey(BigInteger s, BigInteger v)
		{
			Salt = SLib.BigNumberToBytesWithoutLZ(s);
			V = SLib.BigNumberToBytesWithoutLZ(v);
		}
		public byte [] GetSalt() { return Salt; }
		public byte [] GetV() { return V; }
		byte [] Salt;
		byte [] V;
	}
	static SaltedVerificationKey CreateSaltedVerificationKey(HashAlgorithm alg, NGType ngType, String userName,
		final byte [] password, String nHex, String gHex) throws StyloQException
	{
		BigInteger s = null;
		BigInteger v = null;
		s = SLib.GenerateRandomBigNumber(32);
		BigInteger x = calculate_x(alg, s, userName, password);
		NGConstant ng = new NGConstant(ngType, nHex, gHex);
		v = ng.g.modPow(x, ng.N);
		return new SaltedVerificationKey(s, v);
		//S.limit(s.bitLength() / 8);
		//S.clear();
		//S.put(s.toByteArray());
		//V.limit(v.bitLength() / 8);
		//V.clear();
		//V.put(v.toByteArray());
		/*
			int    ok = 1;
			BIGNUM * s = BN_new();
			BIGNUM * v = BN_new();
			BIGNUM * x = 0;
			BN_CTX * ctx = BN_CTX_new();
			NGConstant * ng  = new_ng(ng_type, n_hex, g_hex);
			THROW(s && v && ctx && ng);
			init_random(); // Only happens once
			BN_rand(s, 32, -1, 0);
			x = calculate_x(alg, s, pUserName, pPassword, lenPassword);
			THROW(x);
			BN_mod_exp(v, ng->g, x, ng->N, ctx);
			{
				THROW(rS.Ensure(BN_num_bytes(s)));
				THROW(rV.Ensure(BN_num_bytes(v)));
				BN_bn2bin(s, static_cast<uchar *>(rS.Ptr()));
				BN_bn2bin(v, static_cast<uchar *>(rV.Ptr()));
			}
			CATCHZOK
			delete_ng(ng);
			BN_free(s);
			BN_free(v);
			BN_free(x);
			BN_CTX_free(ctx);
			return ok;
		 */
	}
	static boolean Test() throws StyloQException
	{
		boolean ok = true;
		final HashAlgorithm alg = HashAlgorithm.SRP_SHA1;
		final NGType ng_type = NGType.SRP_NG_8192;
		String username = "testuser";
		String password = "password";
		String n_hex = new String("");
		String g_hex = new String("");
		//ByteBuffer __s = ByteBuffer.allocate(256); // S
		//ByteBuffer __v = ByteBuffer.allocate(256); // V
		SaltedVerificationKey svk = CreateSaltedVerificationKey(alg, ng_type, username, password.getBytes(), n_hex, g_hex);
		for(int i = 0; i < 100; i++) {
			boolean local_ok = false;
			User usr = new User(alg, ng_type, username, password.getBytes(), n_hex, g_hex);
			byte[] __a = usr.StartAuthentication();
			//
			// User -> Host: (username, __a)
			//
			Verifier ver = new Verifier(alg, ng_type, n_hex, g_hex, username, svk.GetSalt(), svk.GetV(), __a);
			if(SLib.GetLen(ver.GetBytesB()) > 0) {
				// Host -> User: (bytes_s, bytes_B)
				//usr.ProcessChallenge(__s, __b, __m);
				byte[] __m = usr.ProcessChallenge(svk.GetSalt(), ver.GetBytesB());
				if(__m != null) {
					// User -> Host: (bytes_M)
					//ver.VerifySession(bytes_M, &bytes_HAMK);
					byte[] __hamk = ver.VerifySession(__m);
					if(__hamk != null) {
						// Host -> User: (HAMK)
						if(usr.VerifySession(__hamk)) {
							local_ok = true;
							ok = true;
						}
						else {
							//printf("Server authentication failed!\n");
							ok = false;
						}
					}
					else {
						//printf("User authentication failed!\n");
						ok = false;
					}
					/*
					ver.VerifySession(static_cast<const uchar *>(__m.PtrC()), &bytes_HAMK);
					if(!bytes_HAMK) {
						printf("User authentication failed!\n");
						ok = 0;
					}
					else {
						// Host -> User: (HAMK)
						usr.VerifySession(bytes_HAMK);
						if(!usr.IsAuthenticated()) {
							printf("Server authentication failed!\n");
							ok = 0;
						}
					}
					 */
				}
				else {
					//printf("User SRP-6a safety check violation!\n");
					ok = false;
				}
			}
			else {
				//printf("Verifier SRP-6a safety check violated!\n");
				ok = false;
			}
		}
		return ok;
		/*
			const uchar * bytes_HAMK = 0;
			SBinaryChunk __s; // S
			SBinaryChunk __v; // V
			SBinaryChunk __b; // B
			SBinaryChunk __m; // M
			SBinaryChunk __a; // A
			uint64 duration;
			const char * username = "testuser";
			const char * password = "password";
			const char * n_hex = 0;
			const char * g_hex = 0;
			SlSRP::HashAlgorithm alg = TEST_HASH;
			SlSRP::NGType ng_type = SlSRP::SRP_NG_8192;    //TEST_NG;
			if(ng_type == SlSRP::SRP_NG_CUSTOM) {
				n_hex = test_n_hex;
				g_hex = test_g_hex;
			}
			const int pw_len = sstrleni(password);
			SlSRP::CreateSaltedVerificationKey2(alg, ng_type, username, (const uchar *)password, pw_len, __s, __v, n_hex, g_hex);
			uint64 start = get_usec();
			for(uint i = 0; i < NITER; i++) {
				//uchar * p_bytes_A = 0;
				//int    len_A = 0;
				char * p_auth_username = 0;
				//usr = srp_user_new(alg, ng_type, username, (const uchar *)password, pw_len, n_hex, g_hex);
				SlSRP::User usr(alg, ng_type, username, (const uchar *)password, pw_len, n_hex, g_hex);
				//srp_user_start_authentication(&usr, &auth_username, &bytes_A, &len_A);
				//usr.StartAuthentication(&p_auth_username, &p_bytes_A, &len_A);
				usr.StartAuthentication(&p_auth_username, __a);
				// User -> Host: (username, bytes_A)
				SlSRP::Verifier ver(alg, ng_type, username, __s, __v, __a, __b, n_hex, g_hex);
				if(!__b.Len()) {
					printf("Verifier SRP-6a safety check violated!\n");
					ok = 0;
				}
				else {
					// Host -> User: (bytes_s, bytes_B)
					//usr.ProcessChallenge(bytes_s, len_s, bytes_B, len_B, &bytes_M, &len_M);
					usr.ProcessChallenge(__s, __b, __m);
					if(!__m.Len()) {
						printf("User SRP-6a safety check violation!\n");
						ok = 0;
					}
					else {
						// User -> Host: (bytes_M)
						//ver.VerifySession(bytes_M, &bytes_HAMK);
						ver.VerifySession(static_cast<const uchar *>(__m.PtrC()), &bytes_HAMK);
						if(!bytes_HAMK) {
							printf("User authentication failed!\n");
							ok = 0;
						}
						else {
							// Host -> User: (HAMK)
							usr.VerifySession(bytes_HAMK);
							if(!usr.IsAuthenticated()) {
								printf("Server authentication failed!\n");
								ok = 0;
							}
						}
					}
				}
			}
			duration = get_usec() - start;
			printf("Usec per call: %d\n", (int)(duration / NITER));
			return 0;
	 */
	}
}
