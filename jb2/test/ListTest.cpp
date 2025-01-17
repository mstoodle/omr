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
#include "Compiler.hpp"
#include "List.hpp"

using namespace OMR::JB2;

static AllocatorRaw rawAllocator;
static Allocator *raw=&rawAllocator;

#define NA ((Allocator *)NULL)

int
main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}

TEST(JB2List, EmptyList) {
    List<int> list(NA, raw);;
    EXPECT_EQ(list.length(), 0) << "fresh list has length zero";
}

TEST(JB2List, PushAndPopFront) {
    List<int> list(NA, raw);
    int x=2; int y=3; int z=5; int p;
    list.push_front(x);
    EXPECT_EQ(list.length(), 1) << "one element list has length 1";
    list.push_front(y);
    EXPECT_EQ(list.length(), 2) << "two element list has length 2";
    list.push_front(z);
    EXPECT_EQ(list.length(), 3) << "three element list has length 3";
    p = list.pop_front();
    EXPECT_EQ(p, 5) << "should pop_front 5 off first";
    p = list.pop_front();
    EXPECT_EQ(p, 3) << "should pop_front 3 off second";
    p = list.pop_front();
    EXPECT_EQ(p, 2) << "should pop_front 2 off third";
    bool d = list.empty();
    EXPECT_TRUE(d) << "snould not be able to pop_front any more items";
    EXPECT_EQ(list.length(), 0) << "should be empty list";
}

TEST(JB2List, PushAndPopBack) {
    List<int> list(NA,raw);
    int x=2; int y=3; int z=5; int p;
    list.push_back(x);
    EXPECT_EQ(list.length(), 1) << "one element list has length 1";
    list.push_back(y);
    EXPECT_EQ(list.length(), 2) << "two element list has length 2";
    list.push_back(z);
    EXPECT_EQ(list.length(), 3) << "three element list has length 3";
    p = list.pop_back();
    EXPECT_EQ(p, 5) << "should pop_front 5 off first";
    p = list.pop_back();
    EXPECT_EQ(p, 3) << "should pop_front 3 off second";
    p = list.pop_back();
    EXPECT_EQ(p, 2) << "should pop_front 2 off third";
    bool d = list.empty();
    EXPECT_TRUE(d) << "snould not be able to pop_front any more items";
    EXPECT_EQ(list.length(), 0) << "should be empty list";
}

TEST(JB2List, PushFrontAndPopBack) {
    List<int *> list(NA,raw);
    int x=2; int y=3; int z=5; int *p;
    list.push_front(&x);
    list.push_front(&y);
    list.push_front(&z);
    p = list.pop_back();
    EXPECT_EQ(*p, 2) << "should pop_back 2 off first";
    p = list.pop_back();
    EXPECT_EQ(*p, 3) << "should pop_back 3 off second";
    p = list.pop_back();
    EXPECT_EQ(*p, 5) << "should pop_back 5 off third";
    bool d = list.empty();
    EXPECT_TRUE(d) << "snould not be able to pop_front any more items";
    EXPECT_EQ(list.length(), 0) << "should be empty list";
}

TEST(JB2List, PushAndPopMix) {
    List<int *> list(NA,raw);
    int x=2; int y=3; int z=5; int *p;
    list.push_front(&x);
    list.push_front(&y);
    p = list.pop_back();
    EXPECT_EQ(*p, 2) << "should pop_back 2 off first";
    p = list.pop_front();
    EXPECT_EQ(*p, 3) << "should pop_front 3 off next";
    EXPECT_EQ(list.length(), 0) << "should be empty list";
    list.push_front(&z);
    list.push_back(&x);
    p = list.pop_front();
    EXPECT_EQ(*p, 5) << "should pop_front 5 off next";
    p = list.pop_back();
    EXPECT_EQ(*p, 2) << "should pop_back 2 off first";
    EXPECT_EQ(list.length(), 0) << "should be empty list";

    list.push_front(&x);
    list.push_back(&x);
    list.push_front(&y);
    list.push_back(&y);
    list.push_front(&z);
    list.push_back(&z);
    p = list.pop_front(); EXPECT_EQ(*p, 5) << "should pop_front 5 off next";
    p = list.pop_front(); EXPECT_EQ(*p, 3) << "should pop_front 3 off next";
    p = list.pop_front(); EXPECT_EQ(*p, 2) << "should pop_front 2 off next";
    p = list.pop_front(); EXPECT_EQ(*p, 2) << "should pop_front 2 off next";
    p = list.pop_front(); EXPECT_EQ(*p, 3) << "should pop_front 3 off next";
    p = list.pop_front(); EXPECT_EQ(*p, 5) << "should pop_front 5 off next";
    EXPECT_EQ(list.length(), 0) << "should be empty list";
}

TEST(JB2List, Iterator) {
    List<int *> list(NA,raw);
    int x=2; int y=3; int z=5; int *p;
    list.push_front(&x);
    list.push_front(&y);
    list.push_front(&z);
    auto it = list.fwdIterator(); EXPECT_TRUE(it.hasItem()) << "should have item";
    p = it.item();                EXPECT_EQ(*p, 5) << "should see 5 first";
    it++; p = it.item();          EXPECT_EQ(*p, 3) << "should see 3 next";
    it++; p = it.item();          EXPECT_EQ(*p, 2) << "should see 2 next";
    it++;                         EXPECT_FALSE(it.hasItem()) << "should not have item";

    it = list.revIterator(); EXPECT_TRUE(it.hasItem()) << "should have item";
    p = it.item();           EXPECT_EQ(*p, 2) << "should see 2 first";
    it--; p = it.item();     EXPECT_EQ(*p, 3) << "should see 3 next";
    it--; p = it.item();     EXPECT_EQ(*p, 5) << "should see 5 next";
    it--;                    EXPECT_FALSE(it.hasItem()) << "should not have item";

    auto fit=list.fwdIterator();
    auto rit=list.revIterator();
    int *pf, *pr;
    pf = fit.item();
    pr = rit.item();
    EXPECT_EQ(*pf, 5) << "fwd should see 5 first";
    EXPECT_EQ(*pr, 2) << "rev should see 2 first";
    fit++; rit--;
    pf = fit.item();
    pr = rit.item();
    EXPECT_EQ(*pf, 3) << "fwd should see 3 next";
    EXPECT_EQ(*pr, 3) << "rev should see 3 next";
    fit++; rit--;
    pf = fit.item();
    pr = rit.item();
    EXPECT_EQ(*pf, 2) << "fwd should see 2 next";
    EXPECT_EQ(*pr, 5) << "rev should see 5 next";
    fit++; rit--;
    EXPECT_FALSE(fit.hasItem()) << "fwd iterator should not have item";
    EXPECT_FALSE(rit.hasItem()) << "rev iterator should not have item";
}

// tests for insertAfter, insertBefore
// tests for remove
// tests for detecting changes to underlying list if copy not made
