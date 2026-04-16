// TEST-STYPE.CPP
// Copyright (c) A.Sobolev 2026
// @codepage UTF-8
// Тесты для классов DataType и сопуствущие
//
#include <pp.h>
#pragma hdrstop

SLTEST_R(SType) // @v12.5.7 @construction
{
	bool   debug_mark = false; // @debug
	SString temp_buf;
	char   sbuf[2048];
	{ // SZString
		(temp_buf = GetSuiteEntry()->InPath).SetLastSlash().Cat("phrases-ru-1251.txt");
		SFile f_in(temp_buf, SFile::mRead);
		if(f_in.IsValid()) {
			SString line_buf;
			constexpr size_t _work_buf_size = 1024;
			char   _data[_work_buf_size];
			char   _min_val[_work_buf_size];
			char   _max_val[_work_buf_size];
			uint   max_text_len = 0;
			while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
				SETMAX(max_text_len, line_buf.Len32());
			}
			memzero(_data, sizeof(_data));
			const  uint _sz = max_text_len+1;
			assert(_sz <= sizeof(_data));
			const  uint styp = MKSTYPE(S_ZSTRING, _sz);
			SLCHECK_EQ(stsize(styp), _sz);
			SLCHECK_EQ(stbase(styp), BTS_STRING);
			stminval(styp, _min_val);
			stmaxval(styp, _max_val);
			SLCHECK_NZ(_min_val[0] == 0);
			SLCHECK_NZ(ismemchr(_max_val, _sz-1, '\xff'));
			f_in.Seek(0, 0);
			while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
				char   d_r[_work_buf_size];
				const  long fmt = 0;
				{
					char * r1 = sttostr(styp, line_buf.cptr(), fmt, sbuf);
					SString & r2 = sttosstr(styp, line_buf.cptr(), fmt, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, d_r, fmt, sbuf));
					SLCHECK_NZ(line_buf == d_r);
					SLCHECK_Z(stcomp(styp, line_buf.cptr(), d_r));
					debug_mark = false; // @debug
				}
			}
		}
	}
	{ // SRaw
		constexpr uint _val_test_count = 200;
		constexpr size_t _work_buf_size = 1024;
		for(uint iter_sz = 4; iter_sz <= 512; iter_sz++) {
			const  uint _sz = iter_sz;
			const  uint styp = MKSTYPE(S_RAW, _sz);
			char   _data[_work_buf_size];
			char   _min_val[_work_buf_size];
			char   _max_val[_work_buf_size];
			SLCHECK_EQ(stsize(styp), _sz);
			SLCHECK_EQ(stbase(styp), BTS_STRING);
			stminval(styp, _min_val);
			stmaxval(styp, _max_val);
			SLCHECK_NZ(ismemchr(_min_val, _sz, 0x00));
			SLCHECK_NZ(ismemchr(_max_val, _sz, 0xff));
			for(uint i = 0; i < _val_test_count; i++) {
				char   d_r[_work_buf_size];
				const  long fmt = 0;
				SLS.GetTLA().Rg.ObfuscateBuffer(_data, _sz);
				{
					// На итерации iter_sz == 190 сбой потому что SRaw::tostr закладывается на то, что длина буфера-получателя не более 256!
					char * r1 = sttostr(styp, _data, fmt, sbuf);
					SString & r2 = sttosstr(styp, _data, fmt, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, fmt, sbuf));
					SLCHECK_Z(memcmp(_data, d_r, _sz));
					SLCHECK_Z(stcomp(styp, _data, d_r));
					debug_mark = false; // @debug
				}
			}
		}
	}
	{ // SGuid
		constexpr uint _val_test_count = 1000;
		constexpr uint _sz = sizeof(S_GUID);
		assert(_sz == 16);
		const  uint styp = MKSTYPE(S_UUID_, _sz);
		S_GUID min_val;
		S_GUID max_val;
		SLCHECK_EQ(stsize(styp), _sz);
		SLCHECK_EQ(stbase(styp), BTS_STRING);
		stminval(styp, &min_val);
		stmaxval(styp, &max_val);
		SLCHECK_NZ(ismemchr(&min_val, sizeof(min_val), 0x00));
		SLCHECK_NZ(ismemchr(&max_val, sizeof(min_val), 0xff));
		for(uint i = 0; i < _val_test_count; i++) {
			S_GUID iter_val(SCtrGenerate_);
			S_GUID d_r;
			const  long fmt = 0;
			{
				char * r1 = sttostr(styp, &iter_val, fmt, sbuf);
				SString & r2 = sttosstr(styp, &iter_val, fmt, temp_buf);
				SLCHECK_Z(isempty(sbuf));
				SLCHECK_Z(temp_buf.IsEmpty());
				SLCHECK_EQ(r1, &sbuf);
				SLCHECK_EQ(&r2, &temp_buf);
				SLCHECK_EQ(temp_buf, sbuf);
				//
				SLCHECK_NZ(stfromstr(styp, &d_r, fmt, sbuf));
				SLCHECK_EQ(iter_val, d_r);
				SLCHECK_Z(stcomp(styp, &iter_val, &d_r));
			}
		}
	}
	{ // SDate
		constexpr uint _sz = 4;
		const  uint styp = MKSTYPE(S_DATE, _sz);
		LDATE  min_val;
		LDATE  max_val;
		SLCHECK_EQ(stsize(styp), _sz);
		SLCHECK_EQ(stbase(styp), BTS_DATE);
		stminval(styp, &min_val);
		stmaxval(styp, &max_val);
		SLCHECK_EQ(min_val, ZERODATE);
		SLCHECK_EQ(max_val, MAXDATEVALID);
		assert(MAXDATEVALID == encodedate(1, 1, 3000));
		{
			const LDATE last_dt = encodedate(31, 12, 2099);
			const long fmt_list[] = {
				MKSFMT(0, DATF_ISO8601CENT),
				MKSFMT(0, DATF_GERMAN|DATF_CENTURY),
				MKSFMT(0, DATF_DMY|DATF_CENTURY),
				MKSFMT(0, DATF_AMERICAN|DATF_CENTURY),
				MKSFMT(0, DATF_SQL),
			};
			for(LDATE iter_dt = encodedate(1, 1, 1801); iter_dt <= last_dt; iter_dt = plusdate(iter_dt, 1)) {
				for(uint fmt_idx = 0; fmt_idx < SIZEOFARRAY(fmt_list); fmt_idx++) {
					const  long fmt = fmt_list[fmt_idx];
					LDATE  d_r;
					{
						char * r1 = sttostr(styp, &iter_dt, fmt, sbuf);
						SString & r2 = sttosstr(styp, &iter_dt, fmt, temp_buf);
						SLCHECK_Z(isempty(sbuf));
						SLCHECK_Z(temp_buf.IsEmpty());
						SLCHECK_EQ(r1, &sbuf);
						SLCHECK_EQ(&r2, &temp_buf);
						SLCHECK_EQ(temp_buf, sbuf);
						//
						SLCHECK_NZ(stfromstr(styp, &d_r, fmt, sbuf));
						SLCHECK_EQ(iter_dt, d_r);
						SLCHECK_Z(stcomp(styp, &iter_dt, &d_r));
						debug_mark = false; // @debug
					}
				}
			}
		}
	}
	{ // STime
		constexpr uint _sz = 4;
		const  uint styp = MKSTYPE(S_TIME, _sz);
		LTIME  min_val;
		LTIME  max_val;
		SLCHECK_EQ(stsize(styp), _sz);
		SLCHECK_EQ(stbase(styp), BTS_TIME);
		stminval(styp, &min_val);
		stmaxval(styp, &max_val);
		SLCHECK_EQ(min_val, ZEROTIME);
		SLCHECK_EQ(max_val, MAXTIME);
		{
			const LTIME last_tm = encodetime(23, 59, 59, 99);
			const long fmt_list[] = {
				MKSFMT(0, TIMF_HMS|TIMF_MSEC),
			};
			for(LTIME iter_tm = encodetime(0, 0, 0, 0); iter_tm <= last_tm; iter_tm.addhs(17)) {
				for(uint fmt_idx = 0; fmt_idx < SIZEOFARRAY(fmt_list); fmt_idx++) {
					const  long fmt = fmt_list[fmt_idx];
					LTIME  d_r;
					{
						char * r1 = sttostr(styp, &iter_tm, fmt, sbuf);
						SString & r2 = sttosstr(styp, &iter_tm, fmt, temp_buf);
						SLCHECK_Z(isempty(sbuf));
						SLCHECK_Z(temp_buf.IsEmpty());
						SLCHECK_EQ(r1, &sbuf);
						SLCHECK_EQ(&r2, &temp_buf);
						SLCHECK_EQ(temp_buf, sbuf);
						//
						SLCHECK_NZ(stfromstr(styp, &d_r, fmt, sbuf));
						SLCHECK_EQ(iter_tm, d_r);
						SLCHECK_Z(stcomp(styp, &iter_tm, &d_r));
					}
				}				
			}
		}
	}
	{ // SDec
		constexpr uint _val_test_count = 1000;
		for(uint iter_sz = 8; iter_sz <= 20; iter_sz++) {
			const  uint _sz = iter_sz;
			const  uint _prec = 2;
			const  uint styp = MKSTYPED(S_DEC, _sz, _prec);
			constexpr size_t _work_buf_size = 64;
			//char   _data[_work_buf_size];
			char   _min_val[_work_buf_size];
			char   _max_val[_work_buf_size];
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_NZ(stisnumber(styp));
			SLCHECK_EQ(stbase(styp), BTS_REAL);
			stminval(styp, _min_val);
			stmaxval(styp, _max_val);
			//SLCHECK_EQ(min_val, -SMathConst::Max_f);
			//SLCHECK_EQ(max_val,  SMathConst::Max_f);
			{
				for(uint i = 0; i < _val_test_count; i++) {
					double d = static_cast<double>(SLS.GetTLA().Rg.GetUniformPos());
					char   d_[_work_buf_size];
					char   d_r[_work_buf_size];
					d *= fpow10i(R0i(d * 10.0));
					if(i & 0x01) {
						d = -d;
					}
					dectodec(d, d_, static_cast<int16>(_sz), static_cast<int16>(_prec));
					//
					const  long _fmt = MKSFMTD(0, _prec, 0);
					char * r1 = sttostr(styp, d_, _fmt, sbuf);
					SString & r2 = sttosstr(styp, d_, _fmt, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, _fmt, sbuf));
					SLCHECK_Z(deccmp(d_, d_r, _sz));
					SLCHECK_Z(stcomp(styp, d_, d_r));
					debug_mark = false; // @debug
				}
			}
		}
	}
	{ // SFloat
		constexpr uint _val_test_count = 1000;
		{ // size == 4
			constexpr uint _sz = 4;
			const  uint styp = MKSTYPE(S_FLOAT, _sz);
			float  min_val = 0.0f;
			float  max_val = 0.0f;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_NZ(stisnumber(styp));
			SLCHECK_EQ(stbase(styp), BTS_REAL);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(min_val, -SMathConst::Max_f);
			SLCHECK_EQ(max_val,  SMathConst::Max_f);
			{
				for(uint i = 0; i < _val_test_count; i++) {
					float  d = static_cast<float>(SLS.GetTLA().Rg.GetUniformPos());
					assert(d <= 1.0f);
					d *= fpow10fi(static_cast<int>(R0f(d * 10.0f)));
					if(i & 0x01) {
						d = -d;
					}
					for(int prec = -2; prec <= 6; prec++) {
						const  long _fmt = MKSFMTD(0, prec, 0);
						char * r1 = sttostr(styp, &d, _fmt, sbuf);
						SString & r2 = sttosstr(styp, &d, _fmt, temp_buf);
						SLCHECK_Z(isempty(sbuf));
						SLCHECK_Z(temp_buf.IsEmpty());
						SLCHECK_EQ(r1, &sbuf);
						SLCHECK_EQ(&r2, &temp_buf);
						SLCHECK_EQ(temp_buf, sbuf);
					}
					//
					//float  d_r;
					//SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					//SLCHECK_EQ(d, d_r);
				}
			}
		}
		{ // size == 8
			constexpr uint _sz = 8;
			const  uint styp = MKSTYPE(S_FLOAT, _sz);
			double min_val = 0.0;
			double max_val = 0.0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_NZ(stisnumber(styp));
			SLCHECK_EQ(stbase(styp), BTS_REAL);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(min_val, -SMathConst::Max);
			SLCHECK_EQ(max_val,  SMathConst::Max);
			{
				for(uint i = 0; i < _val_test_count; i++) {
					double d = static_cast<double>(SLS.GetTLA().Rg.GetUniformPos());
					d *= fpow10i(R0i(d * 10.0));
					if(i & 0x01) {
						d = -d;
					}
					for(int prec = -2; prec <= 6; prec++) {
						const  long _fmt = MKSFMTD(0, prec, 0);
						char * r1 = sttostr(styp, &d, _fmt, sbuf);
						SString & r2 = sttosstr(styp, &d, _fmt, temp_buf);
						SLCHECK_Z(isempty(sbuf));
						SLCHECK_Z(temp_buf.IsEmpty());
						SLCHECK_EQ(r1, &sbuf);
						SLCHECK_EQ(&r2, &temp_buf);
						SLCHECK_EQ(temp_buf, sbuf);
					}
					//
					//float  d_r;
					//SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					//SLCHECK_EQ(d, d_r);
				}
			}
		}
	}
	{  // SUInt
		constexpr uint _val_test_count = 1000;
		{ // size == 1
			constexpr uint _sz = 1;
			const  uint styp = MKSTYPE(S_UINT, _sz);
			uint64 min_val = 0;
			uint64 max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(static_cast<uint8>(min_val), static_cast<uint8>(0));
			SLCHECK_EQ(static_cast<uint8>(max_val), UINT8_MAX);
			SLCHECK_NZ(ismemzero(PTR8(&min_val) + _sz, sizeof(min_val) - _sz));
			SLCHECK_NZ(ismemzero(PTR8(&max_val) + _sz, sizeof(min_val) - _sz));
			for(int i = -127; i <= 127; i++) {
				const  uint8 d = static_cast<uint8>(i);
				uint8  d_r = 0;
				{
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
		{ // size == 2
			constexpr uint _sz = 2;
			const  uint styp = MKSTYPE(S_UINT, _sz);
			uint64 min_val = 0;
			uint64 max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(static_cast<uint16>(min_val), 0);
			SLCHECK_EQ(static_cast<uint16>(max_val), UINT16_MAX);
			SLCHECK_NZ(ismemzero(PTR8(&min_val) + _sz, sizeof(min_val) - _sz));
			SLCHECK_NZ(ismemzero(PTR8(&max_val) + _sz, sizeof(min_val) - _sz));
			{
				{
					uint16 d = 0;
					uint16 d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
				}
				for(uint i = 0; i < _val_test_count; i++) {
					uint16 d = static_cast<uint16>(SLS.GetTLA().Rg.GetUniformIntPos(static_cast<ulong>(max_val)));
					uint16 d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
		{ // size == 4
			constexpr uint _sz = 4;
			const  uint styp = MKSTYPE(S_UINT, _sz);
			uint64 min_val = 0;
			uint64 max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(static_cast<uint32>(min_val), 0U);
			SLCHECK_EQ(static_cast<uint32>(max_val), UINT32_MAX);
			SLCHECK_NZ(ismemzero(PTR8(&min_val) + _sz, sizeof(min_val) - _sz));
			SLCHECK_NZ(ismemzero(PTR8(&max_val) + _sz, sizeof(min_val) - _sz));
			{
				{
					ulong  d = 0;
					ulong  d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
				}
				for(uint i = 0; i < _val_test_count; i++) {
					ulong  d = static_cast<ulong>(SLS.GetTLA().Rg.GetUniformIntPos(static_cast<ulong>(max_val)));
					ulong  d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
		{ // size == 8
			constexpr uint _sz = 8;
			const  uint styp = MKSTYPE(S_UINT, _sz);
			uint64 min_val = 0;
			uint64 max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(min_val, 0ULL);
			SLCHECK_EQ(max_val, UINT64_MAX);
			{
				{
					uint64 d = 0ULL;
					uint64 d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
				}
				for(uint i = 0; i < _val_test_count; i++) {
					uint64 d1 = static_cast<int64>(SLS.GetTLA().Rg.GetUniformIntPos(UINT32_MAX));
					uint64 d2 = static_cast<int64>(SLS.GetTLA().Rg.GetUniformIntPos(UINT32_MAX));
					uint64 d = d1 + d2;
					uint64 d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
	}
	{ // SInt
		constexpr uint _val_test_count = 1000;
		{ // size == 1
			constexpr uint _sz = 1;
			const  uint styp = MKSTYPE(S_INT, _sz);
			int64  min_val = 0;
			int64  max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(static_cast<int8>(min_val), INT8_MIN);
			SLCHECK_EQ(static_cast<int8>(max_val), INT8_MAX);
			SLCHECK_NZ(ismemzero(PTR8(&min_val) + _sz, sizeof(min_val) - _sz));
			SLCHECK_NZ(ismemzero(PTR8(&max_val) + _sz, sizeof(min_val) - _sz));
			for(int i = -127; i <= 127; i++) {
				const  int8 d = static_cast<int8>(i);
				int8   d_r = 0;
				{
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
		{ // size == 2
			constexpr uint _sz = 2;
			const  uint styp = MKSTYPE(S_INT, _sz);
			int64  min_val = 0;
			int64  max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(static_cast<int16>(min_val), INT16_MIN);
			SLCHECK_EQ(static_cast<int16>(max_val), INT16_MAX);
			SLCHECK_NZ(ismemzero(PTR8(&min_val) + _sz, sizeof(min_val) - _sz));
			SLCHECK_NZ(ismemzero(PTR8(&max_val) + _sz, sizeof(min_val) - _sz));
			{
				{
					int16  d = 0;
					int16  d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
				}
				for(uint i = 0; i < _val_test_count; i++) {
					int16  d = static_cast<int16>(SLS.GetTLA().Rg.GetUniformIntPos(static_cast<ulong>(max_val)));
					int16  d_r;
					if(i & 0x01) {
						d = -d;
					}
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
		{ // size == 4
			constexpr uint _sz = 4;
			const  uint styp = MKSTYPE(S_INT, _sz);
			int64  min_val = 0;
			int64  max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(static_cast<int32_t>(min_val), INT32_MIN);
			SLCHECK_EQ(static_cast<int32_t>(max_val), INT32_MAX);
			SLCHECK_NZ(ismemzero(PTR8(&min_val) + _sz, sizeof(min_val) - _sz));
			SLCHECK_NZ(ismemzero(PTR8(&max_val) + _sz, sizeof(min_val) - _sz));
			{
				{
					long   d = 0;
					long   d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
				}
				for(uint i = 0; i < _val_test_count; i++) {
					long   d = static_cast<long>(SLS.GetTLA().Rg.GetUniformIntPos(static_cast<ulong>(max_val)));
					long   d_r;
					if(i & 0x01) {
						d = -d;
					}
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
		{ // size == 8
			constexpr uint _sz = 8;
			const  uint styp = MKSTYPE(S_INT, _sz);
			int64  min_val = 0;
			int64  max_val = 0;
			SLCHECK_NZ(stsize(styp) == _sz);
			SLCHECK_EQ(stbase(styp), BTS_INT);
			stminval(styp, &min_val);
			stmaxval(styp, &max_val);
			SLCHECK_EQ(min_val, INT64_MIN);
			SLCHECK_EQ(max_val, INT64_MAX);
			{
				{
					int64  d = 0LL;
					int64  d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
				}
				for(uint i = 0; i < _val_test_count; i++) {
					int64  d1 = static_cast<int64>(SLS.GetTLA().Rg.GetUniformIntPos(INT32_MAX));
					int64  d2 = static_cast<int64>(SLS.GetTLA().Rg.GetUniformIntPos(INT32_MAX));
					if(i & 0x01) {
						d1 = -d1;
					}
					int64  d = d1 + d2;
					int64  d_r;
					char * r1 = sttostr(styp, &d, 0, sbuf);
					SString & r2 = sttosstr(styp, &d, 0, temp_buf);
					SLCHECK_Z(isempty(sbuf));
					SLCHECK_Z(temp_buf.IsEmpty());
					SLCHECK_EQ(r1, &sbuf);
					SLCHECK_EQ(&r2, &temp_buf);
					SLCHECK_EQ(temp_buf, sbuf);
					//
					SLCHECK_NZ(stfromstr(styp, &d_r, 0, sbuf));
					SLCHECK_EQ(d, d_r);
					SLCHECK_Z(stcomp(styp, &d, &d_r));
				}
			}
		}
	}
	return CurrentStatus;
}
