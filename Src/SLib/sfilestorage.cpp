// SFILESTORAGE.CPP
// Copyright (c) A.Sobolev 2022
//
#include <slib-internal.h>
#pragma hdrstop

class SFileStorage {
public:
	explicit SFileStorage(const char * pBasePath);
	~SFileStorage();
	bool   IsValid() const { return !(State & stError); }
	bool   operator !() const { return !IsValid(); }
	bool   Init(const char * pBasePath);
	SHandle Write_Start(const char * pName);
	bool   Write(SHandle handle, const void * pBuf, size_t size);
	bool   Write_End(SHandle handle, uint64 * pWrittenSize);
	int    PutFile(const char * pName, const void * pBuf, size_t size);
	int    PutFile(const char * pName, const char * pSourceFilePath);
	SHandle GetFile(const char * pName, int64 * pFileSize);
	bool   Read(SHandle handle, void * pBuf, size_t bufSize, size_t * pActualSize);
	//
	// Descr: Закрывает файл, открытый либо функцией Write_Start() либо GetFile()
	//
	bool   CloseFile(SHandle handle);
private:
	bool   ValidateName(const char * pName) const;
	int    MakeFileEntry(const char * pName, SString & rEntryName);

	enum {
		stError  = 0x0001,
		stInited = 0x0002
	};

	const  uint BucketCount;
	const  uint HashSeed;
	uint   State;
	SString BasePath;
	struct InnerWritingBlock {
		InnerWritingBlock();
		SFile  F;
		uint64 TotalWrSize;
	};
	struct InnerReadingBlock {
		InnerReadingBlock();
		SFile  F;
		int64  FileSize;
		uint64 TotalRdSize;
	};
	TSCollection <InnerWritingBlock> WrBlkList;
	TSCollection <InnerReadingBlock> RdBlkList;
};

SFileStorage::InnerWritingBlock::InnerWritingBlock() : TotalWrSize(0)
{
}

SFileStorage::InnerReadingBlock::InnerReadingBlock() : FileSize(0), TotalRdSize(0)
{
}

SFileStorage::SFileStorage(const char * pBasePath) : BucketCount(256), HashSeed(0x1357ace1), State(0)
{
	if(pBasePath)
		Init(pBasePath);
}

SFileStorage::~SFileStorage()
{
}

bool SFileStorage::Init(const char * pBasePath)
{
	State = 0;
	bool    ok = true;
	THROW(!isempty(pBasePath));
	THROW(createDir(pBasePath));
	BasePath = pBasePath;
	State |= stInited;
	CATCH
		State |= stError;
		ok = 0;
	ENDCATCH
	return ok;
}
//
// Правило именования сохраняемых объектов: длина имени больше нуля и меньше или равна 255 символов.
// Допускаются следующие символы: десятичные цифры, строчные ascii-буквы (только строчные - никаких прописных), _-+,.
//
bool SFileStorage::ValidateName(const char * pName) const
{
	bool   ok = true;
	const  size_t len = sstrlen(pName);
	THROW(len >= 1 && len <= 255);
	for(size_t i = 0; i < len; i++) {
		const char c = pName[i];
		THROW(isdec(c) || (c >= 'a' && c <= 'z') || oneof5(c, '_', '+', '-', ',', '.'));
	}
	CATCHZOK
	return ok;
}

int SFileStorage::MakeFileEntry(const char * pName, SString & rEntryName)
{
	rEntryName.Z();
	int    ok = 1;
	THROW(ValidateName(pName));
	THROW(IsValid());
	{
		//char   subdir[64];
		SString & r_subdir = SLS.AcquireRvlStr();
		uint32 hash = SlHash::XX32(pName, sstrlen(pName), HashSeed);
		uint bucket_no = hash % BucketCount;
		r_subdir.CatHex(static_cast<ulong>(bucket_no));
		if(r_subdir.Len() < 2) {
			r_subdir.PadLeft(2 - r_subdir.Len(), '0');
		}
		//ultoa(bucket_no, subdir, 16);
		(rEntryName = BasePath).SetLastDSlash().Cat(r_subdir);
		THROW(createDir(rEntryName));
		rEntryName.SetLastDSlash().Cat(pName);
	}
	CATCHZOK
	return ok;
}

