// SARC.CPP
// Copyright (c) A.Sobolev 2016, 2017, 2018, 2019, 2020, 2021, 2022, 2023, 2024, 2025
// @codepage UTF-8
//
#include <slib-internal.h>
#pragma hdrstop
#include <..\osf\libzip\lib\zip.h>
#include <..\slib\lz4\lz4frame.h>
#include <..\slib\lz4\lz4.h>
#include <zlib.h>
#include <..\slib\bzip2\bzlib.h>
#include <..\osf\libarchive-350\libarchive\archive.h>
#include <..\osf\libarchive-350\libarchive\archive_entry.h> // @v11.6.9
#include <UserEnv.h> // UnloadUserProfile
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
			rs = LZ4_compress_fast_extState(P_Ctx, static_cast<const char *>(pSrc), temp_buf, (int)srcSize, temp_buf.GetSize(), 1/*acceleration*/);
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

/*static*/int SArchive::ConvertLaEntry(void * pLaEntry, SFileEntryPool::Entry & rFe, SString * pFullPath)
{
	int    ok = 1;
	if(pLaEntry) {
		ArchiveEntry * p_entry = static_cast<ArchiveEntry *>(pLaEntry);
		const wchar_t * p_entry_name = archive_entry_pathname_w(p_entry);
		rFe.Size = archive_entry_size(p_entry);
		{
			SString temp_buf;
			temp_buf.CopyUtf8FromUnicode(p_entry_name, sstrlen(p_entry_name), 1);
			{
				size_t rp_ = 0;
				size_t rpb_ = 0;
				const char * p_rp = temp_buf.SearchRChar('/', &rp_);
				const char * p_rpb = temp_buf.SearchRChar('\\', &rpb_);
				size_t _p = 0;
				if(p_rp && p_rpb) {
					_p = MAX(rp_, rpb_)+1;
				}
				else if(p_rp)
					_p = rp_+1;
				else if(p_rpb)
					_p = rpb_+1;
				if(_p == 0) {
					rFe.Name = temp_buf;
					rFe.Path.Z();
				}
				else {
					temp_buf.Sub(_p, temp_buf.Len()-_p, rFe.Name);
					temp_buf.Sub(0, _p-1, rFe.Path);
				}
			}
		}
		if(archive_entry_mtime_is_set(p_entry)) {
			time_t mtm = archive_entry_mtime(p_entry);
			//long   mtmnsec = archive_entry_mtime_nsec(p_entry);
			rFe.ModTm_ = SUniTime_Internal::EpochToNs100(mtm);
		}
		// @v12.1.6 {
		if(pFullPath)
			rFe.GetFullPath(*pFullPath);
		// } @v12.1.6 
	}
	else
		ok = 0;
	return ok;
}

