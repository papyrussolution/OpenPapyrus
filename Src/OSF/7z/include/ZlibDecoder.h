// ZlibDecoder.h

#ifndef __ZLIB_DECODER_H
#define __ZLIB_DECODER_H

//#include "DeflateDecoder.h"

namespace NCompress {
	namespace NZlib {
		const uint32 ADLER_INIT_VAL = 1;

		class COutStreamWithAdler : public ISequentialOutStream, public CMyUnknownImp {
			CMyComPtr<ISequentialOutStream> _stream;
			uint32 _adler;
			uint64 _size;
		public:
			MY_UNKNOWN_IMP
			STDMETHOD(Write) (const void * data, uint32 size, uint32 *processedSize);
			void SetStream(ISequentialOutStream * stream) { _stream = stream; }
			void ReleaseStream() { _stream.Release(); }
			void Init() 
			{ 
				_adler = ADLER_INIT_VAL; 
				_size = 0; 
			}
			uint32 GetAdler() const { return _adler; }
			uint64 GetSize() const { return _size; }
		};

		class CDecoder : public ICompressCoder, public CMyUnknownImp {
			COutStreamWithAdler * AdlerSpec;
			CMyComPtr<ISequentialOutStream> AdlerStream;

			NCompress::NDeflate::NDecoder::CCOMCoder * DeflateDecoderSpec;
			CMyComPtr<ICompressCoder> DeflateDecoder;
		public:
			STDMETHOD(Code) (ISequentialInStream *inStream, ISequentialOutStream *outStream, const uint64 *inSize, const uint64 *outSize, ICompressProgressInfo *progress);
			uint64 GetInputProcessedSize() const { return DeflateDecoderSpec->GetInputProcessedSize() + 2; }
			uint64 GetOutputProcessedSize() const { return AdlerSpec->GetSize(); }
			MY_UNKNOWN_IMP
		};

		static bool inline IsZlib(const Byte * p)
		{
			if((p[0] & 0xF) != 8) // method
				return false;
			if(((uint)p[0] >> 4) > 7) // logar_window_size minus 8.
				return false;
			if((p[1] & 0x20) != 0) // dictPresent
				return false;
			if((((uint32)p[0] << 8) + p[1]) % 31 != 0)
				return false;
			return true;
		}

		// IsZlib_3bytes checks 2 bytes of zlib header and starting byte of Deflate stream

		static bool inline IsZlib_3bytes(const Byte * p)
		{
			if(!IsZlib(p))
				return false;
			unsigned val = p[2];
			unsigned blockType = (val >> 1) & 0x3;
			if(blockType == 3) // unsupported block type for deflate
				return false;
			if(blockType == NCompress::NDeflate::NBlockType::kStored && (val >> 3) != 0)
				return false;
			return true;
		}
	}
}

#endif
