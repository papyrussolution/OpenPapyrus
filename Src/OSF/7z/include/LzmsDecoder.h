// LzmsDecoder.h
// The code is based on LZMS description from wimlib code

#ifndef __LZMS_DECODER_H
#define __LZMS_DECODER_H

// #define SHOW_DEBUG_INFO

#ifdef SHOW_DEBUG_INFO
	#define PRF(x) x
#else
	// #define PRF(x)
#endif
//#include <ICoder.h>

namespace NCompress {
namespace NLzms {
class CBitDecoder {
public:
	const  Byte * _buf;
	uint   _bitPos;
	void   Init(const Byte * buf, size_t size) throw()
	{
		_buf = buf + size;
		_bitPos = 0;
	}
	uint32 GetValue(uint numBits) const
	{
		uint32 v = ((uint32)_buf[-1] << 16) | ((uint32)_buf[-2] << 8) | (uint32)_buf[-3];
		v >>= (24 - numBits - _bitPos);
		return v & ((1 << numBits) - 1);
	}
	void MovePos(uint numBits)
	{
		_bitPos += numBits;
		_buf -= (_bitPos >> 3);
		_bitPos &= 7;
	}
	uint32 ReadBits32(uint numBits)
	{
		uint32 mask = (((uint32)1 << numBits) - 1);
		numBits += _bitPos;
		const Byte * buf = _buf;
		uint32 v = GetUi32(buf - 4);
		if(numBits > 32) {
			v <<= (numBits - 32);
			v |= (uint32)buf[-5] >> (40 - numBits);
		}
		else
			v >>= (32 - numBits);
		_buf = buf - (numBits >> 3);
		_bitPos = numBits & 7;
		return v & mask;
	}
};

const uint k_NumLitSyms = 256;
const uint k_NumLenSyms = 54;
const uint k_NumPosSyms = 799;
const uint k_NumPowerSyms = 8;

const uint k_NumProbBits = 6;
const uint k_ProbLimit = 1 << k_NumProbBits;
const uint k_InitialProb = 48;
const uint32 k_InitialHist = 0x55555555;

const uint k_NumReps = 3;

const uint k_NumMainProbs  = 16;
const uint k_NumMatchProbs = 32;
const uint k_NumRepProbs   = 64;

const uint k_NumHuffmanBits = 15;

template <uint32 m_NumSyms, uint32 m_RebuildFreq, unsigned numTableBits>
class CHuffDecoder : public NCompress::NHuffman::CDecoder<k_NumHuffmanBits, m_NumSyms, numTableBits> {
public:
	uint32 RebuildRem;
	uint32 NumSyms;
	uint32 Freqs[m_NumSyms];

	void Generate() throw()
	{
		uint32 vals[m_NumSyms];
		Byte levels[m_NumSyms];
		// We need to check that our algorithm is OK, when optimal Huffman tree uses more than 15 levels !!!
		Huffman_Generate(Freqs, vals, levels, NumSyms, k_NumHuffmanBits);
		/*
		   for(uint32 i = NumSyms; i < m_NumSyms; i++)
		   levels[i] = 0;
		 */
		this->BuildFull(levels, NumSyms);
	}

	void Rebuild() throw()
	{
		Generate();
		RebuildRem = m_RebuildFreq;
		uint32 num = NumSyms;
		for(uint32 i = 0; i < num; i++)
			Freqs[i] = (Freqs[i] >> 1) + 1;
	}

public:
	void Init(uint32 numSyms = m_NumSyms) throw()
	{
		RebuildRem = m_RebuildFreq;
		NumSyms = numSyms;
		for(uint32 i = 0; i < numSyms; i++)
			Freqs[i] = 1;
		// for(; i < m_NumSyms; i++) Freqs[i] = 0;
		Generate();
	}
};

struct CProbEntry {
	uint32 Prob;
	uint64 Hist;
	void Init()
	{
		Prob = k_InitialProb;
		Hist = k_InitialHist;
	}
	uint32 GetProb() const throw()
	{
		uint32 prob = Prob;
		if(prob == 0)
			prob = 1;
		else if(prob == k_ProbLimit)
			prob = k_ProbLimit - 1;
		return prob;
	}
	void Update(unsigned bit) throw()
	{
		Prob += (int32)(Hist >> (k_ProbLimit - 1)) - (int32)bit;
		Hist = (Hist << 1) | bit;
	}
};

struct CRangeDecoder {
	uint32 range;
	uint32 code;
	const Byte * cur;
	// const Byte *end;

	void Init(const Byte * data, size_t /* size */) throw()
	{
		range = 0xFFFFFFFF;
		code = (((uint32)GetUi16(data)) << 16) | GetUi16(data + 2);
		cur = data + 4;
		// end = data + size;
	}

	void Normalize()
	{
		if(range <= 0xFFFF) {
			range <<= 16;
			code <<= 16;
			// if(cur >= end) throw 1;
			code |= GetUi16(cur);
			cur += 2;
		}
	}

	unsigned Decode(uint32 * state, uint32 numStates, struct CProbEntry * probs)
	{
		uint32 st = *state;
		CProbEntry * entry = &probs[st];
		st = (st << 1) & (numStates - 1);

		uint32 prob = entry->GetProb();

		if(range <= 0xFFFF) {
			range <<= 16;
			code <<= 16;
			// if(cur >= end) throw 1;
			code |= GetUi16(cur);
			cur += 2;
		}

		uint32 bound = (range >> k_NumProbBits) * prob;

		if(code < bound) {
			range = bound;
			* state = st;
			entry->Update(0);
			return 0;
		}
		else {
			range -= bound;
			code -= bound;
			* state = st | 1;
			entry->Update(1);
			return 1;
		}
	}
};

class CDecoder
{
	// CRangeDecoder _rc;
	// CBitDecoder _bs;
	size_t _pos;

	uint32 _reps[k_NumReps + 1];
	uint64 _deltaReps[k_NumReps + 1];

	uint32 mainState;
	uint32 matchState;
	uint32 lzRepStates[k_NumReps];
	uint32 deltaRepStates[k_NumReps];

	struct CProbEntry mainProbs[k_NumMainProbs];

	struct CProbEntry matchProbs[k_NumMatchProbs];

	struct CProbEntry lzRepProbs[k_NumReps][k_NumRepProbs];

	struct CProbEntry deltaRepProbs[k_NumReps][k_NumRepProbs];

	CHuffDecoder<k_NumLitSyms, 1024, 9> m_LitDecoder;
	CHuffDecoder<k_NumPosSyms, 1024, 9> m_PosDecoder;
	CHuffDecoder<k_NumLenSyms, 512, 8> m_LenDecoder;
	CHuffDecoder<k_NumPowerSyms, 512, 6> m_PowerDecoder;
	CHuffDecoder<k_NumPosSyms, 1024, 9> m_DeltaDecoder;

	int32 * _x86_history;

	HRESULT CodeReal(const Byte * in, size_t inSize, Byte * out, size_t outSize);
public:
	CDecoder();
	~CDecoder();

	HRESULT Code(const Byte * in, size_t inSize, Byte * out, size_t outSize);
	const size_t GetUnpackSize() const {
		return _pos;
	}
};
}
}

#endif
