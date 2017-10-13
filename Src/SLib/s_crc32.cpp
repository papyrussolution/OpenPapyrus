// S_CRC32.CPP
// Copyright (c) A.Sobolev 2001, 2003, 2007, 2015, 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop

//#define TEST_ZLIB_IMPLEMENTATION
//#define _LOCAL_USE_SSL

#ifdef _LOCAL_USE_SSL
	#include <openssl/evp.h>
#endif
//
// CRC model
//
struct CrcModel { // cm_t
	enum {
		fRefIn  = 0x0001,
		fRefOut = 0x0002
	};
	CrcModel(uint width, uint poly, uint flags, uint xorot, uint init)
	{
		cm_init = init;
		cm_reg = cm_init;
		Width = width;
		Poly = poly;
		cm_xorot = xorot;
		cm_refin = (flags & fRefIn) ? true : false;
		cm_refot = (flags & fRefOut) ? true : false;
		// Reflect initial value
		if(cm_refin)
			cm_reg = reflect(cm_reg, Width);
	}
	//
	// Returns the value v with the bottom b [0,32] bits reflected.
	// Example: reflect(0x3e23L,3) == 0x3e26
	//
	static uint32 FASTCALL reflect(uint32 v, const uint b)
	{
		uint32 t = v;
		const uint b_1 = (b-1);
		for(uint i = 0; i <= b_1; i++) {
			if(t & 1)
				v |= (1 << (b_1-i));
			else
				v &= ~(1 << (b_1-i));
			t >>= 1;
		}
		return v;
	}
	//
	// Returns a longword whose value is (2^p_cm->cm_width)-1.
	// The trick is to do this portably (e.g. without doing <<32).
	//
	uint32 widmask() const
	{
		return (((1 << (Width-1)) - 1) << 1) | 1;
	}
	uint32 FASTCALL Tab(uint index)
	{
		const  uint32 topbit = (1 << (Width-1));
		uint32 inbyte = (uint32)index;
		if(cm_refin)
			inbyte = reflect(inbyte, 8);
		uint32 c = inbyte << (Width - 8);
		for(uint i = 0; i < 8; i++)
			c = (c & topbit) ? (Poly ^ (c << 1)) : (c << 1);
		if(cm_refin)
			c = reflect(c, Width);
		return (c & widmask());
	}
	//
	// Степень алгоритма, выраженная в битах.  Она всегда на единицу
	// меньше длины полинома, но равна его степени.
	//
	uint8 Width; // Parameter: Width in bits [8,32]
	//
	// Собственно полином. Это битовая величина, которая для удобства
	// может быть представлена шестнадцатеричным числом. Старший бит
	// при этом опускается. Например, если используется полином 10110, то
	// он обозначается числом "06h". Важной особенностью данного параметра является то,
	// что он всегда представляет собой необращенный
	// полином, младшая часть этого параметра во время вычислений всегда
	// является наименее значащими битами делителя вне зависимости от
	// того, какой – "зеркальный" или прямой алгоритм моделируется.
	//
	uint32 Poly; // Parameter: The algorithm's polynomial
	//
	// Этот параметр определяет исходное содержимое регистра на момент
	// запуска вычислений. Именно это значение должно быть занесено в
	// регистр в прямой табличном алгоритме. В принципе, в табличных алгоритмах
	// мы всегда может считать, что регистр инициализируется
	// нулевым значением, а начальное значение комбинируется по XOR с
	// содержимым регистра после N цикла.
	//
	uint32 cm_init;		// Parameter: Initial register value
	//
	// Логический параметр. Если он имеет значение False, байты
	// сообщения обрабатываются, начиная с 7 бита, который считается
	// наиболее значащим, а наименее значащим считается бит 0.
	// Если параметр имеет значение True, то каждый байт перед обработкой обращается.
	//
	bool  cm_refin;		// Parameter: Reflect input bytes?
	//
	// Логический параметр. Если он имеет значение False, то
	// конечное содержимое регистра сразу передается на стадию XorOut, в
	// противном случае, когда параметр имеет значение True,
	// содержимое регистра обращается перед передачей на следующую
	// стадию вычислений.
	//
	bool  cm_refot;		// Parameter: Reflect output CRC?
	//
	// W битное значение, обозначаемое шестнадцатеричным числом. Оно
	// комбинируется с конечным содержимым регистра (после стадии
	// RefOut), прежде чем будет получено окончательное значение контрольной суммы.
	//
	uint32 cm_xorot;		// Parameter: XOR this to output CRC
	//
	// Это поле, собственно, не является частью определения алгоритма, и, в
	// случае противоречия с ним предшествующих параметров, именно эти
	// предыдущие параметры имеют наибольший приоритет.  Данное поле
	// служит контрольным значением, которое может быть использовано для
	// слабой проверки правильности реализации алгоритма.  Поле содержит
	// контрольную сумму, рассчитанную для ASCII строки "123456789"
	// (шестнадцатеричные значение "313233...").
	//
	uint32 cm_reg;          // Context: Context during execution
};
//
//
//
SLAPI CRC32::CRC32()
{
	P_Tab = 0;
}

SLAPI CRC32::~CRC32()
{
	delete P_Tab;
}
/*
                0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2 3 4 5 6 7 8 9 0 1 2
  {"CRC-4",    "1 1 1 1 1"},
  {"CRC-7",    "1 1 0 1 0 0 0 1"},
  {"CRC-8",    "1 1 1 0 1 0 1 0 1"},
  {"CRC-12",   "1 1 0 0 0 0 0 0 0 1 1 1 1"},
  {"CRC-24",   "1 1 0 0 0 0 0 0 0 0 1 0 1 0 0 0 1 0 0 0 0 0 0 0 1"},
  {"CRC-32",   "1 0 1 0 0 0 0 0 0 0 1 0 0 0 0 0 0 1 0 0 1 1 0 0 0 1 1 1 0 0 0 1 0"},
  {"CCITT-4",  "1 0 0 1 1"},
  {"CCITT-5",  "1 1 0 1 0 1"},
  {"CCITT-6",  "1 0 0 0 0 1 1"},
  {"CCITT-16", "1 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 1"},
  {"CCITT-32", "1 0 0 0 0 0 1 0 0 1 1 0 0 0 0 0 1 0 0 0 1 1 1 0 1 1 0 1 1 0 1 1 1"},
  {"WCDMA-8",  "1 1 0 0 1 1 0 1 1"},
  {"WCDMA-12", "1 1 0 0 0 0 0 0 0 1 1 1 1"},
  {"WCDMA-16", "1 0 0 0 1 0 0 0 0 0 0 1 0 0 0 0 1"},
  {"WCDMA-24", "1 1 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 0 1 1 0 0 0 1 1"},
  {"ATM-8",    "1 0 0 0 0 0 1 1 1"},
  {"ANSI-16",  "1 1 0 0 0 0 0 0 0 0 0 0 0 0 1 0 1"},
  {"SDLC-16",  "1 1 0 1 0 0 0 0 0 1 0 0 1 0 1 1 1"},
};
*/