/*static*/int SArchive::List(int provider, int * pFormat, const char * pName, uint flags, SFileEntryPool & rPool)
{
	int    ok = 0;
	SString temp_buf;
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
					SFileEntryPool::Entry fep_entry;
					while(archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
						ConvertLaEntry(p_entry, fep_entry, 0);
						rPool.Add(fep_entry, SFileEntryPool::scanfKeepCase);
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
				int64 offset;
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


/*static*/int SArchive::Inflate(int provider, const char * pName, uint flags, const SFileEntryPool & rPool, const char * pDestPath, StringSet * pResultFnSet)
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
	//CATCHZOK
	if(p_larc) {
		archive_read_finish(p_larc);
		p_larc = 0;
	}
	return ok;
}

/*static*/SPtrHandle SArchive::OpenArchiveEntry(int provider, const char * pArcName, const char * pEntryName)
{
	SPtrHandle h;
	Archive * p_larc = 0;
	LaCbBlock * p_cb_blk = 0;
	THROW(!isempty(pArcName));
	THROW(!isempty(pEntryName));
	THROW(fileExists(pArcName));
	if(provider == providerLA) {
		p_larc = archive_read_new();
		p_cb_blk = new LaCbBlock(0, SKILOBYTE(512));
		THROW(p_cb_blk->F.Open(pArcName, SFile::mRead | (SFile::mBinary|SFile::mNoStd)));
		//archive_read_support_compression_all(p_larc);
		archive_read_support_filter_all(p_larc);
		archive_read_support_format_all(p_larc);
		archive_read_support_format_empty(p_larc); // @v11.7.0
		archive_read_set_seek_callback(p_larc, LaCbSeek);
		const int r = archive_read_open2(p_larc, p_cb_blk, LaCbOpen, LaCbRead, LaCbSkip, LaCbClose);
		THROW(r == 0);
		{
			ArchiveEntry * p_entry = 0;
			SFileEntryPool::Entry fep_entry;
			SString temp_buf;
			SString arc_sub;
			SString entry_name;
			SFsPath::NormalizePath(pEntryName, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, arc_sub);
			while(!h && archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
				ConvertLaEntry(p_entry, fep_entry, 0);
				fep_entry.GetFullPath(entry_name);
				if(SFsPath::NormalizePath(entry_name, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, temp_buf).IsEqiUtf8(arc_sub)) {
					EntryBlock * p_blk = new EntryBlock(provider, p_larc);
					THROW(p_blk);
					p_blk->P_CbBlk = p_cb_blk;
					p_larc = 0; // Объект p_larc переходит в собственность возвращаемого манипулятора 
					p_cb_blk = 0;
					h = p_blk;
				}
			}				
		}
	}
	CATCH
		;
	ENDCATCH
	if(p_larc) {
		archive_read_finish(p_larc);
		p_larc = 0;
	}
	delete p_cb_blk;
	return h;
}

/*static*/int SArchive::ReadArchiveEntry(SPtrHandle h, void * pBuf, size_t size, size_t * pActualSize)
{
	int    ok = 0;
	size_t actual_size = 0;
	if(!!h) {
		EntryBlock * p_ab = static_cast<EntryBlock *>(static_cast<void *>(h));
		assert(p_ab);
		if(p_ab->Magic == EntryBlock::MagicValue) {
			if(p_ab->Provider == providerLA) {
				if(p_ab->H) {
					int r = archive_read_data(static_cast<Archive *>(p_ab->H), pBuf, size);
					if(r < 0)
						ok = 0;
					else {
						actual_size = static_cast<size_t>(r);
						ok = 1;
					}
				}
			}
		}
	}
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

/*static*/int SArchive::SeekArchiveEntry(SPtrHandle h, int64 offs, int whence)
{
	int    ok = 0;
	if(!!h) {
		EntryBlock * p_ab = static_cast<EntryBlock *>(static_cast<void *>(h));
		assert(p_ab);
		if(p_ab->Magic == EntryBlock::MagicValue) {
			if(p_ab->Provider == providerLA) {
				if(p_ab->H) {
					int64 r = archive_seek_data(static_cast<Archive *>(p_ab->H), offs, whence);
					if(r == 0)
						ok = 1;
				}
			}
		}
	}
	return ok;
}

/*static*/int SArchive::CloseArchiveEntry(SPtrHandle h)
{
	int    ok = 0;
	if(!!h) {
		EntryBlock * p_ab = static_cast<EntryBlock *>(static_cast<void *>(h));
		assert(p_ab);
		if(p_ab->Magic == EntryBlock::MagicValue) {
			if(p_ab->Provider == providerLA) {
				if(p_ab->H) {
					archive_read_finish(static_cast<Archive *>(p_ab->H));
					delete p_ab->P_CbBlk;
					p_ab->H = 0;
				}
			}
		}
	}
	return ok;
}

/*static*/int SArchive::Implement_Inflate(int provider, const char * pName, uint flags, const char * pWildcard, const char * pDestPath, StringSet * pResultFnSet)
{
	int    ok = 0;
	Archive * p_larc = 0;
	SString temp_buf;
	if(provider == providerLA) {
		p_larc = archive_read_new();
		LaCbBlock cb_blk(0, SKILOBYTE(512));
		THROW(cb_blk.F.Open(pName, SFile::mRead | (SFile::mBinary|SFile::mNoStd)));
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
			SString base_path;
			SString final_path;
			SString fepentry_full_path;
			if(!isempty(pDestPath)) {
				(base_path = pDestPath).Strip();
			}
			else {
				SFile::GetCurrentDir(base_path);
				//base_path.Z();
			}
			base_path.RmvLastSlash();
			const SFsPath ps_base(base_path);
			while(archive_read_next_header(p_larc, &p_entry) == ARCHIVE_OK) {
				ConvertLaEntry(p_entry, fep_entry, &temp_buf);
				SFsPath::NormalizePath(temp_buf, SFsPath::npfSlash|SFsPath::npfCompensateDotDot, fepentry_full_path);
				bool suited = false;
				if(!isempty(pWildcard)) {
					if(fepentry_full_path.IsEqiUtf8(pWildcard)) // @v12.1.6
						suited = true;
					else if(SFile::WildcardMatch(pWildcard, fep_entry.Name))
						suited = true;
				}
				else
					suited = true;
				if(suited) {
					//rPool.Add(fep_entry);
					{
						const void * p_buf;
						size_t buf_size;
						int64 offset;
						int local_ok = 0;
						int rd_result = archive_read_data_block(p_larc, &p_buf, &buf_size, &offset);
						if(rd_result == ARCHIVE_OK) {
							const SFsPath ps_src(fepentry_full_path);
							SFsPath ps_dest(ps_base);
							if(ps_src.Dir.NotEmpty() && !(flags & fInflateWithoutSub)) {
								(temp_buf = ps_src.Dir).ShiftLeftChr('\\').ShiftLeftChr('/');
								ps_dest.Dir.SetLastSlash().Cat(temp_buf);
							}
							ps_dest.Nam = ps_src.Nam;
							ps_dest.Ext = ps_src.Ext;
							ps_dest.Merge(final_path);
							temp_buf.Z();
							ps_dest.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
							if(!SFile::CreateDir(temp_buf)) {
								; // @todo @err
							}
							else {
								SFile f_out(final_path, SFile::mWrite|SFile::mBinary|SFile::mNoStd);
								do {
									if(!f_out.Write(p_buf, buf_size)) {
										rd_result = ARCHIVE_FAILED;
										; // @todo @err
									}
									else
										rd_result = archive_read_data_block(p_larc, &p_buf, &buf_size, &offset);
								} while(rd_result == ARCHIVE_OK);
								if(rd_result == ARCHIVE_EOF) {
									f_out.SetDateTime(0LL, 0LL, fep_entry.ModTm_);
									f_out.Close();
									CALLPTRMEMB(pResultFnSet, add(final_path));
									local_ok = 1;
								}
							}
						}
					}
				}
			}				
		}
		ok = 1;
	}
	else {
		// @notimpelemted
	}
	CATCHZOK
	if(p_larc) {
		archive_read_finish(p_larc);
		p_larc = 0;
	}
	return ok;
}

/*static*/int SArchive::Inflate(int provider, const char * pName, uint flags, const char * pWildcard, const char * pDestPath, StringSet * pResultFnSet)
{
	return SArchive::Implement_Inflate(provider, pName, flags, pWildcard, pDestPath, pResultFnSet);
}

/*static*/int SArchive::InflateAll(int provider, const char * pName, uint flags, const char * pDestPath, StringSet * pResultFnSet)
{
	return SArchive::Implement_Inflate(provider, pName, flags, 0, pDestPath, pResultFnSet);
}

/*static*/int SArchive::Deflate(int provider, int format, const char * pNameUtf8, uint64 uedArcFormat, const char * pBasePathUtf8, const SFileEntryPool & rPool)
{
	int    ok = 1;
	uint   fault_count_file_open = 0;
	uint   fault_count_file_exists = 0;
	uint   fault_count_getstat = 0;
	SString temp_buf;
	Archive * p_larc = 0;
	ArchiveEntry * p_larc_entry = 0;
	//THROW(UED::BelongToMeta(uedArcFormat, UED_META_DATAFORMAT)); // @todo @err
	switch(provider) {
		case providerLA:
			{
				p_larc_entry = archive_entry_new();
				SString src_full_path;
				SString arc_file_name;
				SFileEntryPool::Entry fe;
				STempBuffer in_buf(SKILOBYTE(512));
				THROW(p_larc = archive_write_new());
				THROW(in_buf.IsValid());
				//archive_write_add_filter_gzip(p_larc);
				//archive_write_set_format_pax_restricted(p_larc); // Note 1
				switch(uedArcFormat) {
					case UED_DATAFORMAT_ZIP:
						archive_write_set_format_zip(p_larc);
						break;
					case UED_DATAFORMAT_SEVENZ:
						archive_write_set_format_7zip(p_larc);
						break;
					case UED_DATAFORMAT_XAR:
						archive_write_set_format_xar(p_larc);
						break;
					default:
						CALLEXCEPT(); // @todo @err
						break;
				}
				//archive_write_open_filename(p_larc, pNameUtf8);
				{
					SStringU & r_name_u = SLS.AcquireRvlStrU();
					r_name_u.CopyFromUtf8Strict(pNameUtf8, sstrlen(pNameUtf8));
					archive_write_open_filename_w(p_larc, r_name_u);
				}
				for(uint fidx = 0; fidx < rPool.GetCount(); fidx++) {
					if(rPool.Get(fidx, &fe, &temp_buf)) {
						SFsPath::NormalizePath(temp_buf, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, src_full_path);
						if(fileExists(src_full_path)) {
							struct _stat st;
							bool get_stat_result = false;
							{
								SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
								get_stat_result = (r_temp_buf_u.CopyFromUtf8(src_full_path) && _wstat(r_temp_buf_u, &st) == 0);
							}
							if(get_stat_result) {
								if(SFsPath::GetRelativePath(pBasePathUtf8, 0, src_full_path, 0, arc_file_name)) {
									arc_file_name.ShiftLeftChr('.').ShiftLeftChr('\\');
									SFile f_in(src_full_path, SFile::mRead|SFile::mBinary|SFile::mNoStd);
									if(f_in.IsValid()) {
										size_t written_size = 0;
										archive_entry_clear(p_larc_entry);
										//archive_entry_set_pathname(p_larc_entry, arc_file_name);
										archive_entry_set_pathname_utf8(p_larc_entry, arc_file_name);
										//archive_entry_set_size(p_larc_entry, st.st_size); // Note 3
										archive_entry_copy_stat(p_larc_entry, &st);
										archive_entry_set_filetype(p_larc_entry, AE_IFREG);
										archive_entry_set_perm(p_larc_entry, 0644);
										archive_write_header(p_larc, p_larc_entry);
										size_t actual_size = 0;
										while(f_in.Read(in_buf, in_buf.GetSize(), &actual_size) && actual_size) {
											la_ssize_t ws = archive_write_data(p_larc, in_buf, actual_size);
											if(ws > 0)
												written_size += ws;
										}
									}
									else {
										fault_count_file_open++;
									}
								}
							}
							else
								fault_count_getstat++;
						}
						else {
							fault_count_file_exists++;
						}
					}
				}
				#if 0 // @example
				void write_archive(const char *outname, const char **filename)
				{
					struct archive_entry *entry;
					struct stat st;
					char buff[8192];
					int len;
					int fd;
					struct archive * a = archive_write_new();
					archive_write_add_filter_gzip(a);
					archive_write_set_format_pax_restricted(a); // Note 1
					archive_write_open_filename(a, outname);
					while(*filename) {
						stat(*filename, &st);
						entry = archive_entry_new(); // Note 2
						archive_entry_set_pathname(entry, *filename);
						archive_entry_set_size(entry, st.st_size); // Note 3
						archive_entry_set_filetype(entry, AE_IFREG);
						archive_entry_set_perm(entry, 0644);
						archive_write_header(a, entry);
						fd = open(*filename, O_RDONLY);
						len = read(fd, buff, sizeof(buff));
						while(len > 0) {
							archive_write_data(a, buff, len);
							len = read(fd, buff, sizeof(buff));
						}
						close(fd);
						archive_entry_free(entry);
						filename++;
					}
					archive_write_close(a); // Note 4
					archive_write_free(a); // Note 5
				}
				#endif // } @example
			}
			break;
		default:
			break;
	}
	CATCHZOK
	archive_entry_free(p_larc_entry);
	if(p_larc) {
		archive_write_close(p_larc);
		archive_write_free(p_larc);
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
		else
			result = -1;
	}
	return result;
}

/*static*/int64 cdecl SArchive::LaCbSkip(Archive * pA, void * pClientData, int64 request)
{
	int64  result = -1;
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk && p_blk->F.IsValid()) {
		const int64 preserve_offs = p_blk->F.Tell64();
		if(p_blk->F.Seek64(request, SEEK_CUR)) {
			const int64 new_offs = p_blk->F.Tell64();
			result = new_offs - preserve_offs;
		}
	}
	return result;
}

