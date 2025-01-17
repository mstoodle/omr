/*******************************************************************************
 * Copyright (c) 2022, 2022 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at http://eclipse.org/legal/epl-2.0
 * or the Apache License, Version 2.0 which accompanies this distribution
 * and is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following Secondary
 * Licenses when the conditions for such availability set forth in the
 * Eclipse Public License, v. 2.0 are satisfied: GNU General Public License,
 * version 2 with the GNU Classpath Exception [1] and GNU General Public
 * License, version 2 with the OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "gtest/gtest.h"
#include "AllocatorRaw.hpp"
#include "BitVector.hpp"

using namespace OMR::JB2;

static Allocator *mem = NULL;

int
main(int argc, char** argv) {
    AllocatorRaw raw;
    mem = &raw;
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(JB2BitVector, EmptyVector) {
    BitVector bv(mem);
    EXPECT_EQ(bv.length(), 0) << "fresh bv has length zero";
}

TEST(JB2BitVector, SetGetBit) {
    BitVector bv(mem);
    bv.setBit(10);
    EXPECT_GT(bv.length(), 10) << "length should at least 10";
    EXPECT_TRUE(bv.getBit(10)) << "bv.getBit(10) should be set";
    EXPECT_FALSE(bv.getBit(9)) << "bv.getBit(9) should not be set";
    EXPECT_FALSE(bv.getBit(8)) << "bv.getBit(8) should not be set";
    EXPECT_FALSE(bv.getBit(0)) << "bv.getBit(0) should not be set";
    EXPECT_FALSE(bv.getBit(100000000)) << "bv.getBit(100000000) should not be set";

    EXPECT_TRUE(bv[10]) << "bv[10] should be set";
    EXPECT_FALSE(bv[9]) << "bv[9] should not be set";
    EXPECT_FALSE(bv[8]) << "bv[8] should not be set";
    EXPECT_FALSE(bv[0]) << "bv[0] should not be set";
    EXPECT_FALSE(bv[100000000]) << "bv[100000000] should not be set";

    bv.setBit(10, false);
    EXPECT_FALSE(bv.getBit(10)) << "bv.getBit(10) should now be cleared";
    EXPECT_FALSE(bv[10]) << "bv[10] should now be cleared";

    BitVector bv2(mem);
    bv2.setBit(64);
    EXPECT_GT(bv2.length(), 64) << "length should be at least 64";
    EXPECT_TRUE(bv2.getBit(64)) << "bv.getBit(64) should be set";
    EXPECT_TRUE(bv2[64]) << "bv[64] should be set";
    EXPECT_FALSE(bv2.getBit(63)) << "bv.getBit(63) should be set";
    EXPECT_FALSE(bv2[63]) << "bv[63] should be set";
    EXPECT_FALSE(bv2.getBit(65)) << "bv.getBit(65) should be set";
    EXPECT_FALSE(bv2[65]) << "bv[64] should be set";

    BitVector bv3(mem);
    bv3.setBit(100000);
    EXPECT_GT(bv3.length(), 100000) << "length should be at least 100000";
    EXPECT_TRUE(bv3.getBit(100000)) << "bv.getBit(100000) should be set";
    EXPECT_TRUE(bv3[100000]) << "bv[100000] should be set";
    EXPECT_FALSE(bv3.getBit(99999)) << "bv.getBit(99999) should be set";
    EXPECT_FALSE(bv3[99999]) << "bv[99999] should be set";
    EXPECT_FALSE(bv3.getBit(100001)) << "bv.getBit(100001) should be set";
    EXPECT_FALSE(bv3[100001]) << "bv[100001] should be set";
}

TEST(JB2BitVector, SetGetMultipleBits) {
    BitVector bv(mem);
    bv.setBit(3); bv.setBit(5); bv.setBit(7);
    EXPECT_GT(bv.length(), 7) << "length should be at least largest index (7)";
    EXPECT_FALSE(bv.getBit(2)) << "bit 2 should not be set";
    EXPECT_TRUE(bv.getBit(3)) << "bit 3 should be set";
    EXPECT_FALSE(bv.getBit(4)) << "bit 4 should not be set";
    EXPECT_TRUE(bv.getBit(5)) << "bit 5 should be set";
    EXPECT_FALSE(bv.getBit(6)) << "bit 6 should not be set";
    EXPECT_TRUE(bv.getBit(7)) << "bit 7 should be set";
    EXPECT_FALSE(bv.getBit(8)) << "bit 8 should not be set";
}

TEST(JB2BitVector, ClearMultipleBits) {
    BitVector bv(mem);
    bv.setBit(3); bv.setBit(5); bv.setBit(7);
    bv.clear();
    EXPECT_GT(bv.length(), 7) << "length should be at least largest index (7)";
    EXPECT_FALSE(bv.getBit(2)) << "bit 2 should not be set";
    EXPECT_FALSE(bv.getBit(3)) << "bit 3 should not be set";
    EXPECT_FALSE(bv.getBit(4)) << "bit 4 should not be set";
    EXPECT_FALSE(bv.getBit(5)) << "bit 5 should not be set";
    EXPECT_FALSE(bv.getBit(6)) << "bit 6 should not be set";
    EXPECT_FALSE(bv.getBit(7)) << "bit 7 should not be set";
    EXPECT_FALSE(bv.getBit(8)) << "bit 8 should not be set";
}

TEST(JB2BitVector, EraseBits) {
    BitVector bv(mem);
    bv.setBit(3); bv.setBit(5); bv.setBit(7);
    bv.erase();
    EXPECT_EQ(bv.length(), 0) << "after erase, length should be zero";
    EXPECT_FALSE(bv.getBit(2)) << "bit 2 should not be set";
    EXPECT_FALSE(bv.getBit(3)) << "bit 3 should not be set";
    EXPECT_FALSE(bv.getBit(4)) << "bit 4 should not be set";
    EXPECT_FALSE(bv.getBit(5)) << "bit 5 should not be set";
    EXPECT_FALSE(bv.getBit(6)) << "bit 6 should not be set";
    EXPECT_FALSE(bv.getBit(7)) << "bit 7 should not be set";
    EXPECT_FALSE(bv.getBit(8)) << "bit 8 should not be set";
    EXPECT_FALSE(bv.getBit(100000000)) << "bit 100000000 should not be set";
    EXPECT_FALSE(bv.getBit(987654321)) << "bit 100000000 should not be set";
}

TEST(JB2BitVector, IterateSingleBit) {
    BitVector bv(mem);
    auto it1 = bv.iterator();
    EXPECT_FALSE(it1.hasItem()) << "empty bv should return empty iterator";

    bv.setBit(10);
    auto it2 = bv.iterator();
    EXPECT_TRUE(it2.hasItem()) << "single bit should return non-empty iterator";
    EXPECT_EQ(it2.item(), 10) << "iterator item should be the set bit (10)";
    it2++; EXPECT_FALSE(it2.hasItem()) << "iterator should be empty now";

    it2.reset();
    EXPECT_TRUE(it2.hasItem()) << "after reset, iterator should have an item again";
    EXPECT_EQ(it2.item(), 10) << "iterator item should be the set bit (10)";
    it2++; EXPECT_FALSE(it2.hasItem()) << "iterator should again be empty now";
}

TEST(JB2BitVector, IterateMultipleBits) {
    BitVector bv(mem);
    bv.setBit(3); bv.setBit(5); bv.setBit(7); bv.setBit(100000);

    auto it = bv.iterator();
    EXPECT_TRUE(it.hasItem()) << "bv should not return empty iterator";
    EXPECT_EQ(it.item(), 3) << "first item should be 3";
    EXPECT_TRUE(it.hasItem()) << "iterator should have more items";
    it++; EXPECT_EQ(it.item(), 5) << "next item should be 5";
    EXPECT_TRUE(it.hasItem()) << "iterator should have more items";
    it++; EXPECT_EQ(it.item(), 7) << "next item should be 7";
    EXPECT_TRUE(it.hasItem()) << "iterator should have more items";
    it++; EXPECT_EQ(it.item(), 100000) << "next item should be 100000";
    it++; EXPECT_FALSE(it.hasItem()) << "iterator should be empty now";

    it.reset();
    EXPECT_TRUE(it.hasItem()) << "after reset, iterator should have items again";
    EXPECT_EQ(it.item(), 3) << "first item should be 3";
    EXPECT_TRUE(it.hasItem()) << "iterator should have more items";
    it++; EXPECT_EQ(it.item(), 5) << "next item should be 5";
    EXPECT_TRUE(it.hasItem()) << "iterator should have more items";
    it++; EXPECT_EQ(it.item(), 7) << "next item should be 7";
    EXPECT_TRUE(it.hasItem()) << "iterator should have more items";
    it++; EXPECT_EQ(it.item(), 100000) << "next item should be 100000";
    it++; EXPECT_FALSE(it.hasItem()) << "iterator should again be empty now";
}

TEST(JB2BitVector, DynamicIterate) {
    BitVector *v = BitVector::newVector(mem, 15, 13);
    v->setBit(8);
    auto it = v->iterator();
    EXPECT_TRUE(it.hasItem()) << "dynamic vector should have two bits to iterate";
    EXPECT_EQ(it.item(), 8) << "First iterated bit should be 8";
    it++; EXPECT_TRUE(it.hasItem()) << "dynamic vector should still have one more bit to iterate";
    EXPECT_EQ(it.item(), 13) << "Second iterated bit should be 13";
    it++; EXPECT_FALSE(it.hasItem()) << "iterator should be empty now";
    delete v;
}

TEST(JB2Vector, BitUnionSameSize) {
    BitVector v1(mem, 15, 5);
    BitVector v2(mem, 15, 10);
    v2 |= v1;
    auto it = v1.iterator();
    EXPECT_TRUE(it.hasItem()) << "v1 should have one item";
    EXPECT_EQ(it.item(), 5) << "v1 bit 5 should not be changed";
    it++; EXPECT_FALSE(it.hasItem()) << "v1 should only have bit 5 set";

    auto it2= v2.iterator();
    EXPECT_TRUE(it2.hasItem()) << "v2 should have two bits set";
    EXPECT_EQ(it2.item(), 5) << "v2 should have bit 5 set";
    it2++; EXPECT_TRUE(it2.hasItem()) << "v2 should have a second bit";
    EXPECT_EQ(it2.item(), 10) << "v2 should have bit 10 set";
    it2++; EXPECT_FALSE(it2.hasItem()) << "v2 should only have bits 5 and 10 set";
}

TEST(JB2Vector, BitIntersectSameSize) {
    BitVector v1(mem, 15, 5);
    BitVector v2(mem, 15, 5);
    v1.setBit(2);
    v2.setBit(12);
    v2 &= v1;
    auto it = v1.iterator();
    EXPECT_TRUE(it.hasItem()) << "v1 should unchanged with two bits";
    EXPECT_EQ(it.item(), 2) << "v1 bit 2 should not be changed";
    it++; EXPECT_TRUE(it.hasItem()) << "v1 should have two bits set";
    EXPECT_EQ(it.item(), 5) << "v1 bit 5 should not be changed";
    it++; EXPECT_FALSE(it.hasItem()) << "v1 should only have two bits set";

    auto it2= v2.iterator();
    EXPECT_TRUE(it2.hasItem()) << "v2 should now have one bit set";
    EXPECT_EQ(it2.item(), 5) << "v2 should have bit 5 set";
    it2++; EXPECT_FALSE(it2.hasItem()) << "v2 should only have bit 5 set";
}

TEST(JB2Vector, BitUnionUnequalSizeOneLonger) {
    BitVector v1(mem, 30, 25);
    BitVector v2(mem, 15, 10);
    v2 |= v1;
    auto it = v1.iterator();
    EXPECT_TRUE(it.hasItem()) << "v1 should have one item";
    EXPECT_EQ(it.item(), 25) << "v1 bit 25 should not be changed";
    it++; EXPECT_FALSE(it.hasItem()) << "v1 should only have bit 25 set";

    auto it2= v2.iterator();
    EXPECT_TRUE(it2.hasItem()) << "v2 should have two bits set";
    EXPECT_EQ(it2.item(), 10) << "v2 should have bit 10 set";
    it2++; EXPECT_TRUE(it2.hasItem()) << "v2 should have a second bit";
    EXPECT_EQ(it2.item(), 25) << "v2 should have bit 25 set";
    it2++; EXPECT_FALSE(it2.hasItem()) << "v2 should only have bits 10 and 25 set";
}

TEST(JB2Vector, BitIntersectUnequalSizeOneLonger) {
    BitVector v1(mem, 30, 5);
    BitVector v2(mem, 15, 5);
    v1.setBit(25);
    v2.setBit(12);
    v2 &= v1;
    auto it = v1.iterator();
    EXPECT_TRUE(it.hasItem()) << "v1 should unchanged with two bits";
    EXPECT_EQ(it.item(), 5) << "v1 bit 5 should not be changed";
    it++; EXPECT_TRUE(it.hasItem()) << "v1 should have two bits set";
    EXPECT_EQ(it.item(), 25) << "v1 bit 25 should not be changed";
    it++; EXPECT_FALSE(it.hasItem()) << "v1 should only have two bits set";

    auto it2= v2.iterator();
    EXPECT_TRUE(it2.hasItem()) << "v2 should now have one bit set";
    EXPECT_EQ(it2.item(), 5) << "v2 should have bit 5 set";
    it2++; EXPECT_FALSE(it2.hasItem()) << "v2 should only have bit 5 set";
}
TEST(JB2Vector, BitUnionUnequalSizeTwoLonger) {
    BitVector v1(mem, 15, 10);
    BitVector v2(mem, 30, 25);
    v2 |= v1;
    auto it = v1.iterator();
    EXPECT_TRUE(it.hasItem()) << "v1 should have one item";
    EXPECT_EQ(it.item(), 10) << "v1 bit 25 should not be changed";
    it++; EXPECT_FALSE(it.hasItem()) << "v1 should only have bit 10 set";

    auto it2= v2.iterator();
    EXPECT_TRUE(it2.hasItem()) << "v2 should have two bits set";
    EXPECT_EQ(it2.item(), 10) << "v2 should have bit 10 set";
    it2++; EXPECT_TRUE(it2.hasItem()) << "v2 should have a second bit";
    EXPECT_EQ(it2.item(), 25) << "v2 should have bit 25 set";
    it2++; EXPECT_FALSE(it2.hasItem()) << "v2 should only have bits 10 and 25 set";
}

TEST(JB2Vector, BitIntersectUnequalSizeTwoLonger) {
    BitVector v1(mem, 15, 5);
    BitVector v2(mem, 30, 5);
    v1.setBit(12);
    v2.setBit(25);
    v2 &= v1;
    auto it = v1.iterator();
    EXPECT_TRUE(it.hasItem()) << "v1 should unchanged with two bits";
    EXPECT_EQ(it.item(), 5) << "v1 bit 5 should not be changed";
    it++; EXPECT_TRUE(it.hasItem()) << "v1 should have two bits set";
    EXPECT_EQ(it.item(), 12) << "v1 bit 12 should not be changed";
    it++; EXPECT_FALSE(it.hasItem()) << "v1 should only have two bits set";

    auto it2= v2.iterator();
    EXPECT_TRUE(it2.hasItem()) << "v2 should now have one bit set";
    EXPECT_EQ(it2.item(), 5) << "v2 should have bit 5 set";
    it2++; EXPECT_FALSE(it2.hasItem()) << "v2 should only have bit 5 set";
}

// should test modification detection, but may need separate test to detect assertions