/*
  Generate a table for a byte-wise 32-bit CRC calculation on the polynomial:
  x^32+x^26+x^23+x^22+x^16+x^12+x^11+x^10+x^8+x^7+x^5+x^4+x^2+x+1.

  Polynomials over GF(2) are represented in binary, one bit per coefficient,
  with the lowest powers in the most significant bit.  Then adding polynomials
  is just exclusive-or, and multiplying a polynomial by x is a right shift by
  one.  If we call the above polynomial p, and represent a byte as the
  polynomial q, also with the lowest power in the most significant bit (so the
  byte 0xb1 is the polynomial x^7+x^3+x+1), then the CRC is (q*x^32) mod p,
  where a mod b means the remainder after dividing a by b.

  This calculation is done using the shift-register method of multiplying and
  taking the remainder.  The register is initialized to zero, and for each
  incoming bit, x^32 is added mod p to the register if the bit is a one (where
  x^32 mod p is p+x^32 = x^26+...+1), and the register is multiplied mod p by
  x (which is shifting right by one and adding x^32 mod p if the bit shifted
  out is a one).  We start with the highest power (least significant bit) of
  q and repeat for all eight bits of q.

  The table is simply the CRC of all possible eight bit values.  This is all
  the information needed to generate CRC's on data a byte at a time for all
  combinations of CRC register values and incoming bytes.
*/
int SLAPI CRC32::MakeTab()
{
	ulong  c;
	uint   n, k;
	//
	// terms of polynomial defining this crc (except x^32):
	//
	static const uint8 p[] = {0,1,2,4,5,7,8,10,11,12,16,22,23,26};
	if(P_Tab)
		return 1;
	P_Tab = new ulong[256];
	if(P_Tab) {
		//
		// make exclusive-or pattern from polynomial (0xedb88320L)
		//
		ulong poly = 0L; // polynomial exclusive-or pattern
		for(n = 0; n < SIZEOFARRAY(p); n++)
			poly |= 1L << (31 - p[n]);
		for(n = 0; n < 256; n++) {
			c = (ulong)n;
			for(k = 0; k < 8; k++)
				c = (c & 1) ? (poly ^ (c >> 1)) : (c >> 1);
			P_Tab[n] = c;
		}
		//
#ifndef NDEBUG
		{
			// norm       reverse in  reverse out
			// 0x04C11DB7 0xEDB88320  0x82608EDB
			CrcModel cm(32, 0x04C11DB7, CrcModel::fRefIn, 0, 0);
            ulong   cm_test[256];
            for(uint i = 0; i < SIZEOFARRAY(cm_test); i++) {
				cm_test[i] = cm.Tab(i);
				assert(cm_test[i] == P_Tab[i]);
            }
		}
#endif // NDEBUG
		//
		return 1;
	}
	else
		return (SLibError = SLERR_NOMEM, 0);
}

ulong SLAPI CRC32::Calc(ulong crc, const uint8 * buf, size_t len)
{
	#define DO1(buf)  crc = P_Tab[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
	#define DO2(buf)  DO1(buf); DO1(buf);
	#define DO4(buf)  DO2(buf); DO2(buf);
	#define DO8(buf)  DO4(buf); DO4(buf);

	if(!buf)
		return 0L;
	if(P_Tab == NULL)
		MakeTab();
	crc = crc ^ 0xffffffffL;
	while(len >= 8) {
		DO8(buf);
		len -= 8;
	}
	if(len)
		do {
			DO1(buf);
		} while(--len);
	return crc ^ 0xffffffffL;

	#undef DO1
	#undef DO2
	#undef DO4
	#undef DO8
}
//
//
//
char * FASTCALL SUpceToUpca(const char * pUpce, char * pUpca)
{
	char   code[32], dest[32];
	STRNSCPY(code, pUpce);
	int    last = code[6] - '0';
	memset(dest, '0', 12);
	dest[11] = 0;
	dest[0] = code[0];
	if(last == 0 || last == 1 || last == 2) {
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[6];

		dest[8] = code[3];
		dest[9] = code[4];
		dest[10] = code[5];
	}
	else if(last == 3) {
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[3];

		dest[9] = code[4];
		dest[10] = code[5];
	}
	else if(last == 4) {
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[3];
		dest[4] = code[4];

		dest[10] = code[5];
	}
	else { // last = 5..9
		dest[1] = code[1];
		dest[2] = code[2];
		dest[3] = code[3];
		dest[4] = code[4];
		dest[5] = code[5];

		dest[10] = code[6];
	}
	return strcpy(pUpca, dest);
}
//
//
//
int SCalcCheckDigit(int alg, const char * pInput, size_t inputLen)
{
	int    cd = 0;
	if(pInput && inputLen) {
		size_t  len = 0;
		size_t  i;
		char    code[128];
		for(i = 0; i < inputLen; i++) {
			const char c = pInput[i];
			if(isdec(c)) {
				if(len >= sizeof(code))
					break;
				code[len++] = c;
			}
			else if(!oneof2(c, '-', ' '))
				break;
		}
		if(len) {
			const int _alg = (alg & ~SCHKDIGALG_TEST);
			const int _do_check = BIN(alg & SCHKDIGALG_TEST);

			if(_alg == SCHKDIGALG_BARCODE) {
				int    c = 0, c1 = 0, c2 = 0;
				const size_t _len = _do_check ? (len-1) : len;
				for(i = 0; i < _len; i++) {
					if((i % 2) == 0)
						c1 += (code[_len-i-1] - '0');
					else
						c2 += (code[_len-i-1] - '0');
				}
				c = c1 * 3 + c2;
				cd = '0' + ((c % 10) ? (10 - c % 10) : 0);
				if(_do_check)
					cd = BIN(cd == code[len-1]);
			}
			else if(_alg == SCHKDIGALG_LUHN) {
				/*
				// Num[1..N] — номер карты, Num[N] — контрольная цифра.
				sum = 0
				for i = 1 to N-1 do
					p = Num[N-i]
					if (i mod 2 <> 0) then
						p = 2*p
						if (p > 9) then
							p = p - 9
						end if
					end if
					sum = sum + p
				next i
				//дополнение до 10
				sum = 10 - (sum mod 10)
				if (sum == 10) then
					sum = 0
				end if
				Num[N] = sum
				*/
				int    s = 0;
				const size_t _len = _do_check ? (len-1) : len;
				for(i = 0; i < _len; i++) {
					int    p = (code[_len - i - 1] - '0');
					if((i & 1) == 0) {
						p <<= 1; // *2
						if(p > 9)
							p -= 9;
					}
					s += p;
				}
				s = 10 - (s % 10);
				if(s == 10)
					s = 0;
				cd = '0' + s;
				if(_do_check)
					cd = BIN(cd == code[len-1]);
			}
			else if(_alg == SCHKDIGALG_RUINN) {
				//int CheckINN(const char * pCode)
				{
					int    r = 1;
					if((_do_check && len == 10) || (!_do_check && len == 9)) {
						const int8 w[] = {2,4,10,3,5,9,4,6,8,0};
						ulong  sum = 0;
						for(i = 0; i < 9; i++) {
							uint   p = (code[i] - '0');
							sum += (w[i] * p);
						}
						cd = '0' + (sum % 11) % 10;
						if(_do_check) {
							cd = BIN(code[9] == cd);
						}
					}
					else if((_do_check && len == 12) || (!_do_check && len == 11)) {
						if(_do_check) {
							const int8 w1[] = {7,2,4,10, 3,5,9,4,6,8,0};
							const int8 w2[] = {3,7,2, 4,10,3,5,9,4,6,8,0};
							ulong  sum1 = 0, sum2 = 0;
							for(i = 0; i < 11; i++) {
								uint   p = (code[i] - '0');
								sum1 += (w1[i] * p);
							}
							for(i = 0; i < 12; i++) {
								uint   p = (code[i] - '0');
								sum2 += (w2[i] * p);
							}
							int    cd1 = (sum1 % 11) % 10;
							int    cd2 = (sum2 % 11) % 10;
							cd = BIN((code[10]-'0') == cd1 && (code[11]-'0') == cd2);
						}
						else {
							cd = -1;
						}
					}
					else
						cd = 0;
				}
			}
			else if(_alg == SCHKDIGALG_RUOKATO) {
			}
			else if(_alg == SCHKDIGALG_RUSNILS) {
			}
		}
	}
	return cd;
}