/*static*/int64 cdecl SArchive::LaCbSeek(Archive * pA, void * pClientData, int64 offset, int whence)
{
	int64  result = -1;
	LaCbBlock * p_blk = static_cast<LaCbBlock *>(pClientData);
	if(p_blk && p_blk->F.IsValid()) {
		const int64 preserve_offs = p_blk->F.Tell64();
		if(p_blk->F.Seek64(offset, whence)) {
			const int64 new_offs = p_blk->F.Tell64();
			result = new_offs;
		}
	}
	return result;
}

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
						fep_entry.ModTm_ = SUniTime_Internal::EpochToNs100(st.mtime);
						Fep.Add(fep_entry, SFileEntryPool::scanfKeepCase);
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
						fep_entry.ModTm_ = SUniTime_Internal::EpochToNs100(mtm);
					}
					Fep.Add(fep_entry, SFileEntryPool::scanfKeepCase);
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
	else {
		CALLEXCEPT();
	}
	CATCH
		H = 0;
		//Type = 0;
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
					const SFsPath ps(p_blk->FileName);
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
        	SFsPath ps(pDestName);
            if(ps.Nam.IsEmpty()) {
				ps.Nam = entry_name;
				ps.Ext.Z();
				ps.Merge(temp_buf);
            }
            else
				temp_buf = pDestName;
			SFsPath::NormalizePath(temp_buf, 0, dest_file_name);
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
		SFsPath::NormalizePath(pSrcFileName, SFsPath::npfSlash, temp_buf);
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
					const SFsPath ps(temp_buf);
					ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
				}
				else {
					SFsPath::NormalizePath(pName, SFsPath::npfSlash, temp_buf);
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
		SFsPath::NormalizePath(pSrcFileName, SFsPath::npfSlash, temp_buf);
        THROW(fileExists(temp_buf));
		THROW(!(flags & (aefDirectory|aefRecursive)));
		if(isempty(pName)) {
			const SFsPath ps(temp_buf);
			ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
		}
		else {
			SFsPath::NormalizePath(pName, SFsPath::npfSlash, temp_buf);
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
						SFsPath::NormalizePath(temp_buf, SFsPath::npfSlash, entry_name);
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
						SFsPath::NormalizePath(temp_buf, SFsPath::npfSlash, entry_name);
						THROW(Helper_AddEntries(rRoot, file_name, rMask, flags)); // @recursion
					}
				}
				else {
					(file_name = local_path).Cat(de_name);
					(temp_buf = rSub).SetLastSlash().Cat(de_name);
                    SFsPath::NormalizePath(temp_buf, SFsPath::npfSlash, entry_name);
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
	const SFsPath ps(pMask);
    ps.Merge(SFsPath::fDrv|SFsPath::fDir, root);
    ps.Merge(SFsPath::fNam|SFsPath::fExt, mask);
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
				SFsPath::ReplaceExt(temp_buf, "log", 1);
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
//
//
//
const uint32 SSystemBackup_StorageVer = 0;
const uint32 Signature_SSystemBackup = 0x590D2EE8U;
const char * SSystemBackup_InfoFileName = "ssbuinfo";

SSystemBackup::Param::Entry::Entry() : Type(etUndef)
{
}
			
int SSystemBackup::Param::Entry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Type, rBuf));
	THROW(pSCtx->Serialize(dir, Uuid, rBuf));
	THROW(pSCtx->Serialize(dir, Value, rBuf));
	THROW(pSCtx->Serialize(dir, Password, rBuf));
	CATCHZOK
	return ok;
}

SSystemBackup::StateBlock::StateBlock()
{
}
		
SSystemBackup::StateBlock::StateBlock(const Param & rP) : P(rP)
{
}
		
SSystemBackup::StateBlock::~StateBlock()
{
}

SSystemBackup::CopyFaultEntry::CopyFaultEntry() : SFile::Stat()
{
}
		
int SSystemBackup::CopyFaultEntry::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(SFile::Stat::Serialize(dir, rBuf, pSCtx));
	THROW(pSCtx->Serialize(dir, Path, rBuf));
	CATCHZOK
	return ok;
}

int SSystemBackup::StateBlock::SearchCopyFaultEntry(const char * pPath, uint * pIdx) const
{
	int    ok = 0;
	for(uint i = 0; !ok && i < CopyFaultList.getCount(); i++) {
		const CopyFaultEntry * p_entry = CopyFaultList.at(i);
		if(p_entry) {
			if(p_entry->Path.IsEqiUtf8(pPath)) {
				ASSIGN_PTR(pIdx, i);
				ok = 1;
			}
		}
	}
	return ok;
}

