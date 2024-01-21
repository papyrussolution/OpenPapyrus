/*
* Copyright 2018 Axel Waggershauser
*/
// SPDX-License-Identifier: Apache-2.0

#include <zxing-internal.h>
#pragma hdrstop
#include "gtest/gtest.h"

TEST(BitHackTest, BitHacks)
{
	using namespace ZXing::BitHacks;

	EXPECT_EQ(SBits::Clz(0U), 32);
	EXPECT_EQ(SBits::Clz(1U), 31);
	EXPECT_EQ(SBits::Clz(0xffffffff), 0);

//	EXPECT_EQ(SBits::Ctz(0), 32);
	EXPECT_EQ(SBits::Ctz(1U), 0);
	EXPECT_EQ(SBits::Ctz(2U), 1);
	EXPECT_EQ(SBits::Ctz(0xffffffff), 0);

	EXPECT_EQ(Reverse(0), 0);
	EXPECT_EQ(Reverse(1), 0x80000000);
	EXPECT_EQ(Reverse(0xffffffff), 0xffffffff);
	EXPECT_EQ(Reverse(0xff00ff00), 0x00ff00ff);

	EXPECT_EQ(SBits::Cpop(0U), 0);
	EXPECT_EQ(SBits::Cpop(1U), 1);
	EXPECT_EQ(SBits::Cpop(2U), 1);
	EXPECT_EQ(SBits::Cpop(0xffffffff), 32);
	EXPECT_EQ(SBits::Cpop(0x11111111U), 8);

	EXPECT_EQ(HighestBitSet(0x1), 0);
	EXPECT_EQ(HighestBitSet(0xffffffff), 31);
	EXPECT_EQ(HighestBitSet(0x1F), 4);
	using V = std::vector<uint32_t>;
	auto checkReverse = [](V&& v1, int p, V&& v2) {
		Reverse(v1, p);
		EXPECT_EQ(v1, v2);
	};
	checkReverse(V{1}, 0, V{0x80000000});
	checkReverse(V{0, 1}, 0, V{0x80000000, 0});
	checkReverse(V{0, 1}, 31, V{1, 0});
	checkReverse(V{0xffffffff, 0}, 16, V{0xffff0000, 0xffff});
}
