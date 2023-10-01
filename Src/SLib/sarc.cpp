// SARC.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023
//
#include <slib-internal.h>
#pragma hdrstop
#include <..\osf\libzip\lib\zip.h>
#include <..\slib\lz4\lz4frame.h>
#include <..\slib\lz4\lz4.h>
#include <zlib.h>
#include <..\slib\bzip2\bzlib.h>
#include <..\osf\libarchive-350\libarchive\archive.h> // @v10.4.4
#include <..\osf\libarchive-350\libarchive\archive_entry.h> // @v11.6.9
//
//
//
static const size_t Default_SCompressor_MaxTempBufSize = SKILOBYTE(256);

SCompressor::SCompressor(int type) : Type(type), P_Ctx(0), MaxTempBufSize(Default_SCompressor_MaxTempBufSize)
{
	assert(oneof3(Type, tLz4, tZLib, tBZip2));
	if(Type == tLz4) {
		/*LZ4F_cctx * p_ctx = 0;
		if(LZ4F_createCompressionContext(&p_ctx, LZ4F_getVersion())) {
			P_Ctx = p_ctx;
		}*/
		LZ4_stream_t * p_ctx = static_cast<LZ4_stream_t *>(SAlloc::M(sizeof(LZ4_stream_t)));
		P_Ctx = p_ctx;
	}
	else if(Type == tBZip2) {
		;
	}
}

SCompressor::~SCompressor()
{
	if(Type == tLz4) {
		/*LZ4F_cctx * p_ctx = (LZ4F_cctx *)P_Ctx;
		LZ4F_freeCompressionContext(p_ctx);
		P_Ctx = 0;*/
		ZFREE(P_Ctx);
	}
}

int SCompressor::SetMaxTempBufSize(size_t sz)
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

