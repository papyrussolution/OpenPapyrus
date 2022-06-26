// SS_BMHR.CPP
// Copyright (c) A.Sobolev 2006, 2007, 2010, 2014, 2016, 2018, 2020
// @codepage UTF-8
// Реализация алгоритма Boyer-Moore-Horspool-Raita
// для поиска подстроки в строке
//
#include <slib-internal.h>
#pragma hdrstop
//
//
//
int SSrchPattern::Cmp_Vect(const uint16 * pV1, const uint16 * pV2) const
{
	for(size_t i = 0; i < Len; i++)
		if(pV1[i] != pV2[i])
			return 0;
	return 1;
}

SString & SSrchPattern::OutputPattern(SString & rBuf) const
{
	rBuf.Z();
	for(size_t i = 0; i < Len; i++)
		rBuf.Space().CatChar(P_Pat[i]);
	return rBuf.CR();
}

SString & SSrchPattern::OutputVect(const uint16 * pVect, SString & rBuf) const
{
	rBuf.Z();
	for(size_t i = 0; i < Len; i++)
		rBuf.CatLongZ(pVect[i], 2);
	return rBuf.CR();
}

int SSrchPattern::Calc_Z(uint16 * pZ, int test)
{
	size_t len = Len;
	size_t L = 0;
	size_t R = 0;
	uint8 * pS = P_Pat;
	for(size_t p = 1; p < len; p++) {
		if(p > R || test) {
			size_t i = 0;
			while((p+i) < len && pS[i] == pS[p+i])
				i++;
			pZ[p] = (uint16)i;
			if(i > 0) {
				size_t r = p+i-1;
				if(r > R) {
					R = r;
					L = p;
				}
			}
		}
		else {
			size_t p_   = p - L;
			size_t beta = R - p + 1;
			int    z_ = pZ[p_];
			if(z_ < static_cast<int>(beta))
				pZ[p] = z_;
			else {
				size_t i = beta;
				while((p+i) < len && pS[i] == pS[p+i])
					i++;
				pZ[p] = static_cast<uint16>(i);
				if(i > 0) {
					size_t r = p+i-1;
					if(r > R) {
						R = r;
						L = p;
					}
				}
			}
		}
	}
	return 1;
}

int SSrchPattern::Calc_N(uint16 * pN, int test)
{
	const size_t len = Len;
	const size_t len_one = len-1;
	size_t L = 0;
	size_t R = 0;
	uint8 * pS = P_Pat;
	for(size_t p = 1; p < len; p++) {
		const size_t len_one_p = len_one-p;
		if(p > R || test) {
			size_t i = 0;
			while((p+i) < len && pS[len_one-i] == pS[len_one_p-i])
				i++;
			pN[len_one_p] = static_cast<uint16>(i);
			if(i > 0) {
				size_t r = p + i - 1;
				if(r > R) {
					R = r;
					L = p;
				}
			}
		}
		else {
			size_t p_   = p - L;
			size_t beta = R - p + 1;
			int    z_ = pN[len_one-p_];
			if(z_ < static_cast<int>(beta)) {
				pN[len_one_p] = z_;
			}
			else {
				size_t i = beta;
				while((p+i) < len && pS[len_one-i] == pS[len_one_p-i])
					i++;
				pN[len_one_p] = static_cast<uint16>(i);
				if(i > 0) {
					size_t r = p + i - 1;
					if(r > R) {
						R = r;
						L = p;
					}
				}
			}
		}
	}
	return 1;
}

