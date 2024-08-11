// TEST-ETC.CPP
// Copyright (c) A.Sobolev 2023, 2024
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
	SString temp_buf;
	GUID win_guid;
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
	// @v11.9.3 {
	{
		S_GUID prev_uuid;
		for(uint i = 0; i < 1000; i++) {
			S_GUID u(SCtrGenerate_);
			S_GUID u2;
			if(i) {
				SLCHECK_NZ(u != prev_uuid);
			}
			u.ToStr(S_GUID::fmtIDL, temp_buf);
			u2.FromStr(temp_buf);
			SLCHECK_NZ(u == u2);
			temp_buf.Quot('{', '}');
			SLCHECK_Z(CLSIDFromString(SUcSwitchW(temp_buf), &win_guid));
			SLCHECK_NZ(u == win_guid);
			//
			prev_uuid = u;
		}
	}
	// } @v11.9.3 
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
	GeodTestRecord() : Azi1(0.0), Azi2(0.0), GeodDistance(0.0), ArcDistance(0.0), M12(0.0), S12(0.0)
	{
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

struct Bechmark_ByRefVsByVal_TestStruct {
	const void * Ptr;
	uint32 A;
	//uint8  BigChunk[1024];
	uint32 B;
};

static __declspec(noinline) cdecl Bechmark_ByRefVsByVal_TestStruct Bechmark_ByRefVsByVal_Func_ByVal(uint arg)
{
	Bechmark_ByRefVsByVal_TestStruct data;
	data.Ptr = (arg & 1) ? "abcdef" : "ghijkl";
	data.A = (arg << 1);
	data.B = (arg >> 2);
	return data;
}

static __declspec(noinline) cdecl void Bechmark_ByRefVsByVal_Func_ByRef(uint arg, Bechmark_ByRefVsByVal_TestStruct & rRef)
{
	rRef.Ptr = (arg & 1) ? "abcdef" : "ghijkl";
	rRef.A = (arg << 1);
	rRef.B = (arg >> 2);
}

static __declspec(noinline) cdecl std::string Bechmark_ByRefVsByVal_Func_StdString_ByVal(uint /*arg*/)
{
	std::string result;
	result.append("abcdef");
	return result;
}

static __declspec(noinline) cdecl void Bechmark_ByRefVsByVal_Func_StdString_ByRef(uint /*arg*/, std::string & rBuf)
{
	rBuf.clear();
	rBuf.append("abcdef");
}

static volatile uint64 Bechmark_ByRefVsByVal_ResultSum = 0;

SLTEST_R(Bechmark_ByRefVsByVal)
{
	// volatile нужны дабы компилятор не пытался оптимизировать циклы и все прочее
	volatile uint round_count = 1000000000U;
	volatile uint sum_a = 0;
	volatile uint sum_b = 0;
	volatile uint sum_len = 0;
	if(pBenchmark == 0) {
		;	
	}
	else if(sstreqi_ascii(pBenchmark, "byref")) {
		for(uint i = 0; i < round_count; ++i) {
			Bechmark_ByRefVsByVal_TestStruct data;
			Bechmark_ByRefVsByVal_Func_ByRef(i, data);
			sum_a += data.A;
			sum_b += data.B;
			sum_len += strlen(static_cast<const char *>(data.Ptr));
		}
	}
	else if(sstreqi_ascii(pBenchmark, "byval")) {
		for(uint i = 0; i < round_count; ++i) {
			Bechmark_ByRefVsByVal_TestStruct data = Bechmark_ByRefVsByVal_Func_ByVal(i);
			sum_a += data.A;
			sum_b += data.B;
			sum_len += strlen(static_cast<const char *>(data.Ptr));
		}
	}
	else if(sstreqi_ascii(pBenchmark, "stdstring_byref")) {
		for(uint i = 0; i < round_count; ++i) {
			std::string data;
			Bechmark_ByRefVsByVal_Func_StdString_ByRef(i, data);
			sum_len += data.length();
		}
	}
	else if(sstreqi_ascii(pBenchmark, "stdstring_byref_recycle")) {
		std::string data;
		for(uint i = 0; i < round_count; ++i) {
			Bechmark_ByRefVsByVal_Func_StdString_ByRef(i, data);
			sum_len += data.length();
		}
	}
	else if(sstreqi_ascii(pBenchmark, "stdstring_byval")) {
		for(uint i = 0; i < round_count; ++i) {
			std::string data = Bechmark_ByRefVsByVal_Func_StdString_ByVal(i);
			sum_len += data.length();
		}
	}
	Bechmark_ByRefVsByVal_ResultSum += (sum_a + sum_b + sum_len);
	return CurrentStatus;
}

SLTEST_R(ObjTypeSymb)
{
	struct Test_ObjSymbEntry {
		const  char * P_Symb;
		long   Id;
		long   HsId;
	};
	static Test_ObjSymbEntry __Test_ObjSymbList[] = {
		{ "UNIT",           PPOBJ_UNIT,          PPHS_UNIT },
		{ "QUOTKIND",       PPOBJ_QUOTKIND,      PPHS_QUOTKIND },
		{ "LOCATION",       PPOBJ_LOCATION,      PPHS_LOCATION },
		{ "GOODS",          PPOBJ_GOODS,         PPHS_GOODS },
		{ "GOODSGROUP",     PPOBJ_GOODSGROUP,    PPHS_GOODSGROUP },
		{ "BRAND",          PPOBJ_BRAND,         PPHS_BRAND },
		{ "GOODSTYPE",      PPOBJ_GOODSTYPE,     PPHS_GOODSTYPE },
		{ "GOODSCLASS",     PPOBJ_GOODSCLASS,    PPHS_GOODSCLASS },
		{ "GOODSARCODE",    PPOBJ_GOODSARCODE,   PPHS_GOODSARCODE },
		{ "PERSON",         PPOBJ_PERSON,        PPHS_PERSON },
		{ "PERSONKIND",     PPOBJ_PERSONKIND,      PPHS_PERSONKIND },
		{ "PERSONSTATUS",   PPOBJ_PRSNSTATUS,    PPHS_PERSONSTATUS },
		{ "PERSONCATEGORY", PPOBJ_PRSNCATEGORY,  PPHS_PERSONCATEGORY },
		{ "GLOBALUSER",     PPOBJ_GLOBALUSERACC, PPHS_GLOBALUSER },
		{ "DL600",          PPOBJ_DL600DATA,     PPHS_DL600 },
		{ "WORLD",          PPOBJ_WORLD,         PPHS_WORLD },
		{ "CITY",           PPOBJ_WORLD | (WORLDOBJ_CITY << 16),    PPHS_CITY },
		{ "COUNTRY",        PPOBJ_WORLD | (WORLDOBJ_COUNTRY << 16), PPHS_COUNTRY },
		{ "QUOT",           PPOBJ_QUOT2,         PPHS_QUOT },
		{ "CURRENCY",       PPOBJ_CURRENCY,      PPHS_CURRENCY },
		{ "CURRATETYPE",    PPOBJ_CURRATETYPE,   PPHS_CURRATETYPE },
		{ "SPECSERIES",     PPOBJ_SPECSERIES,    PPHS_SPECSERIES },
		{ "SCARD",          PPOBJ_SCARD,         PPHS_SCARD },
		{ "SCARDSERIES",    PPOBJ_SCARDSERIES,   PPHS_SCARDSERIES },
		{ "POSNODE",        PPOBJ_CASHNODE,      PPHS_POSNODE },
		{ "CURRATEIDENT",   PPOBJ_CURRATEIDENT,  PPHS_CURRATEIDENT },
		{ "UHTTSCARDOP",    PPOBJ_UHTTSCARDOP,   PPHS_UHTTSCARDOP },
		{ "LOT",            PPOBJ_LOT,           PPHS_LOT },
		{ "BILL",           PPOBJ_BILL,          PPHS_BILL },
		{ "UHTTSTORE",      PPOBJ_UHTTSTORE,     PPHS_UHTTSTORE },
		{ "OPRKIND",        PPOBJ_OPRKIND,       PPHS_OPRKIND },
		{ "WORKBOOK",       PPOBJ_WORKBOOK,      PPHS_WORKBOOK },
		{ "CCHECK",         PPOBJ_CCHECK,        PPHS_CCHECK },
		{ "PROCESSOR",      PPOBJ_PROCESSOR,     PPHS_PROCESSOR },
		{ "TSESSION",       PPOBJ_TSESSION,      PPHS_TSESSION },
		{ "STYLOPALM",      PPOBJ_STYLOPALM,     PPHS_STYLOPALM },
		{ "STYLODEVICE",    PPOBJ_STYLOPALM,     PPHS_STYLODEVICE }
	};

	int    ok = 1;
	SString temp_buf;
	SString symb;
	long   ext_param = 0;
	PPID   obj_type = 0;
	for(uint i = 0; i < SIZEOFARRAY(__Test_ObjSymbList); i++) {
		Test_ObjSymbEntry & r_entry = __Test_ObjSymbList[i];
		ext_param = 0;
		obj_type = 0;
		{
			temp_buf = r_entry.P_Symb;
			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLCHECK_EQ(r_entry.Id, MakeLong(obj_type, ext_param));
			if(r_entry.HsId != PPHS_STYLODEVICE) { // Дублированный (запасной) символ
				SLCHECK_LT(0, DS.GetObjectTypeSymb(r_entry.Id, symb));
				SLCHECK_NZ(sstreqi_ascii(symb, temp_buf));
			}
		}
		{
			(temp_buf = r_entry.P_Symb).ToLower();
			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLCHECK_EQ(r_entry.Id, MakeLong(obj_type, ext_param));
			if(r_entry.HsId != PPHS_STYLODEVICE) { // Дублированный (запасной) символ
				SLCHECK_LT(0, DS.GetObjectTypeSymb(r_entry.Id, symb));
				SLCHECK_NZ(sstreqi_ascii(symb, temp_buf));
			}
		}
	}
	{
		ext_param = 0;
		obj_type = 0;
		{
			temp_buf = "CANTRY";
			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLCHECK_EQ(0L, MakeLong(obj_type, ext_param));
			symb = "abracadabra";
			SLCHECK_EQ(0, DS.GetObjectTypeSymb(31139, symb));
			SLCHECK_NZ(symb.IsEmpty());
		}
		{
			temp_buf = "id";
			obj_type = DS.GetObjectTypeBySymb(temp_buf, &ext_param);
			SLCHECK_EQ(0L, MakeLong(obj_type, ext_param));
			symb = "abracadabra";
			SLCHECK_EQ(0, DS.GetObjectTypeSymb(31139, symb));
			SLCHECK_NZ(symb.IsEmpty());
		}
	}
	return CurrentStatus;
}


struct BarcodeArray_TestEntry {
	const char * P_Barcode;
	double Qtty;
	bool   Preferred;
};

static void BarcodeArray_TestEntry_To_BarcodeArray(const BarcodeArray_TestEntry * pList, size_t listCount, BarcodeArray & rDest)
{
	assert(pList);
	assert(listCount > 0);
	rDest.clear();
	LongArray pref_idx_list;
	for(size_t i = 0; i < listCount; i++) {
		const BarcodeArray_TestEntry & r_entry = pList[i];
		BarcodeTbl::Rec bc_rec;
		bc_rec.GoodsID = 1000;
		STRNSCPY(bc_rec.Code, r_entry.P_Barcode);
		bc_rec.Qtty = r_entry.Qtty;
		if(r_entry.Preferred) {
			pref_idx_list.add(static_cast<long>(i));
		}
		rDest.insert(&bc_rec);
	}
	for(uint j = 0; j < pref_idx_list.getCount(); j++) {
		rDest.SetPreferredItem(pref_idx_list.get(j));
	}
}

SLTEST_R(BarcodeArray)
{
	BarcodeTbl::Rec bc_rec;
	BarcodeArray bc_list;
	{
		const BarcodeArray_TestEntry list[] {
			{ "4600949140018", 1.0, false },
			{ "4607096000899", 1.0, false }			
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 0U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "4690228006845", 1, false },
			{ "4607041425166", 1, false },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 0U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "4690228030901", 1, false },
			{ "4690228030918", 1, false },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 0U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "4690228028885", 1, false },
			{ "4690228028892", 1, false },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 0U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "46902280288", 1, false }, // neither ean nor upc
			{ "46902280287", 1, false }, // neither ean nor upc
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		{
			uint   single_item_idx = 0;
			const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
			SLCHECK_EQ(single_item_idx, 0U);
			SLCHECK_NZ(p_si);
			if(p_si) {
				SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
			}
		}
		{
			uint   single_item_idx = 0;
			const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, BarcodeArray::sifValidEanUpcOnly);
			SLCHECK_EQ(single_item_idx, 0U);
			SLCHECK_Z(p_si);
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "3700353610365", 1, false },
			{ "3700353610389", 1, false },
			{ "8717662020117", 1, true },
			{ "8717662020834", 1, false },
			{ "3700353612048", 1, false },
			{ "3700353611362", 1, false },
			{ "3700353611393", 1, false },
			{ "3700353610327", 1, false },
			{ "5060199510129", 1, false },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 2U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[2].P_Barcode));
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "3700353610365", 1, false },
			{ "3700353610389", 1, false },
			{ "87176620201", 1, true }, // neither ean nor upc
			{ "8717662020834", 1, false },
			{ "3700353612048", 1, false },
			{ "3700353611362", 1, false },
			{ "3700353611393", 1, false },
			{ "3700353610327", 1, false },
			{ "5060199510129", 1, false },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		{
			uint   single_item_idx = 0;
			const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
			SLCHECK_EQ(single_item_idx, 2U);
			SLCHECK_NZ(p_si);
			if(p_si) {
				SLCHECK_NZ(sstreq(p_si->Code, list[2].P_Barcode));
			}
		}
		{
			uint   single_item_idx = 0;
			const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, BarcodeArray::sifValidEanUpcOnly);
			SLCHECK_EQ(single_item_idx, 0U);
			SLCHECK_NZ(p_si);
			if(p_si) {
				SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
			}
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "3700353610365", 1, false },
			{ "3700353610389", 1, false },
			{ "8717662020117", 1, true }, // first preferred item 
			{ "8717662020834", 1, false },
			{ "3700353612048", 1, true }, // second preferred item - it will be preferred because it's position greater than previos preferred item
			{ "3700353611362", 1, false },
			{ "3700353611393", 1, false },
			{ "3700353610327", 1, false },
			{ "5060199510129", 1, false },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 4U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[4].P_Barcode));
		}
	}
	{
		const BarcodeArray_TestEntry list[] {
			{ "4850017004371", 1, true },
		};
		BarcodeArray_TestEntry_To_BarcodeArray(list, SIZEOFARRAY(list), bc_list);
		uint   single_item_idx = 0;
		const BarcodeTbl::Rec * p_si = bc_list.GetSingleItem(&single_item_idx, 0);
		SLCHECK_EQ(single_item_idx, 0U);
		SLCHECK_NZ(p_si);
		if(p_si) {
			SLCHECK_NZ(sstreq(p_si->Code, list[0].P_Barcode));
		}
	}
	return CurrentStatus;
}