int FASTCALL SCalcBarcodeCheckDigitL(const char * pBarcode, size_t len)
{
	int    cd = 0;
	if(pBarcode && len) {
		if(len == 7 && pBarcode[0] == '0') {
			char   code[64];
			memcpy(code, pBarcode, len);
			code[len] = 0;
			len = strlen(SUpceToUpca(code, code));
			cd = SCalcCheckDigit(SCHKDIGALG_BARCODE, code, len);
			cd = isdec(cd) ? (cd - '0') : 0;
		}
		else {
			cd = SCalcCheckDigit(SCHKDIGALG_BARCODE, pBarcode, len);
			cd = isdec(cd) ? (cd - '0') : 0;
		}
	}
	return cd;
}
//
//
//
#if 1 // @construction {

#define CDTCLS_HASH      1
#define CDTCLS_CRYPT     2

#define CDTF_ADDITIVE    0x0001
#define CDTF_REVERSIBLE  0x0002
#define CDTF_OUTSIZEFIX  0x0004
#define CDTF_OUTSIZEMULT 0x0008
#define CDTF_NEEDINIT    0x0010
#define CDTF_NEEDFINISH  0x0020

#define CDT_PHASE_UPDATE   0
#define CDT_PHASE_INIT     1
#define CDT_PHASE_FINISH   2
#define CDT_PHASE_TEST   100

struct SDataTransformAlgorithm {
	int    Alg;
	int    Cls;
	long   Flags;
	uint32 InSizeParam;
	uint32 OutSizeParam;
	const char * P_Symb;
};

int SDataTransform(int alg, int phase, int inpFormat, int outpFormat, const SBaseBuffer & rIn, SBaseBuffer & rOut);
int SDataTransform(int alg, int phase, int inpFormat, int outpFormat, const void * pIn, size_t inLen, void * pOut, size_t outLen, size_t * pResultOutOffs);
uint32 StHash32(int alg, const SBaseBuffer & rIn);
uint32 StCheckSum(int alg, int phase, const SBaseBuffer & rIn);

class SBdtFunct {
public:
	//
	// Descr: Классы алгоритмов
	//
	enum {
		clsUnkn = 0, // Не определенный
		clsHash,     // Хэш-функция
		clsCrypt,    // Криптографический алгоритм
		clsCompr     // Сжатие данных
	};
	enum {
		Unkn = 0,
		Crc32,      // Check sum
		Crc24,      // Check sum
		Crc16,      // Check sum
		Adler32,    // Check sum
		MD2,        // Hash
		MD4,        // Hash
		MD5,        // Hash
		SHA160,     // Hash
		SHA224,     // Hash
		IdeaCrypt,  // Crypt
		IdeaDecrypt // Crypt
	};
	enum {
		fFixedSize     = 0x0001, // Результат операции имеет фиксированных конечный размер
		fKey           = 0x0002, // Операция требует ключа
		fWriteAtFinish = 0x0004, // Результат операции записывается в буфер во время вызова метода Finish()
		fReversible    = 0x0008  // Обратимая функция  
	};
	struct Info {
		void   Set(int alg, int cls, uint flags, int inverseAlg, uint inBufQuant, uint outSize, uint keyLen)
		{
			Alg = alg;
			Cls = cls;
			InverseAlg = inverseAlg;
			Flags = flags;
			InBufQuant = inBufQuant;
			OutSize = outSize;
			KeyLen = keyLen;
		}
        int    Alg;
        int    Cls;
		int    InverseAlg;  // Обратная функция
        uint   Flags;
        uint   InBufQuant;  // Требование к квантованию входящего потока (byte)
        uint   OutSize;     // !0 если результат имеет фиксированный размер (byte)
		uint   KeyLen;      // !0 если требуется ключ фиксированной длины (byte)
	};
	struct Stat {
		size_t InSize;
		size_t OutSize;
		int64  Time;
	};
	struct TransformBlock {
		//
		// Descr: Конструктор для фазы phaseInit
		//
		TransformBlock() : Phase(phaseInit), InBufLen(0)
		{
			assert(Phase == phaseInit);
			P_InBuf = 0;
			P_OutBuf = 0;
			OutBufLen = 0;
			OutBufPos = 0;
		}
		//
		// Descr: Конструктор для фазы phaseUpdate
		//
		TransformBlock(const void * pInBuf, size_t inBufLen, void * pOutBuf, size_t outBufLen) : Phase(phaseUpdate), InBufLen(inBufLen)
		{
			assert(Phase == phaseUpdate);
			P_InBuf = pInBuf;
			P_OutBuf = pOutBuf;
			OutBufLen = outBufLen;
			OutBufPos = 0;
		}
		//
		// Descr: Конструктор для фазы phaseFinish
		//
		TransformBlock(void * pOutBuf, size_t outBufLen) : Phase(phaseFinish), InBufLen(0)
		{
			P_InBuf = 0;
			P_OutBuf = pOutBuf;
			OutBufLen = outBufLen;
			OutBufPos = 0;
		}
		//
		// Descr: Конструктор для получения информации об алгоритме
		//
		TransformBlock(SBdtFunct::Info * pInfo) : Phase(phaseGetInfo), InBufLen(0)
		{
			P_InBuf = 0;
			P_OutBuf = pInfo;
			OutBufLen = pInfo ? sizeof(*pInfo) : 0;
			OutBufPos = 0;
		}
		//
		// Descr: Конструктор для получения статистики после завершения работы алгоритма
		//
		TransformBlock(SBdtFunct::Stat * pStat) : Phase(phaseGetStat), InBufLen(0)
		{
			P_InBuf = 0;
			P_OutBuf = pStat;
			OutBufLen = pStat ? sizeof(*pStat) : 0;
			OutBufPos = 0;
		}
		const int  Phase;
		const void * P_InBuf;
		const size_t InBufLen;
		void * P_OutBuf;
		size_t OutBufLen;
		size_t OutBufPos;
	};
	enum {
		paramKey = 1
	};
	SBdtFunct(int alg);
	~SBdtFunct();
	int    GetInfo(Info & rResult);
	int    GetStat(Stat & rResult);
	int    Init();
	int    SetParam(int param, const void * pData, size_t dataLen);
	int    Update(const void * pInBuf, size_t inBufLen, void * pOutBuf, size_t outBufLen, size_t * pOutOffs);
	int    Finish(void * pOutBuf, size_t outBufLen, size_t * pOutOffs);
private:
	enum {
		phaseInit,
		phaseUpdate,
		phaseFinish,
		phaseGetInfo,
		phaseGetStat
	};
	int    FASTCALL Implement_Transform(TransformBlock & rBlk);

