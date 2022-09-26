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
#include "SemanticVersion.hpp"

using namespace OMR::JitBuilder;

int main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}


#define EXPECT3(m,v,maval,mival,paval) \
    EXPECT_TRUE(v.isValid()) << m; \
    EXPECT_EQ(v.major(),maval); \
    EXPECT_EQ(v.minor(),mival); \
    EXPECT_EQ(v.patch(),paval);

#define EXPECT_VALID_CREATE0(v1,v2,v3,m) \
    do { \
        SemanticVersion v; \
        EXPECT3(m,v,v1,v2,v3); \
    } while (0)

#define EXPECT_VALID_CREATE1(v2,v3,ma,m) \
    do { \
        SemanticVersion v(ma); \
        EXPECT3(m,v,ma,v2,v3); \
    } while(0)

#define EXPECT_VALID_CREATE2(v3,ma,mi,m) \
    do { \
        SemanticVersion v(ma,mi); \
        EXPECT3(m,v,ma,mi,v3); \
    } while(0)

#define EXPECT_VALID_CREATE3(ma,mi,pa,m) \
    do { \
        SemanticVersion v(ma,mi,pa); \
        EXPECT3(m,v,ma,mi,pa); \
    } while(0)

#define EXPECT_VALID_CREATE4(ma,mi,pa,pr,m) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(pr),std::string("")); \
        EXPECT3(m,v,ma,mi,pa); \
    } while(0)

#define EXPECT_VALID_CREATE5(ma,mi,pa,pr,bm,m) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(pr),std::string(bm)); \
        EXPECT3(m, v,ma,mi,pa); \
    } while(0)

TEST(SemVerTest, CreationTests) {
    EXPECT_VALID_CREATE0(0,0,0, "SemanticVersion()");
    EXPECT_VALID_CREATE1(  0,0, 0, "SemanticVersion(0)");
    EXPECT_VALID_CREATE1(  0,0, 1, "SemanticVersion(1)");
    EXPECT_VALID_CREATE1(  0,0, 100, "SemanticVersion(100)");
    EXPECT_VALID_CREATE2(    0, 0,0, "SemanticVersion(0.0)");
    EXPECT_VALID_CREATE2(    0, 0,1, "SemanticVersion(0.1)");
    EXPECT_VALID_CREATE2(    0, 1,0, "SemanticVersion(1.0)");
    EXPECT_VALID_CREATE3(       0,0,0, "SemanticVersion(0.0.0)");
    EXPECT_VALID_CREATE3(       0,0,1, "SemanticVersion(0.0.1)");
    EXPECT_VALID_CREATE3(       0,1,0, "SemanticVersion(0.1.0)");
    EXPECT_VALID_CREATE3(       0,1,1, "SemanticVersion(0.1.1)");
    EXPECT_VALID_CREATE3(       1,0,0, "SemanticVersion(1.0.0)");
    EXPECT_VALID_CREATE3(       2,0,0, "SemanticVersion(2.0.0)");
    EXPECT_VALID_CREATE3(       2,1,0, "SemanticVersion(2.1.0)");
    EXPECT_VALID_CREATE3(       2,1,1, "SemanticVersion(2.1.1)");
    EXPECT_VALID_CREATE4(       1,0,0,"alpha", "SemanticVersion(1.0.0-alpha)");
    EXPECT_VALID_CREATE4(       1,0,0,"alpha.1", "SemanticVersion(1.0.0-alpha.1)");
    EXPECT_VALID_CREATE4(       1,0,0,"alpha.beta", "SemanticVersion(1.0.0-alpha.beta)");
    EXPECT_VALID_CREATE4(       1,0,0,"beta", "SemanticVersion(1.0.0-beta)");
    EXPECT_VALID_CREATE4(       1,0,0,"beta.2", "SemanticVersion(1.0.0-beta.2)");
    EXPECT_VALID_CREATE4(       1,0,0,"beta.11", "SemanticVersion(1.0.0-beta.11)");
    EXPECT_VALID_CREATE4(       1,0,0,"rc.1", "SemanticVersion(1.0.0-rc.1)");
    EXPECT_VALID_CREATE4(       1,0,0,"0.3.7", "SemanticVersion(1.0.0-0.3.7)");
    EXPECT_VALID_CREATE4(       1,0,0,"x.7.z.92", "SemanticVersion(1.0.0-x.7.z.92)");
    EXPECT_VALID_CREATE4(       1,0,0,"x-y-z.–", "SemanticVersion(1.0.0-x-y-z.-)");
    EXPECT_VALID_CREATE5(       1,0,0,"alpha","001", "SemanticVersion(1.0.0-alpha+001)");
    EXPECT_VALID_CREATE5(       1,0,0,"","20130313144700", "SemanticVersion(1.0.0+20130313144700)");
    EXPECT_VALID_CREATE5(       1,0,0,"beta","exp.sha.5114f85", "SemanticVersion(1.0.0-beta+exp.sha.5114f85)");
    EXPECT_VALID_CREATE5(       1,0,0,"","21AF26D3—-117B344092BD", "SemanticVersion(1.0.0+21AF26D3—-117B344092BD)");
}

