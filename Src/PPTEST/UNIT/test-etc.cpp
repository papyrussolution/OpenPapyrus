// TEST-ETC.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Модуль тестирования разных функций. В основном в процессе разработки.
//
#include <pp.h>
#pragma hdrstop
#include <combaseapi.h>
#include "..\slib\bzip3\include\libbz3.h"
#include "libsais.h"
#include "..\OSF\zstd\lib\include\divsufsort.h"
#include <wsctl.h>
//
//
//
SLTEST_R(ImportPo)
{
	// C:\Papyrus\Src\Rsrc\Data\iso-codes
	// C:\Papyrus\Src\Rsrc\Data\iso-codes\iso_15924
	SString temp_buf;
	SString path_buf;
	PoBlock blk(0);
	SDirEntry de;
	SString base_dir("/Papyrus/Src/Rsrc/Data/iso-codes/iso_15924/");
	(temp_buf = base_dir).Cat("*.po");
	for(SDirec sd(temp_buf); sd.Next(&de) > 0;) {
		if(!de.IsSelf() && !de.IsUpFolder() && (de.IsFolder() || de.IsFile())) {
			de.GetNameA(base_dir, temp_buf);
			SFsPath::NormalizePath(temp_buf, SFsPath::npfCompensateDotDot, path_buf);
			blk.Import(path_buf, 0, 0);
		}
	}
	blk.Finish();
	blk.Sort();
	{
		SJson * p_js = blk.ExportToJson();
		if(p_js) {
			p_js->ToStr(temp_buf);
			(path_buf = base_dir).SetLastDSlash().Cat("importpo-out-file.json");
			SFile f_out(path_buf, SFile::mWrite);
			f_out.Write(temp_buf.cptr(), temp_buf.Len());
		}
	}
	return CurrentStatus;
}