	struct State_ {
		State_()
		{
			P_Tab = 0;
			P_Ext = 0;
			P_Ctx = 0;
			Reset();
		}
		void   Reset();

        const void * P_Tab;
		void * P_Ctx; // EVP_CIPHER_CTX
        void * P_Ext;
        union {
        	uint8  B256[256];
        	uint16 B2;
        	uint32 B4;
        } O;
        SBdtFunct::Stat S;
	};
	const int32 A;
	State_ Ste;
	SBaseBuffer Key;

	class Tab4_256 {
		uint32 T[256];
	};
};

void SBdtFunct::State_::Reset()
{
	P_Tab = 0;
	P_Ext = 0;
	P_Ctx = 0;
	MEMSZERO(O);
	MEMSZERO(S);
}

SBdtFunct::SBdtFunct(int alg) : A(alg)
{
	Key.Init();
	Ste.Reset();
}

SBdtFunct::~SBdtFunct()
{
	Key.Destroy();
}

int SBdtFunct::GetInfo(SBdtFunct::Info & rInfo)
{
	TransformBlock blk(&rInfo);
	return Implement_Transform(blk);
}

int SBdtFunct::GetStat(Stat & rResult)
{
	TransformBlock blk(&rResult);
	return Implement_Transform(blk);
}

int SBdtFunct::Init()
{
	TransformBlock blk;
	return Implement_Transform(blk);
}

int SBdtFunct::SetParam(int param, const void * pData, size_t dataLen)
{
	int    ok = 0;
	if(param == paramKey) {
		Key.Destroy();
		if(dataLen) {
			if(Key.Alloc(dataLen)) {
				memcpy(Key.P_Buf, pData, dataLen);
				ok = 1;
			}
			else
				ok = 0;
		}
		else
			ok = 1;
	}
	return ok;
}

int SBdtFunct::Update(const void * pInBuf, size_t inBufLen, void * pOutBuf, size_t outBufLen, size_t * pOutOffs)
{
	TransformBlock blk(pInBuf, inBufLen, pOutBuf, outBufLen);
	int ok = Implement_Transform(blk);
	ASSIGN_PTR(pOutOffs, blk.OutBufPos);
	return ok;
}

int SBdtFunct::Finish(void * pOutBuf, size_t outBufLen, size_t * pOutOffs)
{
	TransformBlock blk(pOutBuf, outBufLen);
	int ok = Implement_Transform(blk);
	ASSIGN_PTR(pOutOffs, blk.OutBufPos);
	return ok;
}

