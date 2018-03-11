// Bcj2.c -- BCJ2 Decoder (Converter for x86 code)
// 2017-04-03 : Igor Pavlov : Public domain 
// 
#include <7z-internal.h>
#pragma hdrstop

// #define SHOW_STAT 
#ifdef SHOW_STAT
	#define PRF(x) x
#else
	#define PRF(x)
#endif
#define CProb uint16
#define kTopValue ((uint32)1 << 24)
#define kNumModelBits 11
#define kBitModelTotal (1 << kNumModelBits)
#define kNumMoveBits 5
#define _IF_BIT_0 ttt = *prob; bound = (p->range >> kNumModelBits) * ttt; if(p->code < bound)
#define _UPDATE_0 p->range = bound; *prob = (CProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
#define _UPDATE_1 p->range -= bound; p->code -= bound; *prob = (CProb)(ttt - (ttt >> kNumMoveBits));

void Bcj2Dec_Init(CBcj2Dec * p)
{
	p->state = BCJ2_DEC_STATE_OK;
	p->ip = 0;
	p->temp[3] = 0;
	p->range = 0;
	p->code = 0;
	for(uint i = 0; i < SIZEOFARRAY(p->probs); i++)
		p->probs[i] = kBitModelTotal >> 1;
}

SRes Bcj2Dec_Decode(CBcj2Dec * p)
{
	if(p->range <= 5) {
		p->state = BCJ2_DEC_STATE_OK;
		for(; p->range != 5; p->range++) {
			if(p->range == 1 && p->code != 0)
				return SZ_ERROR_DATA;
			if(p->bufs[BCJ2_STREAM_RC] == p->lims[BCJ2_STREAM_RC]) {
				p->state = BCJ2_STREAM_RC;
				return SZ_OK;
			}
			p->code = (p->code << 8) | *(p->bufs[BCJ2_STREAM_RC])++;
		}
		if(p->code == 0xFFFFFFFF)
			return SZ_ERROR_DATA;
		p->range = 0xFFFFFFFF;
	}
	else if(p->state >= BCJ2_DEC_STATE_ORIG_0) {
		while(p->state <= BCJ2_DEC_STATE_ORIG_3) {
			Byte * dest = p->dest;
			if(dest == p->destLim)
				return SZ_OK;
			*dest = p->temp[(size_t)p->state - BCJ2_DEC_STATE_ORIG_0];
			p->state++;
			p->dest = dest + 1;
		}
	}
	/*
	   if(BCJ2_IS_32BIT_STREAM(p->state)) {
	   const Byte *cur = p->bufs[p->state];
	   if(cur == p->lims[p->state])
	    return SZ_OK;
	   p->bufs[p->state] = cur + 4;
	   {
	    uint32 val;
	    Byte *dest;
	    SizeT rem;

	    p->ip += 4;
	    val = GetBe32(cur) - p->ip;
	    dest = p->dest;
	    rem = p->destLim - dest;
	    if(rem < 4) {
	      SizeT i;
	      SetUi32(p->temp, val);
	      for(i = 0; i < rem; i++)
	        dest[i] = p->temp[i];
	      p->dest = dest + rem;
	      p->state = BCJ2_DEC_STATE_ORIG_0 + (uint)rem;
	      return SZ_OK;
	    }
	    SetUi32(dest, val);
	    p->temp[3] = (Byte)(val >> 24);
	    p->dest = dest + 4;
	    p->state = BCJ2_DEC_STATE_OK;
	   }
	   }
	 */
	for(;; ) {
		if(BCJ2_IS_32BIT_STREAM(p->state))
			p->state = BCJ2_DEC_STATE_OK;
		else {
			if(p->range < kTopValue) {
				if(p->bufs[BCJ2_STREAM_RC] == p->lims[BCJ2_STREAM_RC]) {
					p->state = BCJ2_STREAM_RC;
					return SZ_OK;
				}
				p->range <<= 8;
				p->code = (p->code << 8) | *(p->bufs[BCJ2_STREAM_RC])++;
			}
			{
				const Byte * src = p->bufs[BCJ2_STREAM_MAIN];
				const Byte * srcLim;
				Byte * dest;
				SizeT num = p->lims[BCJ2_STREAM_MAIN] - src;
				if(num == 0) {
					p->state = BCJ2_STREAM_MAIN;
					return SZ_OK;
				}
				dest = p->dest;
				if(num > (SizeT)(p->destLim - dest)) {
					num = p->destLim - dest;
					if(num == 0) {
						p->state = BCJ2_DEC_STATE_ORIG;
						return SZ_OK;
					}
				}
				srcLim = src + num;
				if(p->temp[3] == 0x0F && (src[0] & 0xF0) == 0x80)
					*dest = src[0];
				else {
					for(;; ) {
						Byte b = *src;
						*dest = b;
						if(b != 0x0F) {
							if((b & 0xFE) == 0xE8)
								break;
							dest++;
							if(++src != srcLim)
								continue;
							break;
						}
						dest++;
						if(++src == srcLim)
							break;
						if((*src & 0xF0) != 0x80)
							continue;
						*dest = *src;
						break;
					}
				}
				num = src - p->bufs[BCJ2_STREAM_MAIN];
				if(src == srcLim) {
					p->temp[3] = src[-1];
					p->bufs[BCJ2_STREAM_MAIN] = src;
					p->ip += (uint32)num;
					p->dest += num;
					p->state = (p->bufs[BCJ2_STREAM_MAIN] == p->lims[BCJ2_STREAM_MAIN]) ? (uint)BCJ2_STREAM_MAIN : (uint)BCJ2_DEC_STATE_ORIG;
					return SZ_OK;
				}
				{
					uint32 bound, ttt;
					CProb * prob;
					Byte b = src[0];
					Byte prev = (Byte)(num == 0 ? p->temp[3] : src[-1]);
					p->temp[3] = b;
					p->bufs[BCJ2_STREAM_MAIN] = src + 1;
					num++;
					p->ip += (uint32)num;
					p->dest += num;
					prob = p->probs + (uint)(b == 0xE8 ? 2 + (uint)prev : (b == 0xE9 ? 1 : 0));
					_IF_BIT_0
					{
						_UPDATE_0
						continue;
					}
					_UPDATE_1
				}
			}
		}
		{
			uint32 val;
			uint   cj = (p->temp[3] == 0xE8) ? BCJ2_STREAM_CALL : BCJ2_STREAM_JUMP;
			const Byte * cur = p->bufs[cj];
			Byte * dest;
			SizeT rem;
			if(cur == p->lims[cj]) {
				p->state = cj;
				break;
			}
			val = GetBe32(cur);
			p->bufs[cj] = cur + 4;
			p->ip += 4;
			val -= p->ip;
			dest = p->dest;
			rem = p->destLim - dest;
			if(rem < 4) {
				SizeT i;
				SetUi32(p->temp, val);
				for(i = 0; i < rem; i++)
					dest[i] = p->temp[i];
				p->dest = dest + rem;
				p->state = BCJ2_DEC_STATE_ORIG_0 + (uint)rem;
				break;
			}
			SetUi32(dest, val);
			p->temp[3] = (Byte)(val >> 24);
			p->dest = dest + 4;
		}
	}
	if(p->range < kTopValue && p->bufs[BCJ2_STREAM_RC] != p->lims[BCJ2_STREAM_RC]) {
		p->range <<= 8;
		p->code = (p->code << 8) | *(p->bufs[BCJ2_STREAM_RC])++;
	}
	return SZ_OK;
}
//
// Bcj2Enc.c -- BCJ2 Encoder (Converter for x86 code)
void Bcj2Enc_Init(CBcj2Enc * p)
{
	p->state = BCJ2_ENC_STATE_OK;
	p->finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
	p->prevByte = 0;
	p->cache = 0;
	p->range = 0xFFFFFFFF;
	p->low = 0;
	p->cacheSize = 1;
	p->ip = 0;
	p->fileIp = 0;
	p->fileSize = 0;
	p->relatLimit = BCJ2_RELAT_LIMIT;
	p->tempPos = 0;
	p->flushPos = 0;
	for(uint i = 0; i < SIZEOFARRAY(p->probs); i++)
		p->probs[i] = kBitModelTotal >> 1;
}

