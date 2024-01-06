// TEST-UED.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование технологии UED
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <ued.h>
#include <ued-id.h>
#if(_MSC_VER >= 1900)
	#include <cmath>
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
			if(false) { // @construction
				static const SColor color_list[] = {
					SClrAqua, SClrBlack, SClrWhite, SClrRed, SClrCyan
				};
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
				temp_buf.Z().CatEq("Distance", dist, MKSFMTD(0, 3, 0));
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
								temp_buf.Z().CatEq("Distance", dist, MKSFMTD(0, 3, 0));								
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
		return CurrentStatus;
	}

#endif // (_MSC_VER >= 1900)