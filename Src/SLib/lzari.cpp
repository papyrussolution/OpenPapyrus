// LZARI.CPP -- A Data Compression Program
// 
// 1989, 2002 Haruhiko Okumura, Starodub A.
// Use, distribute, and modify this program freely.
// Please send me your improved versions.
//   PC-VAN          SCIENCE
//   NIFTY-Serve     PAF01022
//   CompuServe      74050,1022
// 
#include <slib-internal.h>
#pragma hdrstop
//#include <fcntl.h>
//#include <sys/stat.h>
//
// Defines
//
#define N          4096
#define F            60
#define THRESHOLD     2
#define NIL           N
#define M            15
#define Q1      (1UL << M)
#define Q2      (2 * Q1)
#define Q3      (3 * Q1)
#define Q4      (4 * Q1)
#define MAX_CUM (Q1 - 1)
#define N_CHAR  (256 - THRESHOLD + F)
#define LZARI_SIGNATURE 0x52415A4CL

struct  LZAriFileHeader { // size = 128
	char   FileName[64];
	LDATETIME FileCreatDateTime;
	LDATETIME FileLAccDateTime;
	LDATETIME FileLModDateTime;
	long   FileSize;
	long   CRC;
	int32  BlockSize;
	long   Signature;
	char   Reserve[24];
};

class LZAriBit {
public:
	LZAriBit(FILE *, FILE *, int);
	int    FASTCALL PutBit(int);
	int    FlushBitBuffer();
	int    GetBit();
	ulong  CodeSize;
private:
	uint16 Mask;
	uint16 Buffer;
	FILE * P_InFile;
	FILE * P_OutFile;
};

class LZAriTree {
public:
	LZAriTree();
	~LZAriTree();
	int    Init();
	int    InitTree();
	int    InsertNode(int);
	int    DeleteNode(int);

	uchar * P_TextBuf;
	int    MatchPosition;
	int    MatchLength;
private:
	int16 * P_LSon; // left children
	int16 * P_RSon; // right children
	int16 * P_Dad;	// parents
};

class LZAri {
public:
	LZAri();
	~LZAri();
	void   Destroy();
	int    Init(char *, char *, ulong *, int, PercentFunc);
	ulong  TextSize;
private:
	int    Encode(ulong *, PercentFunc);
	int    Decode(PercentFunc);
	int    StartModel();
	int    FASTCALL UpdateModel(int);
	int    FASTCALL Output(int);
	int    FASTCALL EncodeChar(int);
	int    FASTCALL EncodePosition(int);
	int    EncodeEnd();
	int    FASTCALL BinarySearchSym(uint16);
	int    FASTCALL BinarySearchPos(uint16);
	void   StartDecode();
	int    DecodeChar();
	int    DecodePosition();
	int    SetFileInfo();
	int    GetFileInfo(int);
	int    CheckCrc();
   	int    Shifts;
	ulong  Low;
	ulong  High;
	ulong  Value;
	uint16 * P_SymFreq;
	uint16 * P_SymCum;
	uint16 * P_PositionCum;
	int16 * P_CharToSym;
	int16 * P_SymToChar;
	char  * P_Dest;
	char  * P_Src;
	FILE  * P_InFile;
	FILE  * P_OutFile;
	LZAriBit  * P_Bit;
	LZAriTree * P_Tree;
	LZAriFileHeader * P_Header;
};
//
// Bit I/O
//
LZAriBit::LZAriBit(FILE * pInFile, FILE * pOutFile, int compress) : CodeSize(0), Mask(compress ? 128 : 0), Buffer(0), P_InFile(pInFile), P_OutFile(pOutFile)
{
}

int FASTCALL LZAriBit::PutBit(int bit)  /* Output one bit (bit = 0,1) */
{
	int    ok = 1;
	if(bit)
		Buffer |= Mask;
	if((Mask >>= 1) == 0) {
		if(putc(Buffer, P_OutFile) == EOF)
			ok = 0;
		else {
			ok = 1;
			Buffer = 0;
			Mask = 128;
			CodeSize++;
		}
	}
	return ok;
}

