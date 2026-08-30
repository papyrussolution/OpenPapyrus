// TEST-IMAGE.CPP
// Copyright (c) A.Sobolev 2023, 2024, 2026
// @codepage UTF-8
// Тесты работы с изображениями
//
#include <pp.h>
#pragma hdrstop

int ZXing_CreateBarcodeImage(const PPBarcode::BarcodeImageParam & rParam); // @prototype(zxing_client.cpp)

static const SIntToSymbTabEntry TestBcStdSymbList[] = {
	{  BARCSTD_CODE11,      "code11"  },
	{  BARCSTD_CODE11,      "code-11" },
	{  BARCSTD_CODE11,      "code 11" },
	{  BARCSTD_CODE39,      "code39"  },
	{  BARCSTD_CODE39,      "code-39" },
	{  BARCSTD_CODE39,      "code 39" },
	{  BARCSTD_CODE49,      "code49"  },
	{  BARCSTD_CODE49,      "code-49" },
	{  BARCSTD_CODE49,      "code 49" },
	{  BARCSTD_CODE93,      "code93"  },
	{  BARCSTD_CODE93,      "code-93" },
	{  BARCSTD_CODE93,      "code 93" },
	{  BARCSTD_CODE128,     "code128" },
	{  BARCSTD_CODE128,     "code-128" },
	{  BARCSTD_CODE128,     "code 128" },
	{  BARCSTD_PDF417,      "pdf417"   },
	{  BARCSTD_PDF417,      "pdf-417"  },
	{  BARCSTD_PDF417,      "pdf 417"  },
	{  BARCSTD_PDF417,      "pdf"      },
	{  BARCSTD_EAN13,       "ean13"    },
	{  BARCSTD_EAN13,       "ean-13"   },
	{  BARCSTD_EAN13,       "ean 13"   },
	{  BARCSTD_EAN8,        "ean8"     },
	{  BARCSTD_EAN8,        "ean-8" },
	{  BARCSTD_EAN8,        "ean 8" },
	{  BARCSTD_UPCA,        "upca"  },
	{  BARCSTD_UPCA,        "upc-a" },
	{  BARCSTD_UPCA,        "upc a" },
	{  BARCSTD_UPCE,        "upce"  },
	{  BARCSTD_UPCE,        "upc-e" },
	{  BARCSTD_UPCE,        "upc e" },
	{  BARCSTD_QR,          "qr"    },
	{  BARCSTD_QR,          "qr-code"  },
	{  BARCSTD_QR,          "qr code"  },
	{  BARCSTD_QR,          "qrcode"   },
	{  BARCSTD_INTRLVD2OF5, "interleaved2of5" },
	{  BARCSTD_IND2OF5,     "industial2of5"   },
	{  BARCSTD_STD2OF5,     "standard2of5"    },
	{  BARCSTD_ANSI,        "codabar"         },
	{  BARCSTD_MSI,         "msi"             },
	{  BARCSTD_PLESSEY,     "plessey"    },
	{  BARCSTD_POSTNET,     "postnet"    },
	{  BARCSTD_LOGMARS,     "logmars"    },
	{  BARCSTD_DATAMATRIX,  "datamatrix" },
	{  BARCSTD_AZTEC,       "aztec" }, // @v11.9.2
	{  BARCSTD_DATABAR,     "databar" }, // @v11.9.2
	{  BARCSTD_MICROQR,     "microqr" }, // @v11.9.2
	{  BARCSTD_ITF,         "itf" }, // @v11.9.2
	{  BARCSTD_MAXICODE,    "maxicode" }, // @v11.9.2
	{  BARCSTD_RMQR,        "rmqr" }, // @v11.9.2
};

