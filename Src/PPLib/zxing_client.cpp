// zxing_client.cpp
// Copyright (c) A.Sobolev 2023, 2024
// @construction
//
#include <pp.h>
#pragma hdrstop
#include <..\OSF\zxing-cpp\core\src\zxing-internal.h>

using namespace ZXing;


//PPBarcode::BarcodeImageParam

int ZXing_CreateBarcodeImage(const PPBarcode::BarcodeImageParam & rParam)
{
	int    ok = -1;
	if(rParam.Code.NotEmpty() && oneof4(rParam.OutputFormat, SFileFormat::Png, SFileFormat::Jpeg, SFileFormat::Webp, SFileFormat::Bmp)) {
		try {
			int    width = (rParam.Size.x > 0) ? rParam.Size.x : 128;
			int    height = (rParam.Size.y > 0) ? rParam.Size.y : 128;
			int    ecc_level = -1;
			CharacterSet encoding = CharacterSet::Unknown;
			BarcodeFormat format = ZXing::BarcodeFormat::None;
			switch(rParam.Std) {
				case BARCSTD_AZTEC: format = ZXing::BarcodeFormat::Aztec; break;
				case BARCSTD_ANSI: format = ZXing::BarcodeFormat::Codabar; break;
				case BARCSTD_CODE39: format = ZXing::BarcodeFormat::Code39; break;
				case BARCSTD_CODE93: format = ZXing::BarcodeFormat::Code93; break;
				case BARCSTD_CODE128: format = ZXing::BarcodeFormat::Code128; break;
				case BARCSTD_DATABAR: format = ZXing::BarcodeFormat::DataBar; break;
				//case BARCSTD_DATABAR: format = ZXing::BarcodeFormat::DataBarExpanded; break; // @?
				case BARCSTD_DATAMATRIX: format = ZXing::BarcodeFormat::DataMatrix; break;
				case BARCSTD_EAN8: format = ZXing::BarcodeFormat::EAN8; break;
				case BARCSTD_EAN13: format = ZXing::BarcodeFormat::EAN13; break;
				case BARCSTD_ITF: format = ZXing::BarcodeFormat::ITF; break;
				case BARCSTD_MAXICODE: format = ZXing::BarcodeFormat::MaxiCode; break;
				case BARCSTD_PDF417: format = ZXing::BarcodeFormat::PDF417; break;
				case BARCSTD_QR: format = ZXing::BarcodeFormat::QRCode; break;
				case BARCSTD_UPCA: format = ZXing::BarcodeFormat::UPCA; break;
				case BARCSTD_UPCE: format = ZXing::BarcodeFormat::UPCE; break;
				case BARCSTD_MICROQR: format = ZXing::BarcodeFormat::MicroQRCode; break;
				case BARCSTD_RMQR: format = ZXing::BarcodeFormat::RMQRCode; break;
			}
			if(format != ZXing::BarcodeFormat::None) {
				MultiFormatWriter writer = MultiFormatWriter(format).setMargin(rParam.Margin).setEncoding(encoding).setEccLevel(ecc_level);
				std::string _code(rParam.Code);
				BitMatrix matrix = writer.encode(_code, width, height);
				Matrix <uint8_t> bitmap = ToMatrix<uint8_t>(matrix);
				//auto ext = GetExtension(filePath);
				int success = 0;
				SImageBuffer ib;
				const SImageBuffer::PixF f(SImageBuffer::PixF::s8GrayScale);
				ib.Init(bitmap.width(), 0);
				ib.AddLines(bitmap.data(), f, bitmap.height(), 0);
				{
					SString file_name(rParam.OutputFileName);
					SString fn_ext;
					SFileFormat::GetExt(rParam.OutputFormat, fn_ext);
					SFsPath::ReplaceExt(file_name, fn_ext, 0);
					SFile f_out(file_name, SFile::mWrite|SFile::mBinary);
					if(f_out.IsValid()) {
						if(ib.Store(SImageBuffer::StoreParam(rParam.OutputFormat), f_out)) {
							ok = 1;
						}
					}
				}
				/*
				if(ext == "" || ext == "png") {
					success = stbi_write_png(filePath.c_str(), bitmap.width(), bitmap.height(), 1, bitmap.data(), 0);
				}
				else if(ext == "jpg" || ext == "jpeg") {
					success = stbi_write_jpg(filePath.c_str(), bitmap.width(), bitmap.height(), 1, bitmap.data(), 0);
				}
				else if(ext == "svg") {
					success = (std::ofstream(filePath) << ToSVG(matrix)).good();
				}
				if(!success) {
					std::cerr << "Failed to write image: " << filePath << std::endl;
					return -1;
				}
				*/
			}
		} 
		catch(const std::exception& e) {
			std::cerr << e.what() << std::endl;
			ok = 0;
		}
	}
	return ok;
}