int SCompressor::CompressBlock(const void * pSrc, size_t srcSize, SBuffer & rDest, int rate, const void * pExt)
{
	int    ok = 0;
	if(Type == tLz4) {
		int    cb = LZ4_compressBound((int)srcSize);
		if(cb > 0) {
			STempBuffer temp_buf(cb);
			int  rs = 0; // result size
			THROW(temp_buf.IsValid());
			rs = LZ4_compress_fast_extState(P_Ctx, static_cast<const char *>(pSrc), (char *)temp_buf, (int)srcSize, temp_buf.GetSize(), 1/*acceleration*/);
			THROW(rs > 0);
			THROW(rDest.Write(temp_buf, rs));
			ok = rs;
		}
	}
	else if(Type == tBZip2) { // @b11.7.4
		//BZ2_bzBuffToBuffCompress(char * dest, uint* destLen, char * source, uint sourceLen, int blockSize100k, int verbosity, int workFactor);
		/*
		int   work_factor = 30;
		int   verbosity = 0;
		int   block_size100k = 9;
		bz_stream strm;
		int ret;
		//if(dest == NULL || destLen == NULL || source == NULL || blockSize100k < 1 || blockSize100k > 9 || verbosity < 0 || verbosity > 4 || workFactor < 0 || workFactor > 250)
			//return BZ_PARAM_ERROR;
		ret = BZ2_bzCompressInit(&strm, block_size100k, verbosity, work_factor);
		THROW(ret == BZ_OK);
		strm.next_in = source;
		strm.next_out = dest;
		strm.avail_in = sourceLen;
		strm.avail_out = *destLen;
		ret = BZ2_bzCompress(&strm, BZ_FINISH);
		if(ret == BZ_FINISH_OK) 
			goto output_overflow;
		if(ret != BZ_STREAM_END) 
			goto errhandler;
		// normal termination
		*destLen -= strm.avail_out;
		BZ2_bzCompressEnd(&strm);
		return BZ_OK;
	output_overflow:
		BZ2_bzCompressEnd(&strm);
		return BZ_OUTBUFF_FULL;
	errhandler:
		BZ2_bzCompressEnd(&strm);
		//return ret;
		*/
		// ??? copy of LZ4_compressBound
		/*
		size_t cb = (srcSize > 0x7E000000U) ? 0 : (srcSize) + ((srcSize)/255) + 16;
		do {
			const size_t temp_buf_size = MIN(MaxTempBufSize, cb);
			STempBuffer temp_buf(temp_buf_size); // small buf for testing several iterations
		} while(false);
		*/
	}
	else if(Type == tZLib) {
		//int compress2(Byte * dest, uLongf * destLen, const Byte * source, uLong sourceLen, int level)
		int level = (rate == 0) ? Z_DEFAULT_COMPRESSION : (rate / 10);
		{
			z_stream stream;
			int zlib_err;
			const size_t src_size_limit = (uInt)-1;
			size_t current_src_size = srcSize;
			size_t current_src_offs = 0;
			stream.zalloc = /*(alloc_func)*/0;
			stream.zfree = /*(free_func)*/0;
			stream.opaque = /*(void *)*/0;
			zlib_err = deflateInit(&stream, level);
			THROW(zlib_err == Z_OK);
			{
				ulong cb = deflateBound(&stream, current_src_size);
				const size_t temp_buf_size = MIN(MaxTempBufSize, cb);
				STempBuffer temp_buf(temp_buf_size); // small buf for testing several iterations
				THROW(temp_buf.IsValid());
				do {
					stream.next_out = static_cast<Byte *>(temp_buf.vptr());
					stream.avail_out = temp_buf.GetSize();
					stream.next_in = (const Byte *)(PTR8C(pSrc)+current_src_offs);
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

int SCompressor::DecompressBlock(const void * pSrc, size_t srcSize, SBuffer & rDest)
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
			stream.next_in = static_cast<const Byte *>(pSrc);
			// run inflate() on input until output buffer not full 
			do {
				stream.avail_out = temp_buf.GetSize();
				stream.next_out = static_cast<Byte *>(temp_buf.vptr());
				zlib_err = inflate(&stream, Z_NO_FLUSH);
				THROW_S_S(zlib_err != Z_STREAM_ERROR, SLERR_ZLIB_BUFINFLATEFAULT, stream.msg); // state not clobbered 
				switch(zlib_err) {
					case Z_NEED_DICT:
						zlib_err = Z_DATA_ERROR; // and fall through 
					case Z_DATA_ERROR:
					case Z_MEM_ERROR:
						inflateEnd(&stream);
						CALLEXCEPT_S_S(SLERR_ZLIB_BUFINFLATEFAULT, stream.msg);
				}
				{
					const size_t chunk_size = temp_buf.GetSize() - stream.avail_out;
					THROW(rDest.Write(temp_buf, chunk_size));
					written_size += chunk_size;
				}
			} while(stream.avail_out == 0);
			// clean up and return 
			inflateEnd(&stream);
			ok = static_cast<int>(written_size);
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
struct SArc_Bz2_Block {
	SArc_Bz2_Block() : Handle(0), OpenMode(0)
	{
	}
	void * Handle;
	int    OpenMode;
	SString FileName;
};

/*static*/int SArchive::List(int provider, int * pFormat, const char * pName, uint flags, SFileEntryPool & rPool)
{
	int    ok = 0;
	Archive * p_larc = 0;
	switch(provider) {
		case providerLA:
			{
				p_larc = archive_read_new();
				LaCbBlock cb_blk(0, SKILOBYTE(512));
				THROW(cb_blk.F.Open(pName, SFile::mRead | (SFile::mBinary|SFile::mNoStd))); // @v11.6.8 @fix &-->|
				//archive_read_support_compression_all(p_larc);
				archive_read_support_filter_all(p_larc);
				archive_read_support_format_all(p_larc);
				archive_read_support_format_empty(p_larc); // @v11.7.0
				archive_read_set_seek_callback(p_larc, LaCbSeek);
				const int r = archive_read_open2(p_larc, &cb_blk, LaCbOpen, LaCbRead, LaCbSkip, LaCbClose);
				THROW(r == 0);
				{
					ArchiveEntry * p_entry = 0;
					const wchar_t * p_entry_name = 0;
					SFileEntryPool::Entry fep_entry;
					while(archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
						p_entry_name = archive_entry_pathname_w(p_entry);
						fep_entry.Size = archive_entry_size(p_entry);
						fep_entry.Path.CopyUtf8FromUnicode(p_entry_name, sstrlen(p_entry_name), 1);
						if(archive_entry_mtime_is_set(p_entry)) {
							time_t mtm = archive_entry_mtime(p_entry);
							//long   mtmnsec = archive_entry_mtime_nsec(p_entry);
							fep_entry.WriteTime.SetTimeT(mtm);
						}
						rPool.Add(fep_entry);
					}				
				}
				ok = 1;
			}
			break;
		default:
			break;
	}
	CATCHZOK
	if(p_larc) {
		archive_read_finish(p_larc);
		p_larc = 0;
	}
	return ok;
}

#if 0 // {

static void extract(const char *filename)
{
	struct archive *a;
	struct archive *ext;
	struct archive_entry *entry;
	int flags;
	int r;
	/* Select which attributes we want to restore. */
	flags = ARCHIVE_EXTRACT_TIME;
	flags |= ARCHIVE_EXTRACT_PERM;
	flags |= ARCHIVE_EXTRACT_ACL;
	flags |= ARCHIVE_EXTRACT_FFLAGS;
	a = archive_read_new();
	archive_read_support_format_all(a);
	archive_read_support_filter_all(a);
	ext = archive_write_disk_new();
	archive_write_disk_set_options(ext, flags);
	archive_write_disk_set_standard_lookup(ext);
	if((r = archive_read_open_filename(a, filename, 10240)))
		exit(1);
	for(;;) {
		r = archive_read_next_header(a, &entry);
		if(r == ARCHIVE_EOF)
			break;
		if(r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(a));
		if(r < ARCHIVE_WARN)
			exit(1);
		r = archive_write_header(ext, entry);
		if(r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
		else if(archive_entry_size(entry) > 0) {
			//r = copy_data(a, ext);
			//static int copy_data(struct archive *ar, struct archive *aw)
			{
				r = 0;
				const void * buff;
				size_t size;
				la_int64_t offset;
				for(;;) {
					r = archive_read_data_block(a, &buff, &size, &offset);
					if(r == ARCHIVE_EOF) {
						//return (ARCHIVE_OK);
						r = ARCHIVE_OK;
						break;
					}
					if(r < ARCHIVE_OK) {
						//return (r);
						break;
					}
					r = archive_write_data_block(ext, buff, size, offset);
					if(r < ARCHIVE_OK) {
						fprintf(stderr, "%s\n", archive_error_string(aw));
						//return (r);
						break;
					}
				}
			}
			if(r < ARCHIVE_OK)
				fprintf(stderr, "%s\n", archive_error_string(ext));
			if(r < ARCHIVE_WARN)
				exit(1);
		}
		r = archive_write_finish_entry(ext);
		if(r < ARCHIVE_OK)
			fprintf(stderr, "%s\n", archive_error_string(ext));
		if(r < ARCHIVE_WARN)
			exit(1);
	}
	archive_read_close(a);
	archive_read_free(a);
	archive_write_close(ext);
	archive_write_free(ext);
	exit(0);
}
#endif // } 0


/*static*/int SArchive::Inflate(int provider, const char * pName, uint flags, const SFileEntryPool & rPool, const char * pDestPath)
{
	int    ok = 0;
	Archive * p_larc = 0;
	switch(provider) {
		case providerLA:
			{
			}
			break;
		default:
			break;
	}
	CATCHZOK
	if(p_larc) {
		archive_read_finish(p_larc);
		p_larc = 0;
	}
	return ok;
}

/*static*/int SArchive::InflateAll(int provider, const char * pName, uint flags, const char * pDestPath)
{
	int    ok = 0;
	Archive * p_larc = 0;
	switch(provider) {
		case providerLA:
			{
				p_larc = archive_read_new();
				LaCbBlock cb_blk(0, SKILOBYTE(512));
				THROW(cb_blk.F.Open(pName, SFile::mRead | (SFile::mBinary|SFile::mNoStd))); // @v11.6.8 @fix &-->|
				//archive_read_support_compression_all(p_larc);
				archive_read_support_filter_all(p_larc);
				archive_read_support_format_all(p_larc);
				archive_read_support_format_empty(p_larc); // @v11.7.0
				archive_read_set_seek_callback(p_larc, LaCbSeek);
				const int r = archive_read_open2(p_larc, &cb_blk, LaCbOpen, LaCbRead, LaCbSkip, LaCbClose);
				THROW(r == 0);
				{
					ArchiveEntry * p_entry = 0;
					const wchar_t * p_entry_name = 0;
					SFileEntryPool::Entry fep_entry;
					SPathStruc ps;
					SString base_path;
					SString final_path;
					if(!isempty(pDestPath)) {
						(base_path = pDestPath).Strip().RmvLastSlash();
					}
					else
						base_path.Z();
					while(archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
						p_entry_name = archive_entry_pathname_w(p_entry);
						fep_entry.Size = archive_entry_size(p_entry);
						fep_entry.Path.CopyUtf8FromUnicode(p_entry_name, sstrlen(p_entry_name), 1);
						if(archive_entry_mtime_is_set(p_entry)) {
							time_t mtm = archive_entry_mtime(p_entry);
							//long   mtmnsec = archive_entry_mtime_nsec(p_entry);
							fep_entry.WriteTime.SetTimeT(mtm);
						}
						//rPool.Add(fep_entry);
						{
							const void * p_buf;
							size_t buf_size;
							la_int64_t offset;
							int local_ok = 0;
							int rd_result = archive_read_data_block(p_larc, &p_buf, &buf_size, &offset);
							if(rd_result == ARCHIVE_OK) {
								ps.Split(fep_entry.Path);
								if(ps.Dir.NotEmpty()) {
									ps.Dir.ShiftLeftChr('\\').ShiftLeftChr('/');
									(final_path = base_path).SetLastSlash().Cat(ps.Dir);
									ps.Dir = final_path;
								}
								else {
									ps.Dir = base_path;
								}
								if(!createDir(ps.Dir)) {
									; // @todo @err
								}
								else {
									ps.Merge(final_path);
									SFile f_out(final_path, SFile::mWrite|SFile::mBinary);
									do {
										if(!f_out.Write(p_buf, buf_size)) {
											rd_result = ARCHIVE_FAILED;
											; // @todo @err
										}
										else
											rd_result = archive_read_data_block(p_larc, &p_buf, &buf_size, &offset);
									} while(rd_result == ARCHIVE_OK);
									if(rd_result == ARCHIVE_EOF)
										local_ok = 1;
								}
							}
							/*for(;;) {
								int r = archive_read_data_block(p_larc, &p_buf, &buf_size, &offset);
								if(r == ARCHIVE_EOF) {
									//return (ARCHIVE_OK);
									r = ARCHIVE_OK;
									break;
								}
								if(r < ARCHIVE_OK) {
									//return (r);
									break;
								}*/
						}
					}				
				}
				ok = 1;
			}
			break;
		default:
			break;
	}
	CATCHZOK
	if(p_larc) {
		archive_read_finish(p_larc);
		p_larc = 0;
	}
	return ok;
}

/*static*/int SArchive::Deflate(int provider, int format, const char * pName, uint flags, const SFileEntryPool & rPool)
{
	int    ok = 0;
	switch(provider) {
		case providerLA:
			{
			}
			break;
		default:
			break;
	}
	return ok;
}

SArchive::SArchive(int provider) : Provider(provider), H(0), P_Cb_Blk(0), OpenMode(0)
{
	assert(oneof3(Provider, providerLA, providerZip, providerBz2));
}

SArchive::~SArchive()
{
	Close();
}

bool SArchive::IsValid() const { return (H != 0); }

int SArchive::Close()
{
	int    ok = 1;
	if(Provider == providerZip) {
		if(H) {
			THROW(zip_close(static_cast<zip_t *>(H)) == 0);
		}
	}
	else if(Provider == providerBz2) {
		if(H) {
			BZ2_bzclose(static_cast<SArc_Bz2_Block *>(H)->Handle);
			delete static_cast<SArc_Bz2_Block *>(H);
		}
	}
	else if(Provider == providerLA) {
		if(H) {
			if(OpenMode == SFile::mRead) {
				Archive * p_larc = static_cast<struct Archive *>(H);
				archive_read_finish(p_larc);
			}
			else { // @todo
			}
		}	
	}
	Fep.Z(); // @v11.6.9
	CATCHZOK
	H = 0;
	ZDELETE(P_Cb_Blk);
	//Type = tUnkn;
	return ok;
}

// @v10.4.4 {
#if 1 // @construction {
SArchive::LaCbBlock::LaCbBlock(SArchive * pMaster, size_t bufSize) : P_Master(pMaster), Buf(bufSize)
{
}

/*static*/int cdecl SArchive::LaCbOpen(Archive * pA, void * pClientData)
{
	int    err = -1; // 0 - ok
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk) {
		err = p_blk->F.IsValid() ? 0 : 1;
	}
	return err;
}

/*static*/int cdecl SArchive::LaCbClose(Archive * pA, void * pClientData)
{
	int    err = -1; // 0 - ok
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk) {
		if(p_blk->F.IsValid())
			p_blk->F.Close();
		err = 0;
	}
	return err;
}

/*static*/SSIZE_T cdecl SArchive::LaCbRead(Archive * pA, void * pClientData, const void ** ppBuf)
{
	SSIZE_T result = 0;
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk && p_blk->F.IsValid()) {
		*ppBuf = p_blk->Buf.vptr();
		size_t actual_size = 0;
		if(p_blk->F.Read(p_blk->Buf.vptr(), p_blk->Buf.GetSize(), &actual_size)) {
			result = static_cast<SSIZE_T>(actual_size);
		}
	}
	return result;
}

/*static*/int64 cdecl SArchive::LaCbSkip(Archive * pA, void * pClientData, int64 request)
{
	int64  result = 0;
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk && p_blk->F.IsValid()) {
		const int64 preserve_offs = p_blk->F.Tell64();
		p_blk->F.Seek64(request, SEEK_CUR);
		const int64 new_offs = p_blk->F.Tell64();
		result = new_offs - preserve_offs;
	}
	return result;
}

/*static*/int64 cdecl SArchive::LaCbSeek(Archive * pA, void * pClientData, int64 offset, int whence)
{
	int64  result = 0;
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk && p_blk->F.IsValid()) {
		const int64 preserve_offs = p_blk->F.Tell64();
		p_blk->F.Seek64(offset, whence);
		const int64 new_offs = p_blk->F.Tell64();
		result = new_offs;
	}
	return result;
}
#endif // } 0 @construction
// } @v10.4.4

int SArchive::Open(const char * pName, int mode /*SFile::mXXX*/, SArchive::Format * pFmt)
{
	int    ok = 1;
	Close();
	const   int mm = (mode & 0xff);
	OpenMode = mm;
	Fep.Z();
	if(Provider == providerZip) {
		//Type = type;
		int    flags = 0;
		if(mm == SFile::mRead)
			flags = ZIP_RDONLY;
		else if(mm == SFile::mReadWriteTrunc)
			flags = ZIP_TRUNCATE;
		else if(mm == SFile::mReadWrite)
			flags = ZIP_CREATE;
		int    zip_err = 0;
		H = zip_open(pName, flags, &zip_err);
		THROW(H);
		{
			int64 c = zip_get_num_entries(static_cast<const zip_t *>(H), 0 /*zip_flags_t*/);
			if(c > 0) {
				SFileEntryPool::Entry fep_entry;
				for(uint64 i = 0; i < static_cast<uint64>(c); i++) {
					zip_stat_t st;
					if(zip_stat_index(static_cast<zip_t *>(H), i, ZIP_FL_UNCHANGED, &st) == 0) {
						fep_entry.Size = st.size;
						fep_entry.Path = st.name;
						fep_entry.WriteTime.SetTimeT(st.mtime);
						Fep.Add(fep_entry);
					}
				}
			}
		}
	}
	else if(Provider == providerBz2) {
		//Type = type;
		const char * p_mode = 0;
		if(mm == SFile::mRead)
			p_mode = "rb";
		else if(mm == SFile::mWrite)
			p_mode = "wb";
		else {
			; // @error invalid mode
		}
		if(p_mode) {
			SArc_Bz2_Block * p_blk = new SArc_Bz2_Block;
			p_blk->Handle = BZ2_bzopen(pName, "wb");
			if(p_blk->Handle) {
				p_blk->FileName = pName;
				p_blk->OpenMode = mm;
			}
			else {
				ZDELETE(p_blk);
				CALLEXCEPT();
			}
		}
	}
	// @v10.4.4 {
	else if(Provider == providerLA) {
		//Type = type;
		Archive * p_larc = 0;
		if(mm == SFile::mRead) {
			p_larc = archive_read_new();
			H = p_larc;
			THROW(P_Cb_Blk = new LaCbBlock(this, SKILOBYTE(512)));
			THROW(P_Cb_Blk->F.Open(pName, mm | (SFile::mBinary|SFile::mNoStd))); // @v11.6.8 @fix &-->|
			//archive_read_support_compression_all(p_larc);
			archive_read_support_filter_all(p_larc);
			archive_read_support_format_all(p_larc);
			archive_read_support_format_empty(p_larc); // @v11.7.0
			archive_read_set_seek_callback(p_larc, LaCbSeek);
			const int r = archive_read_open2(p_larc, P_Cb_Blk, LaCbOpen, LaCbRead, LaCbSkip, LaCbClose);
			THROW(r == 0);
			{
				ArchiveEntry * p_entry = 0;
				const wchar_t * p_entry_name = 0;
				SFileEntryPool::Entry fep_entry;
				while(archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
					p_entry_name = archive_entry_pathname_w(p_entry);
					fep_entry.Size = archive_entry_size(p_entry);
					fep_entry.Path.CopyUtf8FromUnicode(p_entry_name, sstrlen(p_entry_name), 1);
					if(archive_entry_mtime_is_set(p_entry)) {
						time_t mtm = archive_entry_mtime(p_entry);
						//long   mtmnsec = archive_entry_mtime_nsec(p_entry);
						fep_entry.WriteTime.SetTimeT(mtm);
					}
					Fep.Add(fep_entry);
				}				
			}
		}
		else if(mm == SFile::mWrite) {
			p_larc = archive_write_new();
			H = p_larc;
			THROW(P_Cb_Blk = new LaCbBlock(this, SKILOBYTE(512)));
			THROW(P_Cb_Blk->F.Open(pName, mm | (SFile::mBinary|SFile::mNoStd))); // @v11.6.8 @fix &-->|
		}
		else {
			// @error invalic open mode
		}
		/*else if(mm == SFile::mReadWrite) {
			p_larc_r = archive_read_new();
			p_larc_w = archive_write_new();
		}
		else if(mm == SFile::mReadWriteTrunc) {
			p_larc_r = archive_read_new();
			p_larc_w = archive_write_new();
		}*/
	}
	// } @v10.4.4 
	else {
		CALLEXCEPT();
	}
	CATCH
		H = 0;
		//Type = 0;
		// @v10.4.4 @construction ZDELETE(P_Cb_Blk);
		ok = 0;
	ENDCATCH
	return ok;
}

int64 SArchive::GetEntriesCount() const
{
	int64 c = 0;
	if(H) {
		if(Provider == providerZip) {
			c = static_cast<int64>(Fep.GetCount());
			//c = zip_get_num_entries(static_cast<const zip_t *>(H), 0 /*zip_flags_t*/);
			//if(c < 0)
				//c = 0;
		}
		else if(Provider == providerBz2) {
			if(H)
				c = 1;
		}
	// @v10.4.4 {
//#if 0 // @construction {
		else if(Provider == providerLA) {
			if(P_Cb_Blk) {
				c = static_cast<int64>(Fep.GetCount());
				/*
				Archive * p_larc = static_cast<struct Archive *>(H);
				ArchiveEntry * p_entry = 0;
				while(archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
					c++;
				}
				*/
				//archive_read_finish(p_larc);
			}
		}
//#endif // } @construction 
	// } @v10.4.4 
	}
	return c;
}

int FASTCALL SArchive::GetEntryName(int64 idx, SString & rBuf)
{
	rBuf.Z();
	int    ok = 1;
	if(H) {
		if(Provider == providerZip) {
			const char * p = zip_get_name(static_cast<zip_t *>(H), (uint64)idx, 0);
			if(p)
				rBuf = p;
			else
				ok = 0;
		}
		else if(Provider == providerBz2) {
			if(idx == 0) {
				const SArc_Bz2_Block * p_blk = static_cast<const SArc_Bz2_Block *>(H);
				if(p_blk->Handle && p_blk->FileName.NotEmpty()) {
					const SPathStruc ps(p_blk->FileName);
					rBuf = ps.Nam;
				}
				else
					ok = 0;
			}
			else
				ok = 0;
		}
	}
	return ok;
}

int SArchive::ExtractEntry(int64 idx, const char * pDestName)
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
            if(ps.Nam.IsEmpty()) {
				ps.Nam = entry_name;
				ps.Ext.Z();
				ps.Merge(temp_buf);
            }
            else
				temp_buf = pDestName;
			SPathStruc::NormalizePath(temp_buf, 0, dest_file_name);
        }
		if(Provider == providerZip) {
			THROW(p_zf = zip_fopen_index(static_cast<zip_t *>(H), idx, 0 /*flags*/));
			{
				int64  actual_rd_size = 0;
				STempBuffer buffer(SKILOBYTE(1024));
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
		else if(Provider == providerBz2) {
			SArc_Bz2_Block * p_blk = static_cast<SArc_Bz2_Block *>(H);
			int64  actual_rd_size = 0;
			STempBuffer buffer(SKILOBYTE(1024));
			THROW(buffer.IsValid());
			do {
				actual_rd_size = BZ2_bzread(p_blk->Handle, buffer, buffer.GetSize());
				THROW(actual_rd_size >= 0);
			} while(actual_rd_size == buffer.GetSize());
		}
		else if(Provider == providerLA) { // @v11.7.0
			if(P_Cb_Blk) {
				Archive * p_larc = static_cast<Archive *>(H);
				ArchiveEntry * p_entry = 0;
				SFileEntryPool::Entry fep_entry;
				for(int64 iter_idx = 0; archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK; iter_idx++) {
					if(iter_idx == idx) {
						SFileEntryPool::Entry fep_entry;
						SString full_path;
						if(Fep.Get(static_cast<uint>(iter_idx), &fep_entry, &full_path)) {
							const wchar_t * p_entry_name = archive_entry_pathname_w(p_entry);
							int64 entry_size = archive_entry_size(p_entry);
							
						}
						//
						break;
					}
					//Fep.Add(fep_entry);
				}				
			}
		}
    }
    CATCHZOK
    zip_fclose(p_zf);
    return ok;
}