SLTEST_R(BarcodeOutputAndRecognition)
{
	SString temp_buf;
	{
		for(uint i = 0; i < SIZEOFARRAY(TestBcStdSymbList); i++) {
			const SIntToSymbTabEntry & r_entry = TestBcStdSymbList[i];
			(temp_buf = r_entry.P_Symb).ToUpper();
			int std = PPBarcode::RecognizeStdName(temp_buf);
			SLCHECK_EQ((long)std, (long)r_entry.Id);
			SLCHECK_NZ(PPBarcode::GetStdName(std, temp_buf));
			int _found = 0;
			for(uint j = 0; !_found && j < SIZEOFARRAY(TestBcStdSymbList); j++) {
				const SIntToSymbTabEntry & r_entry2 = TestBcStdSymbList[j];
				if(r_entry2.Id == std && temp_buf.CmpNC(r_entry2.P_Symb) == 0)
					_found = 1;
			}
			SLCHECK_NZ(_found);
		}
	}
	SString line_buf;
	SString code_buf;
	SString input_file_path;
	//SString img_path;
	{
		// D:\Papyrus\Src\PPTEST\DATA\chzn-sigblk-20230906_144733.jpg 
		(input_file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("chzn-sigblk-20230906_144733.jpg");
		TSCollection <PPBarcode::Entry> bc_list;
		SLCHECK_LT(0, PPBarcode::ZXing_RecognizeImage(input_file_path, bc_list));
	}
	(input_file_path = GetSuiteEntry()->InPath).SetLastSlash().Cat("barcode.txt");
    SFile f_in(input_file_path, SFile::mRead);
    while(f_in.ReadLine(line_buf, SFile::rlfChomp|SFile::rlfStrip)) {
		if(line_buf.Divide(':', temp_buf, code_buf) > 0) {
			PPBarcode::BarcodeImageParam bip;
			bip.Std = PPBarcode::RecognizeStdName(temp_buf.Strip());
			if(bip.Std) {
				code_buf.Strip();
				{
					temp_buf = code_buf;
					if(oneof2(bip.Std, BARCSTD_EAN13, BARCSTD_EAN8))
						temp_buf.TrimRight();
					if(bip.Std == BARCSTD_UPCE) {
						while(temp_buf.C(0) == '0')
							temp_buf.ShiftLeft();
						temp_buf.TrimRight();
					}
					bip.Code = temp_buf;
				}
				{
					//
					// PNG
					//
					bip.OutputFormat = SFileFormat::Png;
					(bip.OutputFileName = GetSuiteEntry()->OutPath).SetLastSlash().Cat(code_buf).DotCat("png");
					if(SLCHECK_NZ(PPBarcode::CreateImage(bip))) {
						TSCollection <PPBarcode::Entry> bc_list;
						//
						TSCollection <PPBarcode::Entry> bc_list2;
						if(SLCHECK_LT(0, PPBarcode::ZXing_RecognizeImage(bip.OutputFileName, bc_list2))) {
							SLCHECK_EQ(bc_list2.getCount(), 1U);
							if(bc_list2.getCount()) {
								const PPBarcode::Entry * p_entry = bc_list2.at(0);
								SLCHECK_EQ(p_entry->BcStd, bip.Std);
								SLCHECK_EQ(p_entry->Code, code_buf);
							}
						}
						//
						if(SLCHECK_LT(0, PPBarcode::RecognizeImage(bip.OutputFileName, bc_list))) {
							SLCHECK_EQ(bc_list.getCount(), 1U);
							if(bc_list.getCount()) {
								const PPBarcode::Entry * p_entry = bc_list.at(0);
								SLCHECK_EQ(p_entry->BcStd, bip.Std);
								SLCHECK_EQ(p_entry->Code, code_buf);
							}
						}
					}
				}
				/* { // @v11.9.2 @construction
					//
					// PNG ZXing_CreateBarcodeImage
					//
					bip.OutputFormat = SFileFormat::Png;
					(bip.OutputFileName = GetSuiteEntry()->OutPath).SetLastSlash().Cat(code_buf).CatChar('-').Cat("zxing").DotCat("png");
					if(SLCHECK_NZ(ZXing_CreateBarcodeImage(bip))) {
						TSCollection <PPBarcode::Entry> bc_list;
						//
						TSCollection <PPBarcode::Entry> bc_list2;
						if(SLCHECK_LT(0, PPBarcode::ZXing_RecognizeImage(bip.OutputFileName, bc_list2))) {
							SLCHECK_LE(1U, bc_list2.getCount());
							if(bc_list2.getCount()) {
								for(uint bcidx = 0; bcidx < bc_list2.getCount(); bcidx++) {
									const PPBarcode::Entry * p_entry = bc_list2.at(bcidx);
									SLCHECK_EQ(p_entry->BcStd, bip.Std);
									SLCHECK_EQ(p_entry->Code, code_buf);
								}
							}
						}
						//
						if(SLCHECK_LT(0, PPBarcode::RecognizeImage(bip.OutputFileName, bc_list))) {
							SLCHECK_EQ(bc_list.getCount(), 1U);
							if(bc_list.getCount()) {
								const PPBarcode::Entry * p_entry = bc_list.at(0);
								SLCHECK_EQ(p_entry->BcStd, bip.Std);
								SLCHECK_EQ(p_entry->Code, code_buf);
							}
						}
					}
				}*/
				{
					//
					// SVG
					//
					bip.OutputFormat = SFileFormat::Svg;
					(bip.OutputFileName = GetSuiteEntry()->OutPath).SetLastSlash().Cat(code_buf).DotCat("svg");
					if(SLCHECK_NZ(PPBarcode::CreateImage(bip))) {
						TSCollection <PPBarcode::Entry> bc_list;
						//
						TSCollection <PPBarcode::Entry> bc_list2;
						if(SLCHECK_LT(0, PPBarcode::ZXing_RecognizeImage(bip.OutputFileName, bc_list2))) {
							SLCHECK_EQ(bc_list2.getCount(), 1U);
							if(bc_list2.getCount()) {
								const PPBarcode::Entry * p_entry = bc_list2.at(0);
								SLCHECK_EQ(p_entry->BcStd, bip.Std);
								SLCHECK_EQ(p_entry->Code, code_buf);
							}
						}
						//
						if(SLCHECK_LT(0, PPBarcode::RecognizeImage(bip.OutputFileName, bc_list))) {
							SLCHECK_EQ(bc_list.getCount(), 1U);
							if(bc_list.getCount()) {
								const PPBarcode::Entry * p_entry = bc_list.at(0);
								SLCHECK_EQ(p_entry->BcStd, bip.Std);
								SLCHECK_EQ(p_entry->Code, code_buf);
							}
						}
					}
				}
			}
		}
    }
	return CurrentStatus;
}
//
//
//
int Test_LCMS2(const char * pTestBedPath, const char * pOutputFileName, bool exhaustive); // (Экспериментальное внедрение тестирования библиотеки lcms2) 

SLTEST_R(lcms2)
{
#if _MSC_VER >= 1910
	SString temp_buf;
	SString testbed_path;
	(testbed_path = GetInputPath()).SetLastSlash().Cat("lcms2");
	(temp_buf = "lcms2").CatChar('-').Cat("test").CatChar('-').Cat(getcurdate_(), DATF_YMD|DATF_NODIV|DATF_CENTURY).Cat(getcurtime_(), TIMF_HMS|TIMF_NODIV).DotCat("out");
	const char * p_out_file_name = MakeOutputFilePath(temp_buf);
	SLCHECK_Z(Test_LCMS2(testbed_path, p_out_file_name, true));	
#else
	;
#endif
	return CurrentStatus;
}

SLTEST_R(SDraw)
{
	int    ok = 1;
	SString temp_buf;
	SImageBuffer img_buf;
	SString input_file_name(MakeInputFilePath("test24.png"));
	THROW(SLCHECK_NZ(img_buf.Load(input_file_name)));
	{
		SFsPath ps(input_file_name);
		ps.Ext = "webp";
		ps.Merge(SFsPath::fNam|SFsPath::fExt, temp_buf);
		SImageBuffer::StoreParam sp(SFileFormat::Webp);
		SFile out_file(MakeOutputFilePath(temp_buf), SFile::mWrite|SFile::mBinary);
		THROW(out_file.IsValid());
		THROW(SLCHECK_NZ(img_buf.Store(sp, out_file)));
	}
	{
		SBuffer buf;
		SFile out_file(buf, SFile::mWrite);
		SImageBuffer::StoreParam sp(SFileFormat::Png);
		THROW(SLCHECK_NZ(img_buf.Store(sp, out_file)));
		{
			SImageBuffer img_buf2;
			SBuffer * p_buf = out_file;
			THROW(SLCHECK_NZ(p_buf));
			THROW(SLCHECK_NZ(img_buf2.Load(SFileFormat::Png, *p_buf)));
			THROW(SLCHECK_NZ(img_buf2.IsEq(img_buf)));
		}
		{
			THROW(SLCHECK_NZ(img_buf.Load(MakeInputFilePath("test10.jpg"))));
			SImageBuffer::StoreParam sp_jpeg(SFileFormat::Jpeg);
			sp_jpeg.Quality = 50;
			SFile out_file_jpeg(MakeInputFilePath("test10-out.jpg"), SFile::mWrite|SFile::mBinary);
			THROW(SLCHECK_NZ(img_buf.Store(sp_jpeg, out_file_jpeg)))
		}
	}
	CATCHZOK
	return CurrentStatus;
}