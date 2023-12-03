// zxing_client.cpp
// Copyright (c) A.Sobolev 2023
// @construction
//
#include <pp.h>
#pragma hdrstop
#include <..\OSF\zxing-cpp\core\src\zxing-internal.h>

using namespace ZXing;

int ZXing_RecognizeBarcodeImage(const char * pInpFileName, TSCollection <PPBarcode::Entry> & rList)
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
						r_result.bytes();
					}
				}
			}
		}
	}
	CATCHZOK
	delete p_fig;
	return ok;
}