int SSrchPattern::Calc_L(const uint16 * pN, uint16 * pL, uint16 * pl)
{
	size_t p;
	const size_t len = Len;
	//
	// max_j - максимальный префикс pS, который одновременно
	// является суффиксом pS.
	// Необходим для вычисления вектора l'
	//
	size_t max_j = 0;
	for(p = 0; p < len; p++) {
		//
		// Вычисляем вектор L'
		// Определение: Для каждого i L'(i) - наибольшая позиция //
		// меньшая чем Len и такая, что pS[i..(Len-1)] совпадает с
		// суффиксом pS[0..L'(i)], а символ, предшествующий этому
		// суффиксу, не равен pS[i-1]. Если такой позиции нет, то
		// L'(i) = 0.
		//
		// Пример: если pS = "cabdabdab", то L'(4) = 5, L'(7) = 2.
		//
		// Для вычисления вектора L' за линейное время используем
		// вычисленный вектор N по правилу:
		// L'(i) = наибольшему индексу p, меньшему Len-1 и такому,
		// что N(p) = (Len-i))
		//
		const size_t N = pN[p];
		if(N) {
			pL[len-N] = static_cast<uint16>(p);
			if(N == p+1)
				max_j = N;
		}
	}
	//
	// Вычисляем вектор l'
	// Определение: l'(i) определяет длину наибольшего суффикса
	// pS[i..Len-1], который является префиксом pS, если такой существует.
	// Если не существует, то l'(i) = 0.
	//
	//  abcde 00000
	//  abcab 22220
	//  aaaaa 54321
	//
	// Специальный случай: вся строка pS состоит из
	// Len одинаковых символов.
	//
	if(max_j == len) {
		for(p = 0; p < len; p++)
			*pl++ = static_cast<uint16>(len - p);
	}
	else if(max_j > 0) {
		size_t lim = (len-max_j+1);
		for(p = 0; p < lim; p++)
			*pl++ = static_cast<uint16>(max_j);
	}
	return 1;
}
//
//
//
SSrchPattern::SSrchPattern(const char * pPattern, long flags, int alg) :
	P_Pat(0), P_PatAlloc(0), P_PreprocBuf(0), PatSize(0), PreprocSize(0), Flags(0), HashSize(0)
{
	Init(pPattern, flags, alg);
}

SSrchPattern::~SSrchPattern()
{
	SAlloc::F(P_PatAlloc);
	SAlloc::F(P_PreprocBuf);
}

size_t FASTCALL SnapUpSize(size_t i); // @prototype

#define QUANT 4

int SSrchPattern::AllocPreprocBuf()
{
	int   ok = 1;
	size_t sz = 0;
	if(Alg == algBmBadChr) {
		sz = 256 * sizeof(uint16);
	}
	else if(Alg == algBmGoodSfx) {
		sz = (256 + Len * 3) * sizeof(uint16);
	}
	if(Flags & fNoCase)
		sz += 256; // collation sequence
	if(sz > PreprocSize) {
		P_PreprocBuf = static_cast<uint16 *>(SAlloc::R(P_PreprocBuf, sz));
		if(P_PreprocBuf)
			PreprocSize = sz;
		else
			ok = 0;
	}
	return ok;
}

#define DO8(op) op;op; op;op; op;op ;op;op;

int SSrchPattern::Preprocess()
{
	int    ok = 1;
	memzero(P_PreprocBuf, PreprocSize);
	size_t len = Len;
	if(len > 0) {
		size_t i;
		if(Flags & fNoCase) {
			if(Alg == algBmGoodSfx)
				i = (256 + Len * 3) * sizeof(uint16);
			else if(Alg == algBmBadChr)
				i = 256 * sizeof(uint16);
			else
				i = 0;
			uint8 * p_coll = reinterpret_cast<uint8 *>(P_PreprocBuf) + i;
			for(i = 0; i < 256;) {
				DO8(p_coll[i] = ToLower866(i); i++;);
			}
			for(i = 0; i < len; i++)
				P_Pat[i] = p_coll[P_Pat[i]];
		}
		if(Alg == algBmBadChr || Alg == algBmGoodSfx) {
			//
			// Препроцессинг по правилу плохого символа
			//
			uint32 * p_buf32 = reinterpret_cast<uint32 *>(P_PreprocBuf);
			long   f = MakeLong(len, len);
			for(i = 0; i < 256/2;) {
				DO8(p_buf32[i++] = f);
			}
			uint16 * p_buf16 = P_PreprocBuf;
			for(i = 0; i < len-1; i++)
				p_buf16[P_Pat[i]] = static_cast<uint16>(len - 1 - i);
		}
		if(Alg == algBmGoodSfx) {
			//
			// Препроцессинг по правилу хорошего суффикса
			//
			uint16 * p_N = P_PreprocBuf + 256;
			uint16 * p_L = P_PreprocBuf + 256 + len;
			uint16 * p_l = P_PreprocBuf + 256 + len * 2;
			Calc_N(p_N, 0);
			Calc_L(p_N, p_L, p_l);
			//
			// Если элемент L-вектора равен нулю, то устанавливаем его равным соответствующему элементу l-вектора
			//
			for(i = 0; i < len; i++) {
				uint16 v = p_L[i];
				p_L[i] = static_cast<uint16>(v ? (len - 1 - v) : (len - p_l[i]));
			}
		}
	}
	return ok;
}