int SArchive::AddEntry(const char * pSrcFileName, const char * pName, int flags)
{
    int    ok = 1;
	SString temp_buf;
	zip_source_t * p_zsrc = 0;
    THROW(IsValid());
	if(Provider == providerZip) {
		SPathStruc::NormalizePath(pSrcFileName, SPathStruc::npfSlash, temp_buf);
        THROW(fileExists(temp_buf));
		{
			int64   new_entry_idx = 0;
			if(flags & aefDirectory) {
				(temp_buf = pName).Transf(CTRANSF_OUTER_TO_UTF8);
				new_entry_idx = zip_dir_add(static_cast<zip_t *>(H), temp_buf, 0);
				THROW(new_entry_idx >= 0);
			}
			else {
				if(isempty(pName)) {
					const SPathStruc ps(temp_buf);
					ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
				}
				else {
					SPathStruc::NormalizePath(pName, SPathStruc::npfSlash, temp_buf);
				}
				{
					SString src_file_name(pSrcFileName);
					src_file_name.Transf(CTRANSF_OUTER_TO_UTF8);
					THROW(p_zsrc = zip_source_file(static_cast<zip_t *>(H), src_file_name, 0, -1));
					new_entry_idx = zip_file_add(static_cast<zip_t *>(H), temp_buf.Transf(CTRANSF_OUTER_TO_UTF8), p_zsrc, ZIP_FL_OVERWRITE);
					THROW(new_entry_idx >= 0);
					p_zsrc = 0;
				}
			}
		}
	}
	else if(Provider == providerBz2) {
		SPathStruc::NormalizePath(pSrcFileName, SPathStruc::npfSlash, temp_buf);
        THROW(fileExists(temp_buf));
		THROW(!(flags & (aefDirectory|aefRecursive)));
		if(isempty(pName)) {
			const SPathStruc ps(temp_buf);
			ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
		}
		else {
			SPathStruc::NormalizePath(pName, SPathStruc::npfSlash, temp_buf);
		}
	}
	CATCH
		if(Provider == providerZip && p_zsrc) {
			zip_source_free(p_zsrc);
			p_zsrc = 0;
		}
		ok = 0;
	ENDCATCH
    return ok;
}

