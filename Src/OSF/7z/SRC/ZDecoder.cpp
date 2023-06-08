// ZDecoder.cpp

#include <7z-internal.h>
#pragma hdrstop
#include <7z-ifcs.h>

namespace NCompress {
	namespace NZ {
		static const uint32 kBufferSize = (1 << 20);
		static const Byte kNumBitsMask = 0x1F;
		static const Byte kBlockModeMask = 0x80;
		static const uint kNumMinBits = 9;
		static const uint kNumMaxBits = 16;

		void CDecoder::Free()
		{
			ZFREE(_parents);
			ZFREE(_suffixes);
			ZFREE(_stack);
		}
		CDecoder::~CDecoder() 
		{
			Free();
		}
		HRESULT CDecoder::CodeReal(ISequentialInStream * inStream, ISequentialOutStream * outStream,
			const uint64 * /* inSize */, const uint64 * /* outSize */, ICompressProgressInfo * progress)
		{
			CInBuffer inBuffer;
			COutBuffer outBuffer;
			PackSize = 0;
			if(!inBuffer.Create(kBufferSize))
				return E_OUTOFMEMORY;
			inBuffer.SetStream(inStream);
			inBuffer.Init();
			if(!outBuffer.Create(kBufferSize))
				return E_OUTOFMEMORY;
			outBuffer.SetStream(outStream);
			outBuffer.Init();
			Byte buf[kNumMaxBits + 4];
			{
				if(inBuffer.ReadBytes(buf, 3) < 3)
					return S_FALSE;
				if(buf[0] != 0x1F || buf[1] != 0x9D)
					return S_FALSE; ;
			}
			Byte   prop = buf[2];
			if((prop & 0x60) != 0)
				return S_FALSE;
			uint   maxbits = prop & kNumBitsMask;
			if(maxbits < kNumMinBits || maxbits > kNumMaxBits)
				return S_FALSE;
			uint32 numItems = 1 << maxbits;
			// Speed optimization: blockSymbol can contain unused velue.
			if(maxbits != _numMaxBits || _parents == 0 || _suffixes == 0 || _stack == 0) {
				Free();
				_parents = (uint16 *)SAlloc::M(numItems * sizeof(uint16)); if(_parents == 0) return E_OUTOFMEMORY;
				_suffixes = (Byte *)SAlloc::M(numItems * sizeof(Byte)); if(_suffixes == 0) return E_OUTOFMEMORY;
				_stack = (Byte *)SAlloc::M(numItems * sizeof(Byte)); if(_stack == 0) return E_OUTOFMEMORY;
				_numMaxBits = maxbits;
			}
			uint64 prevPos = 0;
			uint32 blockSymbol = ((prop & kBlockModeMask) != 0) ? 256 : ((uint32)1 << kNumMaxBits);
			uint   numBits = kNumMinBits;
			uint32 head = (blockSymbol == 256) ? 257 : 256;
			bool   needPrev = false;
			uint   bitPos = 0;
			uint   numBufBits = 0;

			_parents[256] = 0; // virus protection
			_suffixes[256] = 0;
			HRESULT res = S_OK;

			for(;;) {
				if(numBufBits == bitPos) {
					numBufBits = (uint)inBuffer.ReadBytes(buf, numBits) * 8;
					bitPos = 0;
					uint64 nowPos = outBuffer.GetProcessedSize();
					if(progress && nowPos - prevPos >= (1 << 13)) {
						prevPos = nowPos;
						uint64 packSize = inBuffer.GetProcessedSize();
						RINOK(progress->SetRatioInfo(&packSize, &nowPos));
					}
				}
				uint   bytePos = bitPos >> 3;
				uint32 symbol = buf[bytePos] | ((uint32)buf[(size_t)bytePos + 1] << 8) | ((uint32)buf[(size_t)bytePos + 2] << 16);
				symbol >>= (bitPos & 7);
				symbol &= (1 << numBits) - 1;
				bitPos += numBits;
				if(bitPos > numBufBits)
					break;
				if(symbol >= head) {
					res = S_FALSE;
					break;
				}
				if(symbol == blockSymbol) {
					numBufBits = bitPos = 0;
					numBits = kNumMinBits;
					head = 257;
					needPrev = false;
					continue;
				}
				uint32 cur = symbol;
				uint   i = 0;
				while(cur >= 256) {
					_stack[i++] = _suffixes[cur];
					cur = _parents[cur];
				}
				_stack[i++] = (Byte)cur;
				if(needPrev) {
					_suffixes[(size_t)head - 1] = (Byte)cur;
					if(symbol == head - 1)
						_stack[0] = (Byte)cur;
				}
				do {
					outBuffer.WriteByte((_stack[--i]));
				} while(i > 0);
				if(head < numItems) {
					needPrev = true;
					_parents[head++] = (uint16)symbol;
					if(head > ((uint32)1 << numBits)) {
						if(numBits < maxbits) {
							numBufBits = bitPos = 0;
							numBits++;
						}
					}
				}
				else
					needPrev = false;
			}
			PackSize = inBuffer.GetProcessedSize();
			HRESULT res2 = outBuffer.Flush();
			return (res == S_OK) ? res2 : res;
		}

		STDMETHODIMP CDecoder::Code(ISequentialInStream * inStream, ISequentialOutStream * outStream,
					const uint64 * inSize, const uint64 * outSize, ICompressProgressInfo * progress)
		{
			try { return CodeReal(inStream, outStream, inSize, outSize, progress); }
			catch(const CInBufferException &e) { return e.ErrorCode; }
			catch(const COutBufferException &e) { return e.ErrorCode; }
			catch(...) { return S_FALSE; }
		}

		bool CheckStream(const Byte * data, size_t size)
		{
			if(size < 3)
				return false;
			if(data[0] != 0x1F || data[1] != 0x9D)
				return false;
			Byte prop = data[2];
			if((prop & 0x60) != 0)
				return false;
			uint   maxbits = prop & kNumBitsMask;
			if(maxbits < kNumMinBits || maxbits > kNumMaxBits)
				return false;
			uint32 numItems = 1 << maxbits;
			uint32 blockSymbol = ((prop & kBlockModeMask) != 0) ? 256 : ((uint32)1 << kNumMaxBits);
			uint   numBits = kNumMinBits;
			uint32 head = (blockSymbol == 256) ? 257 : 256;
			uint   bitPos = 0;
			uint   numBufBits = 0;
			Byte   buf[kNumMaxBits + 4];
			data += 3;
			size -= 3;
			// printf("\n\n");
			for(;;) {
				if(numBufBits == bitPos) {
					uint   num = (numBits < size) ? numBits : (uint)size;
					memcpy(buf, data, num);
					data += num;
					size -= num;
					numBufBits = num * 8;
					bitPos = 0;
				}
				uint   bytePos = bitPos >> 3;
				uint32 symbol = buf[bytePos] | ((uint32)buf[bytePos + 1] << 8) | ((uint32)buf[bytePos + 2] << 16);
				symbol >>= (bitPos & 7);
				symbol &= (1 << numBits) - 1;
				bitPos += numBits;
				if(bitPos > numBufBits) {
					// printf("  OK", symbol);
					return true;
				}
				// printf("%3X ", symbol);
				if(symbol >= head)
					return false;
				if(symbol == blockSymbol) {
					numBufBits = bitPos = 0;
					numBits = kNumMinBits;
					head = 257;
					continue;
				}
				if(head < numItems) {
					head++;
					if(head > ((uint32)1 << numBits)) {
						if(numBits < maxbits) {
							numBufBits = bitPos = 0;
							numBits++;
						}
					}
				}
			}
		}
	}
}
