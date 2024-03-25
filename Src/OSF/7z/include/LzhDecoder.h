// LzhDecoder.h

#ifndef __COMPRESS_LZH_DECODER_H
#define __COMPRESS_LZH_DECODER_H

namespace NCompress {
	namespace NLzh {
		namespace NDecoder {
			const uint kMatchMinLen = 3;
			const uint kMatchMaxLen = 256;
			const uint NC = (256 + kMatchMaxLen - kMatchMinLen + 1);
			const uint NUM_CODE_BITS = 16;
			const uint NUM_DIC_BITS_MAX = 25;
			const uint NT = (NUM_CODE_BITS + 3);
			const uint NP = (NUM_DIC_BITS_MAX + 1);
			const uint NPT = NP; // Max(NT, NP)

			class CCoder : public ICompressCoder, public CMyUnknownImp {
				CLzOutWindow _outWindow;
				NBitm::CDecoder <CInBuffer> _inBitStream;
				int _symbolT;
				int _symbolC;
				NHuffman::CDecoder <NUM_CODE_BITS, NPT> _decoderT;
				NHuffman::CDecoder <NUM_CODE_BITS, NC> _decoderC;

				class CCoderReleaser {
				public:
					CCoderReleaser(CCoder * coder) : _coder(coder) 
					{
					}
					void Disable() { _coder = NULL; }
					~CCoderReleaser() 
					{
						if(_coder) 
							_coder->_outWindow.Flush();
					}
				private:
					CCoder * _coder;
				};

				friend class CCoderReleaser;

				bool ReadTP(uint num, uint numBits, int spec);
				bool ReadC();
				HRESULT CodeReal(uint64 outSize, ICompressProgressInfo * progress);
			public:
				MY_UNKNOWN_IMP

				uint32 DictSize;
				bool FinishMode;
				STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
				void SetDictSize(unsigned dictSize) { DictSize = dictSize; }
				CCoder() : DictSize(1 << 16), FinishMode(false) 
				{
				}
				uint64 GetInputProcessedSize() const { return _inBitStream.GetProcessedSize(); }
			};
		}
	}
}

#endif