int SArchive::Helper_AddEntries(const SString & rRoot, const SString & rSub, const SString & rMask, int flags)
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
				de.GetNameA(de_name);
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

int SArchive::AddEntries(const char * pMask, int flags)
{
	SString root;
	SString sub;
	SString mask;
	const SPathStruc ps(pMask);
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

void TestSArchive()
{
	int    ok = 1;
	SString temp_buf;
	{
		SArchive arc(SArchive::providerLA);
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("Test_Directory.7z");
		THROW(arc.Open(temp_buf, SFile::mRead, 0));
	}
	/*{
		SCompressor c(SCompressor::tZLib);
		TestCompressor(c);
	}
	{
		SCompressor c(SCompressor::tLz4);
		TestCompressor(c);
	}*/
	//const  char * p_root = "d:/papyrus/src/pptest";
	// "D:\Papyrus\Src\PPTEST\DATA\Test Directory\Test Directory Level 2\Directory With Many Files"
	{
		SArchive arc(SArchive::providerZip);
		//(temp_buf = p_root).SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		THROW(arc.Open(temp_buf, SFile::mReadWrite, 0));
		{
			//SDirEntry de;
			//SString src_dir = "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files";
			//SString src_dir = "D:/Papyrus/Src/PPTEST/DATA";
			//(temp_buf = src_dir).SetLastSlash().Cat("*.*");
			SString src_dir;
			SLS.QueryPath("testroot", src_dir);
			//(temp_buf = src_dir).SetLastSlash().Cat("data").SetLastSlash().Cat("*.*");
			(temp_buf = src_dir).SetLastSlash().Cat("data").SetLastSlash().Cat("Test Directory").SetLastSlash().Cat("TDR").SetLastSlash().Cat("*.*");
			//"D:\Papyrus\Src\PPTEST\DATA\Test Directory\TDR"
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
		SArchive arc(SArchive::providerZip);
		//(temp_buf = p_root).SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		THROW(arc.Open(temp_buf, SFile::mRead, 0));
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