#define EXPECT_CORE3(ma,mi,pa ) \
    do { \
        SemanticVersion v(ma,mi,pa); \
        EXPECT_EQ(v.coreVersion(), #ma "." #mi "." #pa) << "SemanticVersion(" #ma "," #mi "," #pa ")"; \
    } while (0)

#define EXPECT_CORE4(ma,mi,pa,pr ) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(pr),std::string("")); \
        EXPECT_EQ(v.coreVersion(), #ma "." #mi "." #pa) ; \
    } while (0)

#define EXPECT_CORE4bm(ma,mi,pa,bm ) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(""),std::string(bm)); \
        EXPECT_EQ(v.coreVersion(), #ma "." #mi "." #pa); \
    } while (0)

#define EXPECT_CORE5(ma,mi,pa,pr,bm ) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(pr),std::string(bm)); \
        EXPECT_EQ(v.coreVersion(), #ma "." #mi "." #pa); \
    } while (0)

TEST(SemVerTest, CoreNaming) {
    EXPECT_CORE3(0,0,0);
    EXPECT_CORE3(0,0,1);
    EXPECT_CORE3(0,1,0);
    EXPECT_CORE3(1,0,0);
    EXPECT_CORE3(1,2,3);
    EXPECT_CORE4(1,0,0,"alpha");
    EXPECT_CORE4(1,0,0,"alpha.1");
    EXPECT_CORE4(1,0,0,"alpha.beta");
    EXPECT_CORE4bm(1,0,0,"20130313144700");
    EXPECT_CORE5(1,0,0,"alpha","001");
}

#define EXPECT_NAME3(ma,mi,pa,m) \
    do { \
        SemanticVersion v(ma,mi,pa); \
        EXPECT_EQ(v.semver(), #ma "." #mi "." #pa) << m; \
    } while (0)

#define EXPECT_NAME4(ma,mi,pa,pr,m) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(pr),std::string("")); \
        EXPECT_EQ(v.semver(), #ma "." #mi "." #pa "-" pr)  << m; \
    } while (0)

#define EXPECT_NAME4bm(ma,mi,pa,bm,m) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(""),std::string(bm)); \
        EXPECT_EQ(v.semver(), #ma "." #mi "." #pa "+" bm) << m; \
    } while (0)

#define EXPECT_NAME5(ma,mi,pa,pr,bm,m) \
    do { \
        SemanticVersion v(ma,mi,pa,std::string(pr),std::string(bm)); \
        EXPECT_EQ(v.semver(), #ma "." #mi "." #pa "-" pr "+" bm) << m; \
    } while (0)

TEST(SemVerTest, FullNaming) {
    EXPECT_NAME3(0,0,0, "v0.0.0");
    EXPECT_NAME3(0,0,1, "v0.0.1");
    EXPECT_NAME3(0,1,0, "v0.1.0");
    EXPECT_NAME3(1,0,0, "v1.0.0");
    EXPECT_NAME3(1,2,3, "v1.2.3");
    EXPECT_NAME4(1,0,0,"alpha", "preRelease with one non-numeric identifier");
    EXPECT_NAME4(1,0,0,"alpha.1", "preRelease with one numeric identifier");
    EXPECT_NAME4(1,0,0,"alpha.beta", "preRelease with two non-numeric identifiers");
    EXPECT_NAME4bm(1,0,0,"20130313144700", "only build metadata");
    EXPECT_NAME5(1,0,0,"alpha","001", "preRelease and build metadata");
}

#define EXPECT_COMPATIBLE(v1,v2,m) EXPECT_TRUE(v1.isCompatibleWith(v2)) << m
#define EXPECT_INCOMPATIBLE(v1,v2,m) EXPECT_FALSE(v1.isCompatibleWith(v2)) << m

TEST(SemVerTest, Compatibility) {
    SemanticVersion v1(3,1,0);
    SemanticVersion v2(3,1,1);
    SemanticVersion v3(3,2,0);
    SemanticVersion v4(4,0,0);
    SemanticVersion v5(3,2,1);
    EXPECT_COMPATIBLE(v2, v1, "only patch version increase");
    EXPECT_COMPATIBLE(v1, v2, "only patch version decrease");
    EXPECT_COMPATIBLE(v3, v1, "only minor version increase");
    EXPECT_INCOMPATIBLE(v1, v3, "only minor version decrease");
    EXPECT_INCOMPATIBLE(v4, v1, "only major version increase");
    EXPECT_INCOMPATIBLE(v1, v4, "only major version decrease");
    EXPECT_COMPATIBLE(v5, v1, "minor and patch increase");
    EXPECT_INCOMPATIBLE(v1, v5, "minor and patch decrease");

    SemanticVersion v6(3,0,0,"alpha","");
    EXPECT_INCOMPATIBLE(v6, v5, "preRelease comes before normal release");
    EXPECT_COMPATIBLE(v5, v6, "normal release comes after preRelease");

    SemanticVersion v7(3,0,0,"","001");
    EXPECT_COMPATIBLE(v5, v7, "build meta data or not does not affect compatibility");
    EXPECT_INCOMPATIBLE(v7, v5, "build meta data or not does not affect incompatibility");

    SemanticVersion v8(3,0,0,"alpha","001");
    EXPECT_INCOMPATIBLE(v8, v5, "preRelease incompatible with normal release even with build metadata");
    EXPECT_COMPATIBLE(v5, v8, "normal release compatible with preRelese even with build metadata");
}

TEST(SemVerTest, Precedence) {
}
