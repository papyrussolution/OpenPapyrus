// TEST-UED.CPP
// Copyright (c) A.Sobolev 2023
// @codepage UTF-8
// Тестирование технологии UED
//
#include <pp.h>
#pragma hdrstop
#include <sartre.h>
#include <ued.h>
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
		{
			SGeoPosLL gp(40.67241045687091, -74.24130029528962);
			uint64 ued_gp = UED::ConvertGeoLoc(gp);
			SGeoPosLL gp_;
			UED::StraightenGeoLoc(ued_gp, gp_);
		}
		{
			const double angle_list[] = { -11.9, -45.0, -180.1, 10.5, 0.0, 30.0, 45.0, 60.0, 180.0, 10.0, 270.25, 359.9 };
			for(uint i = 0; i < SIZEOFARRAY(angle_list); i++) {
				uint64 ued_a = UED::ConvertPlanarAngle_Deg(angle_list[i]);
				double angle_;
				UED::StraightenPlanarAngle_Deg(ued_a, angle_);
				SLCHECK_NZ(feqeps(angle_, angle_list[i], 1E-6));
			}
			{
				const double src_angle = SMathConst::Pi / 4;
				uint64 ued_a = UED::ConvertPlanarAngle_Deg(src_angle * 180.0/SMathConst::Pi);
				double angle_;
				UED::StraightenPlanarAngle_Deg(ued_a, angle_);
				SLCHECK_NZ(feqeps(angle_, 45.0, 1E-6));
			}
			{
				const double src_angle = SMathConst::Pi / 6;
				uint64 ued_a = UED::ConvertPlanarAngle_Deg(src_angle * 180.0/SMathConst::Pi);
				double angle_;
				UED::StraightenPlanarAngle_Deg(ued_a, angle_);
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
		return CurrentStatus;
	}

#endif // (_MSC_VER >= 1900)