int LZAriBit::FlushBitBuffer()  /* Send remaining bits */
{
	int    ok = 1;
	for(int i = 0; i < 7; i++) {
		ok = PutBit(0);
		if(ok != 1)
			break;
	}
	return ok;
}

int LZAriBit::GetBit()  /* Get one bit (0 or 1) */
{
	if(P_InFile) {
		if((Mask >>= 1) == 0) {
			Buffer = getc(P_InFile);
			Mask = 128;
		}
	}
	return ((Buffer & Mask) != 0);
}
//
// LZSS with multiple binary trees
//
LZAriTree::LZAriTree() : P_TextBuf(0), P_LSon(0), P_RSon(0), P_Dad(0), MatchPosition(0), MatchLength(0)
{
}

LZAriTree::~LZAriTree()
{
	delete [] P_TextBuf;
	delete [] P_LSon;
	delete [] P_RSon;
	delete [] P_Dad;
}

int LZAriTree::Init()
{
	int    ok = 1;
	P_TextBuf = new uchar[N+F-1];
	P_LSon    = new int16[N+1];
	P_RSon    = new int16[N+257];
	P_Dad     = new int16[N+1];
	if(!P_TextBuf || !P_LSon || !P_RSon || !P_Dad)
		ok = 0;
	return ok;
}

int LZAriTree::InitTree()  /* Initialize trees */
{
	int  ok = 1, i = 0;
	/*
		For i = 0 to N - 1, rson[i] and lson[i] will be the right and
		left children of node i.  These nodes need not be initialized.
		Also, dad[i] is the parent of node i.  These are initialized to
		NIL (= N), which stands for 'not used.'
		For i = 0 to 255, rson[N + i + 1] is the root of the tree
		for strings that begin with character i.  These are initialized
		to NIL.  Note there are 256 trees.
	*/
	if(P_RSon && P_Dad) {
		for(i = N + 1; i <= N + 256; i++)
			P_RSon[i] = NIL; /* root */
		for(i = 0; i < N; i++)
			P_Dad[i] = NIL; /* node */
	}
	else
		ok = 0;
	return ok;
}

int LZAriTree::InsertNode(int r)
{
	/*
		Inserts string of length F, text_buf[r..r+F-1], into one of the
		trees (text_buf[r]'th tree) and returns the longest-match position
		and length via the global variables match_position and match_length.
		If match_length = F, then removes the old node in favor of the new
		one, because the old one will be deleted sooner.
		Note r plays double role, as tree node and position in buffer.
	*/
	uchar * key = 0;
	int    ok = 1, i = 0, p = 0, cmp = 1, temp = 0, ret = 0;
	if(P_RSon && P_LSon && P_Dad && P_TextBuf) {
		key = &P_TextBuf[r];
		p = N + 1 + key[0];
		P_RSon[r] = P_LSon[r] = NIL;
		MatchLength = 0;
		for(;;) {
			if(cmp >= 0) {
				if(P_RSon[p] != NIL)
					p = P_RSon[p];
				else {
					P_RSon[p] = r;
					P_Dad[r] = p;
					ret = 1;
					break;
				}
			}
			else {
				if(P_LSon[p] != NIL)
					p = P_LSon[p];
				else {
					P_LSon[p] = r;
					P_Dad[r] = p;
					ret = 1;
					break;
				}
			}
			for(i = 1; i < F; i++)
				if((cmp = key[i] - P_TextBuf[p + i]) != 0)
					break;
			if(i > THRESHOLD) {
				if(i > MatchLength) {
					MatchPosition = (r - p) & (N - 1);
					if((MatchLength = i) >= F)
						break;
				}
				else if(i == MatchLength) {
					if((temp = (r - p) & (N - 1)) < MatchPosition)
						MatchPosition = temp;
				}
			}
		}
		if(!ret) {
			P_Dad[r] = P_Dad[p];
			P_LSon[r] = P_LSon[p];
			P_RSon[r] = P_RSon[p];
			P_Dad[P_LSon[p]] = r;
			P_Dad[P_RSon[p]] = r;
			if(P_RSon[P_Dad[p]] == p)
				P_RSon[P_Dad[p]] = r;
			else
				P_LSon[P_Dad[p]] = r;
			P_Dad[p] = NIL; /* remove p */
		}
	}
	else
		ok = 0;
	return ok;
}

