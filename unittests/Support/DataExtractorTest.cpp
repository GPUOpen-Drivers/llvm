//===- llvm/unittest/Support/DataExtractorTest.cpp - DataExtractor tests --===//
//
// Part of the LLVM Project, under the Apache License v2.0 with LLVM Exceptions.
// See https://llvm.org/LICENSE.txt for license information.
// SPDX-License-Identifier: Apache-2.0 WITH LLVM-exception
//
//===----------------------------------------------------------------------===//

#include "llvm/Support/DataExtractor.h"
#include "llvm/Testing/Support/Error.h"
#include "gtest/gtest.h"
using namespace llvm;

namespace {

const char numberData[] = "\x80\x90\xFF\xFF\x80\x00\x00\x00";
const char stringData[] = "hellohello\0hello";
const char leb128data[] = "\xA6\x49";
const char bigleb128data[] = "\xAA\xA9\xFF\xAA\xFF\xAA\xFF\x4A";

TEST(DataExtractorTest, OffsetOverflow) {
  DataExtractor DE(StringRef(numberData, sizeof(numberData)-1), false, 8);
  EXPECT_FALSE(DE.isValidOffsetForDataOfSize(-2U, 5));
}

TEST(DataExtractorTest, UnsignedNumbers) {
  DataExtractor DE(StringRef(numberData, sizeof(numberData)-1), false, 8);
  uint64_t offset = 0;

  EXPECT_EQ(0x80U, DE.getU8(&offset));
  EXPECT_EQ(1U, offset);
  offset = 0;
  EXPECT_EQ(0x8090U, DE.getU16(&offset));
  EXPECT_EQ(2U, offset);
  offset = 0;
  EXPECT_EQ(0x8090FFFFU, DE.getU32(&offset));
  EXPECT_EQ(4U, offset);
  offset = 0;
  EXPECT_EQ(0x8090FFFF80000000ULL, DE.getU64(&offset));
  EXPECT_EQ(8U, offset);
  offset = 0;
  EXPECT_EQ(0x8090FFFF80000000ULL, DE.getAddress(&offset));
  EXPECT_EQ(8U, offset);
  offset = 0;

  uint32_t data[2];
  EXPECT_EQ(data, DE.getU32(&offset, data, 2));
  EXPECT_EQ(0x8090FFFFU, data[0]);
  EXPECT_EQ(0x80000000U, data[1]);
  EXPECT_EQ(8U, offset);
  offset = 0;

  // Now for little endian.
  DE = DataExtractor(StringRef(numberData, sizeof(numberData)-1), true, 4);
  EXPECT_EQ(0x9080U, DE.getU16(&offset));
  EXPECT_EQ(2U, offset);
  offset = 0;
  EXPECT_EQ(0xFFFF9080U, DE.getU32(&offset));
  EXPECT_EQ(4U, offset);
  offset = 0;
  EXPECT_EQ(0x80FFFF9080ULL, DE.getU64(&offset));
  EXPECT_EQ(8U, offset);
  offset = 0;
  EXPECT_EQ(0xFFFF9080U, DE.getAddress(&offset));
  EXPECT_EQ(4U, offset);
  offset = 0;

  EXPECT_EQ(data, DE.getU32(&offset, data, 2));
  EXPECT_EQ(0xFFFF9080U, data[0]);
  EXPECT_EQ(0x80U, data[1]);
  EXPECT_EQ(8U, offset);
}

TEST(DataExtractorTest, SignedNumbers) {
  DataExtractor DE(StringRef(numberData, sizeof(numberData)-1), false, 8);
  uint64_t offset = 0;

  EXPECT_EQ(-128, DE.getSigned(&offset, 1));
  EXPECT_EQ(1U, offset);
  offset = 0;
  EXPECT_EQ(-32624, DE.getSigned(&offset, 2));
  EXPECT_EQ(2U, offset);
  offset = 0;
  EXPECT_EQ(-2137980929, DE.getSigned(&offset, 4));
  EXPECT_EQ(4U, offset);
  offset = 0;
  EXPECT_EQ(-9182558167379214336LL, DE.getSigned(&offset, 8));
  EXPECT_EQ(8U, offset);
}

TEST(DataExtractorTest, Strings) {
  DataExtractor DE(StringRef(stringData, sizeof(stringData)-1), false, 8);
  uint64_t offset = 0;

  EXPECT_EQ(stringData, DE.getCStr(&offset));
  EXPECT_EQ(11U, offset);
  EXPECT_EQ(nullptr, DE.getCStr(&offset));
  EXPECT_EQ(11U, offset);
}

TEST(DataExtractorTest, LEB128) {
  DataExtractor DE(StringRef(leb128data, sizeof(leb128data)-1), false, 8);
  uint64_t offset = 0;

  EXPECT_EQ(9382ULL, DE.getULEB128(&offset));
  EXPECT_EQ(2U, offset);
  offset = 0;
  EXPECT_EQ(-7002LL, DE.getSLEB128(&offset));
  EXPECT_EQ(2U, offset);

  DataExtractor BDE(StringRef(bigleb128data, sizeof(bigleb128data)-1), false,8);
  offset = 0;
  EXPECT_EQ(42218325750568106ULL, BDE.getULEB128(&offset));
  EXPECT_EQ(8U, offset);
  offset = 0;
  EXPECT_EQ(-29839268287359830LL, BDE.getSLEB128(&offset));
  EXPECT_EQ(8U, offset);
}

TEST(DataExtractorTest, LEB128_error) {
  DataExtractor DE(StringRef("\x81"), false, 8);
  uint64_t Offset = 0;
  EXPECT_EQ(0U, DE.getULEB128(&Offset));
  EXPECT_EQ(0U, Offset);

  Offset = 0;
  EXPECT_EQ(0U, DE.getSLEB128(&Offset));
  EXPECT_EQ(0U, Offset);
}

TEST(DataExtractorTest, Cursor_tell) {
  DataExtractor DE(StringRef("AB"), false, 8);
  DataExtractor::Cursor C(0);
  // A successful read operation advances the cursor
  EXPECT_EQ('A', DE.getU8(C));
  EXPECT_EQ(1u, C.tell());

  // An unsuccessful one doesn't.
  EXPECT_EQ(0u, DE.getU16(C));
  EXPECT_EQ(1u, C.tell());

  // And neither do any subsequent operations.
  EXPECT_EQ(0, DE.getU8(C));
  EXPECT_EQ(1u, C.tell());

  consumeError(C.takeError());
}

TEST(DataExtractorTest, Cursor_takeError) {
  DataExtractor DE(StringRef("AB"), false, 8);
  DataExtractor::Cursor C(0);
  // Initially, the cursor is in the "success" state.
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());