static Bool FASTCALL RangeEnc_ShiftLow(CBcj2Enc * p)
{
	if((uint32)p->low < (uint32)0xFF000000 || (uint32)(p->low >> 32) != 0) {
		Byte * buf = p->bufs[BCJ2_STREAM_RC];
		do {
			if(buf == p->lims[BCJ2_STREAM_RC]) {
				p->state = BCJ2_STREAM_RC;
				p->bufs[BCJ2_STREAM_RC] = buf;
				return True;
			}
			*buf++ = (Byte)(p->cache + (Byte)(p->low >> 32));
			p->cache = 0xFF;
		} while(--p->cacheSize);
		p->bufs[BCJ2_STREAM_RC] = buf;
		p->cache = (Byte)((uint32)p->low >> 24);
	}
	p->cacheSize++;
	p->low = (uint32)p->low << 8;
	return False;
}

static void Bcj2Enc_Encode_2(CBcj2Enc * p)
{
	if(BCJ2_IS_32BIT_STREAM(p->state)) {
		Byte * cur = p->bufs[p->state];
		if(cur == p->lims[p->state])
			return;
		SetBe32(cur, p->tempTarget);
		p->bufs[p->state] = cur + 4;
	}
	p->state = BCJ2_ENC_STATE_ORIG;
	for(;; ) {
		if(p->range < kTopValue) {
			if(RangeEnc_ShiftLow(p))
				return;
			p->range <<= 8;
		}
		{
			{
				const Byte * src = p->src;
				const Byte * srcLim;
				Byte * dest;
				SizeT num = p->srcLim - src;
				if(p->finishMode == BCJ2_ENC_FINISH_MODE_CONTINUE) {
					if(num <= 4)
						return;
					num -= 4;
				}
				else if(num == 0)
					break;
				dest = p->bufs[BCJ2_STREAM_MAIN];
				if(num > (SizeT)(p->lims[BCJ2_STREAM_MAIN] - dest)) {
					num = p->lims[BCJ2_STREAM_MAIN] - dest;
					if(num == 0) {
						p->state = BCJ2_STREAM_MAIN;
						return;
					}
				}
				srcLim = src + num;
				if(p->prevByte == 0x0F && (src[0] & 0xF0) == 0x80)
					*dest = src[0];
				else {
					for(;; ) {
						Byte b = *src;
						*dest = b;
						if(b != 0x0F) {
							if((b & 0xFE) == 0xE8)
								break;
							dest++;
							if(++src != srcLim)
								continue;
							break;
						}
						dest++;
						if(++src == srcLim)
							break;
						if((*src & 0xF0) != 0x80)
							continue;
						*dest = *src;
						break;
					}
				}
				num = src - p->src;
				if(src == srcLim) {
					p->prevByte = src[-1];
					p->bufs[BCJ2_STREAM_MAIN] = dest;
					p->src = src;
					p->ip += (uint32)num;
					continue;
				}
				{
					Byte context = (Byte)(num == 0 ? p->prevByte : src[-1]);
					Bool needConvert;
					p->bufs[BCJ2_STREAM_MAIN] = dest + 1;
					p->ip += (uint32)num + 1;
					src++;
					needConvert = False;
					if((SizeT)(p->srcLim - src) >= 4) {
						uint32 relatVal = GetUi32(src);
						if((p->fileSize == 0 || (uint32)(p->ip + 4 + relatVal - p->fileIp) < p->fileSize) && ((relatVal + p->relatLimit) >> 1) < p->relatLimit)
							needConvert = True;
					}
					{
						Byte b = src[-1];
						CProb * prob = p->probs + (uint)(b == 0xE8 ? 2 + (uint)context : (b == 0xE9 ? 1 : 0));
						uint   ttt = *prob;
						uint32 bound = (p->range >> kNumModelBits) * ttt;
						if(!needConvert) {
							p->range = bound;
							*prob = (CProb)(ttt + ((kBitModelTotal - ttt) >> kNumMoveBits));
							p->src = src;
							p->prevByte = b;
							continue;
						}
						p->low += bound;
						p->range -= bound;
						*prob = (CProb)(ttt - (ttt >> kNumMoveBits));
						{
							uint32 relatVal = GetUi32(src);
							uint32 absVal;
							p->ip += 4;
							absVal = p->ip + relatVal;
							p->prevByte = src[3];
							src += 4;
							p->src = src;
							{
								uint   cj = (b == 0xE8) ? BCJ2_STREAM_CALL : BCJ2_STREAM_JUMP;
								Byte * cur = p->bufs[cj];
								if(cur == p->lims[cj]) {
									p->state = cj;
									p->tempTarget = absVal;
									return;
								}
								SetBe32(cur, absVal);
								p->bufs[cj] = cur + 4;
							}
						}
					}
				}
			}
		}
	}
	if(p->finishMode != BCJ2_ENC_FINISH_MODE_END_STREAM)
		return;
	for(; p->flushPos < 5; p->flushPos++)
		if(RangeEnc_ShiftLow(p))
			return;
	p->state = BCJ2_ENC_STATE_OK;
}

