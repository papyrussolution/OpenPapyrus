// TEST-SARC.CPP
// Copyright (c) A.Sobolev 2023, 2024
//
#include <pp.h>
#pragma hdrstop

/*static void GetUniqArcFileName(uint64 uedDataFormat, SString & rBuf)
{
	rBuf.Z();
	SString temp_buf;
	SLS.QueryPath("testroot", temp_buf);
	long _c = 0;
	do {
		(rBuf = temp_buf).SetLastSlash().Cat("out").SetLastSlash().Cat("TestArchive");
		if(_c) {
			rBuf.CatChar('-').Cat(_c);
		}
		rBuf.Dot().Cat("7z");
		_c++;
	} while(fileExists(rBuf));
}*/

SLTEST_R(SArchive)
{
	SString temp_buf;
	SString arc_name;
	StrAssocArray deflate_result_list;
	StrAssocArray inflate_result_list;
	{
		SFile f;
		arc_name = "/Papyrus/Src/Rsrc/Data/iso-codes.7z?iso-codes/data/iso_639-3.json";
		if(f.Open(arc_name, SFile::mRead)) {
			int debug_counter = 0;
			uint8 buf[1024];
			size_t actual_size = 0;
			while(f.Read(buf, 1, &actual_size) > 0) {
				debug_counter++;
			}
		}
	}
	{
		SFile f;
		arc_name = "/Papyrus/Src/Rsrc/Data/iso-codes.7z?iso-codes/data/iso_639-3.json";
		if(f.Open(arc_name, SFile::mRead)) {
			int debug_counter = 0;
			SFile::ReadLineCsvContext ctx(',');
			StringSet ss;
			SString line_buf;
			while(f.ReadLine(line_buf, 0) > 0) {
				debug_counter++;
			}
		}
	}
	{
		SFile f;
		arc_name = "/Papyrus/Src/Rsrc/Data/iso-codes.7z?iso-codes/data/iso_639-3.json";
		if(f.Open(arc_name, SFile::mRead)) {
			int debug_counter = 0;
			SFile::ReadLineCsvContext ctx(',');
			StringSet ss;
			SString line_buf;
			while(f.ReadLineCsv(ctx, ss)) {
				debug_counter++;
			}
		}
	}
	{
		SFile f;
		arc_name = "D:/DEV/Resource/Data/ETC/USA-Huge-Leak-250-Million-250807711.7z?382.csv";
		if(f.Open(arc_name, SFile::mRead)) {
			SFile::ReadLineCsvContext ctx(',');
			StringSet ss;
			while(f.ReadLineCsv(ctx, ss)) {
				for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
					
				}
			}
		}
	}
	{
		SFile f;
		arc_name = "D:/DEV/Resource/Data/ETC/chf.su.7z?www/install/install.css";
		if(f.Open(arc_name, SFile::mRead)) {
			
		}
	}
	{
		int   r;
		//SArchive arc(SArchive::providerLA);
		{
			SString base_path;
			SLS.QueryPath("testroot", temp_buf);
			(base_path = temp_buf).SetLastSlash().Cat("data").SetLastSlash().Cat("Test Directory");
			SFsPath::NormalizePath(base_path, SFsPath::npfCompensateDotDot|SFsPath::npfKeepCase, temp_buf);
			base_path = temp_buf;
			SFileEntryPool fep;
			fep.Scan(base_path, "*.*", SFileEntryPool::scanfRecursive|SFileEntryPool::scanfKeepCase);
			{
				SLS.QueryPath("testroot", temp_buf);
				long _c = 0;
				do {
					(arc_name = temp_buf).SetLastSlash().Cat("out").SetLastSlash().Cat("TestArchive");
					if(_c) {
						arc_name.CatChar('-').Cat(_c);
					}
					arc_name.Dot().Cat("7z");
					_c++;
				} while(fileExists(arc_name));
				// 
				SArchive::Deflate(SArchive::providerLA, 0, arc_name, UED_DATAFORMAT_SEVENZ, base_path, fep);
			}
			{
				SLS.QueryPath("testroot", temp_buf);
				long _c = 0;
				do {
					(arc_name = temp_buf).SetLastSlash().Cat("out").SetLastSlash().Cat("TestArchive");
					if(_c) {
						arc_name.CatChar('-').Cat(_c);
					}
					arc_name.Dot().Cat("zip");
					_c++;
				} while(fileExists(arc_name));
				// 
				SArchive::Deflate(SArchive::providerLA, 0, arc_name, UED_DATAFORMAT_ZIP, base_path, fep);
			}
			{
				SLS.QueryPath("testroot", temp_buf);
				long _c = 0;
				do {
					(arc_name = temp_buf).SetLastSlash().Cat("out").SetLastSlash().Cat("TestArchive");
					if(_c) {
						arc_name.CatChar('-').Cat(_c);
					}
					arc_name.Dot().Cat("xar");
					_c++;
				} while(fileExists(arc_name));
				// 
				SArchive::Deflate(SArchive::providerLA, 0, arc_name, UED_DATAFORMAT_XAR, base_path, fep);
			}
		}
		{
			SLS.QueryPath("testroot", temp_buf);
			(arc_name = temp_buf).SetLastSlash().Cat("data").SetLastSlash().Cat("Test_Directory.7z");
			int arc_format = 0;
			SFileEntryPool fep;
			r = SArchive::List(SArchive::providerLA, &arc_format, arc_name, 0, fep);
			THROW(SLCHECK_NZ(r));
			SLCHECK_EQ(fep.GetCount(), 2263U);
			//
			{
				SString out_path;
				{
					long   _cntr = 0;
					SLS.QueryPath("testroot", out_path);
					out_path.SetLastSlash().Cat("out").SetLastSlash();
					do {
						(temp_buf = out_path).Cat("sarc-test-out");
						if(_cntr++)
							temp_buf.CatChar('-').CatLongZ(_cntr, 3);
					} while(SFile::IsDir(temp_buf));
					SFile::CreateDir(temp_buf);
					SArchive::InflateAll(SArchive::providerLA, arc_name, 0, temp_buf, 0);
					{
						SDirecDiffPool ddp;
						ddp.Run(temp_buf, 0);
					}
				}
				{
					long   _cntr = 0;
					SLS.QueryPath("testroot", out_path);
					out_path.SetLastSlash().Cat("out").SetLastSlash();
					do {
						(temp_buf = out_path).Cat("sarc-test-out");
						if(_cntr++)
							temp_buf.CatChar('-').CatLongZ(_cntr, 3);
					} while(SFile::IsDir(temp_buf));
					SFile::CreateDir(temp_buf);
					SArchive::Inflate(SArchive::providerLA, arc_name, 0, "*.jpg", temp_buf, 0);
				}
				/*
				r = arc.Open(temp_buf, SFile::mRead, 0);
				THROW(SLCHECK_NZ(r));
				SLCHECK_EQ(arc.GetEntriesCount(), 2263LL);
				{
					SLS.QueryPath("testroot", out_path);
					out_path.SetLastSlash().Cat("out").SetLastSlash();
					do {
						(temp_buf = out_path).Cat("sarc-test-out");
						if(_cntr++)
							temp_buf.CatChar('-').CatLongZ(_cntr, 3);
					} while(SFile::IsDir(temp_buf));
					SFile::CreateDir(temp_buf);
					for(uint i = 0; i < arc.GetEntriesCount(); i++) {
						arc.ExtractEntry(i, temp_buf);
					}
				}*/
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