  // It remains "success" after a successful read.
  EXPECT_EQ('A', DE.getU8(C));
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());

  // An unsuccessful read sets the error state.
  EXPECT_EQ(0u, DE.getU32(C));
  EXPECT_THAT_ERROR(C.takeError(), Failed());

  // Once set the error sticks until explicitly cleared.
  EXPECT_EQ(0u, DE.getU32(C));
  EXPECT_EQ(0, DE.getU8(C));
  EXPECT_THAT_ERROR(C.takeError(), Failed());

  // At which point reads can be succeed again.
  EXPECT_EQ('B', DE.getU8(C));
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());
}

TEST(DataExtractorTest, Cursor_chaining) {
  DataExtractor DE(StringRef("ABCD"), false, 8);
  DataExtractor::Cursor C(0);

  // Multiple reads can be chained without trigerring any assertions.
  EXPECT_EQ('A', DE.getU8(C));
  EXPECT_EQ('B', DE.getU8(C));
  EXPECT_EQ('C', DE.getU8(C));
  EXPECT_EQ('D', DE.getU8(C));
  // And the error checked at the end.
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());
}

#if defined(GTEST_HAS_DEATH_TEST) && defined(_DEBUG)
TEST(DataExtractorDeathTest, Cursor) {
  DataExtractor DE(StringRef("AB"), false, 8);

  // Even an unused cursor must be checked for errors:
  EXPECT_DEATH(DataExtractor::Cursor(0),
               "Success values must still be checked prior to being destroyed");

  {
    auto C = std::make_unique<DataExtractor::Cursor>(0);
    EXPECT_EQ(0u, DE.getU32(*C));
    // It must also be checked after an unsuccessful operation.
    // destruction.
    EXPECT_DEATH(C.reset(), "unexpected end of data");
    EXPECT_THAT_ERROR(C->takeError(), Failed());
  }
  {
    auto C = std::make_unique<DataExtractor::Cursor>(0);
    EXPECT_EQ('A', DE.getU8(*C));
    // Same goes for a successful one.
    EXPECT_DEATH(
        C.reset(),
        "Success values must still be checked prior to being destroyed");
    EXPECT_THAT_ERROR(C->takeError(), Succeeded());
  }
  {
    auto C = std::make_unique<DataExtractor::Cursor>(0);
    EXPECT_EQ('A', DE.getU8(*C));
    EXPECT_EQ(0u, DE.getU32(*C));
    // Even if a successful operation is followed by an unsuccessful one.
    EXPECT_DEATH(C.reset(), "unexpected end of data");
    EXPECT_THAT_ERROR(C->takeError(), Failed());
  }
  {
    auto C = std::make_unique<DataExtractor::Cursor>(0);
    EXPECT_EQ(0u, DE.getU32(*C));
    EXPECT_EQ(0, DE.getU8(*C));
    // Even if an unsuccessful operation is followed by one that would normally
    // succeed.
    EXPECT_DEATH(C.reset(), "unexpected end of data");
    EXPECT_THAT_ERROR(C->takeError(), Failed());
  }
}
#endif

TEST(DataExtractorTest, getU8_vector) {
  DataExtractor DE(StringRef("AB"), false, 8);
  DataExtractor::Cursor C(0);
  SmallVector<uint8_t, 2> S;

  DE.getU8(C, S, 4);
  EXPECT_THAT_ERROR(C.takeError(), Failed());
  EXPECT_EQ("", toStringRef(S));

  DE.getU8(C, S, 2);
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());
  EXPECT_EQ("AB", toStringRef(S));
}

TEST(DataExtractorTest, skip) {
  DataExtractor DE(StringRef("AB"), false, 8);
  DataExtractor::Cursor C(0);

  DE.skip(C, 4);
  EXPECT_THAT_ERROR(C.takeError(), Failed());
  EXPECT_EQ(0u, C.tell());

  DE.skip(C, 2);
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());
  EXPECT_EQ(2u, C.tell());
}

TEST(DataExtractorTest, eof) {
  DataExtractor DE(StringRef("A"), false, 8);
  DataExtractor::Cursor C(0);

  EXPECT_FALSE(DE.eof(C));

  EXPECT_EQ(0, DE.getU16(C));
  EXPECT_FALSE(DE.eof(C));
  EXPECT_THAT_ERROR(C.takeError(), Failed());

  EXPECT_EQ('A', DE.getU8(C));
  EXPECT_TRUE(DE.eof(C));
  EXPECT_THAT_ERROR(C.takeError(), Succeeded());
}
}