struct SRecPageManager_DataEntry {
	static SRecPageManager_DataEntry * Generate(uint size)
	{
		SRecPageManager_DataEntry * p_result = 0;
		if(size) {
			p_result = static_cast<SRecPageManager_DataEntry *>(SAlloc::M(size+sizeof(SRecPageManager_DataEntry)));
			if(p_result) {
				p_result->Size = size;
				p_result->RowId = 0;
				SLS.GetTLA().Rg.ObfuscateBuffer(p_result+1, size);
			}
		}
		return p_result;
	}
	uint   Size;
	uint64 RowId;
};

class SlTestFixtureRecPageManager {
public:
	SlTestFixtureRecPageManager()
	{
	}
	//
	// ARG(rMgr IN): Менеджер записей
	// ARG(pPage IN): Страница, в которую вставляются записи
	// ARG(maxRecSize IN): Максимальный размер записи (фактический размер генерируется случайным образом, но не более maxRecSize)
	// ARG(rDataList OUT): Коллекция, в которую добавляется дубликат вставленной записи для последующей верификации
	// ARG(rStat OUT): Статистика страницы, полученная после вставки записи, для последующей верификации
	//
	int InsertOnPage(SRecPageManager & rMgr, SDataPage_ * pPage, const uint maxRecSize, SCollection & rDataList, SDataPageHeader::Stat & rStat)
	{
		int    ok = 1;
		uint rs = SLS.GetTLA().Rg.GetUniformIntPos(maxRecSize+1);
		assert(rs > 0);
		SRecPageManager_DataEntry * p_entry = SRecPageManager_DataEntry::Generate(rs);
		assert(p_entry);
		TSVector <SRecPageFreeList::Entry> free_list;
		int  gflpr = rMgr.GetFreeListForPage(pPage, free_list);
		uint tail_size = 0;
		const SRecPageFreeList::Entry * p_free_entry = SRecPageFreeList::FindOptimalFreeEntry(free_list, p_entry->Size, &tail_size);
		if(p_free_entry) {
			const uint64 rowid = p_free_entry->RowId;
			THROW(p_entry->Size <= p_free_entry->FreeSize);
			int wr = rMgr.WriteToPage(pPage, rowid, p_entry+1, p_entry->Size);
			THROW(wr > 0);
			THROW(pPage->GetStat(rStat, &free_list));
			p_entry->RowId = rowid;
			rDataList.insert(p_entry);
			p_entry = 0;
		}
		else {
			SAlloc::F(p_entry);							
			ok = -1;
		}
		CATCHZOK
		return ok;
	}
	//
	// ARG(rMgr IN): Менеджер записей
	// ARG(pPage IN): Страница, в которую вставляются записи
	// ARG(maxRecSize IN): Максимальный размер записи (фактический размер генерируется случайным образом, но не более maxRecSize)
	// ARG(rDataList OUT): Коллекция, в которую добавляется дубликат вставленной записи для последующей верификации
	//
	int UpdateOnPage(SRecPageManager & rMgr, SDataPage_ * pPage, const uint maxRecSize, SCollection & rDataList, uint dataListEntryIdx)
	{
		assert(pPage != 0);
		int    ok = 1;
		uint   rs = SLS.GetTLA().Rg.GetUniformIntPos(maxRecSize+1);
		assert(rs > 0);
		SRecPageManager_DataEntry * p_entry = static_cast<SRecPageManager_DataEntry *>(rDataList.at(dataListEntryIdx));
		assert(p_entry);
		SRecPageManager_DataEntry * p_upd_entry = SRecPageManager_DataEntry::Generate(rs);
		assert(p_upd_entry);
		uint64 rowid = p_entry->RowId;
		SDataPageHeader::Stat st;
		STempBuffer rec_buf(maxRecSize*2); // В это буфер мы будем считывать записи для верификации
		assert(rec_buf.IsValid());
		const int updr = rMgr.UpdateOnPage(pPage, rowid, p_upd_entry+1, p_upd_entry->Size);
		THROW(updr);
		THROW(pPage->GetStat(st, 0));
		{
			p_upd_entry->RowId = rowid;
			if(updr > 0) {
				rDataList.atFree(dataListEntryIdx);
				p_entry = 0;
				rDataList.atInsert(dataListEntryIdx, p_upd_entry);

				SRecPageManager_DataEntry * p_upd_entry2 = static_cast<SRecPageManager_DataEntry *>(rDataList.at(dataListEntryIdx));
				assert(p_upd_entry2 == p_upd_entry);

				uint  seq = 0;
				uint  ofs = 0;
				THROW(SRecPageManager::SplitRowId(p_upd_entry->RowId, &seq, &ofs));
				const uint read_result = pPage->Read(ofs, rec_buf, rec_buf.GetSize());
				THROW(read_result);
				if(read_result) {
					THROW(read_result == p_upd_entry->Size);
					THROW(memcmp(rec_buf, (p_upd_entry+1), read_result) == 0);
				}
			}
			else
				ok = -1; // Новая запись слишком велика для изменения в пределех одной страницы
		}
		THROW(pPage->GetStat(st, 0));
		THROW(rMgr.VerifyFreeList());
		CATCH
			ok = 0;
		ENDCATCH
		return ok;
	}
};