SLTEST_R(HASHTAB)
{
	SString in_buf;
	SString line_buf;
	{
		const uint test_iter_count = 1000000;
		const size_t ht_size_tab[] = { 10, 100, 1000, 100000 };
		for(uint hts_idx = 0; hts_idx < SIZEOFARRAY(ht_size_tab); hts_idx++) {
			size_t ht_size = ht_size_tab[hts_idx];
			uint   _count = 0;
			SStrCollection ptr_collection;
			PtrHashTable ht(ht_size);

			(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("email-list.txt");
			SFile inf(in_buf, SFile::mRead);
			THROW(SLCHECK_NZ(inf.IsValid()));
			while(inf.ReadLine(line_buf, SFile::rlfChomp)) {
				char * p_str = newStr(line_buf);
				THROW(SLCHECK_NZ(ptr_collection.insert(p_str)));
				//
				// Нечетные позиции вставляем в кэш, четные - нет
				//
				if(_count % 2) {
					THROW(SLCHECK_NZ(ht.Add(p_str, _count+1, 0)));
				}
				else {
					//
				}
				_count++;
			}
			THROW(SLCHECK_EQ(ptr_collection.getCount(), _count));
			for(uint i = 0; i < test_iter_count; i++) {
				uint idx = SLS.GetTLA().Rg.GetUniformInt(_count);
				THROW(SLCHECK_LT((long)idx, (long)_count));
				char * p_str = ptr_collection.at(idx);
				{
					uint val = 0;
					uint pos = 0;
					if(idx % 2) {
						SLCHECK_NZ(ht.Search(p_str, &val, &pos));
						void * ptr = ht.Get(pos);
						SLCHECK_NZ(ptr);
						SLCHECK_EQ(ptr, (const void *)p_str);
						SLCHECK_EQ(val, idx+1);
					}
					else {
						SLCHECK_Z(ht.Search(p_str, &val, &pos));
					}
				}
			}
		}
	}
	{
		//
		// 
		//
		const size_t ht_size_tab[] = { 10, 100, 1000, 100000 };
		for(uint hts_idx = 0; hts_idx < SIZEOFARRAY(ht_size_tab); hts_idx++) {
			size_t ht_size = ht_size_tab[hts_idx];
			uint   _count = 0;
			SStrCollection ptr_collection;
			TokenSymbHashTable tsht(ht_size);

			(in_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("email-list.txt");
			SFile inf(in_buf, SFile::mRead);
			THROW(SLCHECK_NZ(inf.IsValid()));
			while(inf.ReadLine(line_buf, SFile::rlfChomp)) {
				char * p_str = newStr(line_buf);
				THROW(SLCHECK_NZ(ptr_collection.insert(p_str)));
				_count++;
			}
			THROW(SLCHECK_EQ(ptr_collection.getCount(), _count));
			{
				for(long key = 1; key < ptr_collection.getCountI(); key++) {
					SLCHECK_Z(tsht.Get(key, 0));
					SLCHECK_NZ(tsht.Put(key, ptr_collection.at(key-1)));
				}
			}
			{
				for(long key = 1; key < ptr_collection.getCountI(); key++) {
					SLCHECK_EQ(tsht.Put(key, ptr_collection.at(key-1)), 1);
				}
			}
			{
				for(long key = 1; key < ptr_collection.getCountI(); key++) {
					SLCHECK_NZ(tsht.Get(key, &line_buf));
					SLCHECK_EQ(line_buf, ptr_collection.at(key-1));
				}
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
//
// @sandbox {
// Отработка итераторов
//
SLTEST_R(iterator)
{
	class _Container {
	public:
		class Iterator {
			const _Container & R_Obj;
		public:
			Iterator(const _Container & rObj) : R_Obj(rObj), Idx(0)
			{
			}
			Iterator(const Iterator & rS) : R_Obj(rS.R_Obj), Idx(rS.Idx)
			{
			}
			bool operator == (const Iterator & rS) const
			{
				return (&rS.R_Obj == &R_Obj && rS.Idx == Idx);
			}
			bool operator != (const Iterator & rS) const
			{
				return (&rS.R_Obj != &R_Obj || rS.Idx != Idx);
			}
			const int * operator * () const { return &R_Obj[Idx]; }
			const int & operator & () const { return R_Obj[Idx]; }
			Iterator & operator++ ()
			{
				if(Idx < R_Obj.C)
					Idx++;
				return *this;
			}
			Iterator operator++ (int)
			{
				Iterator preserve(*this);
				if(Idx < R_Obj.C)
					Idx++;
				return preserve;
			}
			uint   Idx;
		};
		_Container() : C(0)
		{
			MEMSZERO(Items);
		}
		const int & operator [](size_t idx) const
		{ 
			static int dummy = 0;
			return (idx < C) ? Items[idx] : dummy;
		}
		Iterator begin() const
		{
			Iterator it(*this);
			return it;
		}
		Iterator end() const
		{
			Iterator it(*this);
			it.Idx = C;
			return it;
		}
		int    Items[1024];
		uint   C;
	};
	{
		_Container c;
		int pattern_sum = 0;
		{
			for(uint i = 0; i < 103; i++) {
				c.Items[i] = (int)(i+1);
				c.C++;
				pattern_sum += (i+1);
			}
		}
		{
			int s = 0;
			for(auto item : c) {
				s += *item;
			}
			assert(s == pattern_sum);
		}
	}
	return CurrentStatus;
}

// } @sandbox

SLTEST_R(GUID) // @v11.7.11
{
	{
		S_GUID u;
		SLCHECK_NZ(u.IsZero());
		u.Generate();
		SLCHECK_Z(u.IsZero());
		S_GUID u2(u);
		SLCHECK_EQ(u2, u);
		u2.Z();
		SLCHECK_Z(u2 == u);
		SLCHECK_Z(u2);
	}
	{
		const char * p_guid_text1 = "{076A7660-6891-4E8B-A9D7-E7A8B074267B}";
		const char * p_guid_text2 = "076A7660-6891-4E8B-A9D7-E7A8B074267B";
		const char * p_guid_text3 = "076a7660-6891-4e8b-a9d7-e7a8b074267b";
		SString temp_buf;
		GUID win_guid;
		S_GUID u;
		S_GUID u2;
		u.FromStr(p_guid_text1);
		u2.FromStr(p_guid_text2);
		SLCHECK_Z(CLSIDFromString(SUcSwitchW(p_guid_text1), &win_guid));
		SLCHECK_Z(SMem::Cmp(&win_guid, &u, sizeof(S_GUID)));
		SLCHECK_NZ(u2 == u);
		u.ToStr(S_GUID::fmtIDL, temp_buf);
		SLCHECK_EQ(temp_buf, p_guid_text2);
		u.ToStr(S_GUID::fmtIDL|S_GUID::fmtLower, temp_buf);
		SLCHECK_EQ(temp_buf, p_guid_text3);
	}
	return CurrentStatus;
}

SLTEST_R(bzip3)
{
	{
		//SetInfo("Compressing shakespeare.txt back and forth in memory");
		// Read the entire "shakespeare.txt" file to memory:
		SFile f_inp(MakeInputFilePath("shakespeare.txt"), SFile::mRead|SFile::mBinary);
		//FILE * fp = fopen("shakespeare.txt", "rb");
		//fseek(fp, 0, SEEK_END);
		//int64 fsize = 0LL;
		STempBuffer in_buf(SKILOBYTE(16));
		size_t in_buf_size = 0;
		//f_inp.CalcSize(&fsize);
		//size_t size = ftell(fp);
		//fseek(fp, 0, SEEK_SET);
		THROW(SLCHECK_NZ(f_inp.IsValid()));
		THROW(SLCHECK_NZ(in_buf.IsValid()));
		THROW(SLCHECK_NZ(f_inp.ReadAll(in_buf, 0, &in_buf_size)));
		assert(in_buf.GetSize() >= in_buf_size);
		{
			//uint8 * buffer = (uint8 *)SAlloc::M(size);
			//fread(buffer, 1, size, fp);
			//fclose(fp);
			// Compress the file:
			size_t out_size = bz3_bound(in_buf_size);
			STempBuffer out_buf(out_size);
			//uint8 * outbuf = (uint8 *)SAlloc::M(out_size);
			THROW(SLCHECK_NZ(out_buf.IsValid()));
			THROW(SLCHECK_EQ(bz3_compress(SMEGABYTE(1), in_buf.ucptr(), static_cast<uint8 *>(out_buf.vptr()), in_buf_size, &out_size), BZ3_OK));
			//printf("%d => %d\n", size, out_size);
			{
				// Decompress the file.
				size_t inflated_size = in_buf_size * 2;
				STempBuffer inflate_buf(inflated_size);
				THROW(SLCHECK_NZ(inflate_buf.IsValid()));
				THROW(SLCHECK_EQ(bz3_decompress(out_buf.ucptr(), static_cast<uint8 *>(inflate_buf.vptr()), out_size, &inflated_size), BZ3_OK));
				THROW(SLCHECK_EQ(inflated_size, in_buf_size));
				{
					bool debug_mark = false;
					for(uint i = 0; i < inflated_size; i++) {
						if(inflate_buf[i] != in_buf[i]) {
							debug_mark = true;
						}
					}
				}
				THROW(SLCHECK_Z(memcmp(inflate_buf.vcptr(), in_buf.vcptr(), in_buf_size)));
				//printf("%d => %d\n", out_size, size);
				//SAlloc::F(buffer);
				//SAlloc::F(outbuf);
				//return 0;
			}
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

struct TestFixtureSuffixArray {
	TestFixtureSuffixArray() : SfxArray_Sais(0), SfxArray_DivSufSort(0), InBuf(SKILOBYTE(16)), InBufSize(0)
	{
	}
	int Init(const char * pInFileName)
	{
		int    ok = 1;
		SFile f_inp(pInFileName, SFile::mRead|SFile::mBinary);
		THROW(f_inp.IsValid());
		THROW(InBuf.IsValid());
		THROW(f_inp.ReadAll(InBuf, 0, &InBufSize));
		assert(InBuf.GetSize() >= InBufSize);
		THROW(SfxArray_Sais = new int32[InBufSize]);
		THROW(SfxArray_DivSufSort = new int32[InBufSize]);
		CATCHZOK
		return ok;
	}
	~TestFixtureSuffixArray()
	{
		delete [] SfxArray_Sais;
		delete [] SfxArray_DivSufSort;
	}
	STempBuffer InBuf;
	size_t InBufSize;
	int32 * SfxArray_Sais;
	int32 * SfxArray_DivSufSort;
};

SLTEST_FIXTURE(SuffixArray, TestFixtureSuffixArray)
{
	// int32_t libsais(const uint8_t * T, int32_t * SA, int32_t n, int32_t fs, int32_t * freq);
	// int divsufsort(const uchar * T, int * SA, int n, int openMP);
	// benchmark=sais;divsufsort
	int    bm = -1;
	if(pBenchmark == 0) {
		THROW(SLCHECK_NZ(F.Init(MakeInputFilePath("shakespeare.txt"))));
		bm = 0;
	}
	else if(sstreqi_ascii(pBenchmark, "sais"))
		bm = 1;
	else if(sstreqi_ascii(pBenchmark, "divsufsort"))
		bm = 2;
	//SFile f_inp(MakeInputFilePath("shakespeare.txt"), SFile::mRead|SFile::mBinary);
	//STempBuffer in_buf(SKILOBYTE(16));
	//int32 * sfxarray_sais = 0;
	//int32 * sfxarray_divsufsort = 0;
	//size_t in_buf_size = 0;
	
	//f_inp.CalcSize(&fsize);
	//size_t size = ftell(fp);
	//fseek(fp, 0, SEEK_SET);
	
	//THROW(SLCHECK_NZ(f_inp.IsValid()));
	//THROW(SLCHECK_NZ(in_buf.IsValid()));
	//THROW(SLCHECK_NZ(f_inp.ReadAll(in_buf, 0, &in_buf_size)));
	//assert(in_buf.GetSize() >= in_buf_size);
	if(bm == 0) {
		//sfxarray_sais = new int32[in_buf_size];
		THROW(SLCHECK_Z(libsais(F.InBuf.ucptr(), (int32_t *)F.SfxArray_Sais, F.InBufSize, 0, 0/*freq*/)));
		//sfxarray_divsufsort = new int32[in_buf_size];
		THROW(SLCHECK_Z(divsufsort(F.InBuf.ucptr(), (int *)F.SfxArray_DivSufSort, F.InBufSize, 0)));
		THROW(SLCHECK_Z(memcmp(F.SfxArray_Sais, F.SfxArray_DivSufSort, F.InBufSize * sizeof(int32))));
		{
			const char * p_pattern = "trophies";
			LongArray pos_list_fallback;
			LongArray pos_list;
			SaIndex saidx;
			saidx.SetText(F.InBuf, F.InBufSize);
			saidx.Utf8ToLower();
			THROW(SLCHECK_NZ(saidx.Build()));
			uint cf = saidx.Search_fallback(p_pattern, &pos_list_fallback);
			uint c = saidx.Search(p_pattern, &pos_list);
			pos_list_fallback.sort();
			pos_list.sort();
			SLCHECK_LE(0U, cf);
			SLCHECK_EQ(cf, c);
			SLCHECK_NZ(pos_list.IsEq(&pos_list_fallback));
		}
	}
	else if(bm == 1) {
		memzero(F.SfxArray_Sais, F.InBufSize * sizeof(int));
		THROW(SLCHECK_Z(libsais(F.InBuf.ucptr(), (int32_t *)F.SfxArray_Sais, F.InBufSize, 0, 0/*freq*/)));
	}
	else if(bm == 2) {
		memzero(F.SfxArray_DivSufSort, F.InBufSize);
		THROW(SLCHECK_Z(divsufsort(F.InBuf.ucptr(), (int *)F.SfxArray_DivSufSort, F.InBufSize, 0)));
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}

SLTEST_R(WinToken)
{
	SetInfo("@construction");
	/*
	HANDLE h = SSystem::GetLocalSystemProcessToken();
	SSystem::WinUserBlock wub;
	uint   guhf = 0;
	BOOL   loaded_profile = false;
	PROFILEINFO profile_info;
	HANDLE h_cmd_pipe = 0;
	wub.UserName = _T("petroglif\\sobolev");
	wub.Password = _T("AntonSobolev1969");
	bool guhr = SSystem::GetUserHandle(wub, guhf, loaded_profile, profile_info, h_cmd_pipe);
	*/
	return CurrentStatus;
}

SLTEST_R(WsCtl)
{
#if 1 // {	
	const char * p_js_text = "{\"account\": {\"login\": \"abc\", \"password\": \"Аб\\tВгД\" }, \"app\": { \"enable\": [ \"abc\", \"123\" ], \"disable\" : [ \"def\", \"456\", \"fheroes2.exe\" ] }}";
	WsCtl_ClientPolicy cp;
	SString temp_buf;
	SJson * p_js = SJson::Parse(p_js_text);
	SJson * p_js2 = 0;
	{
		THROW(SLCHECK_NZ(p_js));
		SLCHECK_NZ(cp.FromJsonObj(p_js));
		SLCHECK_EQ(cp.SysUser, "abc");
		SLCHECK_EQ(cp.SysPassword, "Аб\tВгД");
		SLCHECK_EQ(cp.SsAppEnabled.getCount(), 2U);
		SLCHECK_EQ(cp.SsAppDisabled.getCount(), 3U);
		{
			for(uint ssp = 0, i = 0; cp.SsAppEnabled.get(&ssp, temp_buf); i++) {
				if(i == 0) {
					SLCHECK_EQ(temp_buf, "abc");
				}
				else if(i == 1) {
					SLCHECK_EQ(temp_buf, "123");
				}
			}
		}
		{
			for(uint ssp = 0, i = 0; cp.SsAppDisabled.get(&ssp, temp_buf); i++) {
				if(i == 0) {
					SLCHECK_EQ(temp_buf, "def");
				}
				else if(i == 1) {
					SLCHECK_EQ(temp_buf, "456");
				}
				else if(i == 2) {
					SLCHECK_EQ(temp_buf, "fheroes2.exe");
				}
			}
		}
	}
	{
		p_js2 = cp.ToJsonObj();
		SLCHECK_NZ(p_js2);
		WsCtl_ClientPolicy cp2;
		SLCHECK_NZ(cp2.FromJsonObj(p_js2));
		SLCHECK_NZ(cp2 == cp);
	}
	{
		// Для этого блока нужны права значительные права доступа к реестру. По-этому его блокируем и отрабатываем только под отладчиком.
		const bool enable_test = false;
		if(enable_test) {
			int r = cp.Apply();
			SLCHECK_NZ(r);
		}
	}
	CATCH
		;
	ENDCATCH
	delete p_js;
	delete p_js2;
#endif // } 0
	return CurrentStatus;
}
//
//
//
/*
    latitude at point 1, lat1 (degrees, exact)
    longitude at point 1, lon1 (degrees, always 0)
    azimuth at point 1, azi1 (clockwise from north in degrees, exact)
    latitude at point 2, lat2 (degrees, accurate to 10e-18 deg)
    longitude at point 2, lon2 (degrees, accurate to 10e-18 deg)
    azimuth at point 2, azi2 (degrees, accurate to 10e-18 deg)
    geodesic distance from point 1 to point 2, s12 (meters, exact)
    arc distance on the auxiliary sphere, a12 (degrees, accurate to 10e-18 deg)
    reduced length of the geodesic, m12 (meters, accurate to 0.1 pm)
    the area under the geodesic, S12 (m2, accurate to 1 mm2)
*/
struct GeodTestRecord {
	GeodTestRecord()
	{
		Azi1 = 0.0;
		Azi2 = 0.0;
		GeodDistance = 0.0;
		ArcDistance = 0.0;
		M12 = 0.0;
		S12 = 0.0;
	}
	bool   FASTCALL IsEq(const GeodTestRecord & rS) const
	{
		bool   ok = true;
		THROW(P1 == rS.P1);
		THROW(P2 == rS.P2);
		THROW(feqeps(Azi1, rS.Azi1, 1E-7));
		THROW(feqeps(Azi2, rS.Azi2, 1E-7));
		THROW(feqeps(GeodDistance, rS.GeodDistance, 1E-4));
		THROW(feqeps(ArcDistance, rS.ArcDistance, 1E-7));
		THROW(feqeps(M12, rS.M12, 1E-4));
		THROW(feqeps(S12, rS.S12, 6));
		CATCHZOK
		return ok;
	}
	SGeoPosLL P1;        // Point 1 (degrees, exact). Lon always 0.
	double Azi1;         // Azimuth at point 1 (clockwise from north in degrees, exact)
	SGeoPosLL P2;        // Point 2 (degrees, exact).
	double Azi2;         // Azimuth at point 2 (degrees, accurate to 10e-18 deg)
	double GeodDistance; // Geodesic distance [P1, P2] (meters, exact)
	double ArcDistance;  // Arc distance [P1, P2] (degrees, accurate to 10e-18 deg)
	double M12;          // reduced length of the geodesic, m12 (meters, accurate to 0.1 pm)
	double S12;          // the area under the geodesic, S12 (m2, accurate to 1 mm2)
};

SLTEST_R(SGeo)
{
	SString in_file_name(MakeInputFilePath("GeodTest.dat"));
	SGeo   sg;
	uint   line_no = 0;
	SString line_buf;
	SString temp_buf;
	{
		StringSet ss;
		SFile f_in(in_file_name, SFile::mRead);
		THROW(SLCHECK_NZ(f_in.IsValid()));
		while(f_in.ReadLine(line_buf, SFile::rlfChomp)) {
			line_no++;
			volatile int  _test_result = 0; // @debug
			GeodTestRecord rec;
			ss.Z();
			line_buf.Tokenize(" \t", ss);
			uint   fld_count = 0;
			for(uint ssp = 0; ss.get(&ssp, temp_buf);) {
				fld_count++;
				switch(fld_count) {
					case 1: rec.P1.Lat = temp_buf.ToReal(); break;
					case 2: rec.P1.Lon = temp_buf.ToReal(); break;
					case 3: rec.Azi1 = temp_buf.ToReal(); break;
					case 4: rec.P2.Lat = temp_buf.ToReal(); break;
					case 5: rec.P2.Lon = temp_buf.ToReal(); break;
					case 6: rec.Azi2 = temp_buf.ToReal(); break;
					case 7: rec.GeodDistance = temp_buf.ToReal(); break;
					case 8: rec.ArcDistance = temp_buf.ToReal(); break;
					case 9: rec.M12 = temp_buf.ToReal(); break;
					case 10: rec.S12 = temp_buf.ToReal(); break;
				}
			}
			SLCHECK_EQ(fld_count, 10U);
			if(fld_count == 10) {
				{
					GeodTestRecord test_rec;
					double _m12 = 0.0;
					double _m21 = 0.0;
					test_rec.ArcDistance = sg.Direct(rec.P1, rec.Azi1, 0, rec.GeodDistance, &test_rec.P2, &test_rec.Azi2, &test_rec.GeodDistance, &test_rec.M12, &_m12, &_m21, &test_rec.S12);
					//test_rec.P1 = rec.P1;
					//test_rec.Azi1 = rec.Azi1;
					//SLCHECK_NZ(test_rec.IsEq(rec));

					//SLCHECK_NZ(test_rec.P1 == rec.P1);
					SLCHECK_NZ(test_rec.P2 == rec.P2);
					//SLCHECK_EQ_TOL(test_rec.Azi1, rec.Azi1, 1E-1);
					SLCHECK_EQ_TOL(test_rec.Azi2, rec.Azi2, 1E-1);
					SLCHECK_EQ_TOL(test_rec.GeodDistance, rec.GeodDistance, 1E-4);
					SLCHECK_EQ_TOL(test_rec.ArcDistance, rec.ArcDistance, 1E-7);
					SLCHECK_EQ_TOL(test_rec.M12, rec.M12, 1E-4);
					// SLCHECK_EQ_TOL(test_rec.S12, rec.S12, 10);

					test_rec.ArcDistance = sg.Direct(rec.P1, rec.Azi1, sg.GEOD_ARCMODE, rec.ArcDistance, &test_rec.P2, &test_rec.Azi2, &test_rec.GeodDistance, &test_rec.M12, &_m12, &_m21, &test_rec.S12);
					//test_rec.P1 = rec.P1;
					//test_rec.Azi1 = rec.Azi1;
					//SLCHECK_NZ(test_rec.IsEq(rec));

					//SLCHECK_NZ(test_rec.P1 == rec.P1);
					SLCHECK_NZ(test_rec.P2 == rec.P2);
					//SLCHECK_EQ_TOL(test_rec.Azi1, rec.Azi1, 1E-1);
					SLCHECK_EQ_TOL(test_rec.Azi2, rec.Azi2, 1E-1);
					SLCHECK_EQ_TOL(test_rec.GeodDistance, rec.GeodDistance, 1E-4);
					SLCHECK_EQ_TOL(test_rec.ArcDistance, rec.ArcDistance, 1E-7);
					SLCHECK_EQ_TOL(test_rec.M12, rec.M12, 1E-4);
					// SLCHECK_EQ_TOL(test_rec.S12, rec.S12, 10);
				}
				{
					GeodTestRecord test_rec;
					double _m12 = 0.0;
					double _m21 = 0.0;
					test_rec.ArcDistance = sg.Inverse(rec.P1, rec.P2, &test_rec.GeodDistance, &test_rec.Azi1, &test_rec.Azi2, &test_rec.M12, &_m12, &_m21, &test_rec.S12);
					//test_rec.P1 = rec.P1;
					//test_rec.P2 = rec.P2;
					//SLCHECK_NZ(test_rec.IsEq(rec));

					//SLCHECK_NZ(test_rec.P1 == rec.P1);
					//SLCHECK_NZ(test_rec.P2 == rec.P2);
					SLCHECK_EQ_TOL(test_rec.Azi1, rec.Azi1, 1E-1);
					SLCHECK_EQ_TOL(test_rec.Azi2, rec.Azi2, 1E-1);
					SLCHECK_EQ_TOL(test_rec.GeodDistance, rec.GeodDistance, 1E-4);
					SLCHECK_EQ_TOL(test_rec.ArcDistance, rec.ArcDistance, 1E-7);
					SLCHECK_EQ_TOL(test_rec.M12, rec.M12, 1E-4);
					// SLCHECK_EQ_TOL(test_rec.S12, rec.S12, 10);
				}
            }
		}
	}
	CATCH
		CurrentStatus = 0;
	ENDCATCH
	return CurrentStatus;
}
