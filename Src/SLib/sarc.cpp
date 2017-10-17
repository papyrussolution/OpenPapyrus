// SARC.CPP
// Copyright (c) A.Sobolev 2016, 2017
//
#include <slib.h>
#include <tv.h>
#pragma hdrstop
#include <..\osf\libzip\lib\zip.h>

SLAPI SArchive::SArchive()
{
	Type = 0;
	H = 0;
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
        	SPathStruc ps;
        	ps.Split(pDestName);
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
			if(isempty(pName)) {
				SPathStruc ps;
				ps.Split(temp_buf);
				ps.Merge(SPathStruc::fNam|SPathStruc::fExt, temp_buf);
			}
			else {
                SPathStruc::NormalizePath(pName, SPathStruc::npfSlash, temp_buf);
			}
			if(flags & aefDirectory) {
				new_entry_idx = zip_dir_add((zip_t *)H, pName, 0);
				THROW(new_entry_idx >= 0);
			}
			else {
				THROW(p_zsrc = zip_source_file((zip_t *)H, pSrcFileName, 0, -1));
				new_entry_idx = zip_file_add((zip_t *)H, temp_buf, p_zsrc, ZIP_FL_OVERWRITE);
				THROW(new_entry_idx >= 0);
				p_zsrc = 0;
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
		(local_path = rRoot).SetLastSlash().Cat(rSub).SetLastSlash();
		(temp_buf = local_path).Cat(rMask.NotEmpty() ? rMask : "*.*");
		SDirEntry de;
		for(SDirec dir(temp_buf); dir.Next(&de) > 0;) {
			if(!de.IsSelf() && !de.IsUpFolder()) {
				if(de.IsFolder()) {
					if(flags & aefRecursive) {
						(file_name = local_path).Cat(de.FileName);
						if(rSub.NotEmpty()) {
							(temp_buf = rSub).SetLastSlash().Cat(de.FileName);
						}
						else {
							temp_buf.Z().Cat(de.FileName);
						}
						SPathStruc::NormalizePath(temp_buf, SPathStruc::npfSlash, entry_name);
						THROW(AddEntry(file_name, entry_name, flags | aefDirectory));
						//
						if(rSub.NotEmpty()) {
							(file_name = rSub).SetLastSlash().Cat(de.FileName);
							(temp_buf = rSub).SetLastSlash().Cat(de.FileName);
						}
						else {
							file_name.Z().Cat(de.FileName);
							temp_buf.Z().Cat(de.FileName);
						}
						SPathStruc::NormalizePath(temp_buf, SPathStruc::npfSlash, entry_name);
						THROW(Helper_AddEntries(rRoot, file_name, rMask, flags)); // @recursion
					}
				}
				else {
					(file_name = local_path).Cat(de.FileName);
					(temp_buf = rSub).SetLastSlash().Cat(de.FileName);
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
	SPathStruc ps;
	ps.Split(pMask);
    ps.Merge(SPathStruc::fDrv|SPathStruc::fDir, root);
    ps.Merge(SPathStruc::fNam|SPathStruc::fExt, mask);
	return Helper_AddEntries(root, sub, mask, flags);
}

void SLAPI TestSArchive()
{
	int    ok = 1;
	const  char * p_root = "d:/papyrus/src/pptest";
	SString temp_buf;
	SArchive arc;
	// "D:\Papyrus\Src\PPTEST\DATA\Test Directory\Test Directory Level 2\Directory With Many Files"
	{
		(temp_buf = p_root).SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
		THROW(arc.Open(SArchive::tZip, temp_buf, SFile::mReadWrite));
		{
			//SDirEntry de;
			//SString src_dir = "D:/Papyrus/Src/PPTEST/DATA/Test Directory/Test Directory Level 2/Directory With Many Files";
			SString src_dir = "D:/Papyrus/Src/PPTEST/DATA";
			(temp_buf = src_dir).SetLastSlash().Cat("*.*");
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
		(temp_buf = p_root).SetLastSlash().Cat("out").SetLastSlash().Cat("zip_test.zip");
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
	CATCH
		ok = 0;
	ENDCATCH
}