int SFileStorage::PutFile(const char * pName, const void * pBuf, size_t size)
{
	int    ok = 1;
	SString real_path;
	THROW(size == 0 || pBuf != 0);
	THROW(IsValid());
	THROW(MakeFileEntry(pName, real_path));
	{
		SFile f(real_path, SFile::mWrite|SFile::mBinary);
		THROW(f.IsValid());
		if(size) {
			THROW(f.Write(pBuf, size));
		}
	}
	CATCHZOK
	return ok;
}

int SFileStorage::PutFile(const char * pName, const char * pSourceFilePath)
{
	const  int64 size_limit_for_copy_in_one_chunk = SMEGABYTE(4);
	int    ok = 1;
	SHandle h;
	SString real_path;
	THROW(fileExists(pSourceFilePath));
	THROW(IsValid());
	{
		int64 fsize = 0;
		SFile f_in(pSourceFilePath, SFile::mRead|SFile::mBinary);
		THROW(f_in.IsValid());
		THROW(f_in.CalcSize(&fsize));
		if(fsize == 0) {
			THROW(MakeFileEntry(pName, real_path));
			{
				SFile f(real_path, SFile::mWrite|SFile::mBinary);
				THROW(f.IsValid());
			}
		}
		else if(fsize <= size_limit_for_copy_in_one_chunk) {
			THROW(MakeFileEntry(pName, real_path));
			{
				size_t actual_size = 0;
				STempBuffer cbuf(static_cast<size_t>(fsize+256));
				THROW(cbuf.IsValid());
				THROW(f_in.Read(cbuf, static_cast<size_t>(fsize), &actual_size));
				{
					SFile f(real_path, SFile::mWrite|SFile::mBinary);
					THROW(f.IsValid());
					THROW(static_cast<int64>(actual_size) == fsize);
					THROW(f.Write(cbuf, actual_size));
				}
			}
		}
		else {
			const size_t chunk_size = SMEGABYTE(4);
			uint64 total_wr_size = 0;
			size_t actual_size = 0;
			int64  total_rd_size = 0;
			STempBuffer cbuf(chunk_size+256);
			THROW(h = Write_Start(pName));
			do {
				assert(fsize > total_rd_size);
				size_t _rd_sz = static_cast<size_t>(MIN(static_cast<int64>(chunk_size), (fsize - total_rd_size)));
				THROW(f_in.Read(cbuf, _rd_sz, &actual_size));
				THROW(Write(h, cbuf, actual_size));
				total_rd_size += static_cast<int64>(actual_size);
			} while(total_rd_size < fsize);
			THROW(Write_End(h, &total_wr_size));
			assert(total_rd_size == total_wr_size);
			h = 0;
		}
	}
	CATCHZOK
	if(h) {
		Write_End(h, 0);
	}
	return ok;
}

SHandle SFileStorage::Write_Start(const char * pName)
{
	SHandle result;
	InnerWritingBlock * p_item = 0;
	SString entry_name;
	THROW(MakeFileEntry(pName, entry_name));
	THROW(p_item = new InnerWritingBlock());
	THROW(p_item->F.Open(entry_name, SFile::mWrite|SFile::mBinary));
	WrBlkList.insert(p_item);
	result = p_item;
	p_item = 0;
	CATCH
		result = 0;
	ENDCATCH
	delete p_item;
	return result;
}