SLTEST_FIXTURE(SRecPageManager, SlTestFixtureRecPageManager)
{
	bool debug_mark = false;
	{
		SRecPageManager mgr(SMEGABYTE(4));
		SDataPage_ * p_page = mgr.AllocatePage(SDataPageHeader::tRecord);
		if(p_page) {
			const uint page_offs_list[] = { 32U, 65U, 130U, 259U, 517U };
			for(uint ofsidx = 0; ofsidx < SIZEOFARRAY(page_offs_list); ofsidx++) {
				const uint page_offs = page_offs_list[ofsidx];
				for(uint total_size = 1; total_size < SMEGABYTE(3); total_size++) {
					SDataPageHeader::RecPrefix pfx_wr;
					SDataPageHeader::RecPrefix pfx_rd;
					pfx_wr.SetTotalSize(total_size, SDataPageHeader::RecPrefix::fDeleted);
					const uint32 wr = p_page->WriteRecPrefix(page_offs, pfx_wr);
					SLCHECK_NZ(wr);
					SLCHECK_EQ(pfx_wr.PayloadSize+pfx_wr.GetPrefixSize(), pfx_wr.TotalSize);
					const uint32 rr = p_page->ReadRecPrefix(page_offs, pfx_rd);
					SLCHECK_NZ(rr);
					SLCHECK_EQ(rr, wr);
					SLCHECK_EQ(pfx_wr.PayloadSize, pfx_rd.PayloadSize);
					SLCHECK_EQ(pfx_wr.TotalSize, pfx_rd.TotalSize);
					SLCHECK_EQ((pfx_wr.Flags & SDataPageHeader::RecPrefix::fDeleted), (pfx_rd.Flags & SDataPageHeader::RecPrefix::fDeleted));
				}
			}
		}
	}
	{
		//
		// Тестирование работы в рамках единственной страницы данных
		//
		const uint   max_rec_size = 500;
		const uint   page_size = SKILOBYTE(512);
		const uint32 page_type = SDataPageHeader::tRecord;
		uint8 rec_buf[max_rec_size*2]; // В это буфер мы будем считывать записи для верификации
		SRecPageManager mgr(page_size);
		SDataPage_ * p_page = mgr.AllocatePage(page_type);
		SLCHECK_NZ(p_page);
		if(p_page) {
			//
			// Каждая итерация следующиего цикла: 
			// -- вставляет записи на страницу до тех пор пока там есть место,
			// -- проверяет валидность вставленных записей
			// -- удаляет все записи
			// таким образом, в конце цикла страница пуста и мы можем повторить то же самое снова
			//
			for(uint iter_idx = 0; iter_idx < 40; iter_idx++) {
				SCollection data_list;
				SDataPageHeader::Stat stat;
				//
				// Вставляем случайные записи сколько возможно
				//
				do {
					int iopr = F.InsertOnPage(mgr, p_page, max_rec_size, data_list, stat);
					SLCHECK_NZ(iopr);
				} while(CurrentStatus && stat.UsableBlockSize > 0);
				SLCHECK_NZ(data_list.getCount());
				{
					//
					// Проверяем вставленные записи
					//
					for(uint i = 0; i < data_list.getCount(); i++) {
						const SRecPageManager_DataEntry * p_entry = static_cast<const SRecPageManager_DataEntry *>(data_list.at(i));
						if(p_entry) {
							uint  seq = 0;
							uint  ofs = 0;
							SLCHECK_NZ(SRecPageManager::SplitRowId(p_entry->RowId, &seq, &ofs));
							const uint read_result = p_page->Read(ofs, rec_buf, sizeof(rec_buf));
							SLCHECK_NZ(read_result);
							if(read_result) {
								SLCHECK_EQ(read_result, p_entry->Size);
								SLCHECK_Z(memcmp(rec_buf, (p_entry+1), read_result));
							}
						}
					}
				}
				{
					//
					// Удаляем вставленные записи группами по 2 или 3 записи
					// для проверки слияния свободных областей
					//
					int _2_3_trigger = 0; // 0 - 2 records to delete, 1 - 3 records to delete
					uint recs_deleted = 0;
					const uint org_recs_count = data_list.getCount();
					while(data_list.getCount()) {
						uint recs_to_delete = 0;
						if(_2_3_trigger)
							recs_to_delete = MIN(3U, data_list.getCount());
						else
							recs_to_delete = MIN(2U, data_list.getCount());
						assert(oneof3(recs_to_delete, 1, 2, 3) && recs_to_delete <= data_list.getCount());
						//
						if(recs_to_delete == 3) {
							// удаляем в порядке 3 - 1 - 2
							uint64 rowid_list[3];
							rowid_list[0] = static_cast<const SRecPageManager_DataEntry *>(data_list.at(2))->RowId;
							rowid_list[1] = static_cast<const SRecPageManager_DataEntry *>(data_list.at(0))->RowId;
							rowid_list[2] = static_cast<const SRecPageManager_DataEntry *>(data_list.at(1))->RowId;
							for(uint j = 0; j < SIZEOFARRAY(rowid_list); j++) {
								SLCHECK_NZ(mgr.DeleteFromPage(p_page, rowid_list[j]));
								SLCHECK_NZ(mgr.VerifyFreeList());
								recs_deleted++;
							}
							data_list.atFree(0);
							data_list.atFree(0);
							data_list.atFree(0);
						}
						else if(recs_to_delete == 2) {
							// удаляем в порядке 2 - 1
							uint64 rowid_list[2];
							rowid_list[0] = static_cast<const SRecPageManager_DataEntry *>(data_list.at(1))->RowId;
							rowid_list[1] = static_cast<const SRecPageManager_DataEntry *>(data_list.at(0))->RowId;
							for(uint j = 0; j < SIZEOFARRAY(rowid_list); j++) {
								SLCHECK_NZ(mgr.DeleteFromPage(p_page, rowid_list[j]));
								SLCHECK_NZ(mgr.VerifyFreeList());
								recs_deleted++;
							}
							data_list.atFree(0);
							data_list.atFree(0);
						}
						else {
							assert(recs_to_delete == 1);
							// удаляем в порядке 2 - 1
							uint64 rowid_list[1];
							rowid_list[0] = static_cast<const SRecPageManager_DataEntry *>(data_list.at(0))->RowId;
							for(uint j = 0; j < SIZEOFARRAY(rowid_list); j++) {
								SLCHECK_NZ(mgr.DeleteFromPage(p_page, rowid_list[j]));
								SLCHECK_NZ(mgr.VerifyFreeList());
								recs_deleted++;
							}
							data_list.atFree(0);
						}
						//
						_2_3_trigger = !_2_3_trigger;
					}
					{
						SDataPageHeader::Stat st;
						SLCHECK_NZ(p_page->GetStat(st, 0));
						SLCHECK_NZ(st.IsPageFree());
					}
				}
			}
			{
				// Тест изменения записи в рамках единственной страницы
				// Сейчас страница p_page пуста
				//
				// План теста: 
				// - Единственная запись:
				//   -- Генерируем случайную запись и вставляем ее на страницу затем
				//      многократно генерируем новые данные случайного размера и обновляем ими вставленную изначально запись
				// - Две последние записи:
				//   -- Вставляем на страницу случайные записи насколько хватает места
				//      Предпоследнюю запись пытаемся многократно изменить
				SCollection data_list;
				{
					//
					// Тестирование вставки и изменения единственной записи на странице
					//
					TSVector <SRecPageFreeList::Entry> free_list;
					uint rs = SLS.GetTLA().Rg.GetUniformIntPos(max_rec_size+1);
					if(rs > 0) {		
						SRecPageManager_DataEntry * p_entry = SRecPageManager_DataEntry::Generate(rs);
						if(p_entry) {
							int gflpr = mgr.GetFreeListForPage(p_page, free_list);
							uint tail_size = 0;
							const SRecPageFreeList::Entry * p_free_entry = SRecPageFreeList::FindOptimalFreeEntry(free_list, p_entry->Size, &tail_size);
							if(p_free_entry) {
								const uint64 rowid = p_free_entry->RowId;
								SLCHECK_LE(p_entry->Size, p_free_entry->FreeSize);
								int wr = mgr.WriteToPage(p_page, rowid, p_entry+1, p_entry->Size);
								SLCHECK_NZ(wr > 0);
								p_entry->RowId = rowid;
								data_list.insert(p_entry);
								p_entry = 0;
								const uint data_list_idx = data_list.getCount()-1;
								for(uint i = 0; i < 1000; i++) {
									int uopr = F.UpdateOnPage(mgr, p_page, max_rec_size, data_list, data_list_idx);
									SLCHECK_NZ(uopr > 0); // Запись единственная по этому не может быть, что не хватило места на странице
								}
							}
						}
					}
				}
				{
					//
					// Тестирование изменения последней записи на странице
					//   вставляем записи до тех пор пока не останется менее max_rec_size свободных байт
					//   далее вставляем последнюю запись и модифицируем ее много раз
					//
					SDataPageHeader::Stat stat;
					do {
						int iopr = F.InsertOnPage(mgr, p_page, max_rec_size, data_list, stat);
						SLCHECK_NZ(iopr);
					} while(CurrentStatus && stat.UsableBlockSize > max_rec_size);
					if(CurrentStatus) {
						//
						// Теперь вставляем последнюю запись, которую будем нещадно модифицировать
						//
						const uint preserve_data_list_count = data_list.getCount();
						int iopr = 0;
						do {
							iopr = F.InsertOnPage(mgr, p_page, max_rec_size, data_list, stat);
						} while(iopr < 0);
						SLCHECK_NZ(iopr > 0);
						SLCHECK_EQ(data_list.getCount(), preserve_data_list_count+1);
						if(CurrentStatus) {
							const uint data_list_idx = data_list.getCount()-1;
							uint  success_count = 0;
							uint  iter_no = 0;
							for(; CurrentStatus && iter_no < 10000; iter_no++) {
								int uopr = F.UpdateOnPage(mgr, p_page, max_rec_size, data_list, data_list_idx);
								SLCHECK_NZ(uopr);
								if(uopr > 0)
									success_count++;
							}
							debug_mark = true;
						}
					}
				}
				{
					// Теперь мы меняем произвольные записи на странице много раз
					if(CurrentStatus) {
						uint  success_count = 0;
						uint  iter_no = 0;
						for(; CurrentStatus && iter_no < 100000; iter_no++) {
							uint   data_list_idx = SLS.GetTLA().Rg.GetUniformIntPos(data_list.getCount()+1)-1;
							assert(data_list_idx < data_list.getCount());
							int uopr = F.UpdateOnPage(mgr, p_page, max_rec_size, data_list, data_list_idx);
							SLCHECK_NZ(uopr);
							if(uopr > 0)
								success_count++;
						}
						debug_mark = true;
					}
				}
			}
		}
	}
	{
		const uint page_size_list[] = { 512, 512 * 2, 512 * 3, 512 * 4, 512 * 5, 512 * 6, 512 * 1024, SMEGABYTE(2) };
		for(uint psi = 0; psi < SIZEOFARRAY(page_size_list); psi++) {
			const uint page_size = page_size_list[psi];
			for(uint seq = 1; seq < SMEGABYTE(1); seq += 7) {
				uint offset_list[1024];
				uint offset_list_count = 0;
				{
					for(uint i = sizeof(SDataPageHeader); i < 128; i++) {
						offset_list[offset_list_count++] = i;
					}
				}
				offset_list[offset_list_count++] = page_size-1;
				{
					for(uint i = 129; offset_list_count < SIZEOFARRAY(offset_list) && i < page_size; i += 119) {
						offset_list[offset_list_count++] = i;
					}
				}
				SLCHECK_Z(SRecPageManager::MakeRowId(page_size, seq, page_size)); // ошибочное смещение
				SLCHECK_Z(SRecPageManager::MakeRowId(page_size, 0, page_size-7)); // ошибочный номер страницы (0)
				for(uint oi = 0; oi < offset_list_count; oi++) {
					const uint offs = offset_list[oi];
					uint64 row_id = SRecPageManager::MakeRowId(page_size, seq, offs);
					SLCHECK_NZ(row_id);
					uint seq__ = 0;
					uint ofs__ = 0;
					SRecPageManager::SplitRowId_WithPageSizeCheck(row_id, page_size, &seq__, &ofs__);
					SLCHECK_EQ(seq__, seq);
					SLCHECK_EQ(ofs__, offs);
				}
			}			
		}
	}
	{
		SCollection data_list;
		const uint entry_count = 1000;
		const uint max_rec_size = 500;
		SRecPageManager rm(4096);
		{
			for(uint i = 0; i < entry_count; i++) {
				uint rs = SLS.GetTLA().Rg.GetUniformIntPos(max_rec_size+1);
				if(rs > 0) {
					SRecPageManager_DataEntry * p_entry = SRecPageManager_DataEntry::Generate(rs);
					if(p_entry) {
						uint64 row_id = 0;
						const int write_result = rm.Write(&row_id, SDataPageHeader::tRecord, p_entry+1, p_entry->Size);
						SLCHECK_NZ(write_result);
						if(write_result) {
							p_entry->RowId = row_id;
							data_list.insert(p_entry);
						}
						else
							SAlloc::F(p_entry);
					}
				}
			}
		}
		{
			for(uint i = 0; i < data_list.getCount(); i++) {
				const SRecPageManager_DataEntry * p_entry = static_cast<const SRecPageManager_DataEntry *>(data_list.at(i));
				if(p_entry) {
					uint8 rec_buf[max_rec_size*2];
					const uint read_result = rm.Read(p_entry->RowId, rec_buf, sizeof(rec_buf));
					SLCHECK_NZ(read_result);
					if(read_result) {
						SLCHECK_EQ(read_result, p_entry->Size);
						SLCHECK_Z(memcmp(rec_buf, (p_entry+1), read_result));
					}
				}
			}
		}
	}
	return CurrentStatus;
}

