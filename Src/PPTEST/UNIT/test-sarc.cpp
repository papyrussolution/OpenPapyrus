// TEST-SARC.CPP
// Copyright (c) A.Sobolev 2023
//
#include <pp.h>
#pragma hdrstop

SLTEST_R(SArchive)
{
	SString temp_buf;
	{
		SArchive arc(SArchive::providerLA);
		SLS.QueryPath("testroot", temp_buf);
		temp_buf.SetLastSlash().Cat("data").SetLastSlash().Cat("Test_Directory.7z");
		int r = arc.Open(temp_buf, SFile::mRead, 0);
		THROW(SLCHECK_NZ(r));
		SLCHECK_EQ(arc.GetEntriesCount(), 2263LL);
		{
			SString out_path;
			long   _cntr = 0;
			SLS.QueryPath("testroot", out_path);
			out_path.SetLastSlash().Cat("out").SetLastSlash();
			do {
				(temp_buf = out_path).Cat("sarc-test-out");
				if(_cntr++)
					temp_buf.CatChar('-').CatLongZ(_cntr, 3);
			} while(IsDirectory(temp_buf));
			createDir(temp_buf);
			for(uint i = 0; i < arc.GetEntriesCount(); i++) {
				arc.ExtractEntry(i, temp_buf);
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
