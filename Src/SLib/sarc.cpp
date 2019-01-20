// SARC.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018, 2019
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <..\osf\libzip\lib\zip.h>
#include <..\osf\lz4\lz4frame.h>
#include <..\osf\lz4\lz4.h>
#include <zlib.h>
//
//
//
static const size_t Default_SCompressor_MaxTempBufSize = SKILOBYTE(256);

SLAPI SCompressor::SCompressor(int type) : Type(type), P_Ctx(0), MaxTempBufSize(Default_SCompressor_MaxTempBufSize)
{
	assert(oneof2(Type, tLz4, tZLib));
	if(Type == tLz4) {
		/*LZ4F_cctx * p_ctx = 0;
		if(LZ4F_createCompressionContext(&p_ctx, LZ4F_getVersion())) {
			P_Ctx = p_ctx;
		}*/
		LZ4_stream_t * p_ctx = (LZ4_stream_t *)SAlloc::M(sizeof(LZ4_stream_t));
		P_Ctx = p_ctx;
	}
}

SLAPI SCompressor::~SCompressor()
{
	if(Type == tLz4) {
		/*LZ4F_cctx * p_ctx = (LZ4F_cctx *)P_Ctx;
		LZ4F_freeCompressionContext(p_ctx);
		P_Ctx = 0;*/
		ZFREE(P_Ctx);
	}
}

int SLAPI SCompressor::SetMaxTempBufSize(size_t sz)
{
	int    ok = 1;
	if(sz == 0)
		MaxTempBufSize = Default_SCompressor_MaxTempBufSize;
	else if(sz < SMEGABYTE(64))
		MaxTempBufSize = sz;
	else {
		ok = 0;
	}
	return ok;
}