void Bcj2Enc_Encode(CBcj2Enc * p)
{
	PRF(printf("\n"));
	PRF(printf("---- ip = %8d   tempPos = %8d   src = %8d\n", p->ip, p->tempPos, p->srcLim - p->src));
	if(p->tempPos != 0) {
		uint   extra = 0;
		for(;; ) {
			const Byte * src = p->src;
			const Byte * srcLim = p->srcLim;
			uint   finishMode = p->finishMode;
			p->src = p->temp;
			p->srcLim = p->temp + p->tempPos;
			if(src != srcLim)
				p->finishMode = BCJ2_ENC_FINISH_MODE_CONTINUE;
			PRF(printf("     ip = %8d   tempPos = %8d   src = %8d\n", p->ip, p->tempPos, p->srcLim - p->src));
			Bcj2Enc_Encode_2(p);
			{
				uint   num = (uint)(p->src - p->temp);
				uint   tempPos = p->tempPos - num;
				uint   i;
				p->tempPos = tempPos;
				for(i = 0; i < tempPos; i++)
					p->temp[i] = p->temp[(size_t)i + num];
				p->src = src;
				p->srcLim = srcLim;
				p->finishMode = (EBcj2Enc_FinishMode)finishMode;
				if(p->state != BCJ2_ENC_STATE_ORIG || src == srcLim)
					return;
				if(extra >= tempPos) {
					p->src = src - tempPos;
					p->tempPos = 0;
					break;
				}
				p->temp[tempPos] = src[0];
				p->tempPos = tempPos + 1;
				p->src = src + 1;
				extra++;
			}
		}
	}
	PRF(printf("++++ ip = %8d   tempPos = %8d   src = %8d\n", p->ip, p->tempPos, p->srcLim - p->src));
	Bcj2Enc_Encode_2(p);
	if(p->state == BCJ2_ENC_STATE_ORIG) {
		const  Byte * src = p->src;
		uint   rem = (uint)(p->srcLim - src);
		uint   i;
		for(i = 0; i < rem; i++)
			p->temp[i] = src[i];
		p->tempPos = rem;
		p->src = src + rem;
	}
}
//