int SSrchPattern::Init(const char * pPattern, long flags, int alg)
{
	Flags = flags;
	Len = sstrlen(pPattern);
	Alg = alg;
	if((Len+4) > PatSize) {
		PatSize = SnapUpSize(Len+4);
		P_PatAlloc = static_cast<uint8 *>(SAlloc::R(P_PatAlloc, PatSize));
		if(!P_PatAlloc) {
			PatSize = 0;
			return 0;
		}
	}
	P_Pat = oneof2(Alg, algBmBadChr, algBmGoodSfx) ? (PTR8(P_PatAlloc) + Len % 4) : PTR8(P_PatAlloc);
	memcpy(P_Pat, pPattern, Len);
	return (AllocPreprocBuf() && Preprocess()) ? 1 : 0;
}

size_t SSrchPattern::GetLen() const
{
	return Len;
}

int SSrchPattern::Search(const char * pText, size_t start, size_t end, size_t * pPos) const
{
	if(Len > 0) {
		if(Len == 1 || Alg == algDefault) {
			uint8  pat0 = P_Pat[0];
			uint8  U = (Flags & fNoCase) ? toupper(pat0) : pat0;
			const  uint8 * p = 0;
			const  uint8 * p2 = 0;
			size_t text_len = 0;
			if(Flags & fNoCase)
				if(Alg == algBmGoodSfx)
					text_len = (256 + Len * 3) * sizeof(uint16);
				else if(Alg == algBmBadChr)
					text_len = 256 * sizeof(uint16);
			const uint8 * p_coll = (reinterpret_cast<const uint8 *>(P_PreprocBuf) + text_len);
			do {
				const  uint8 * p_text = reinterpret_cast<const uint8 *>(pText)+start;
				text_len = end-start;
				if(U != pat0) {
					size_t s8 = text_len / 8;
					size_t r8 = text_len % 8;
					p = p_text;
					if(s8) do {
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
						if(*p == pat0 || *p == U) goto __succ; ++p;
					} while(--s8);
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ; ++p;
					if(r8-- == 0) goto __fail; if(*p == pat0 || *p == U) goto __succ;
__fail:
					p = 0;

				}
				else
					p = PTR8C(memchr(p_text, pat0, text_len));
				if(p) {
__succ:
					if(Len == 1) {
						ASSIGN_PTR(pPos, (p-PTR8C(pText)));
						return 1;
					}
					else if((text_len-(p-p_text)) >= Len) {
						if(Flags & fNoCase) {
							size_t i = 1;
							while(i < Len)
								if(P_Pat[i] != p_coll[p[i]])
									break;
								else
									++i;
							if(i == Len) {
								ASSIGN_PTR(pPos, (p-PTR8C(pText)));
								return 1;
							}
						}
						else if(memcmp(p+1, P_Pat+1, Len-1) == 0) {
							ASSIGN_PTR(pPos, (p-PTR8C(pText)));
							return 1;
						}
						start = (p-PTR8C(pText))+1;
					}
					else
						p = 0;
				}
			} while(p);
		}
		else if(Alg == algBmBadChr)
			return Search_BC(pText, start, end, pPos);
		else if(Alg == algBmGoodSfx)
			return Search_GS(pText, start, end, pPos);
	}
	return 0;
}

int SSrchPattern::Search_GS(const char * pText, size_t start, size_t end, size_t * pPos) const
{
	const  uint8 * p_text = reinterpret_cast<const uint8 *>(pText);
	const  size_t le   = Len - 1;
	const  uint8  last  = P_Pat[le];
	const  uint16 * p_gs_shift_ptr = P_PreprocBuf+256+Len+1;
	if(Flags & fNoCase) {
		const uint8 * p_coll = reinterpret_cast<const uint8 *>(P_PreprocBuf) + (256 + Len * 3) * sizeof(uint16);
		for(size_t i = start+le; i < end;) {
			uint8  cur = p_coll[p_text[i]];
			if(cur == last) {
				size_t j = le;
				for(size_t k = i; p_coll[p_text[--k]] == P_Pat[--j];) {
					if(j == 0) {
						ASSIGN_PTR(pPos, k);
						return 1;
					}
				}
				i += p_gs_shift_ptr[j];
			}
			else
				i += P_PreprocBuf[cur];
		}
	}
	else {
		for(size_t i = start+le; i < end;) {
			uint8  cur  = p_text[i];
			if(cur == last) {
				size_t j = le;
				for(size_t k = i; p_text[--k] == P_Pat[--j];) {
					if(j == 0) {
						ASSIGN_PTR(pPos, k);
						return 1;
					}
				}
				i += p_gs_shift_ptr[j];
			}
			else
				i += P_PreprocBuf[cur];
		}
	}
	return 0;
}