/*static*/int PPBarcode::ZXing_RecognizeImage(const char * pInpFileName, TSCollection <PPBarcode::Entry> & rList)
{
	rList.clear();
	int    ok = -1;
	SDrawFigure * p_fig = 0;
	const SImageBuffer * p_ib = 0;
	SImageBuffer ib__;
	THROW_SL(p_fig = SDrawFigure::CreateFromFile(pInpFileName, 0));
	if(p_fig->GetKind() == SDrawFigure::kImage) {
		p_ib = &static_cast<const SDrawImage *>(p_fig)->GetBuffer();
	}
	else if(p_fig->GetKind()) { // Вероятно, векторная фигура
		SPoint2F sz = p_fig->GetSize();
		if(sz.x <= 0.0f || sz.y <= 0.0f) {
			sz.Set(300.0f);
		}
		else {
			sz.x *= 2.0f;
			sz.y *= 2.0f;
		}
		THROW_SL(ib__.Init((uint)sz.x, (uint)sz.y));
		THROW_SL(p_fig->TransformToImage(0, ib__));
		p_ib = &ib__;
	}
	if(p_ib) {
    	const uint src_width = p_ib->GetWidth();
    	const uint src_height = p_ib->GetHeight();
		//uint8 * p_zxing_img_buf = static_cast<uint8 *>(SAlloc::C(src_height * src_width, sizeof(uint8)));
    	//THROW_MEM(p_zxing_img_buf);
		//
		{
			DecodeHints hints;
			hints.setTextMode(TextMode::HRI);
			hints.setEanAddOnSymbol(EanAddOnSymbol::Read);
			//hints.characterSet()
			ImageView image(p_ib->GetData(), src_width, src_height, ImageFormat::XRGB);
			Results results = ReadBarcodes(image, hints);
			if(results.size()) {
				for(uint i = 0; i < results.size(); i++) {
					Result & r_result = results.at(i);
					if(r_result.isValid()) {
						PPBarcode::Entry * p_new_entry = rList.CreateNewItem();
						THROW_SL(p_new_entry);
						switch(r_result.format()) {
							case ZXing::BarcodeFormat::Aztec: p_new_entry->BcStd = BARCSTD_AZTEC; break;
							case ZXing::BarcodeFormat::Codabar: p_new_entry->BcStd = BARCSTD_ANSI; break;
							case ZXing::BarcodeFormat::Code39: p_new_entry->BcStd = BARCSTD_CODE39; break;
							case ZXing::BarcodeFormat::Code93: p_new_entry->BcStd = BARCSTD_CODE93; break;
							case ZXing::BarcodeFormat::Code128: p_new_entry->BcStd = BARCSTD_CODE128; break;
							case ZXing::BarcodeFormat::DataBar: p_new_entry->BcStd = BARCSTD_DATABAR; break;
							case ZXing::BarcodeFormat::DataBarExpanded: p_new_entry->BcStd = BARCSTD_DATABAR; break; // @?
							case ZXing::BarcodeFormat::DataMatrix: p_new_entry->BcStd = BARCSTD_DATAMATRIX; break;
							case ZXing::BarcodeFormat::EAN8: p_new_entry->BcStd = BARCSTD_EAN8; break;
							case ZXing::BarcodeFormat::EAN13: p_new_entry->BcStd = BARCSTD_EAN13; break;
							case ZXing::BarcodeFormat::ITF: p_new_entry->BcStd = BARCSTD_ITF; break;
							case ZXing::BarcodeFormat::MaxiCode: p_new_entry->BcStd = BARCSTD_MAXICODE; break;
							case ZXing::BarcodeFormat::PDF417: p_new_entry->BcStd = BARCSTD_PDF417; break;
							case ZXing::BarcodeFormat::QRCode: p_new_entry->BcStd = BARCSTD_QR; break;
							case ZXing::BarcodeFormat::UPCA: p_new_entry->BcStd = BARCSTD_UPCA; break;
							case ZXing::BarcodeFormat::UPCE: p_new_entry->BcStd = BARCSTD_UPCE; break;
							case ZXing::BarcodeFormat::MicroQRCode: p_new_entry->BcStd = BARCSTD_MICROQR; break;
							case ZXing::BarcodeFormat::RMQRCode: p_new_entry->BcStd = BARCSTD_RMQR; break;
						}
						{
							const std::string txt = r_result.text(ZXing::TextMode::Plain);
							p_new_entry->Code.CatN(txt.c_str(), txt.length());
						}
						ok = 1;
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_fig;
	return ok;
}