int LZAriTree::DeleteNode(int p)  /* Delete node p from tree */
{
	int    q = 0, ok = 1;
	if(P_RSon && P_LSon && P_Dad) {
		if(P_Dad[p] == NIL)
			ok = -1; /* not in tree */
		else {
			if(P_RSon[p] == NIL)
				q = P_LSon[p];
			else if(P_LSon[p] == NIL)
				q = P_RSon[p];
			else {
				q = P_LSon[p];
				if(P_RSon[q] != NIL) {
					do {
						q = P_RSon[q];
					} while(P_RSon[q] != NIL);
					P_RSon[P_Dad[q]] = P_LSon[q];
					P_Dad[P_LSon[q]] = P_Dad[q];
					P_LSon[q] = P_LSon[p];
					P_Dad[P_LSon[p]] = q;
				}
				P_RSon[q] = P_RSon[p];
				P_Dad[P_RSon[p]] = q;
			}
			P_Dad[q] = P_Dad[p];
			if(P_RSon[P_Dad[p]] == p)
				P_RSon[P_Dad[p]] = q;
			else
				P_LSon[P_Dad[p]] = q;
			P_Dad[p] = NIL;
		}
	}
	else
		ok = 0;
	return ok;
}
//
// Arithmetic Compression
//
LZAri::LZAri() : TextSize(0), Low(0), High(Q4), Value(0), Shifts(0), P_SymFreq(0), P_SymCum(0), P_PositionCum(0),
	P_CharToSym(0), P_SymToChar(0), P_Bit(0), P_Tree(0), P_Header(0), P_Src(0), P_Dest(0), P_InFile(0), P_OutFile(0)
{
}

LZAri::~LZAri()
{
	Destroy();
}

void LZAri::Destroy()
{
	delete [] P_SymFreq;
	delete [] P_SymCum;
	delete [] P_PositionCum;
	delete [] P_CharToSym;
	delete [] P_SymToChar;
	delete P_Bit;
	delete P_Tree;
	delete P_Header;
	delete [] P_Src;
	delete [] P_Dest;
	SFile::ZClose(&P_InFile);
	SFile::ZClose(&P_OutFile);
	P_SymFreq = 0;
	P_SymCum = 0;
	P_PositionCum = 0;
	P_CharToSym = 0;
	P_SymToChar = 0;
	P_Bit = 0;
	P_Tree = 0;
	P_Header = 0;
	P_Src = 0;
	P_Dest = 0;
}