bool SFileStorage::Write(SHandle handle, const void * pBuf, size_t size)
{
	bool    ok = true;
	InnerWritingBlock * p_item = 0;
	THROW(handle);
	THROW(size == 0 || pBuf);
	for(uint i = 0; i < WrBlkList.getCount(); i++) {
		p_item = WrBlkList.at(i);
		if(p_item == handle)
			break;
		else
			p_item = 0;
	}
	THROW(p_item);
	THROW(p_item->F.IsValid());
	if(size) {
		THROW(p_item->F.Write(pBuf, size));
		p_item->TotalWrSize += size;
	}
	CATCHZOK
	return ok;
}

bool SFileStorage::Write_End(SHandle handle, uint64 * pWrittenSize)
{
	bool    ok = true;
	uint64  written_size = 0;
	uint    item_idx = 0;
	InnerWritingBlock * p_item = 0;
	THROW(handle);
	for(uint i = 0; i < WrBlkList.getCount(); i++) {
		p_item = WrBlkList.at(i);
		if(p_item == handle) {
			item_idx = i;
			break;
		}
		else
			p_item = 0;
	}
	THROW(p_item);
	if(p_item->F.IsValid())
		p_item->F.Close();
	written_size = p_item->TotalWrSize;
	WrBlkList.atFree(item_idx);
	CATCHZOK
	ASSIGN_PTR(pWrittenSize, written_size);
	return ok;
}

bool SFileStorage::CloseFile(SHandle handle)
{
	bool    ok = true;
	uint    witem_idx = 0;
	uint    ritem_idx = 0;
	THROW(handle);
	{
		InnerWritingBlock * p_w_item = 0;
		{
			for(uint i = 0; i < WrBlkList.getCount(); i++) {
				p_w_item = WrBlkList.at(i);
				if(p_w_item == handle) {
					witem_idx = i;
					break;
				}
				else
					p_w_item = 0;
			}
		}
		if(p_w_item) {
			if(p_w_item->F.IsValid())
				p_w_item->F.Close();
			WrBlkList.atFree(witem_idx);
		}
		else {
			InnerReadingBlock * p_r_item = 0;
			for(uint i = 0; i < RdBlkList.getCount(); i++) {
				p_r_item = RdBlkList.at(i);
				if(p_r_item == handle) {
					ritem_idx = i;
					break;
				}
				else
					p_r_item = 0;
			}
			if(p_r_item) {
				if(p_r_item->F.IsValid())
					p_r_item->F.Close();
				RdBlkList.atFree(ritem_idx);
			}
		}
	}
	CATCHZOK
	return ok;
}

SHandle SFileStorage::GetFile(const char * pName, int64 * pFileSize)
{
	SHandle result;
	int64  fsize = 0;
	InnerReadingBlock * p_item = 0;
	SString entry_name;
	THROW(MakeFileEntry(pName, entry_name));
	THROW(p_item = new InnerReadingBlock());
	THROW(p_item->F.Open(entry_name, SFile::mRead|SFile::mBinary));
	THROW(p_item->F.CalcSize(&fsize));
	p_item->FileSize = fsize;
	RdBlkList.insert(p_item);
	result = p_item;
	p_item = 0;
	CATCH
		result = 0;
	ENDCATCH
	ASSIGN_PTR(pFileSize, fsize);
	delete p_item;
	return result;
}

bool SFileStorage::Read(SHandle handle, void * pBuf, size_t bufSize, size_t * pActualSize)
{
	bool    ok = true;
	size_t actual_size = 0;
	InnerReadingBlock * p_item = 0;
	THROW(handle);
	THROW(pBuf && bufSize > 0);
	for(uint i = 0; i < RdBlkList.getCount(); i++) {
		p_item = RdBlkList.at(i);
		if(p_item == handle)
			break;
		else
			p_item = 0;
	}
	THROW(p_item);
	THROW(p_item->F.IsValid());
	{
		THROW(p_item->F.Read(pBuf, bufSize, &actual_size));
		p_item->TotalRdSize += actual_size;
	}
	CATCHZOK
	ASSIGN_PTR(pActualSize, actual_size);
	return ok;
}