int SSrchPattern::Search_BC(const char * pText, size_t start, size_t end, size_t * pPos) const
{
	const uint8 * p_text = reinterpret_cast<const uint8 *>(pText);
	const int    minus_one = static_cast<int>(Len - 2);
	const size_t le = Len - 1;
	const uint8  last  = P_Pat[le];
	const uint8  first = P_Pat[0];
	if(Flags & fNoCase) {
		const uint8 * p_coll = reinterpret_cast<const uint8 *>(P_PreprocBuf) + 256 * sizeof(uint16);
		for(size_t i = le + start; i < end;) {
			const uint8 cur = p_coll[p_text[i]];
			if(cur == last) {
				int    k = i - 1;
				int    j = minus_one;
				while(k > -1 && j > -1 && p_coll[p_text[k]] == P_Pat[j]) {
					--k;
					--j;
				}
				if(j == -1) {
					ASSIGN_PTR(pPos, k+1);
					return 1;
				}
			}
			i += P_PreprocBuf[cur];
		}
	}
	else {
		for(size_t i = le + start; i < end;) {
			const uint8 cur = p_text[i];
			if(cur == last && memcmp(p_text+i-le, P_Pat, le) == 0) {
				ASSIGN_PTR(pPos, i-le);
				return 1;
			}
			i += P_PreprocBuf[cur];
		}
	}
	return 0;
}
//
//
//
SSearchReplaceParam::SSearchReplaceParam() : Flags(0)
{
}

SSearchReplaceParam & SSearchReplaceParam::Z()
{
	Flags = 0;
	Pattern.Z();
	Replacer.Z();
	return *this;
}
//
//
//
#if 0 // {

int Alg_SS_Z(const uint8 * pStr, SFile * pF)
{
	int    err = 0;
	size_t len = strlen((const char *)pStr);
	const size_t Z_bias = 0;
	const size_t N_bias = len;
	const size_t L_bias = 2 * len;
	const size_t l_bias = 3 * len;
	uint16 * p_buf = new uint16[len*4];
	uint16 * p_buf_test = new uint16[len*4];
	memzero(p_buf, sizeof(uint16)*len*4);
	memzero(p_buf_test, sizeof(uint16)*len*4);
	Ss_Z_Blk z(len);
	SString out_buf;
	if(pF) {
		pF->WriteLine(z.OutputPattern(pStr, out_buf));
	}
	z.Calc_Z(pStr, p_buf + Z_bias, 0);
	z.Calc_Z(pStr, p_buf_test + Z_bias, 1);
	if(pF) {
		if(z.Cmp_Vect(p_buf + Z_bias, p_buf_test + Z_bias) == 0) {
			pF->WriteLine("ERROR: CALCULATION Z\n");
			err++;
		}
		pF->WriteLine(z.OutputVect(p_buf + Z_bias, out_buf));
		if(err == 1)
			pF->WriteLine(z.OutputVect(p_buf_test + Z_bias, out_buf));
	}
	z.Calc_N(pStr, p_buf + N_bias, 0);
	z.Calc_N(pStr, p_buf_test + N_bias, 1);
	if(pF) {
		if(z.Cmp_Vect(p_buf + N_bias, p_buf_test + N_bias) == 0) {
			pF->WriteLine("ERROR: CALCULATION N\n");
			err++;
		}
		pF->WriteLine(z.OutputVect(p_buf + N_bias, out_buf));
		if(err == 2)
			pF->WriteLine(z.OutputVect(p_buf_test + N_bias, out_buf));
	}
	z.Calc_L(p_buf + N_bias, p_buf + L_bias, p_buf + l_bias);
	if(pF) {
		pF->WriteLine(z.OutputVect(p_buf + L_bias, out_buf));
		pF->WriteLine(z.OutputVect(p_buf + l_bias, out_buf));
	}
	delete p_buf;
	delete p_buf_test;
	return err ? 0 : 1;
}

int Test_Alg_SS_Z(const char * pInputFileName)
{
	int    ok = 1;
	size_t sz = 0;
	char out_file_name[MAXPATH];
	STRNSCPY(out_file_name, pInputFileName);
	replaceExt(out_file_name, "OUT", 1);
	MemLeakTracer mlt;
	SFile file(pInputFileName, SFile::mRead | SFile::mBinary);
	SFile out_file(out_file_name, SFile::mWrite);
	if(file.IsValid()) {
		SString word_buf;
		while(ok && file.ReadLine(word_buf, SFile::rlfChomp)) {
			word_buf.ToLower();
			Alg_SS_Z((const uint8 *)(const char *)word_buf, &out_file);
		}
	}
	return ok;
}

#endif // } 0