int LZAri::Init(char * pSrc, char * pDest, ulong * pFileSize, int compress, PercentFunc pf)
{
	EXCEPTVAR(SLibError);
	int    ok = 1;
	ulong  file_size = 0;
	if(pSrc && pDest) {
		int    signature_ok = 1;
		P_SymFreq     = new uint16[N_CHAR+1];
		P_SymCum      = new uint16[N_CHAR+1];
		P_PositionCum = new uint16[N+1];
		P_CharToSym   = new int16[N_CHAR];
		P_SymToChar   = new int16[N_CHAR+1];
		P_Tree        = new LZAriTree;
		P_Header      = new LZAriFileHeader;
		P_Src = new char[MAX_PATH];
		P_Dest        = new char[MAX_PATH];
		SLS.SetAddedMsgString(pSrc);
		THROW_V(P_SymFreq && P_SymCum && P_PositionCum && P_CharToSym && P_SymToChar && P_Tree && P_Header, SLERR_NOMEM);
		strnzcpy(P_Src, pSrc, MAX_PATH);
		strnzcpy(P_Dest, pDest, MAX_PATH);
		THROW_V((P_InFile = fopen(P_Src, "rb")) != NULL, SLERR_OPENFAULT);
		THROW_V((P_OutFile = fopen(P_Dest, "w+b")) != NULL, SLERR_OPENFAULT);
		P_Bit = new LZAriBit(P_InFile, P_OutFile, compress);
		THROW_V(P_Bit, SLERR_NOMEM);
		THROW_V(GetFileInfo(compress) > 0, SLERR_NOMEM);
		ok = P_Tree->Init();
		if(compress) {
			THROW(Encode(&file_size, pf) > 0);
		}
		else if(oneof2(P_Header->Signature, LZARI_SIGNATURE, 0)) {
			THROW(Decode(pf) > 0);
			THROW(CheckCrc() > 0);
			file_size = P_Header->FileSize;
		}
		else
			signature_ok = 0;
		if(signature_ok) {
			THROW_V(SetFileInfo(), SLERR_NOMEM);
			ASSIGN_PTR(pFileSize, file_size);
		}
		else {
			fclose(P_OutFile);
			SFile::Remove(P_Dest);
			ok = -1;
		}
	}
	CATCHZOK
	return ok;
}
//
// If you are not familiar with arithmetic compression, you should read
// I. E. Witten, R. M. Neal, and J. G. Cleary,
// Communications of the ACM, Vol. 30, pp. 520-540 (1987), from which much have been borrowed.
//
int LZAri::StartModel()  /* Initialize model */
{
	int    ok = 1;
	if(P_SymCum && P_CharToSym && P_SymToChar && P_SymFreq && P_PositionCum) {
		P_SymCum[N_CHAR] = 0;
		for(int sym = N_CHAR; sym >= 1; sym--) {
			P_CharToSym[sym-1] = sym;
			P_SymToChar[sym] = sym-1;
			P_SymFreq[sym] = 1;
			P_SymCum[sym-1] = P_SymCum[sym] + P_SymFreq[sym];
		}
		P_SymFreq[0] = 0; /* sentinel (!= P_SymFreq[1]) */
		P_PositionCum[N] = 0;
		for(int i = N; i >= 1; i--)
			P_PositionCum[i-1] = P_PositionCum[i] + 10000 / (i + 200);
		/* empirical distribution function (quite tentative) */
		/* Please devise a better mechanism! */
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL LZAri::UpdateModel(int sym)
{
	int    ok = 1, i = 0, c = 0, ch_i = 0, ch_sym = 0;
	if(P_SymCum && P_SymFreq && P_SymToChar && P_CharToSym) {
		if(P_SymCum[0] >= MAX_CUM) {
			for(i = N_CHAR; i > 0; i--) {
				P_SymCum[i] = c;
				c += (P_SymFreq[i] = (P_SymFreq[i] + 1) >> 1);
			}
			P_SymCum[0] = c;
		}
		i = sym;
		while(P_SymFreq[i] == P_SymFreq[i-1])
			i--;
		if(i < sym) {
			ch_i   = P_SymToChar[i];
			ch_sym = P_SymToChar[sym];
			P_SymToChar[i] = ch_sym;
			P_SymToChar[sym] = ch_i;
			P_CharToSym[ch_i] = sym;
			P_CharToSym[ch_sym] = i;
		}
		P_SymFreq[i]++;
		while(--i >= 0)
			P_SymCum[i]++;
	}
	else
		ok = 0;
	return ok;
}

int FASTCALL LZAri::Output(int bit)  /* Output 1 bit, followed by its complements */
{
	int    ok = P_Bit->PutBit(bit);
	while(Shifts > 0 && ok > 0) {
		ok = P_Bit->PutBit(!bit);
		Shifts--;
   	}
	return ok;
}

int FASTCALL LZAri::EncodeChar(int ch)
{
	int    ok = 1;
	ulong  range = High - Low;
	int    sym = P_CharToSym[ch];
	High = Low + (range * P_SymCum[sym-1]) / P_SymCum[0];
	Low += (range * P_SymCum[sym]) / P_SymCum[0];
	for(;;) {
		if(High <= Q2) {
			THROW(Output(0) > 0);
		}
		else if(Low >= Q2) {
			THROW(Output(1) > 0);
			Low -= Q2;
			High -= Q2;
		}
		else if(Low >= Q1 && High <= Q3) {
			Shifts++;
			Low -= Q1;
			High -= Q1;
		}
		else
			break;
		Low += Low;
		High += High;
	}
	UpdateModel(sym);
	CATCHZOK
	return ok;
}

int FASTCALL LZAri::EncodePosition(int position)
{
	int    ok = 1;
	ulong  range = High - Low;
	High = Low + (range * P_PositionCum[position]) / P_PositionCum[0];
	Low += (range * P_PositionCum[position+1]) / P_PositionCum[0];
	for(;;) {
		if(High <= Q2) {
			THROW(Output(0) > 0);
		}
		else if(Low >= Q2) {
			THROW(Output(1) > 0);
			Low -= Q2;
			High -= Q2;
		}
		else if(Low >= Q1 && High <= Q3) {
			Shifts++;
			Low -= Q1;
			High -= Q1;
		}
		else
			break;
		Low += Low;
		High += High;
	}
	CATCHZOK
	return ok;
}

int LZAri::EncodeEnd()
{
	int    ok = 1;
	Shifts++;
	THROW(Output((Low < Q1) ? 0 : 1) > 0);
	THROW(P_Bit->FlushBitBuffer() > 0); /* flush bits remaining in buffer */
	CATCHZOK
	return ok;
}

int FASTCALL LZAri::BinarySearchSym(uint16 x)
		/* 1      if x >= sym_cum[1],
		   N_CHAR if sym_cum[N_CHAR] > x,
		   i such that sym_cum[i - 1] > x >= sym_cum[i] otherwise */
{
	int    i = 1, j = N_CHAR;
	while(i < j) {
		int    k = (i + j) / 2;
		if(P_SymCum[k] > x)
			i = k + 1;
		else
			j = k;
	}
	return i;
}
/*
	0 if x >= position_cum[1],
	N - 1 if position_cum[N] > x,
	i such that position_cum[i] > x >= position_cum[i+1] otherwise
*/
int FASTCALL LZAri::BinarySearchPos(uint16 x)
{
	int    i = 1, j = N;
	while(i < j) {
		int    k = (i + j) / 2;
		if(P_PositionCum[k] > x)
			i = k + 1;
		else
			j = k;
	}
	return i - 1;
}

void LZAri::StartDecode()
{
	for(int i = 0; i < M + 2; i++)
		Value = 2 * Value + P_Bit->GetBit();
}

int LZAri::DecodeChar()
{
	int    ch = 0;
	ulong  range = High - Low;
	int    sym = BinarySearchSym(static_cast<uint16>(((Value - Low + 1) * P_SymCum[0] - 1) / range));
	High = Low + (range * P_SymCum[sym - 1]) / P_SymCum[0];
	Low += (range * P_SymCum[sym]) / P_SymCum[0];
	for(;;) {
		if(Low >= Q2) {
			Value -= Q2;
			Low -= Q2;
			High -= Q2;
		}
		else if(Low >= Q1 && High <= Q3) {
			Value -= Q1;
			Low -= Q1;
			High -= Q1;
		}
		else if(High > Q2)
			break;
		Low += Low;
		High += High;
		Value = 2 * Value + P_Bit->GetBit();
	}
	ch = P_SymToChar[sym];
	UpdateModel(sym);
	return ch;
}

int LZAri::DecodePosition()
{
	ulong  range = High - Low;
	int    position = BinarySearchPos(static_cast<uint16>(((Value - Low + 1) * P_PositionCum[0] - 1) / range));
	High = Low + (range * P_PositionCum[position]) / P_PositionCum[0];
	Low += (range * P_PositionCum[position+1]) / P_PositionCum[0];
	for(;;) {
		if(Low >= Q2) {
			Value -= Q2;
			Low -= Q2;
			High -= Q2;
		}
		else if(Low >= Q1 && High <= Q3) {
			Value -= Q1;
			Low -= Q1;
			High -= Q1;
		}
		else if(High > Q2)
			break;
		Low += Low;
		High += High;
		Value = 2 * Value + P_Bit->GetBit();
	}
	return position;
}
//
// Encode and Decode
//
int LZAri::Encode(ulong * pFileSize, PercentFunc pf)
{
	int    ok = -1, i = 0, c = 0, len = 0, r = N - F, s = 0, last_match_length = 0;
	ulong  print_count = 0, file_size = TextSize;
	THROW_S(fwrite(P_Header, sizeof(LZAriFileHeader), 1, P_OutFile), SLERR_WRITEFAULT);
	file_size = TextSize = P_Header->FileSize;
	if(!TextSize)
		ok = -1;
	else {
		P_Bit->CodeSize += sizeof(LZAriFileHeader);
		TextSize = 0;
		THROW_S(StartModel() > 0 && P_Tree->InitTree() > 0, SLERR_CANTCOMPRESS);
		for(i = s; i < r; i++)
			P_Tree->P_TextBuf[i] = ' ';
		for(len = 0; len < F && (c = getc(P_InFile)) != EOF; len++)
			P_Tree->P_TextBuf[r+len] = static_cast<uchar>(c);
		TextSize = len;
		for(i = 1; i <= F; i++)
			THROW(P_Tree->InsertNode(r - i) > 0);
		THROW(P_Tree->InsertNode(r) > 0);
		do {
			if(P_Tree->MatchLength > len)
				P_Tree->MatchLength = len;
			if(P_Tree->MatchLength <= THRESHOLD) {
				P_Tree->MatchLength = 1;
				THROW(EncodeChar(P_Tree->P_TextBuf[r]) > 0);
			}
			else {
				THROW(EncodeChar(255 - THRESHOLD + P_Tree->MatchLength) > 0);
				THROW(EncodePosition(P_Tree->MatchPosition - 1) > 0);
			}
			last_match_length = P_Tree->MatchLength;
			for(i = 0; i < last_match_length && (c = getc(P_InFile)) != EOF; i++) {
				THROW(P_Tree->DeleteNode(s));
				P_Tree->P_TextBuf[s] = static_cast<uchar>(c);
				if(s < F - 1)
					P_Tree->P_TextBuf[s+N] = static_cast<uchar>(c);
				s = (s + 1) & (N - 1);
				r = (r + 1) & (N - 1);
				THROW(P_Tree->InsertNode(r) > 0);
			}
			if((TextSize += i) > print_count) {
				if(pf)
					THROW_S(pf(print_count / 1024, file_size / 1024, P_Src, 1), SLERR_USERBREAK);
				print_count += 1024;
			}
			while(i++ < last_match_length) {
				THROW(P_Tree->DeleteNode(s));
				s = (s + 1) & (N - 1);
				r = (r + 1) & (N - 1);
				if(--len)
					THROW(P_Tree->InsertNode(r) > 0);
			}
		} while(len > 0);
		THROW(EncodeEnd() > 0);
		ok = 1;
	}
	ASSIGN_PTR(pFileSize, P_Bit->CodeSize);
	CATCHZOK
	return ok;
}

int LZAri::Decode(PercentFunc pf)
{
	int    i = 0, j = 0, k = 0, r = N - F, c = 0, ok = 0;
	ulong  count = 0, file_size = 0, print_count = 0;
	file_size = TextSize = P_Header->FileSize;
	if(!TextSize)
		ok = -1;
	else {
		StartDecode();
		THROW_S(StartModel() > 0, SLERR_CANTDECOMPRESS);
		for(i = 0; i < N - F; i++)
			P_Tree->P_TextBuf[i] = ' ';
		for(count = 0; count < TextSize;) {
			c = DecodeChar();
			if(c < 256) {
 				putc(c, P_OutFile);
				P_Tree->P_TextBuf[r++] = static_cast<uchar>(c);
				r &= (N - 1);
				count++;
			}
			else {
				i = (r - DecodePosition() - 1) & (N - 1);
				j = c - 255 + THRESHOLD;
				for(k = 0; k < j; k++) {
					c = P_Tree->P_TextBuf[(i + k) & (N - 1)];
					putc((uchar)c, P_OutFile);
					P_Tree->P_TextBuf[r++] = static_cast<uchar>(c);
					r &= (N - 1);
					count++;
				}
			}
			if(count > print_count) {
				if(pf)
					THROW_S(pf(count / 1024, file_size / 1024, P_Src, 1), SLERR_USERBREAK);
				print_count += 1024;
			}
		}
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int LZAri::SetFileInfo()
{
	int    ok = -1;
	if(P_Header) {
		const LDATETIME creation_time    = P_Header->FileCreatDateTime;
		const LDATETIME last_access_time = P_Header->FileLAccDateTime;
		const LDATETIME last_modif_time  = P_Header->FileLModDateTime;
	// Set output file info
#ifndef __WIN32__
		rewind(P_OutFile);
		THROW(setFileTime(fileno(P_OutFile), &creation_time, &last_access_time, &last_modif_time) > 0);
#else
		fclose(P_OutFile);
		P_OutFile = 0;
		
		SIntHandle h_file = ::CreateFile(SUcSwitch(P_Dest), GENERIC_WRITE, 0, NULL, OPEN_EXISTING, FILE_ATTRIBUTE_NORMAL, 0);
		THROW_S(h_file, SLERR_OPENFAULT);
		SFile::SetTime(h_file, &creation_time, &last_access_time, &last_modif_time);
		if(h_file > 0)
			CloseHandle(h_file);
#endif
		ok = 1;
	}
	CATCHZOK
	return ok;
}

int LZAri::GetFileInfo(int compress)
{
	//EXCEPTVAR(SLibError);
	int    ok = -1;
	const size_t buf_size = 512;
	char  * p_buf = 0;
	SString filename;
	uint32 len = 0;
	ulong  crc = 0;
	SCRC32 _crc32;
	THROW_S(p_buf = new char[buf_size], SLERR_NOMEM);
	if(P_Header) {
	   	memzero(P_Header, sizeof(*P_Header));
		if(compress) {
			fclose(P_InFile);
			P_InFile = 0;
			SFsPath ps;
			SFile::Stat fs;
			SFile::GetStat(P_Src, 0, &fs, 0);
			THROW_S_S((P_InFile = fopen(P_Src, "rb")) != NULL, SLERR_OPENFAULT, P_Src);
			ps.Split(P_Src);
			(filename = ps.Nam).Cat(ps.Ext);
			crc = 0;
			while((len = fread(p_buf, 1, buf_size, P_InFile)) > 0)
				crc = _crc32.Calc(crc, p_buf, len);
			rewind(P_InFile);
			P_Header->FileSize  = (long)fs.Size;
			P_Header->FileCreatDateTime = fs.CrtTime;
			P_Header->FileLAccDateTime  = fs.AccsTime;
			P_Header->FileLModDateTime  = fs.ModTime;
			P_Header->CRC       = (long)crc;
			P_Header->Signature = LZARI_SIGNATURE;
			filename.CopyTo(P_Header->FileName, sizeof(P_Header->FileName));
		}
		else {
			THROW_S_S(fread(P_Header, sizeof(LZAriFileHeader), 1, P_InFile), SLERR_READFAULT, P_Src);
		}
		ok = 1;
	}
	CATCHZOK
	delete [] p_buf;
	return ok;
}

int LZAri::CheckCrc()
{
	// @v10.0.0 EXCEPTVAR(SLibError);
	const size_t buf_size = 512;
	int    ok = 1;
	char * p_buf = 0;
	uint32 len = 0;
	ulong  crc = 0;
	SCRC32 _crc32;
	THROW_S(p_buf = new char[buf_size], SLERR_NOMEM);
	rewind(P_OutFile);
	while((len = fread(p_buf, 1, buf_size, P_OutFile)) > 0)
		crc = _crc32.Calc(crc, p_buf, len);
	THROW_S_S((long)crc == P_Header->CRC, SLERR_INVALIDCRC, "");
	CATCHZOK
	delete [] p_buf;
	return ok;
}

int DoCompress(const char * pSrc, const char * pDest, int64 * pFileSize, int compress, PercentFunc pf)
{
	char   src_path[MAX_PATH], dest_path[MAX_PATH];
	STRNSCPY(src_path, pSrc);
	STRNSCPY(dest_path, pDest);
	LZAri  lz_ari;
	ulong  size = 0;
	int    ok = lz_ari.Init(src_path, dest_path, &size, compress, pf);
	ASSIGN_PTR(pFileSize, (int64)size);
	return ok;
}
