/*
 * Copyright 2016 Nu-book Inc.
 * Copyright 2016 ZXing authors
 */
// SPDX-License-Identifier: Apache-2.0

#pragma once

namespace ZXing::MaxiCode {
	class Reader : public ZXing::Reader {
	public:
		using ZXing::Reader::Reader;
		Result decode(const BinaryBitmap& image) const override;
	};
} // namespace ZXing::MaxiCode
