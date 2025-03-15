// TEST-UED.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2025
// @codepage UTF-8
// Тестирование технологии UED
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>

uint64 UedEncodeRange(uint64 upp, uint granulation, uint bits, double value);
bool UedDecodeRange(uint64 v, uint64 upp, uint granulation, uint bits, double * pResult);

#include <unicode\uclean.h>
#include <unicode\brkiter.h>
#include <unicode\measunit.h>
#include <unicode\measfmt.h>
#include <unicode\unumberformatter.h>

using namespace U_ICU_NAMESPACE;

SLTEST_R(UED)
{
	int    ok = 1;
	SString temp_buf;
	SStringU temp_buf_u;
	SString path_test_root;
	SLS.QueryPath("testroot", path_test_root);
	{
		// UedEncodeRange/UedDecodeRange
		const char * p_max_err_file_name = MakeOutputFilePath("ued_rangeenc_max_err.log");
		SFile f_max_err(p_max_err_file_name, SFile::mWrite);
		{
			SString max_err_log_buf;
			struct BitsEntry {
				uint   Bits; // Количество бит для представления числа
				double Tolerance; // Минимальная точность (для проверки результата)
			};
			static const BitsEntry bits2_list[] = { {64, 1E-9}, {56, 1E-9}, {48, 1E-9}, {33, 1E-5}, {32, 1E-5}, {28, 1E-3}, {24, 1E-2} };
			//static const uint bits_list[] = { 64, 56, 48, 33, 32, 24 };
			static const uint64 upp_list[] = {1ULL, 2ULL, 180ULL, 360ULL, 255ULL};
			static const double inc_list[] = {1.0/7200.0, 0.5, 0.125, 0.3, 1.0/3.0, 0.001};
			static const uint gran_list[] = { /*5,*/3600, 36000 };
			for(uint bitsidx = 0; bitsidx < SIZEOFARRAY(bits2_list); bitsidx++) {
				const uint bits = bits2_list[bitsidx].Bits;
				const double tol = bits2_list[bitsidx].Tolerance;//(bits <= 24) ? 1E-2 : ((bits <= 33) ? 1E-5 : 1E-9);
				for(uint uppidx = 0; uppidx < SIZEOFARRAY(upp_list); uppidx++) {
					const uint64 upp = upp_list[uppidx];
					for(uint incidx = 0; incidx < SIZEOFARRAY(inc_list); incidx++) {
						const double inc = inc_list[incidx];
						for(uint granidx = 0; granidx < SIZEOFARRAY(gran_list); granidx++) {
							const uint granularity = gran_list[granidx];
							double _max_err = 0.0;
							for(double i = 0; i <= (double)upp; i += inc) {
								uint64 u = UedEncodeRange(upp, granularity, bits, i);
								double rv;
								UedDecodeRange(u, upp, granularity, bits, &rv);
								const double _err = fabs(rv - i);
								SETMAX(_max_err, _err);
								const double _frac = ffrac(i);
								if(_frac == 0.0 || _frac == 0.5 || _frac == 0.25) {
									if(false && !SLCHECK_EQ(rv, i)) {
										ok = 0;
									}
								}
								else {
									if(!SLCHECK_EQ_TOL(rv, i, tol)) {
										ok = 0;
									}
								}
							}
							max_err_log_buf.Z().CatEq("bits", bits).Space().CatEq("upp", upp).Space().
								CatEq("increment", inc, MKSFMTD(0, 6, 0)).Space().
								CatEq("granularity", granularity).CatDiv(':', 2);
							if(_max_err == 0.0)
								max_err_log_buf.Cat("zero");
							else
								slprintf_cat(max_err_log_buf, "%.8E", _max_err);
							f_max_err.WriteLine(max_err_log_buf.CR());
						}
					}
				}
			}
		}
	}
	{ /* @construction*/ 
		const SphericalDirection test_val_list[] = {
			{ 0.0, 0.0 }, // up
			{ 90.0, 0.0 }, // right
			{ 90.0, 180.0 }, // left
			{ 180.0, 0.0 }, // down
			{ 90.0, 90.0 }, // forward
			{ 90.0, 270.0 }, // backward
			{ 73.13, 101.58 }, 
			{ 73.139, 101.583 },
			{ 14.0 + 5.0/60.0 + 51.0/3600.0, 103 + 59.0/60.0 + 8.0/3600.0},
		};
		for(uint i = 0; i < SIZEOFARRAY(test_val_list); i++) {
			SphericalDirection sd(test_val_list[i]); 
			uint64 ued_ = UED::SetRaw_SphDir(sd);
			SLCHECK_NZ(ued_);
			SLCHECK_NZ(UED::BelongToMeta(ued_, UED_META_SPHERDIR));
			SphericalDirection sd_;
			SLCHECK_NZ(UED::GetRaw_SphDir(ued_, sd_));
			SLCHECK_NZ(sd_.IsEqTol(sd, 1e-5));
		}
	}/**/
	{
		if(true) { // @construction
			static const SColor color_list[] = { SClrAqua, SClrBlack, SClrWhite, SClrRed, SClrCyan };
			static const uint alpha_list[] = { 2, 17, 120, 255 };
			for(uint aidx = 0; aidx < SIZEOFARRAY(alpha_list); aidx++) {
				for(uint cidx = 0; cidx < SIZEOFARRAY(color_list); cidx++) {
					SColor c(color_list[cidx]);
					c.SetAlpha(alpha_list[aidx]);
					uint64 ued = UED::SetRaw_Color(c);
					SColor c2;
					SLCHECK_NZ(UED::GetRaw_Color(ued, c2));
					SLCHECK_EQ(c, c2);
				}
			}
		}
	}
	{
		static const uint64 ued_time_meta_list[] = {
			UED_META_TIME_MSEC, UED_META_TIME_SEC, UED_META_TIME_MIN, 
			UED_META_TIME_HR, UED_META_DATE_DAY, UED_META_DATE_MON, UED_META_DATE_YR
		};
		bool debug_mark = false;
		for(uint mi = 0; mi < SIZEOFARRAY(ued_time_meta_list); mi++) {
			const uint64 meta = ued_time_meta_list[mi];
			for(uint i = 0; i < 10000; i++) {
				SUniTime_Internal ut(SCtrGenerate_);
				uint64 ued = UED::_SetRaw_Time(meta, ut);
				SLCHECK_NZ(ued);
				SLCHECK_NZ(UED::BelongToMeta(ued, meta));
				SUniTime_Internal ut2;
				UED::_GetRaw_Time(ued, ut2);
				if(ut2.Cmp(ut, meta) != UED_CMP_EQUAL) {
					debug_mark = true;
				}
				SLCHECK_NZ(ut2.Cmp(ut, meta) == UED_CMP_EQUAL);
			}
		}
	}
	{
		SGeo geo;
		{
			SGeoPosLL gp(40.67241045687091, -74.24130029528962);
			uint64 ued_gp = UED::SetRaw_GeoLoc(gp);
			double dist = 0.0;
			SGeoPosLL gp_;
			SLCHECK_NZ(UED::GetRaw_GeoLoc(ued_gp, gp_));
			SLCHECK_EQ_TOL(gp_.Lat, gp.Lat, 1e-4);
			SLCHECK_EQ_TOL(gp_.Lon, gp.Lon, 1e-4);
			geo.Inverse(gp, gp_, &dist, 0, 0, 0, 0, 0, 0);
			temp_buf.Z().CatEq("Distance", dist, MKSFMTD_030);
			SetInfo(temp_buf);
		}
		{
			//C:\Papyrus\Src\PPTEST\DATA\cities\cities.csv 
			SString in_file_name;
			SString line_buf;
			(in_file_name = path_test_root).SetLastSlash().Cat("data").SetLastSlash().Cat("cities").SetLastSlash().Cat("cities.csv");
			SFile f_in(in_file_name, SFile::mRead);
			SFile::ReadLineCsvContext csv_ctx(',');
			StringSet ss_rec;
			if(f_in.IsValid()) {
				uint rec_no = 0;
				while(f_in.ReadLineCsv(csv_ctx, ss_rec)) {
					rec_no++;
					if(rec_no > 1) {
						uint fld_no = 0;
						// id,name,state_id,state_code,state_name,country_id,country_code,country_name,latitude,longitude,wikiDataId
						double lat = 0.0;
						double lon = 0.0;
						for(uint ssp = 0; ss_rec.get(&ssp, temp_buf);) {
							fld_no++;
							if(fld_no == 9) {
								lat = temp_buf.ToReal_Plain();
							}
							else if(fld_no == 10) {
								lon = temp_buf.ToReal_Plain();
							}
						}
						SGeoPosLL gp(lat, lon);
						if(gp.IsValid()) {
							uint64 ued_gp = UED::SetRaw_GeoLoc(gp);
							double dist = 0.0;
							SGeoPosLL gp_;
							SLCHECK_NZ(UED::GetRaw_GeoLoc(ued_gp, gp_));
							SLCHECK_EQ_TOL(gp_.Lat, gp.Lat, 1e-5);
							SLCHECK_EQ_TOL(gp_.Lon, gp.Lon, 1e-5);
							geo.Inverse(gp, gp_, &dist, 0, 0, 0, 0, 0, 0);
							temp_buf.Z().CatEq("Distance", dist, MKSFMTD_030);								
						}
					}
				}
			}
		}
	}
	{
		const double angle_list[] = { -11.9, -45.0, -180.1, 10.5, 0.0, 30.0, 45.0, 60.0, 180.0, 10.0, 270.25, 359.9 };
		for(uint i = 0; i < SIZEOFARRAY(angle_list); i++) {
			uint64 ued_a = UED::SetRaw_PlanarAngleDeg(angle_list[i]);
			double angle_;
			UED::GetRaw_PlanarAngleDeg(ued_a, angle_);
			SLCHECK_NZ(feqeps(angle_, angle_list[i], 1E-6));
		}
		{
			const double src_angle = SMathConst::Pi / 4;
			uint64 ued_a = UED::SetRaw_PlanarAngleDeg(src_angle * 180.0/SMathConst::Pi);
			double angle_;
			UED::GetRaw_PlanarAngleDeg(ued_a, angle_);
			SLCHECK_NZ(feqeps(angle_, 45.0, 1E-6));
		}
		{
			const double src_angle = SMathConst::Pi / 6;
			uint64 ued_a = UED::SetRaw_PlanarAngleDeg(src_angle * 180.0/SMathConst::Pi);
			double angle_;
			UED::GetRaw_PlanarAngleDeg(ued_a, angle_);
			SLCHECK_NZ(feqeps(angle_, 30.0, 1E-6));
		}
	}
	{
		UErrorCode icu_st = U_ZERO_ERROR;
		/*{
			PPGetPath(PPPATH_BIN, temp_buf);
			u_setDataDirectory(temp_buf);
			u_init(&icu_st);
		}*/
		//
		MeasureUnit avl_units[2048];
		icu_st = U_ZERO_ERROR;
		Locale lcl("ru_RU");
		MeasureFormat mf(lcl, UMEASFMT_WIDTH_WIDE, icu_st);
		int avl_count = MeasureUnit::getAvailable(avl_units, SIZEOFARRAY(avl_units), icu_st);
		//UnicodeString measure_data_buf_list[24]; // really 11 needed
		//SStringU measure_text_list_u[24];
		//MeasureUnit mu("kilogram");
		for(int i = 0; i < avl_count; i++) {
			UnicodeString udn = mf.getUnitDisplayName(avl_units[i], icu_st);
			//getMeasureData(lcl, avl_units[i], UNUM_UNIT_WIDTH_FULL_NAME, ""/*unitDisplayCase*/, measure_data_buf_list, icu_st);
			//for(uint j = 0; j < 11; j++) {
			//	measure_text_list_u[j].Z().CatN(reinterpret_cast<const wchar_t *>(measure_data_buf_list[j].getBuffer()), measure_data_buf_list[j].length());
			//}
			temp_buf_u.Z().CatN(reinterpret_cast<const wchar_t *>(udn.getBuffer()), udn.length());
		}
	}
	{
		const SrUedContainer_Rt * p_uedc = DS.GetUedContainer();
		SLCHECK_NZ(p_uedc);
		if(p_uedc) {
			uint64 ued;
			SColor clr;
			SLCHECK_NZ(UED::GetRaw_Color(p_uedc->SearchSymb("Azure", UED_META_COLORRGB), clr));
			SLCHECK_EQ(clr, SColor(SClrAzure));
			SLCHECK_NZ(UED::GetRaw_Color(p_uedc->SearchSymb("lightstEElblue", UED_META_COLORRGB), clr));
			SLCHECK_EQ(clr, SColor(SClrLightsteelblue));
			SLCHECK_NZ(UED::GetRaw_Color(p_uedc->SearchSymb("mediumvioletreD", UED_META_COLORRGB), clr));
			SLCHECK_EQ(clr, SColor(SClrMediumvioletred));
			//
			ued = UED::SetRaw_Color(SClrDarkgoldenrod);
			p_uedc->GetSymb(ued, temp_buf);
			SLCHECK_NZ(temp_buf.IsEqiUtf8("DarkGoldenRod"));
		}
	}
	{
		struct TextToUedAssoc {
			const char * P_Text;
			uint64 Ued;
			uint64 ImplicitMeta;
		};
		static const TextToUedAssoc test_true_list[] = {
			{ "#statu.ru", UED_STATU_RU, 0ULL },
			{ "  #lingua.fr", UED_LINGUA_FR, UED_META_LINGUA },
			{ "#fr", UED_LINGUA_FR, UED_META_LINGUA },
			{ "\t#meta.uom  ", UED_META_UOM, 0ULL },
			{ "#uom.candela", UED_UOM_CANDELA, 0ULL },
			{ "#fermi", UED_UOM_FERMI, UED_META_UOM },
			{ "#local_app_data", UED_FSKNOWNFOLDER_LOCAL_APP_DATA, UED_META_FSKNOWNFOLDER },
		};
		const SrUedContainer_Rt * p_uedc = DS.GetUedContainer();
		SLCHECK_NZ(p_uedc);
		if(p_uedc) {
			SStrScan scan;
			for(uint i = 0; i < SIZEOFARRAY(test_true_list); i++) {
				const TextToUedAssoc & r_item = test_true_list[i];
				scan.Set(r_item.P_Text, 0);
				uint64 ued = p_uedc->Recognize(scan, r_item.ImplicitMeta, SrUedContainer_Base::rfPrefixSharp);
				SLCHECK_EQ(ued, r_item.Ued);
			}
		}
	}
	{
		SString src_file_name;
		SString out_path;
		SString c_path;
		SString java_path;
		SString rt_out_path;
		uint   flags = 0; // prcssuedfForceUpdatePlDecl, prcssuedfTolerant

		(src_file_name = path_test_root).SetLastSlash().Cat("data").SetLastSlash().Cat("ued").SetLastSlash().Cat("ued-test.txt");
		(out_path = path_test_root).SetLastSlash().Cat("out").SetLastSlash().Cat("ued");
		SFile::CreateDir(out_path);
		c_path = out_path;
		java_path = out_path;
		PPLogger logger(PPLogger::fStdErr);
		int    r = ProcessUed(src_file_name, out_path, rt_out_path, c_path, java_path, flags, &logger);
	}
	{
		//static uint64 SetRaw_Oid(SObjID oid);
		//static bool   GetRaw_Oid(uint64 ued, SObjID & rOid);
		static const SObjID oid_list[] = {
			{ PPOBJ_GOODS, 0 },
			{ PPOBJ_GOODS, 1 },
			{ PPOBJ_GOODS, 0x7ffffffe },
			{ PPOBJ_PERSON, 0 },
			{ PPOBJ_PERSON, 1 },
			{ PPOBJ_PERSON, 0x7ffffffe },
			{ PPOBJ_LOCATION, 0 },
			{ PPOBJ_LOCATION, 1 },
			{ PPOBJ_LOCATION, 0x7ffffffe },
			{ PPOBJ_BILL, 0 },
			{ PPOBJ_BILL, 1 },
			{ PPOBJ_BILL, 0x7ffffffe },
			{ PPOBJ_LOT, 0 },
			{ PPOBJ_LOT, 1 },
			{ PPOBJ_LOT, 0x7ffffffe },
		};
		for(uint i = 0; i < SIZEOFARRAY(oid_list); i++) {
			const SObjID & r_oid_pattern = oid_list[i];
			SObjID oid;
			const  uint64 ued = UED::SetRaw_Oid(r_oid_pattern);
			SLCHECK_NZ(ued);
			{
				uint64 meta = 0;
				switch(r_oid_pattern.Obj) {
					case PPOBJ_GOODS: meta = UED_META_PRV_WARE; break;
					case PPOBJ_PERSON: meta = UED_META_PRV_PERSON; break;
					case PPOBJ_LOCATION: meta = UED_META_PRV_LOCATION; break;
					case PPOBJ_BILL: meta = UED_META_PRV_DOC; break;
					case PPOBJ_LOT: meta = UED_META_PRV_LOT; break;
				}
				if(meta) {
					SLCHECK_EQ(UED::GetMeta(ued), meta);
				}
			}
			SLCHECK_NZ(UED::GetRaw_Oid(ued, oid));
			SLCHECK_EQ(oid, r_oid_pattern);
		}
	}
	return CurrentStatus;
}