#if SLTEST_RUNNING // {

int DummyProc_SFileStorage() { return 1; } // @forcelink

SLTEST_R(SFileStorage)
{
	int    ok = 1;
	const  uint target_item_count = 2317U;
	const  uint seedseed = 1;
	const  ulong rng_max = SKILOBYTE(512);
	const  ulong rng_big_max = SMEGABYTE(16);
	const  ulong rng_gen_max = 0x7fffffffUL;
	SHandle hw;
	SHandle hr;
	SString path = GetSuiteEntry()->InPath;
	SString out_path = GetSuiteEntry()->OutPath;
	SString storage_base_path;
	SString temp_buf;
	SString _name;
	UuidArray uuid_list;
	SRandGenerator srg;
	(storage_base_path = out_path).SetLastSlash().Cat("sfilestorage-test");
	SFileStorage fs(storage_base_path);
	STempBuffer buff(rng_big_max);
	STempBuffer small_buff(SKILOBYTE(32));
	THROW(SLTEST_CHECK_NZ(buff.IsValid()));
	THROW(SLTEST_CHECK_NZ(small_buff.IsValid()));
	{
		for(uint i = 0; i < target_item_count; i++) {
			const S_GUID uuid(SCtrGenerate_);
			const ulong seed = SlHash::XX32(&uuid, sizeof(uuid), seedseed);
			_name.Z().Cat(uuid, S_GUID::fmtLower);
			srg.Set(seed);
			const bool is_big_file = ((seed & 0x0007) == 0);
			const ulong file_size = srg.GetUniformInt(is_big_file ? rng_big_max : rng_max);
			//
			if(is_big_file) {
				ulong current_wr_size = 0;
				ulong small_buff_offs = 0;
				ulong flashed_size = 0;
				THROW(SLTEST_CHECK_Z(small_buff.GetSize() % sizeof(ulong)));
				hw = fs.Write_Start(_name);
				THROW(SLTEST_CHECK_NZ(hw));
				while(current_wr_size < file_size) {
					const ulong value = srg.GetUniformInt(rng_gen_max);
					const ulong sz_to_write = MIN(sizeof(value), (file_size-current_wr_size));
					THROW(SLTEST_CHECK_NZ((small_buff_offs+sz_to_write) <= small_buff.GetSize()));
					memcpy(small_buff+small_buff_offs, &value, sz_to_write);
					current_wr_size += sz_to_write;
					small_buff_offs += sz_to_write;
					if(small_buff_offs == small_buff.GetSize()) {
						const ulong sz_to_flash = MIN(small_buff_offs, (file_size-flashed_size));
						THROW(SLTEST_CHECK_NZ(sz_to_flash == small_buff_offs || (current_wr_size >= file_size)));
						THROW(SLTEST_CHECK_NZ(fs.Write(hw, small_buff, sz_to_flash)));
						flashed_size += sz_to_flash;
						small_buff_offs = 0;
					}
				}
				if(file_size > flashed_size) {
					THROW(SLTEST_CHECK_LE((file_size - flashed_size), small_buff_offs));
					THROW(SLTEST_CHECK_NZ(fs.Write(hw, small_buff, (file_size - flashed_size))));
					flashed_size += small_buff_offs;
					small_buff_offs = 0;				
				}
				{
					uint64 written_size = 0;
					THROW(SLTEST_CHECK_NZ(fs.Write_End(hw, &written_size)));
					THROW(SLTEST_CHECK_EQ(written_size, static_cast<uint64>(flashed_size)));
				}
			}
			else {
				ulong current_wr_size = 0;
				while(current_wr_size < file_size) {
					const ulong value = srg.GetUniformInt(rng_gen_max);
					memcpy(buff+current_wr_size, &value, sizeof(value));
					current_wr_size += sizeof(value);
				}
				THROW(SLTEST_CHECK_NZ(fs.PutFile(_name, buff, file_size))); // file_size - not current_wr_size
			}
			THROW(SLTEST_CHECK_NZ(uuid_list.insert(&uuid)));
		}
	}
	{
		THROW(SLTEST_CHECK_EQ(uuid_list.getCount(), target_item_count));
		uuid_list.shuffle();
		for(uint i = 0; i < target_item_count; i++) {
			const S_GUID uuid = uuid_list.at(i);
			const ulong seed = SlHash::XX32(&uuid, sizeof(uuid), seedseed);
			_name.Z().Cat(uuid, S_GUID::fmtLower);
			srg.Set(seed);
			const bool is_big_file = ((seed & 0x0007) == 0);
			const ulong file_size = srg.GetUniformInt(is_big_file ? rng_big_max : rng_max);
			//
			int64 fsize = 0;
			hr = fs.GetFile(_name, &fsize);
			THROW(SLTEST_CHECK_NZ(hr));
			THROW(SLTEST_CHECK_EQ(fsize, static_cast<int64>(file_size)));
			if(is_big_file) {
				ulong  read_size = 0;
				uint   vidx = 0;
				uint   iter_no = 0;
				while(read_size < file_size) {
					iter_no++;
					const ulong size_to_read = MIN((file_size - read_size), small_buff.GetSize());
					size_t actual_size = 0;
					ulong  small_buf_read_size = 0;
					THROW(SLTEST_CHECK_NZ(fs.Read(hr, small_buff, size_to_read, &actual_size)));
					THROW(SLTEST_CHECK_EQ(static_cast<ulong>(actual_size), size_to_read));
					THROW(SLTEST_CHECK_LE(static_cast<ulong>(actual_size), static_cast<ulong>(small_buff.GetSize())));
					while(small_buf_read_size < actual_size) {
						const ulong value = srg.GetUniformInt(rng_gen_max);
						const ulong size_to_check = MIN(sizeof(value), (actual_size - small_buf_read_size));
						if(size_to_check == sizeof(value)) {
							THROW(SLTEST_CHECK_EQ(*reinterpret_cast<const ulong *>(small_buff+small_buf_read_size), value));
						}
						else {
							THROW(SLTEST_CHECK_LT(size_to_check, static_cast<ulong>(sizeof(value))));
							THROW(SLTEST_CHECK_Z(memcmp(small_buff+small_buf_read_size, &value, size_to_check)));
						}
						small_buf_read_size += size_to_check;
						read_size += size_to_check;
						vidx++;
					}
				}
				THROW(SLTEST_CHECK_EQ(read_size, file_size));
			}
			else {
				size_t actual_size = 0;
				ulong  read_size = 0;
				THROW(SLTEST_CHECK_NZ(fs.Read(hr, buff, buff.GetSize(), &actual_size)));
				THROW(SLTEST_CHECK_EQ(static_cast<ulong>(actual_size), file_size));
				for(uint vidx = 0; vidx < actual_size / sizeof(ulong); vidx++) {
					const ulong value = srg.GetUniformInt(rng_gen_max);
					THROW(SLTEST_CHECK_EQ(*reinterpret_cast<const ulong *>(buff+read_size), value));
					read_size += sizeof(ulong);
				}
				if(read_size < file_size) {
					THROW(SLTEST_CHECK_LT((file_size-read_size), sizeof(ulong)));
					const ulong value = srg.GetUniformInt(rng_gen_max);
					THROW(SLTEST_CHECK_Z(memcmp(buff+read_size, &value, (file_size-read_size))));
				}
			}
			fs.CloseFile(hr);
			hr = 0;
		}
	}
	CATCHZOK
	if(hw)
		fs.CloseFile(hw);
	if(hr)
		fs.CloseFile(hr);
	RemoveDir(storage_base_path);
	return BIN(CurrentStatus == 1);
}

#endif