SLTEST_R(PPExprParser)
{
	struct TestExpr {
		const char * P_Expr;
		double EstResult;
	};
	const TestExpr expr_list_valid[] = {
		//{ "sieve[0.0; 5?1; 6?2; 7?3](5.5)", 2.0 },
		{ "0.719 = 0.7 + 0.019", 1.0 },
		{ "0.719 != 0.7 + 0.019", 0.0 },
		{ "0.719 != 0.7 * 0.019", 1.0 },
		{ "1.5 > 1.4", 1.0 },
		{ "0.5 > 1.4", 0.0 },
		{ "0.5 >= 1.4", 0.0 },
		{ "1.5 >= 1.5", 1.0 },
		{ "1.5 >= 1.0001", 1.0 },
		{ "-5 <= 0.0", 1.0 },
		{ "-5 <= -2 * 2.5", 1.0 },
		{ "-1.5 * 2 - 2 = -2 * 2.5", 1.0 },
		{ "sin(pi/2)", sin(SMathConst::Pi/2.0) },
		{ "cos(pi/4)", cos(SMathConst::Pi/4.0) },
		{ "1", 1.0 },
		{ "1 + 1000", 1001.0 },
		{ "2 + 1000 * 7", 7002.0 },
	};
	const double tolerance = 1.0e-14;
	for(uint i = 0; i < SIZEOFARRAY(expr_list_valid); i++) {
		double result = 0.0;
		const char * p_expr = expr_list_valid[i].P_Expr;
		const double est_result = expr_list_valid[i].EstResult;
		SLCHECK_NZ(PPExprParser::CalcExpression(p_expr, &result, 0/*const PPCalcFuncList * pFuncList*/, 0/*ExprEvalContext * pCtx*/));
		SLCHECK_EQ_TOL(result, est_result, tolerance);
	}
	return CurrentStatus;
}