int FASTCALL SBdtFunct::Implement_Transform(TransformBlock & rBlk)
{
	static uint _Tab_Crc32_Idx = 0; // SlSession SClassWrapper

	int    ok = 1;
	if(rBlk.Phase == phaseInit) {
		Ste.Reset();
	}
    switch(A) {
		case SBdtFunct::Crc32:
			switch(rBlk.Phase) {
				case phaseGetInfo:
					{
						Info * p_inf = (Info *)rBlk.P_OutBuf;
						if(p_inf)
							p_inf->Set(A, clsHash, fFixedSize|fWriteAtFinish, 0, 0, 4, 0);
					}
					break;
				case SBdtFunct::phaseInit:
					{
						int    do_init_tab = 0;
						if(!_Tab_Crc32_Idx) {
							TSClassWrapper <SBdtFunct::Tab4_256> cls;
							_Tab_Crc32_Idx = SLS.CreateGlobalObject(cls);
							do_init_tab = 1;
						}
						uint32 * p_tab = (uint32 *)SLS.GetGlobalObject(_Tab_Crc32_Idx);
						THROW(p_tab);
						if(do_init_tab) {
							// norm       reverse in  reverse out
							// 0x04C11DB7 0xEDB88320  0x82608EDB
							CrcModel cm(32, 0x04C11DB7, CrcModel::fRefIn, 0, 0);
							for(uint i = 0; i < 256; i++)
								p_tab[i] = cm.Tab(i);
						}
						Ste.P_Tab = p_tab;
						Ste.O.B4 = 0;
					}
					break;
				case SBdtFunct::phaseUpdate:
					{
						const ulong * p_tab = (const ulong *)Ste.P_Tab;
						THROW(p_tab);

						#define DO1(buf)  crc = p_tab[((int)crc ^ (*buf++)) & 0xff] ^ (crc >> 8);
						#define DO2(buf)  DO1(buf); DO1(buf);
						#define DO4(buf)  DO2(buf); DO2(buf);
						#define DO8(buf)  DO4(buf); DO4(buf);

						uint32 crc = Ste.O.B4;
						crc = crc ^ 0xffffffffL;
						size_t len = rBlk.InBufLen;
						const uint8 * p_buf = PTR8(rBlk.P_InBuf);
						while(len >= 8) {
							DO8(p_buf);
							len -= 8;
						}
						if(len)
							do {
								DO1(p_buf);
							} while(--len);
						Ste.O.B4 = crc ^ 0xffffffffL;
						Ste.S.InSize += rBlk.InBufLen;
						Ste.S.OutSize = sizeof(crc);

						#undef DO1
						#undef DO2
						#undef DO4
						#undef DO8
					}
					break;
				case SBdtFunct::phaseFinish:
					{
						if(rBlk.P_OutBuf) {
							THROW(rBlk.OutBufLen >= sizeof(Ste.O.B4));
							PTR32(rBlk.P_OutBuf)[0] = Ste.O.B4;
							rBlk.OutBufPos = sizeof(Ste.O.B4);
						}
					}
					break;
				case SBdtFunct::phaseGetStat:
					{
                        Stat * p_stat = (Stat *)rBlk.P_OutBuf;
						assert(!p_stat || rBlk.OutBufLen >= sizeof(Stat));
                        ASSIGN_PTR(p_stat, Ste.S);
					}
					break;
			}
			break;
		case SBdtFunct::Crc24:
			switch(rBlk.Phase) {
				case SBdtFunct::phaseInit:
					break;
				case SBdtFunct::phaseUpdate:
					break;
				case SBdtFunct::phaseFinish:
					break;
			}
			break;
		case SBdtFunct::Crc16:
			switch(rBlk.Phase) {
				case SBdtFunct::phaseInit:
					break;
				case SBdtFunct::phaseUpdate:
					break;
				case SBdtFunct::phaseFinish:
					break;
			}
			break;
		case SBdtFunct::Adler32:
			switch(rBlk.Phase) {
				case phaseGetInfo:
					{
						Info * p_inf = (Info *)rBlk.P_OutBuf;
						if(p_inf)
							p_inf->Set(A, clsHash, fFixedSize|fWriteAtFinish, 0, 0, 4, 0);
					}
					break;
				case phaseInit:
					{
						Ste.P_Tab = 0;
						Ste.O.B4 = 1/*__adler32(0, 0, 0)*/;
					}
					break;
				case phaseUpdate:
					//Ste.O.B4 = __adler32(Ste.O.B4, PTR8(rBlk.P_InBuf), rBlk.InBufLen);
					//static ulong __adler32(ulong value, const uint8 * pBuf, size_t len)
					{
						#define DO1(buf, i)  { value += (buf)[i]; sum2 += value; }
						#define DO2(buf, i)  DO1(buf, i); DO1(buf, i+1);
						#define DO4(buf, i)  DO2(buf, i); DO2(buf, i+2);
						#define DO8(buf, i)  DO4(buf, i); DO4(buf, i+4);
						#define DO16(buf)    DO8(buf, 0); DO8(buf, 8);
						ulong value = Ste.O.B4;
						const uint8 * p_buf = PTR8(rBlk.P_InBuf);
						size_t len = rBlk.InBufLen;
						const uint _base_ = 65521U; // largest prime smaller than 65536
						const uint _nmax_ = 5552;   // _nmax_ is the largest n such that 255n(n+1)/2 + (n+1)(BASE-1) <= 2^32-1
						ulong  result = 1;
						uint   n;
						ulong  sum2 = (value >> 16) & 0xffff; // split Adler-32 into component sums
						value &= 0xffff;
						if(len == 1) { // in case user likes doing a byte at a time, keep it fast
							value += p_buf[0];
							if(value >= _base_)
								value -= _base_;
							sum2 += value;
							if(sum2 >= _base_)
								sum2 -= _base_;
							result = (value | (sum2 << 16));
						}
						else if(p_buf) { // initial Adler-32 value (deferred check for len == 1 speed)
							if(len < 16) { // in case short lengths are provided, keep it somewhat fast
								while(len--) {
									value += *p_buf++;
									sum2 += value;
								}
								if(value >= _base_)
									value -= _base_;
								sum2 %= _base_; // only added so many _base_'s
								result = (value | (sum2 << 16));
							}
							else {
								// do length NMAX blocks -- requires just one modulo operation
								while(len >= _nmax_) {
									len -= _nmax_;
									n = _nmax_ / 16; // NMAX is divisible by 16
									do {
										DO16(p_buf); // 16 sums unrolled
										p_buf += 16;
									} while(--n);
									value %= _base_;
									sum2 %= _base_;
								}
								// do remaining bytes (less than NMAX, still just one modulo)
								if(len) { // avoid modulos if none remaining
									while(len >= 16) {
										len -= 16;
										DO16(p_buf);
										p_buf += 16;
									}
									while(len--) {
										value += *p_buf++;
										sum2 += value;
									}
									value %= _base_;
									sum2 %= _base_;
								}
								result = (value | (sum2 << 16)); // return recombined sums
							}
						}
						Ste.O.B4 = result;
						#undef DO1
						#undef DO2
						#undef DO4
						#undef DO8
						#undef DO16
					}
					Ste.S.InSize += rBlk.InBufLen;
					Ste.S.OutSize = sizeof(Ste.O.B4);
					break;
				case phaseFinish:
					if(rBlk.P_OutBuf) {
						THROW(rBlk.OutBufLen >= sizeof(Ste.O.B4));
						PTR32(rBlk.P_OutBuf)[0] = Ste.O.B4;
						rBlk.OutBufPos = sizeof(Ste.O.B4);
					}
					break;
				case SBdtFunct::phaseGetStat:
					{
                        Stat * p_stat = (Stat *)rBlk.P_OutBuf;
						assert(!p_stat || rBlk.OutBufLen >= sizeof(Stat));
                        ASSIGN_PTR(p_stat, Ste.S);
					}
					break;
			}
			break;
		case SBdtFunct::MD2:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::MD4:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::MD5:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::SHA160:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
		case SBdtFunct::SHA224:
			switch(rBlk.Phase) {
				case phaseInit:
					break;
				case phaseUpdate:
					break;
				case phaseFinish:
					break;
			}
			break;
#ifdef _LOCAL_USE_SSL // {
		case SBdtFunct::IdeaCrypt:
		case SBdtFunct::IdeaDecrypt:
			switch(rBlk.Phase) {
				case phaseGetInfo:
					{
						Info * p_inf = (Info *)rBlk.P_OutBuf;
						if(p_inf) {
							p_inf->Set(A, clsCrypt, fKey|fReversible, 
								((A == SBdtFunct::IdeaDecrypt) ? SBdtFunct::IdeaCrypt : SBdtFunct::IdeaDecrypt), 0, 0, 16);
						}
					}
					break;
				case phaseInit:
					{
						uint16 iv[4];
						uint8  key[16];
						memzero(iv, sizeof(iv));
						THROW(Key.Size == 16 && Key.P_Buf);
						memcpy(key, Key.P_Buf, Key.Size);
						Ste.P_Ctx = EVP_CIPHER_CTX_new();
						Ste.P_Tab = EVP_idea_cbc(); // EVP_idea_cfb();
						if(A == SBdtFunct::IdeaCrypt) {
							THROW(EVP_EncryptInit((EVP_CIPHER_CTX *)Ste.P_Ctx, (const EVP_CIPHER *)Ste.P_Tab, key, PTR8(iv)));
						}
						else {
							THROW(EVP_DecryptInit((EVP_CIPHER_CTX *)Ste.P_Ctx, (const EVP_CIPHER *)Ste.P_Tab, key, PTR8(iv)));
						}
						//Ste.P_Ext = new IDEACFB(iv, key, BIN(A == SBdtFunct::IdeaDecrypt)); 
					}
					break;
				case phaseUpdate:
					THROW(Ste.P_Ctx);
					THROW(Ste.P_Tab);
					if(rBlk.InBufLen > 0) {
						THROW(rBlk.OutBufLen >= rBlk.InBufLen);
						THROW(rBlk.P_InBuf && rBlk.P_OutBuf);
						//memcpy(rBlk.P_OutBuf, rBlk.P_InBuf, rBlk.InBufLen);
						//((IDEACFB *)Ste.P_Ext)->run(PTR8(rBlk.P_OutBuf), rBlk.InBufLen);
						//rBlk.OutBufPos = rBlk.InBufLen;

						int    out_len = (int)rBlk.OutBufLen;
						if(A == SBdtFunct::IdeaCrypt) {
							THROW(EVP_EncryptUpdate((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len, PTR8(rBlk.P_InBuf), rBlk.InBufLen));
						}
						else {
							THROW(EVP_DecryptUpdate((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len, PTR8(rBlk.P_InBuf), rBlk.InBufLen));
						}
						rBlk.OutBufPos = (size_t)out_len;
					}
					break;
				case phaseFinish:
					//delete (IDEACFB *)Ste.P_Ext;
					//Ste.P_Ext = 0;

					THROW(Ste.P_Ctx);
					int    out_len = (int)rBlk.OutBufLen;
					if(A == SBdtFunct::IdeaCrypt) {
						THROW(EVP_EncryptFinal((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len));
					}
					else {
						THROW(EVP_DecryptFinal((EVP_CIPHER_CTX *)Ste.P_Ctx, PTR8(rBlk.P_OutBuf), &out_len));
					}
					rBlk.OutBufPos = (size_t)out_len;
					EVP_CIPHER_CTX_free((EVP_CIPHER_CTX *)Ste.P_Ctx);
					break;
			}
			break;
#endif // } _LOCAL_USE_SSL 
		default:
			break;
    }
    CATCHZOK
	return ok;
}

#endif // } 0 @construction

struct BdtTestItem {
	BdtTestItem()
	{
		Flags = 0;
		OutLen = 0;
		Seek = 0;
	}
	~BdtTestItem()
	{
	}
	enum {
		fIn         = 0x0001,
		fOut        = 0x0002,
		fKey        = 0x0004,
		fSalt       = 0x0008,
		fLabel      = 0x0010,
		fIKM        = 0x0020,
		fXTS        = 0x0040,
		fOutLen     = 0x0080,
		fIterations = 0x0100,
		fSecret     = 0x0200,
		fPassphrase = 0x0400,
		fSeek       = 0x0800,
		fNonce      = 0x1000
	};
	struct Buffer : private SBaseBuffer {
	public:
		Buffer()
		{
			SBaseBuffer::Init();
			DataLen = 0;
		}
		~Buffer()
		{
			SBaseBuffer::Destroy();
		}
		size_t GetLen() const
		{
			return DataLen;
		}
		void * GetBuf()
		{
			return P_Buf;
		}
		int    Put(const void * pData, size_t dataLen)
		{
			int    ok = 1;
			if(pData) {
				if(dataLen <= Size || Alloc(dataLen)) {
					memcpy(P_Buf, pData, dataLen);
					DataLen = dataLen;
				}
				else
					ok = 0;
			}
			else {
				DataLen = 0;
			}
			return ok;
		}
	private:
		size_t DataLen;
	};
	static int _DecodeTestData(const SString & rSrc, BdtTestItem::Buffer & rDest, STempBuffer & rTempBuffer)
	{
		int    ok = 1;
		size_t req_size = rSrc.Len(); // На самом деле Len()/2 но подстрахуемся
		size_t real_bin_size = 0;
		if(req_size > rTempBuffer.GetSize())
			THROW(rTempBuffer.Alloc(req_size));
		THROW(rSrc.DecodeHex(0, rTempBuffer, rTempBuffer.GetSize(), &real_bin_size));
		THROW(rDest.Put(rTempBuffer, real_bin_size));
		CATCHZOK
		return ok;
	}
	long   Flags;
	size_t OutLen;       // "OutputLen"
	uint   Iterations;   // "Iterations"
	uint   Seek;         // "Seek"
    BdtTestItem::Buffer In;      // "In"
    BdtTestItem::Buffer Out;     // "Out" || "Output"
    BdtTestItem::Buffer Key;     // "Key" || "Secret" || "Passphrase"
    BdtTestItem::Buffer Salt;    // "Salt"
    BdtTestItem::Buffer Label;   // "Label"
    BdtTestItem::Buffer IKM;     // "IKM"
    BdtTestItem::Buffer XTS;     // "XTS"
    BdtTestItem::Buffer Nonce;   // "Nonce"

//Salt = 000102030405060708090A0B0C
//Label = F0F1F2F3F4F5F6F7F8F9
//OutputLen  = 42
//Secret = 0B0B0B0B0B0B0B0B0B0B0B
//Output = 085A01EA1B10F36933068B56EFA5AD81A4F14B822F5B091568A9CDD4F155FDA2C22E422478D305F3F896
//IKM  = 0B0B0B0B0B0B0B0B0B0B0B
//XTS = 000102030405060708090A0B0C

//Iterations = 6
//Passphrase = ftlkfbxdtbjbvllvbwiw


};

int SLAPI ReadBdtTestData(const char * pFileName, const char * pSetSymb, TSCollection <BdtTestItem> & rData)
{
    int    ok = 1;
	STempBuffer temp_data_buffer(4096);
    THROW(fileExists(pFileName));
    THROW(temp_data_buffer.IsValid());
    {
    	SString line_buf;
    	SString hdr_buf, data_buf;
    	SString set_name;
    	BdtTestItem * p_current_item = 0;
    	SFile f_in(pFileName, SFile::mRead);
		while(f_in.ReadLine(line_buf)) {
			line_buf.Chomp();
			if(line_buf.NotEmptyS()) {
                if(line_buf.C(0) == '#') { // comment
				}
				else if(line_buf.C(0) == '[') {
					size_t cpos = 0;
                    if(line_buf.StrChr(']', &cpos)) {
						line_buf.Sub(1, cpos-1, set_name);
                    }
				}
				else if(isempty(pSetSymb) || set_name.CmpNC(pSetSymb) == 0) {
					THROW(SETIFZ(p_current_item, rData.CreateNewItem()));
                    if(line_buf.Divide('=', hdr_buf, data_buf) > 0) {
                        hdr_buf.Strip();
                        data_buf.Strip();
                        if(hdr_buf.CmpNC("In") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fIn)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->In, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fIn;
							}
                        }
                        else if(hdr_buf.CmpNC("Out") == 0 || hdr_buf.CmpNC("Output") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fOut)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Out, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fOut;
							}
                        }
                        else if(hdr_buf.CmpNC("Key") == 0) {
							if(!(p_current_item->Flags & (BdtTestItem::fKey|BdtTestItem::fSecret|BdtTestItem::fPassphrase))) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Key, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fKey;
							}
                        }
                        else if(hdr_buf.CmpNC("Secret") == 0) {
							if(!(p_current_item->Flags & (BdtTestItem::fKey|BdtTestItem::fSecret|BdtTestItem::fPassphrase))) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Key, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fSecret;
							}
                        }
                        else if(hdr_buf.CmpNC("Passphrase") == 0) {
							if(!(p_current_item->Flags & (BdtTestItem::fKey|BdtTestItem::fSecret|BdtTestItem::fPassphrase))) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Key, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fPassphrase;
							}
                        }
                        else if(hdr_buf.CmpNC("Salt") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fSalt)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Salt, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fSalt;
							}
                        }
                        else if(hdr_buf.CmpNC("Label") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fLabel)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Label, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fLabel;
							}
                        }
                        else if(hdr_buf.CmpNC("IKM") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fIKM)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->IKM, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fIKM;
							}
                        }
                        else if(hdr_buf.CmpNC("XTS") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fXTS)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->XTS, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fXTS;
							}
                        }
                        else if(hdr_buf.CmpNC("Nonce") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fNonce)) {
								THROW(BdtTestItem::_DecodeTestData(data_buf, p_current_item->Nonce, temp_data_buffer));
								p_current_item->Flags |= BdtTestItem::fNonce;
							}
                        }
                        else if(hdr_buf.CmpNC("OutputLen") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fOutLen)) {
								p_current_item->OutLen = (size_t)data_buf.ToLong();
								p_current_item->Flags |= BdtTestItem::fOutLen;
							}
                        }
                        else if(hdr_buf.CmpNC("Iterations") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fIterations)) {
								p_current_item->Iterations = (uint)data_buf.ToLong();
								p_current_item->Flags |= BdtTestItem::fIterations;
							}
                        }
                        else if(hdr_buf.CmpNC("Seek") == 0) {
							if(!(p_current_item->Flags & BdtTestItem::fSeek)) {
								p_current_item->Seek = (uint)data_buf.ToLong();
								p_current_item->Flags |= BdtTestItem::fSeek;
							}
                        }
                    }
				}
			}
			else {
				p_current_item = 0;
			}
		}
    }
    CATCHZOK
    return ok;
}