int SSystemBackup::StateBlock::Write(const char * pFileName, uint32 buId)
{
	int    ok = 1;
	SString temp_buf;
	SBuffer sbuf;
	SSerializeContext sctx;
	SBinaryChunk bc;
	THROW_S_S(!isempty(pFileName), SLERR_INVPARAM, __FUNCTION__"/pFileName");
	H.Init(buId);
	{
		(temp_buf = pFileName).SetLastSlash().Cat(SSystemBackup_InfoFileName);
		SFile f_out(temp_buf, SFile::mWrite|SFile::mBinary);
		THROW(f_out.IsValid());
		THROW(P.Serialize(+1, sbuf, &sctx));
		THROW(TSCollection_Serialize(CopyFaultList, +1, sbuf, &sctx));
		THROW(bc.Cat(&H, sizeof(H)));
		THROW(bc.Cat(sbuf.constptr(), sbuf.GetAvailableSize()));
		THROW(f_out.Write(bc.PtrC(), bc.Len()));
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::StateBlock::Read(const char * pFileName)
{
	int    ok = 1;
	SString temp_buf;
	BackupInfoHeader hdr;
	THROW_S_S(!isempty(pFileName), SLERR_INVPARAM, __FUNCTION__"/pFileName");
	{
		size_t in_file_actual_size = 0;
		STempBuffer in_buf(SKILOBYTE(4));
		(temp_buf = pFileName).SetLastSlash().Cat(SSystemBackup_InfoFileName);
		SFile f_in(temp_buf, SFile::mRead|SFile::mBinary);
		THROW(f_in.IsValid());
		THROW_S_S(f_in.ReadAll(in_buf, 0, &in_file_actual_size), SLERR_SSYSBU_INFORDFAULT, "io");
		THROW_S_S(in_file_actual_size > sizeof(BackupInfoHeader), SLERR_SSYSBU_INFORDFAULT, "file-size");
		assert(in_buf.GetSize() >= in_file_actual_size); // @paranoic
		{
			const BackupInfoHeader * p_hdr = static_cast<const BackupInfoHeader *>(in_buf.vcptr());
			THROW_S_S(p_hdr->Signature == Signature_SSystemBackup, SLERR_SSYSBU_INFORDFAULT, "signature");
			THROW_S_S(p_hdr->Ver == SSystemBackup_StorageVer, SLERR_SSYSBU_INFORDFAULT, "version");
			THROW_S_S(checkdate(p_hdr->Dtm.d), SLERR_SSYSBU_INFORDFAULT, "time");
			H = *p_hdr;
			{
				SBuffer sbuf;
				SSerializeContext sctx;
				THROW(sbuf.Write(PTR8C(in_buf.vcptr()) + sizeof(BackupInfoHeader), in_file_actual_size-sizeof(BackupInfoHeader)));
				THROW_S_S(P.Serialize(-1, sbuf, &sctx), SLERR_SSYSBU_INFORDFAULT, "serialize");
				THROW_S_S(TSCollection_Serialize(CopyFaultList, -1, sbuf, &sctx), SLERR_SSYSBU_INFORDFAULT, "serialize");
			}
		}
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::StateBlock::Dump(const char * pFileName)
{
	int    ok = 1;
	SString temp_buf;
	SString out_buf;
	SFile f_out(pFileName, SFile::mWrite);
	THROW(f_out.IsValid());
	{
		// header
		out_buf.Z();
		out_buf.CatEq("Signature", H.Signature).CR();
		out_buf.CatEq("HdrVer", H.Ver).CR();
		out_buf.CatEq("BuId", H.BuId).CR();
		out_buf.CatEq("Time", H.Dtm, DATF_ISO8601CENT, 0).CR();
	}
	{
		// param
		out_buf.CatEq("ParamVer", P.Ver).CR();
		out_buf.CatEq("UedDataFormat", P.UedDataFormat).CR();
		out_buf.CatEq("RestoreFlags", P.RestoreFlags).CR();
		out_buf.CatEq("BackupPath", P.BackupPath).CR();
		out_buf.Cat("Entries").Colon().CR();
		if(P.L.getCount()) {
			for(uint i = 0; i < P.L.getCount(); i++) {
				const Param::Entry * p_entry = P.L.at(i);
				if(p_entry) {
					const char * p_type_text = 0;
					switch(p_entry->Type) {
						case Param::etPath: p_type_text = "path"; break;
						case Param::etProfile: p_type_text = "profile"; break;
						case Param::etReg: p_type_text = "reg"; break;
						default: p_type_text = "undefined"; break;
					}
					out_buf.Tab().CatEq("type", p_type_text).CR();
					out_buf.Tab().CatEq("uuid", p_entry->Uuid, S_GUID::fmtIDL).CR();
					out_buf.Tab().CatEq("value", p_entry->Value).CR();
					out_buf.Tab().CatEq("password", p_entry->Password.NotEmpty() ? "+" : "-").CR();
				}
				else
					out_buf.Tab().Cat("null").CR();
			}
		}
		else
			out_buf.Tab().Cat("none").CR();
	}
	{
		// copy-fault-list
		out_buf.Cat("copy-fault-list").CR();
		if(CopyFaultList.getCount()) {
			for(uint i = 0; i < CopyFaultList.getCount(); i++) {
				const CopyFaultEntry * p_entry = CopyFaultList.at(i);
				if(p_entry) {
					out_buf.Tab().Cat(p_entry->Path).CR();
				}
				else
					out_buf.Tab().Cat("null").CR();
			}
		}
		else
			out_buf.Tab().Cat("none").CR();
	}
	f_out.WriteLine(out_buf);
	CATCHZOK
	return ok;
}

int SSystemBackup::WriteBackupInfo(const char * pPath, uint32 buId) { return St.Write(pPath, buId); }
int SSystemBackup::ReadBackupInfo(const char * pPath) { return St.Read(pPath); }

uint32 SSystemBackup::GetLastBackupId(const char * pPath, LDATETIME * pDtm) const
{
	uint32 result = 0;
	SString path(pPath);
	LDATETIME last_bu_dtm = ZERODATETIME;
	if(path.NotEmptyS()) {
		SString temp_buf;
		uint32 max_ex_id = 0;
		SDirEntry de;
		(temp_buf = path).SetLastSlash().Cat("*.*");
		for(SDirec dir(temp_buf); dir.Next(&de) > 0;) {
			if(de.IsFolder() && !de.IsSelf() && !de.IsUpFolder()) {
				temp_buf.CopyUtf8FromUnicode(de.Name, sstrlen(de.Name), 1);
				if(temp_buf.HasPrefixIAscii("bu")) {
					temp_buf.ShiftLeft(2);
					uint32 i = temp_buf.ToULong();
					SETMAX(max_ex_id, i);
					last_bu_dtm.SetNs100(de.ModTm_);
				}
			}
		}
		result = max_ex_id;
	}
	ASSIGN_PTR(pDtm, last_bu_dtm);
	return result;
}
	
uint32 SSystemBackup::MakeBackupId() const
{
	uint32 result = GetLastBackupId(St.P.BackupPath, 0);
	return (result >= 0) ? (result+1) : 0;
}

SString & SSystemBackup::MakeTerminalBackupPath(const SString & rBasePath, uint32 buId, SString & rBuf) const
{
	rBuf.Z();
	if(buId > 0 && /*P.BackupPath*/rBasePath.NotEmpty()) {
		rBuf.Cat(/*P.BackupPath*/rBasePath).SetLastSlash().Cat("bu").CatLongZ(buId, 8);
	}
	return rBuf;
}

SSystemBackup::BackupInfoHeader::BackupInfoHeader()
{
	Init(0);
}

SSystemBackup::BackupInfoHeader & SSystemBackup::BackupInfoHeader::Init(uint32 buId)
{
	Signature = Signature_SSystemBackup;
	Ver = SSystemBackup_StorageVer;
	BuId = buId;
	Dtm = getcurdatetime_();
	return *this;
}

SSystemBackup::Param::Param() : Ver(SSystemBackup_StorageVer), UedDataFormat(UED_DATAFORMAT_SEVENZ), Flags(0), RestoreFlags(0)
{
}
		
SSystemBackup::Param::Param(const Param & rS) : Ver(SSystemBackup_StorageVer) /* Ver не будет скопирована функцией Copy() */
{
	Copy(rS);
}
		
SSystemBackup::Param::~Param()
{
}
		
SSystemBackup::Param & FASTCALL SSystemBackup::Param::operator = (const Param & rS)
{
	Copy(rS);
	return *this;
}

int FASTCALL SSystemBackup::Param::Copy(const Param & rS)
{
	// Ver не копируем!
	UedDataFormat = rS.UedDataFormat;
	Flags = rS.Flags;
	RestoreFlags = rS.RestoreFlags;
	BackupPath = rS.BackupPath;
	TSCollection_Copy(L, rS.L);
	return 1;
}

int SSystemBackup::Param::Serialize(int dir, SBuffer & rBuf, SSerializeContext * pSCtx)
{
	int    ok = 1;
	THROW(pSCtx->Serialize(dir, Ver, rBuf));
	THROW(pSCtx->Serialize(dir, UedDataFormat, rBuf));
	THROW(pSCtx->Serialize(dir, Flags, rBuf));
	THROW(pSCtx->Serialize(dir, RestoreFlags, rBuf));
	THROW(pSCtx->Serialize(dir, BackupPath, rBuf));
	THROW(TSCollection_Serialize(L, dir, rBuf, pSCtx));
	CATCHZOK
	return ok;
}

static IMPL_CMPFUNC(SSystemBackup_Param_Entry, p1, p2)
{
	const SSystemBackup::Param::Entry * p_entry1 = static_cast<const SSystemBackup::Param::Entry *>(p1);
	const SSystemBackup::Param::Entry * p_entry2 = static_cast<const SSystemBackup::Param::Entry *>(p2);
	int si = CMPSIGN(p_entry1->Type, p_entry2->Type);
	return si ? si :  p_entry1->Value.CmpiUtf8(p_entry2->Value);
}
		
void SSystemBackup::Param::SortEntries()
{
	L.sort(PTR_CMPFUNC(SSystemBackup_Param_Entry));
}

int SSystemBackup::Param::AddPathEntry(const SString & rValue)
{
	int    ok = 1;
	if(rValue.NotEmpty()) {
		Entry * p_new_entry = L.CreateNewItem();
		if(p_new_entry) {
			p_new_entry->Type = etPath;
			p_new_entry->Uuid.Generate();
			p_new_entry->Value = rValue;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}
		
int SSystemBackup::Param::AddRegEntry(const SString & rValue)
{
	int    ok = 1;
	if(rValue.NotEmpty()) {
		Entry * p_new_entry = L.CreateNewItem();
		if(p_new_entry) {
			p_new_entry->Type = etReg;
			p_new_entry->Uuid.Generate();
			p_new_entry->Value = rValue;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}
		
int SSystemBackup::Param::AddProfileEntry(const SString & rUser, const SString & rPassword)
{
	int    ok = 1;
	if(rUser.NotEmpty()) {
		Entry * p_new_entry = L.CreateNewItem();
		if(p_new_entry) {
			p_new_entry->Type = etProfile;
			p_new_entry->Uuid.Generate();
			p_new_entry->Value = rUser;
			p_new_entry->Password = rPassword;
		}
		else
			ok = 0;
	}
	else
		ok = 0;
	return ok;
}

SSystemBackup::SSystemBackup()
{
}

SSystemBackup::SSystemBackup(const Param & rP) : St(rP)
{
	if(St.P.BackupPath.NotEmptyS()) {
		SString temp_buf;
		SFsPath::NormalizePath(St.P.BackupPath, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, temp_buf);
		St.P.BackupPath = temp_buf;
	}

}

SSystemBackup::~SSystemBackup()
{
}

int SSystemBackup::Helper_BackupPath(const SString & rDestinationPath, const SString & rPathToBackupBase, const SString & rPathToBackup, StringSet & rRecurList, void * extraPtr)
{
	int    ok = 1;
	if(rRecurList.searchNcUtf8(rPathToBackup, 0, 0)) {
		ok = -1;
	}
	else {
		rRecurList.add(rPathToBackup);

		bool   debug_mark = false;
		bool   dest_path_created = false;
		SString temp_buf;
		SString base_path;
		SString src_dir_utf8;
		SString src_file_name;
		SString dest_file_name;
		SDirEntry de;
		SFsPath ps;
		(base_path = rPathToBackup).SetLastSlash();
		(temp_buf = base_path).Cat("*.*");
		/*
			_A_ARCH 0x20
				Archive. Set whenever the file is changed and cleared by the BACKUP command. Value: 0x20.
			_A_HIDDEN 0x02
				Hidden file. Not often seen with the DIR command, unless you use the /AH option. Returns information about normal files and files that have this attribute. Value: 0x02.
			_A_NORMAL 0x00
				Normal. File has no other attributes set and can be read or written to without restriction. Value: 0x00.
			_A_RDONLY 0x01
				Read-only. File can't be opened for writing and a file that has the same name can't be created. Value: 0x01.
			_A_SUBDIR 0x10
				Subdirectory. Value: 0x10.
			_A_SYSTEM 0x04
				System file. Not ordinarily seen with the DIR command, unless the /A or /A:S option is used. Value: 0x04.
		*/
		for(SDirec direc(temp_buf); direc.Next(&de) > 0;) {
			/*if(de.IsSymLink()) {
				; // symbolic-link
				debug_mark = true;
			}
			else*/
			if(de.Attr & SFile::attrReparsePoint && de.ReparsePointTag) {
				bool copy_reparse_point_file_result = false;
				de.GetNameUtf8(base_path, src_file_name);
				SFile::Stat st;
				SBinarySet ext_set;
				if(SFile::GetStat(src_file_name, 0, &st, &ext_set)) {
					SBinaryChunk reparse_buf;
					if(ext_set.Get(SFile::Stat::sbiRaparseTag, &reparse_buf)) {
						if(SFsPath::GetRelativePath(rPathToBackupBase, FILE_ATTRIBUTE_DIRECTORY, rPathToBackup, 0, temp_buf)) {
							temp_buf.ShiftLeftChr('.').ShiftLeftChr('\\');
							(dest_file_name = rDestinationPath).SetLastSlash().Cat(temp_buf).SetLastSlash();
							de.GetNameUtf8(dest_file_name, temp_buf);
							dest_file_name = temp_buf;
							if(!dest_path_created) {
								ps.Split(dest_file_name);
								ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
								if(SFile::CreateDir(temp_buf)) {
									dest_path_created = true;
								}
							}
							if(dest_path_created) {
								SStringU & r_temp_buf_u = SLS.AcquireRvlStrU();
								dest_file_name.CopyToUnicode(r_temp_buf_u);
								SIntHandle h_dest;
								if(st.Attr & SFile::attrSubdir) {
									if(::CreateDirectory(r_temp_buf_u, 0)) {
										h_dest = SFile::ForceCreateFile(r_temp_buf_u, GENERIC_WRITE, FILE_SHARE_READ|FILE_SHARE_WRITE|FILE_SHARE_DELETE, 0/*secur_attr*/, 
											OPEN_EXISTING, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, SFile::fileforcefEmptyRetry);
									}
								}
								else {
									h_dest = SFile::ForceCreateFile(r_temp_buf_u, GENERIC_WRITE, 0/*share*/, 0/*secur_attr*/, 
										CREATE_ALWAYS, FILE_FLAG_BACKUP_SEMANTICS|FILE_FLAG_OPEN_REPARSE_POINT, SFile::fileforcefEmptyRetry);
								}
								if(!!h_dest) {
									if(SFile::SetReparsePoint(h_dest, reparse_buf)) {
										copy_reparse_point_file_result = true;
										{
											FILE_BASIC_INFO dest_file_basic_info;
											dest_file_basic_info.CreationTime.QuadPart = st.CrtTm_;
											dest_file_basic_info.LastAccessTime.QuadPart = st.AccsTm_;
											dest_file_basic_info.LastWriteTime.QuadPart = st.ModTm_;
											dest_file_basic_info.FileAttributes = st.Attr;
											SetFileInformationByHandle(h_dest, FileBasicInfo, &dest_file_basic_info, sizeof(dest_file_basic_info)); // @todo @checkerr
										}
									}
									CloseHandle(h_dest);
								}
							}
						}
					}
				}
			}
			else if(de.IsFolder()) {
				if(!de.IsSelf() && !de.IsUpFolder()) {
					de.GetNameUtf8(base_path, src_file_name);
					src_dir_utf8.CopyUtf8FromUnicode(de.Name, sstrlen(de.Name), 1);
					if(SFsPath::GetRelativePath(rPathToBackupBase, FILE_ATTRIBUTE_DIRECTORY, rPathToBackup, 0, temp_buf)) {
						temp_buf.ShiftLeftChr('.').ShiftLeftChr('\\');
						(dest_file_name = rDestinationPath).SetLastSlash().Cat(temp_buf);
						de.GetNameUtf8(dest_file_name, temp_buf);
						if(SFile::CreateDirByTemplate(temp_buf, src_file_name)) {
							;
						}
						else {
							{
								CopyFaultEntry * p_fault_entry = St.CopyFaultList.CreateNewItem();
								THROW(p_fault_entry);
								p_fault_entry->Path = src_file_name;
								*static_cast<SFile::Stat *>(p_fault_entry) = de;
							}
							; // @todo @err
						}
					}
					//
					Helper_BackupPath(rDestinationPath, rPathToBackupBase, src_file_name, rRecurList, extraPtr); // @recursion
				}
			}
			else {
				de.GetNameUtf8(base_path, src_file_name);
				if(SFsPath::GetRelativePath(rPathToBackupBase, FILE_ATTRIBUTE_DIRECTORY, rPathToBackup, 0, temp_buf)) {
					temp_buf.ShiftLeftChr('.').ShiftLeftChr('\\');
					(dest_file_name = rDestinationPath).SetLastSlash().Cat(temp_buf).SetLastSlash();
					de.GetNameUtf8(dest_file_name, temp_buf);
					dest_file_name = temp_buf;
					if(!dest_path_created) {
						ps.Split(dest_file_name);
						ps.Merge(SFsPath::fDrv|SFsPath::fDir, temp_buf);
						if(SFile::CreateDir(temp_buf)) {
							dest_path_created = true;
						}
					}
					if(dest_path_created) {
						const int cr = SCopyFile(src_file_name, dest_file_name, 0, FILE_SHARE_READ, 0);
						if(!cr) {
							{
								CopyFaultEntry * p_fault_entry = St.CopyFaultList.CreateNewItem();
								THROW(p_fault_entry);
								p_fault_entry->Path = src_file_name;
								*static_cast<SFile::Stat *>(p_fault_entry) = de;
							}
							debug_mark = true;
							; // @todo @err
						}
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::BackupProfile(uint32 buId, const SString & rProfileName, const SString & rPw)
{
	int    ok = 1;
	SString temp_buf;
	SString sub_path;
	temp_buf = rProfileName;
	THROW(temp_buf.NotEmptyS()); // @todo @err
	MakeTerminalBackupPath(St.P.BackupPath, buId, sub_path).SetLastSlash().Cat("profile").SetLastSlash().Cat(temp_buf);
	THROW(SFile::CreateDir(sub_path));
	{
		SSystem::AccountInfo acc_info;
		THROW(SSystem::GetAccountNameInfo(rProfileName, acc_info));
		acc_info.Sid.ToStr(temp_buf);
		{
			SSystem::UserProfileInfo profile_info;
			SPtrHandle h_token = SSystem::Logon(0, rProfileName, rPw, SSystem::logontypeInteractive, &profile_info);
			if(h_token) {
				if(SFile::IsDir(acc_info.ProfilePath)) {
					SString & r_profile_path = SLS.AcquireRvlStr();
					r_profile_path.CopyUtf8FromUnicode(acc_info.ProfilePath, acc_info.ProfilePath.Len(), 1);
					(temp_buf = sub_path).SetLastSlash().Cat("path");
					StringSet recur_list;
					THROW(Helper_BackupPath(temp_buf, r_profile_path, r_profile_path, recur_list, 0));
				}
				if(profile_info.ProfileRegKey) {
					SString fn;
					WinRegKey key(reinterpret_cast<HKEY>(static_cast<void *>(profile_info.ProfileRegKey)));
					THROW(key.IsValid());
					(temp_buf = sub_path).SetLastSlash().Cat("reg");
					THROW(key.Save(temp_buf));
				}
				//
				// ::UnloadUserProfile(h_token, profile_info.ProfileRegKey);
				::CloseHandle(h_token);
			}
			else {
				CALLEXCEPT(); // @todo @err
			}
		}
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::RestoreProfile(const SString & rTerminalBackupPath, const Param::Entry * pEntry)
{
	int    ok = -1;
	SString temp_buf;
	assert(pEntry && pEntry->Type == Param::etProfile);
	{
		const SString profile_name(pEntry->Value);
		const SString pw(pEntry->Password);
		SSystem::AccountInfo acc_info;
		THROW(SSystem::GetAccountNameInfo(pEntry->Value, acc_info));
		acc_info.Sid.ToStr(temp_buf);
		{
			SSystem::UserProfileInfo profile_info;
			SPtrHandle h_token = SSystem::Logon(0, profile_name, pw, SSystem::logontypeInteractive, &profile_info);
			if(h_token) {
				SString bu_path;
				if(SFile::IsDir(acc_info.ProfilePath)) {
					/*
					SString & r_profile_path = SLS.AcquireRvlStr();
					r_profile_path.CopyUtf8FromUnicode(acc_info.ProfilePath, acc_info.ProfilePath.Len(), 1);
					(temp_buf = sub_path).SetLastSlash().Cat("path");
					StringSet recur_list;
					THROW(Helper_BackupPath(temp_buf, r_profile_path, r_profile_path, recur_list, 0));
					*/
					SString org_path;
					acc_info.ProfilePath.CopyToUtf8(org_path, 1);
					(bu_path = rTerminalBackupPath).SetLastSlash().Cat("profile").SetLastSlash().Cat(profile_name).SetLastSlash().Cat("path");
					SDirecDiffPool ddp;
					const int rr = ddp.Run(bu_path, org_path);
					if(ddp.L.getCount()) {
						// @debug {
						{
							SLS.QueryPath("out", temp_buf);
							temp_buf.SetLastSlash().Cat("SDirecDiffPool-on_restore-profile-debug-out.txt");
							SFile f_out(temp_buf, SFile::mWrite);
							StringSet ss_report;
							ddp.MakeReport(ss_report);
							for(uint ssp = 0; ss_report.get(&ssp, temp_buf);) {
								f_out.WriteLine(temp_buf.CR());
							}
							f_out.WriteBlancLine();
						}
						// } @debug 
						Helper_RestorePath(bu_path, org_path, &ddp.L);
					}
				}
				if(profile_info.ProfileRegKey) {
					SString fn;
					WinRegKey key(reinterpret_cast<HKEY>(static_cast<void *>(profile_info.ProfileRegKey)));
					THROW(key.IsValid());
					(bu_path = rTerminalBackupPath).SetLastSlash().Cat("profile").SetLastSlash().Cat(profile_name).SetLastSlash().Cat("reg");
					THROW(key.Restore(bu_path));
				}
				//
				// (по-моему, это - лишнее: убивает каталог профиля - ну и зачем восстанавливали?) ::UnloadUserProfile(h_token, profile_info.ProfileRegKey);
				::CloseHandle(h_token);
			}
			else {
				CALLEXCEPT(); // @todo @err
			}
		}
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::BackupReg(uint32 buId, const S_GUID & rUuid, const SString & rReg)
{
	int    ok = 1;
	SString sub_path;
	assert(rUuid);
	MakeTerminalBackupPath(St.P.BackupPath, buId, sub_path).SetLastSlash().Cat("reg");
	THROW(SFile::CreateDir(sub_path));
	//
	{
		WinRegKey key(rReg, 1);
		THROW(key.IsValid());
		//fn.EncodeMime64(rReg.cptr(), rReg.Len());
		sub_path.SetLastSlash().Cat(rUuid, S_GUID::fmtPlain);
		THROW(key.Save(sub_path));
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::RestoreReg(const SString & rTerminalBackupPath, const Param::Entry * pEntry)
{
	int    ok = 1;
	SString sub_path;
	assert(pEntry && pEntry->Type == Param::etReg);
	THROW(pEntry && pEntry->Type == Param::etReg); 
	(sub_path = rTerminalBackupPath).SetLastSlash().Cat("reg");
	THROW(SFile::IsDir(sub_path)); // @todo @err
	{
		sub_path.SetLastSlash().Cat(pEntry->Uuid, S_GUID::fmtPlain);
		WinRegKey key(pEntry->Value, 1);
		THROW(key.IsValid());
		THROW(key.Restore(sub_path));
	}
	CATCHZOK
	return ok;
}

int SSystemBackup::Dump(const char * pFileName)
{
	return St.Dump(pFileName);
}

int SSystemBackup::Backup()
{
	int    ok = 1;
	uint32 bu_id = 0;
	SString temp_buf;
	SString sub_path;
	SString terminal_backup_path;
	if(St.P.L.getCount()) {
		THROW(St.P.BackupPath.NotEmpty());
		bu_id = MakeBackupId();
		THROW(bu_id);
		temp_buf = St.P.BackupPath;
		SFsPath::NormalizePath(temp_buf, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, St.P.BackupPath);
		THROW(MakeTerminalBackupPath(St.P.BackupPath, bu_id, terminal_backup_path).NotEmptyS());
		THROW(SFile::CreateDir(terminal_backup_path));
		St.P.SortEntries();
		for(uint i = 0; i < St.P.L.getCount(); i++) {
			const Param::Entry * p_entry = St.P.L.at(i);
			if(p_entry) {
				if(p_entry->Value.NotEmpty()) {
					switch(p_entry->Type) {
						case Param::etPath: 
							{
								SFsPath::NormalizePath(p_entry->Value, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, temp_buf);
								(sub_path = terminal_backup_path).SetLastSlash().Cat(p_entry->Uuid, S_GUID::fmtPlain);
								StringSet recur_list;
								THROW(Helper_BackupPath(sub_path, temp_buf, temp_buf, recur_list, 0)); 
							}
							break;
						case Param::etProfile: 
							THROW(BackupProfile(bu_id, p_entry->Value, p_entry->Password)); 
							break;
						case Param::etReg: 
							THROW(BackupReg(bu_id, p_entry->Uuid, p_entry->Value)); 
							break;
					}
				}
			}
		}
		WriteBackupInfo(terminal_backup_path, bu_id);
		if(St.P.Flags & Param::fDumpAfterBackup) {
			Dump((temp_buf = terminal_backup_path).SetLastSlash().Cat("dump"));
		}
	}
	else
		ok = -1;
	CATCHZOK
	return ok;
}

int SSystemBackup::Helper_RestorePath(const SString & rBuPath, const SString & rOrgPath, const TSCollection <SDirecDiffPool::Entry> * pDiffList)
{
	//
	// Если обработка какого либо элемента расхождения каталогов завершилась ошибкой, то мы не будет прекращать 
	// дальнейшую обработку, но лишь выведем информацию об ошибке в журнал.
	//
	int    ok = 1;
	SString inner_bu_path;
	SString inner_org_path;
	for(uint entry_idx = 0; entry_idx < SVector::GetCount(pDiffList); entry_idx++) {
		const SDirecDiffPool::Entry * p_diff_entry = pDiffList->at(entry_idx);
		if(p_diff_entry) {
			(inner_bu_path = rBuPath).SetLastSlash().Cat(p_diff_entry->Name);
			(inner_org_path = rOrgPath).SetLastSlash().Cat(p_diff_entry->Name);
			if(p_diff_entry->Kind == SDirecDiffPool::kAbsenceLeft) {
				if(St.P.RestoreFlags & rfRemoveNewEntries) {
					if(St.SearchCopyFaultEntry(inner_org_path, 0)) {
						; // @todo @log Указанный файл отсутствует в резервной копии из-за того, что мы не смогли его скопировать - не пытаемся его удалять!
					}
					else {
						// Удалить элемент, созданный после создания образа
						const int r = SFile::Remove(inner_org_path);
						if(!r) {
							// @todo @logerr
							ok = 0;
						}
					}
				}
			}
			else if(p_diff_entry->Kind == SDirecDiffPool::kAbsenceRight) {
				if(St.P.RestoreFlags & rfRestoreRemovedEntries) {
					// Восстановить элемент, удаленный после создания образа
					const int r = SCopyFile(inner_bu_path, inner_org_path, 0, FILE_SHARE_READ, 0);
					if(!r) {
						// @todo @logerr
						ok = 0;
					}
				}
			}
			else if(p_diff_entry->Kind == SDirecDiffPool::kDiffType || (p_diff_entry->Kind & (SDirecDiffPool::kDiffSize|SDirecDiffPool::kDiffModTime))) {
				// Удалить элемент из каталога назначения и затем скопировать аналог из образа в каталог назначения //		
				const int r1 = SFile::Remove(inner_org_path);
				if(!r1) {
					// @todo @logerr
					ok = 0;
				}
				else {
					const int r2 = SCopyFile(inner_bu_path, inner_org_path, 0, FILE_SHARE_READ, 0);
					if(!r2) {
						// @todo @logerr
						ok = 0;
					}
				}
			}
			else if(p_diff_entry->Kind & SDirecDiffPool::kDiffSubDir) {
				// Рекурсивно восстанавливаем подкаталог
				const int r = Helper_RestorePath(inner_bu_path, inner_org_path, p_diff_entry->P_RecurDir); // @recursion
				if(!r) {
					// @todo @logerr
					ok = 0;
				}
			}
			else {
				// @todo @logerr (неожиданный результат сравнения каталогов)
				ok = 0;
			}
		}
	}
	return ok;
}

int SSystemBackup::RestorePath(const SString & rTerminalBackupPath, const Param::Entry * pEntry)
{
	int    ok = -1;
	SString temp_buf;
	assert(pEntry && pEntry->Type == Param::etPath);
	SString bu_path; // Путь в резервной копии (left)
	SString org_path; // Оригинальный путь который необходимо восстановить (right)
	org_path = pEntry->Value;
	(bu_path = rTerminalBackupPath).SetLastSlash().Cat(pEntry->Uuid, S_GUID::fmtPlain);
	SDirecDiffPool ddp;
	const int rr = ddp.Run(bu_path, org_path);
	if(ddp.L.getCount()) {
		// @debug {
		{
			SLS.QueryPath("out", temp_buf);
			temp_buf.SetLastSlash().Cat("SDirecDiffPool-on_restore-debug-out.txt");
			SFile f_out(temp_buf, SFile::mWrite);
			StringSet ss_report;
			ddp.MakeReport(ss_report);
			for(uint ssp = 0; ss_report.get(&ssp, temp_buf);) {
				f_out.WriteLine(temp_buf.CR());
			}
			f_out.WriteBlancLine();
		}
		// } @debug 
		const int r = Helper_RestorePath(bu_path, org_path, &ddp.L); // @recursion
		if(!r) {
			// @todo @logerr
			ok = 0;
		}
		/*for(uint i = 0; i < ddp.L.getCount(); i++) {
			const SDirecDiffPool::Entry * p_entry = ddp.L.at(i);
			int r = Helper_RestorePath(bu_path, org_path, p_entry);
			if(!r) {
				// @todo @logerr
			}
		}*/
	}
	return ok;
}

int SSystemBackup::Restore(const char * pBackupPath, uint32 buId, uint flags)
{
	int    ok = 1;
	SString temp_buf;
	SString terminal_backup_path;
	//(temp_buf = pBackupPath).Strip();
	SFsPath::NormalizePath(pBackupPath, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, temp_buf);
	THROW(SFile::IsDir(temp_buf));
	if(buId == _FFFF32) {
		buId = GetLastBackupId(temp_buf, 0);
	}
	if(buId > 0 && buId != _FFFF32) {
		//(temp_buf = terminal_path).
		THROW(MakeTerminalBackupPath(temp_buf, buId, terminal_backup_path).NotEmptyS());
		THROW(ReadBackupInfo(terminal_backup_path));
		St.P.RestoreFlags |= flags;
		{
			for(uint i = 0; i < St.P.L.getCount(); i++) {
				const Param::Entry * p_entry = St.P.L.at(i);
				if(p_entry) {
					switch(p_entry->Type) {
						case Param::etPath:
							THROW(RestorePath(terminal_backup_path, p_entry));
							break;
						case Param::etReg:
							THROW(RestoreReg(terminal_backup_path, p_entry));
							break;
						case Param::etProfile:
							THROW(RestoreProfile(terminal_backup_path, p_entry));
							break;
					}
				}
			}
		}
	}
	CATCHZOK
	return ok;
}

void Test_SSystemBackup()
{
	SString temp_buf;
	SSystem::AccountInfo acc_info;

	// "D:\Papyrus\Src\PPTEST\DATA\Test Directory\"
	// "D:\Papyrus\Src\PPTEST\DATA\Test Directory 2\"
	{
		SLS.QueryPath("out", temp_buf);
		temp_buf.SetLastSlash().Cat("SDirecDiffPool-debug-out.txt");
		SFile f_out(temp_buf, SFile::mWrite);
		StringSet ss_report;
		SDirecDiffPool dp;
		dp.Run("D:\\Papyrus\\Src\\PPTEST\\DATA\\Test Directory", "D:\\Papyrus\\Src\\PPTEST\\DATA\\Test Directory 2");
		dp.MakeReport(ss_report);
		for(uint ssp = 0; ss_report.get(&ssp, temp_buf);) {
			f_out.WriteLine(temp_buf.CR());
		}
		f_out.WriteBlancLine();
		//
		// "C:/Users/Администратор" "D:/__TEMP__\\ssystem_backup/Администратор"
		dp.Run("C:/Users/Администратор", "D:/__TEMP__\\ssystem_backup/Администратор");
		dp.MakeReport(ss_report);
		for(uint ssp = 0; ss_report.get(&ssp, temp_buf);) {
			f_out.WriteLine(temp_buf.CR());
		}
		f_out.WriteBlancLine();
	}
	{
		SFile::Stat stat;
		SBinarySet bs_stat;
		SFile::GetStat("D:\\Papyrus\\Src\\PPTEST\\DATA\\symlink-to-Имя файла, состоящее из символов в кодировке 1251", 0, &stat, &bs_stat);
		SFile::GetStat("d:\\papyrus", 0, &stat, &bs_stat);
		SFile::GetStat("C:\\Users\\Administrator\\Application Data", 0, &stat, &bs_stat);
	}

	bool ganir = SSystem::GetAccountNameInfo(/*"wsctl-client"*/"sobolev", acc_info);
	const char * p_base_backup_path = "d:/__temp__/ssystem_backup";
	{
		SSystemBackup::Param param;
		{
			param.BackupPath = p_base_backup_path;
			int cfr = 0;
			/*{
				SStringU src_name_u;
				SStringU dest_name_u;
				param.BackupPath.CopyToUnicode(dest_name_u);
				(temp_buf = "C:/Documents and Settings/Администратор").CopyToUnicode(src_name_u);
				cfr = ::CopyFileW(src_name_u, dest_name_u, false);
			}*/
			param.BackupPath = "d:/__temp__/ssystem_backup";
			param.AddPathEntry(temp_buf = /*"C:/Python38"*//*"C:/Documents and Settings/Администратор"*/"D:/__TEMP__/c_d&s/Администратор");
			{
				SString u("wsctl-client");
				SString p("123");
				param.AddProfileEntry(u, p);
			}
			{
				acc_info.Sid.ToStr(temp_buf);
				SString reg_key_buf;
				//(reg_key_buf = "HKEY_USERS").SetLastSlash().Cat(temp_buf);
				param.AddRegEntry((reg_key_buf = "HKEY_LOCAL_MACHINE").SetLastSlash().Cat("software\\Eclipse Foundation"));
			}
			param.Flags |= SSystemBackup::Param::fDumpAfterBackup;
			SSystemBackup sb(param);
			sb.Backup();
		}
		{
			SSystemBackup sb;
			sb.Restore(p_base_backup_path, _FFFF32, SSystemBackup::rfRemoveNewEntries|SSystemBackup::rfRestoreRemovedEntries);
		}
	}
}