int SLAPI SCompressor::CompressBlock(const void * pSrc, size_t srcSize, SBuffer & rDest, int rate, const void * pExt)
{
	int    ok = 0;
	if(Type == tLz4) {
		int    cb = LZ4_compressBound((int)srcSize);
		if(cb > 0) {
			STempBuffer temp_buf(cb);
			int  rs = 0; // result size
			THROW(temp_buf.IsValid());
			rs = LZ4_compress_fast_extState(P_Ctx, (const char *)pSrc, (char *)temp_buf, (int)srcSize, temp_buf.GetSize(), 1/*acceleration*/);
			THROW(rs > 0);
			THROW(rDest.Write(temp_buf, rs));
			ok = rs;
		}
	}
	else if(Type == tZLib) {
		//int ZEXPORT compress2(Bytef * dest, uLongf * destLen, const Bytef * source, uLong sourceLen, int level)
		int level = (rate == 0) ? Z_DEFAULT_COMPRESSION : (rate / 10);
		{
			z_stream stream;
			int zlib_err;
			const size_t src_size_limit = (uInt)-1;
			size_t current_src_size = srcSize;
			size_t current_src_offs = 0;
			stream.zalloc = (alloc_func)0;
			stream.zfree = (free_func)0;
			stream.opaque = (void *)0;
			zlib_err = deflateInit(&stream, level);
			THROW(zlib_err == Z_OK);
			{
				ulong cb = deflateBound(&stream, current_src_size);
				const size_t temp_buf_size = MIN(MaxTempBufSize, cb);
				STempBuffer temp_buf(temp_buf_size); // small buf for testing several iterations
				THROW(temp_buf.IsValid());
				do {
					stream.next_out = (Bytef *)(char *)temp_buf;
					stream.avail_out = temp_buf.GetSize();
					stream.next_in = (z_const Bytef*)(PTR8(pSrc)+current_src_offs);
					stream.avail_in = current_src_size;
					const uint prev_total_out = stream.total_out;
					const uint prev_total_in = stream.total_in;
					zlib_err = deflate(&stream, current_src_size ? Z_NO_FLUSH : Z_FINISH);
					THROW(rDest.Write(temp_buf, stream.total_out - prev_total_out));
					current_src_offs += (stream.total_in - prev_total_in);
					current_src_size -= (stream.total_in - prev_total_in);
				} while(!zlib_err);
				ok = stream.total_out;
				deflateEnd(&stream);
				if(zlib_err != Z_STREAM_END)
					ok = 0;
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SCompressor::DecompressBlock(const void * pSrc, size_t srcSize, SBuffer & rDest)
{
	int    ok = 0;
	if(Type == tLz4) {
		size_t part_buf_size = srcSize * 8;
		STempBuffer temp_buf(part_buf_size);
		int rs = LZ4_decompress_safe((const char *)pSrc, temp_buf, (int)srcSize, temp_buf.GetSize());
		if(rs > 0) {
			THROW(rDest.Write(temp_buf, (size_t)rs));
			ok = rs;
		}
	}
	else if(Type == tZLib) {
		int    zlib_err = 0;
		size_t written_size = 0;
		z_stream stream;
		MEMSZERO(stream);
		zlib_err = inflateInit(&stream);
		THROW(zlib_err == Z_OK);
		{
			const size_t temp_buf_size = MaxTempBufSize;
			STempBuffer temp_buf(temp_buf_size);
			// decompress until deflate stream ends or end of file 
			stream.avail_in = srcSize;
			stream.next_in = (z_const Bytef*)pSrc;
			// run inflate() on input until output buffer not full 
			do {
				stream.avail_out = temp_buf.GetSize();
				stream.next_out = (Bytef *)(char *)temp_buf;
				zlib_err = inflate(&stream, Z_NO_FLUSH);
				THROW(zlib_err != Z_STREAM_ERROR); // state not clobbered 
				switch(zlib_err) {
					case Z_NEED_DICT:
						zlib_err = Z_DATA_ERROR; // and fall through 
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						inflateEnd(&stream);
						CALLEXCEPT();
				}
				{
					const size_t chunk_size = temp_buf.GetSize() - stream.avail_out;
					THROW(rDest.Write(temp_buf, chunk_size));
					written_size += chunk_size;
				}
			} while(stream.avail_out == 0);
			// clean up and return 
			inflateEnd(&stream);
			ok = (int)written_size;
			if(zlib_err != Z_STREAM_END)
				ok = 0;
		}
	}
	CATCHZOK
	return ok;
}
//
//
//
SLAPI SArchive::SArchive() : Type(0), H(0)
{
}

SLAPI SArchive::~SArchive()
{
	Close();
}

int SLAPI SArchive::IsValid() const
{
	return BIN(H != 0);
}

int SLAPI SArchive::Close()
{
	int    ok = 1;
	if(Type == tZip) {
		if(H) {
			THROW(zip_close((zip_t *)H) == 0);
		}
	}
	CATCHZOK
	H = 0;
	Type = tUnkn;
	return ok;
}

int SLAPI SArchive::Open(int type, const char * pName, int mode /*SFile::mXXX*/)
{
	int    ok = 1;
	Close();
	if(type == tZip) {
		Type = type;
		int    flags = 0;
		long   m = (mode & ~(SFile::mBinary | SFile::mDenyRead | SFile::mDenyWrite | SFile::mNoStd | SFile::mNullWrite));
		if(m == SFile::mRead)
			flags = ZIP_RDONLY;
		else if(m == SFile::mReadWriteTrunc)
			flags = ZIP_TRUNCATE;
		else if(m == SFile::mReadWrite)
			flags = ZIP_CREATE;
		int    zip_err = 0;
		H = zip_open(pName, flags, &zip_err);
		THROW(H);
	}
	else {
		CALLEXCEPT();
	}
	CATCH
		H = 0;
		Type = 0;
		ok = 0;
	ENDCATCH
	return ok;
}

int64 SLAPI SArchive::GetEntriesCount() const
{
	int64 c = 0;
	if(H) {
		if(Type == tZip) {
			c = zip_get_num_entries((const zip_t *)H, 0 /*zip_flags_t*/);
			if(c < 0)
				c = 0;
		}
	}
	return c;
}

int FASTCALL SArchive::GetEntryName(int64 idx, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	if(H) {
		if(Type == tZip) {
			const char * p = zip_get_name((zip_t *)H, (uint64)idx, 0);
			if(p)
				rBuf = p;
			else
				ok = 0;
		}
	}
	return ok;
}

int SLAPI SArchive::ExtractEntry(int64 idx, const char * pDestName)
{
    int    ok = 1;
    zip_file_t * p_zf = 0;
    if(H) {
		SString entry_name;
		SString dest_file_name;
		SString temp_buf;
		SFile f_dest;
        THROW(GetEntryName(idx, entry_name));
        {
        	SPathStruc ps(pDestName);
            if(ps.Nam.Empty()) {
				ps.Nam = entry_name;
				ps.Ext.Z();
				ps.Merge(temp_buf);
            }
            else
				temp_buf = pDestName;
			SPathStruc::NormalizePath(temp_buf, 0, dest_file_name);
        }
		THROW(p_zf = zip_fopen_index((zip_t *)H, idx, 0 /*flags*/));
		{
			int64  actual_rd_size = 0;
			STempBuffer buffer(1024*1024);
			THROW(buffer.IsValid());
			THROW(f_dest.Open(dest_file_name, SFile::mWrite|SFile::mBinary));
			do {
				actual_rd_size = zip_fread(p_zf, buffer, buffer.GetSize());
				THROW(actual_rd_size >= 0);
				if(actual_rd_size > 0)
					THROW(f_dest.Write(buffer, (size_t)actual_rd_size));
			} while(actual_rd_size == buffer.GetSize());
		}
    }
    CATCHZOK
    zip_fclose(p_zf);
    return ok;
}

int SLAPI SArchive::AddEntry(const char * pSrcFileName, const char * pName, int flags)
{
    int    ok = 1;
	zip_source_t * p_zsrc = 0;
    THROW(IsValid());
	if(Type == tZip) {
		SString temp_buf;
		SPathStruc::NormalizePath(pSrcFileName, SPathStruc::npfSlash, temp_buf);
        THROW(fileExists(temp_buf));
		{
			int64   new_entry_idx = 0;
			if(flags & aefDirectory) {
				(temp_buf = pName).Transf(CTRANSF_OUTER_TO_UTF8);
				new_entry_idx = zip_dir_add((zip_t *)H, temp_buf, 0);
				THROW(new_entry_idx >= 0);
			}
			else {
				if(isempty(pName)) {
					SPathStruc ps(temp_buf);
					ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
				}
				else {
					SPathStruc::NormalizePath(pName, SPathStruc::npfSlash, temp_buf);
				}
				{
					SString src_file_name = pSrcFileName;
					src_file_name.Transf(CTRANSF_OUTER_TO_UTF8);
					THROW(p_zsrc = zip_source_file((zip_t *)H, src_file_name, 0, -1));
					new_entry_idx = zip_file_add((zip_t *)H, temp_buf.Transf(CTRANSF_OUTER_TO_UTF8), p_zsrc, ZIP_FL_OVERWRITE);
					THROW(new_entry_idx >= 0);
					p_zsrc = 0;
				}
			}
		}
	}
	CATCH
		if(Type == tZip && p_zsrc) {
			zip_source_free(p_zsrc);
			p_zsrc = 0;
		}
		ok = 0;
	ENDCATCH
    return ok;
}

int SLAPI SArchive::Helper_AddEntries(const SString & rRoot, const SString & rSub, const SString & rMask, int flags)
{
	int    ok = 1;
	THROW(IsValid());
	{
		SString temp_buf;
		SString local_path;
		SString entry_name;
		SString file_name;
		SString de_name;
		(local_path = rRoot).SetLastSlash().Cat(rSub).SetLastSlash();
		(temp_buf = local_path).Cat(rMask.NotEmpty() ? rMask : "*.*");
		SDirEntry de;
		for(SDirec dir(temp_buf); dir.Next(&de) > 0;) {
			if(!de.IsSelf() && !de.IsUpFolder()) {
				de_name = de.FileName;
				if(de.IsFolder()) {
					if(flags & aefRecursive) {
						(file_name = local_path).Cat(de_name);
						if(rSub.NotEmpty()) {
							(temp_buf = rSub).SetLastSlash().Cat(de_name);
						}
						else {
							temp_buf.Z().Cat(de_name);
						}
						SPathStruc::NormalizePath(temp_buf, SPathStruc::npfSlash, entry_name);
						THROW(AddEntry(file_name, entry_name, flags | aefDirectory));
						//
						if(rSub.NotEmpty()) {
							(file_name = rSub).SetLastSlash().Cat(de_name);
							(temp_buf = rSub).SetLastSlash().Cat(de_name);
						}
						else {
							file_name.Z().Cat(de_name);
							temp_buf.Z().Cat(de_name);
						}
						SPathStruc::NormalizePath(temp_buf, SPathStruc::npfSlash, entry_name);
						THROW(Helper_AddEntries(rRoot, file_name, rMask, flags)); // @recursion
					}
				}
				else {
					(file_name = local_path).Cat(de_name);
					(temp_buf = rSub).SetLastSlash().Cat(de_name);
                    SPathStruc::NormalizePath(temp_buf, SPathStruc::npfSlash, entry_name);
					THROW(AddEntry(file_name, entry_name, flags));
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SLAPI SArchive::AddEntries(const char * pMask, int flags)
{
	SString root;
	SString sub;
	SString mask;
	SPathStruc ps(pMask);
    ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, root);
    ps.Merge(SPathStruc::fNam|SPathStruc::fExt, mask);
	return Helper_AddEntries(root, sub, mask, flags);
}

static int TestCompressor(SCompressor & rC)
{
	int   ok = 1;
	SString temp_buf;
	STempBuffer pattern_buffer(SKILOBYTE(256));
	SBuffer compress_buf;
	SBuffer decompress_buf;
	{
		//D:\Papyrus\Src\PPTEST\DATA\phrases-ru-1251.txt 
		SString pattern_text;
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("phrases-ru-1251.txt");
		SFile f_in(temp_buf, SFile::mRead);
		//SLS.QueryPath("testroot", temp_buf);
		//temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("phrases-ru-1251.txt.out");
		//SFile f_out(temp_buf, SFile::mWrite);
		while(f_in.ReadLine(temp_buf)) {
			pattern_text.Cat(temp_buf);
		}
		{
			compress_buf.Z();
			decompress_buf.Z();
			int cs = 0;
			int ds = 0;
			const size_t org_size = pattern_text.Len()+1;
			cs = rC.CompressBlock(pattern_text, org_size, compress_buf, 0, 0);
			assert(cs > 0 && cs == (int)compress_buf.GetAvailableSize());
			ds = rC.DecompressBlock(compress_buf.GetBuf(0), compress_buf.GetAvailableSize(), decompress_buf);
			assert(ds > 0 && ds == org_size && ds == (int)decompress_buf.GetAvailableSize());
			assert(memcmp(decompress_buf.GetBuf(0), pattern_text, org_size) == 0);
		}
	}
	{
		LongArray ivec;
		for(long j = 0; j < 1000000; j++) {
			ivec.add(j);
		}
		{
			compress_buf.Z();
			decompress_buf.Z();
			int cs = 0;
			int ds = 0;
			const size_t org_size = ivec.getItemSize() * ivec.getCount();
			cs = rC.CompressBlock(ivec.dataPtr(), org_size, compress_buf, 0, 0);
			assert(cs > 0 && cs == (int)compress_buf.GetAvailableSize());
			ds = rC.DecompressBlock(compress_buf.GetBuf(0), compress_buf.GetAvailableSize(), decompress_buf);
			assert(ds > 0 && ds == org_size && ds == (int)decompress_buf.GetAvailableSize());
			assert(memcmp(decompress_buf.GetBuf(0), ivec.dataPtr(), org_size) == 0);
		}
	}
	SlThreadLocalArea & r_tla = SLS.GetTLA();
	r_tla.Rg.ObfuscateBuffer(pattern_buffer, pattern_buffer.GetSize());
	for(uint i = 1; i <= pattern_buffer.GetSize(); i++) {
		compress_buf.Z();
		decompress_buf.Z();
		int cs = 0;
		int ds = 0;
		cs = rC.CompressBlock(pattern_buffer, i, compress_buf, 0, 0);
		assert(cs > 0 && cs == (int)compress_buf.GetAvailableSize());
		ds = rC.DecompressBlock(compress_buf.GetBuf(0), compress_buf.GetAvailableSize(), decompress_buf);
		assert(ds > 0 && ds == i && ds == (int)decompress_buf.GetAvailableSize());
		assert(memcmp(decompress_buf.GetBuf(0), pattern_buffer, i) == 0);
	}
	return ok;
}

void SLAPI TestSArchive()
{
	int    ok = 1;
	SString temp_buf;
	{
		SCompressor c(SCompressor::tZLib);
		TestCompressor(c);
	}
	{
		SCompressor c(SCompressor::tLz4);
		TestCompressor(c);
	}
	//const  char * p_root = "d:/papyrus/src/pptest";
	SArchive arc;
	// "D:\Papyrus\Src\PPTEST\DATA\Test Directory\Test Directory Level 2\Directory With Many Files"
	{
		//(temp_buf = p_root).SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		THROW(arc.Open(SArchive::tZip, temp_buf, SFile::mReadWrite));
		{
			//SDirEntry de;
			//SString src_dir = "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files";
			//SString src_dir = "D:/Papyrus/Src/PPTEST/DATA";
			//(temp_buf = src_dir).SetLastSlash().Cat("*.*");
			SString src_dir;
			SLS.QueryPath("testroot", src_dir);
			(temp_buf = src_dir).SetLastSlash().Cat("data").SetLastSlash().Cat("*.*");
			THROW(arc.AddEntries(temp_buf, SArchive::aefRecursive));
			/*
			for(SDirec dir(temp_buf); dir.Next(&de) > 0;) {
				if(de.IsFile()) {
					(temp_buf = src_dir).SetLastSlash().Cat(de.FileName);
					THROW(arc.AddEntry(temp_buf, 0, 0));
				}
			}
			*/
		}
		THROW(arc.Close());
	}
	{
		//(temp_buf = p_root).SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		THROW(arc.Open(SArchive::tZip, temp_buf, SFile::mRead));
		{
			int64 c = arc.GetEntriesCount();
			if(c > 0) {
				SPathStruc::ReplaceExt(temp_buf, "log", 1);
				SFile f_log(temp_buf, SFile::mWrite);
				for(int64 i = 0; i < c; i++) {
					THROW(arc.GetEntryName(i, temp_buf));
					f_log.WriteLine(temp_buf.CR());
				}
			}
		}
		arc.Close();
	}
	CATCHZOK
}