#if SLTEST_RUNNING // {

#ifdef TEST_ZLIB_IMPLEMENTATION
	#include <zlib.h>
#endif

SLTEST_R(BDT)
{
	int    ok = 1;
#ifdef _LOCAL_USE_SSL // {
	OpenSSL_add_all_ciphers();
#endif
	{
		// CRC32
		SString in_file_name = MakeInputFilePath("crc32.vec");
		TSCollection <BdtTestItem> data_set;
		THROW(SLTEST_CHECK_NZ(ReadBdtTestData(in_file_name, "CRC32", data_set)));
		{
			SBdtFunct fu(SBdtFunct::Crc32);
			for(uint i = 0; i < data_set.getCount(); i++) {
				BdtTestItem * p_item = data_set.at(i);
				if(p_item) {
					SLTEST_CHECK_EQ(p_item->Out.GetLen(), sizeof(uint32));
					uint32 pattern_value = PTR32(p_item->Out.GetBuf())[0];
					PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
					PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
					PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
					//
					{
						uint32 result;
						size_t out_offs = 0;
						size_t _o;
						THROW(SLTEST_CHECK_NZ(fu.Init()));
						THROW(SLTEST_CHECK_NZ(fu.Update(p_item->In.GetBuf(), p_item->In.GetLen(), &result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLTEST_CHECK_NZ(fu.Finish(&result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLTEST_CHECK_EQ(out_offs, sizeof(result)));
						SLTEST_CHECK_Z(memcmp(&result, &pattern_value, p_item->Out.GetLen()));
					}
					{
						//
						// Проверка старой реализации (которой всю жизнь пользуемся)
						//
						CRC32 cc;
						uint32 result = cc.Calc(0, (const uint8 *)p_item->In.GetBuf(), p_item->In.GetLen());
						SLTEST_CHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
#ifdef TEST_ZLIB_IMPLEMENTATION
					{
						//
						// Проверка реализации zlib
						//
						uint32 result = crc32_z(0, (const uint8 *)p_item->In.GetBuf(), p_item->In.GetLen());
						SLTEST_CHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
#endif
				}
			}
		}
	}
	{
		// ADLER32
		SString in_file_name = MakeInputFilePath("adler32.vec");
		TSCollection <BdtTestItem> data_set;
		THROW(SLTEST_CHECK_NZ(ReadBdtTestData(in_file_name, "Adler32", data_set)));
		{
			SBdtFunct fu(SBdtFunct::Adler32);
			for(uint i = 0; i < data_set.getCount(); i++) {
				BdtTestItem * p_item = data_set.at(i);
				if(p_item) {
					SLTEST_CHECK_EQ(p_item->Out.GetLen(), sizeof(uint32));
					uint32 pattern_value = PTR32(p_item->Out.GetBuf())[0];
					PTR16(&pattern_value)[0] = swapw(PTR16(&pattern_value)[0]);
					PTR16(&pattern_value)[1] = swapw(PTR16(&pattern_value)[1]);
					PTR32(&pattern_value)[0] = swapdw(PTR32(&pattern_value)[0]);
					//
					{
						uint32 result;
						size_t out_offs = 0;
						size_t _o;
						THROW(SLTEST_CHECK_NZ(fu.Init()));
						THROW(SLTEST_CHECK_NZ(fu.Update(p_item->In.GetBuf(), p_item->In.GetLen(), &result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLTEST_CHECK_NZ(fu.Finish(&result, sizeof(result), &_o)));
						out_offs += _o;
						THROW(SLTEST_CHECK_EQ(out_offs, sizeof(result)));
						SLTEST_CHECK_Z(memcmp(&result, &pattern_value, p_item->Out.GetLen()));
					}
#ifdef TEST_ZLIB_IMPLEMENTATION
					{
						//
						// Проверка реализации zlib
						//
						uint32 result = adler32_z(0, 0, 0);
						result = adler32_z(result, (const uint8 *)p_item->In.GetBuf(), p_item->In.GetLen());
						SLTEST_CHECK_Z(memcmp(&result, &pattern_value, sizeof(result)));
					}
#endif
				}
			}
		}
	}
	{
#ifdef _LOCAL_USE_SSL // {
		// IDEA
		SString in_file_name = MakeInputFilePath("idea.vec");
		TSCollection <BdtTestItem> data_set;
		THROW(SLTEST_CHECK_NZ(ReadBdtTestData(in_file_name, "IDEA", data_set)));
		{
			SBdtFunct fu_direct(SBdtFunct::IdeaCrypt);
			SBdtFunct fu_inverse(SBdtFunct::IdeaDecrypt);
			for(uint i = 0; i < data_set.getCount(); i++) {
				BdtTestItem * p_item = data_set.at(i);
				if(p_item && (p_item->Flags & p_item->fIn) && (p_item->Flags & p_item->fOut) && (p_item->Flags & p_item->fKey)) {
					THROW(SLTEST_CHECK_EQ(p_item->Key.GetLen(), 16));
					THROW(SLTEST_CHECK_EQ(p_item->In.GetLen(), p_item->Out.GetLen()));
					//
					{
						STempBuffer result(MAX(p_item->Out.GetLen(), 1024)+16);
						STempBuffer result_decrypt(MAX(p_item->Out.GetLen(), 1024)+16);
						size_t out_offs = 0;
						size_t _o;
						size_t crypt_size = 0;
						THROW(SLTEST_CHECK_NZ(result.IsValid()));
						THROW(SLTEST_CHECK_NZ(result_decrypt.IsValid()));
						memzero(result, result.GetSize());
						THROW(SLTEST_CHECK_NZ(fu_direct.SetParam(SBdtFunct::paramKey, p_item->Key.GetBuf(), p_item->Key.GetLen())));
						THROW(SLTEST_CHECK_NZ(fu_direct.Init()));
						THROW(SLTEST_CHECK_NZ(fu_direct.Update(p_item->In.GetBuf(), p_item->In.GetLen(), result, result.GetSize(), &_o)));
						out_offs += _o;
						THROW(SLTEST_CHECK_NZ(fu_direct.Finish(result+out_offs, result.GetSize()-out_offs, &_o)));
						out_offs += _o;
						crypt_size = out_offs;
						//THROW(SLTEST_CHECK_EQ(out_offs, p_item->In.GetLen()));
						{
							int   lr = 0;
							SLTEST_CHECK_Z(lr = memcmp(result, p_item->Out.GetBuf(), p_item->Out.GetLen()));
							SLTEST_CHECK_Z(lr);
						}
						//
						out_offs = 0;
						memzero(result_decrypt, result.GetSize());
						THROW(SLTEST_CHECK_NZ(fu_inverse.SetParam(SBdtFunct::paramKey, p_item->Key.GetBuf(), p_item->Key.GetLen())));
						THROW(SLTEST_CHECK_NZ(fu_inverse.Init()));
						THROW(SLTEST_CHECK_NZ(fu_inverse.Update(result, crypt_size, result_decrypt, result_decrypt.GetSize(), &_o)));
						out_offs += _o;
						// Для финализации здесь надо подложить последний блок
						THROW(SLTEST_CHECK_NZ(fu_inverse.Finish(result_decrypt+out_offs-_o, _o, &_o)));
						out_offs += _o;
						//THROW(SLTEST_CHECK_EQ(out_offs, p_item->In.GetLen()));
						SLTEST_CHECK_Z(memcmp(result_decrypt, p_item->In.GetBuf(), p_item->In.GetLen()));
					}
				}
			}
		}
#endif // } _LOCAL_USE_SSL 
	}
	CATCHZOK
	return CurrentStatus;
}

SLTEST_R(CalcCheckDigit)
{
	int    ok = 1;
	SString in_file_name = MakeInputFilePath("CalcCheckDigit.txt");
	SString line_buf, left, right;
	SFile f_inp;
	THROW(SLTEST_CHECK_NZ(f_inp.Open(in_file_name, SFile::mRead)));
	while(f_inp.ReadLine(line_buf)) {
		line_buf.Chomp();
		if(line_buf.Divide(':', left, right) > 0) {
			right.Strip();
			if(left.CmpNC("upc") == 0 || left.CmpNC("ean") == 0) {
				SLTEST_CHECK_NZ(isdec(SCalcCheckDigit(SCHKDIGALG_BARCODE, right, right.Len()-1)));
				SLTEST_CHECK_EQ(SCalcCheckDigit(SCHKDIGALG_BARCODE|SCHKDIGALG_TEST, right, right.Len()), 1L);
			}
			else if(left.CmpNC("inn") == 0) {
				SLTEST_CHECK_EQ(SCalcCheckDigit(SCHKDIGALG_RUINN|SCHKDIGALG_TEST, right, right.Len()), 1L);
			}
			else if(left.CmpNC("luhn") == 0) {
				SLTEST_CHECK_NZ(isdec(SCalcCheckDigit(SCHKDIGALG_LUHN, right, right.Len()-1)));
				SLTEST_CHECK_EQ(SCalcCheckDigit(SCHKDIGALG_LUHN|SCHKDIGALG_TEST, right, right.Len()), 1L);
			}
		}
	}
	CATCHZOK
	return CurrentStatus;
}

#endif // } SLTEST_RUNNING
