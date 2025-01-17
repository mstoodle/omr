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
#include "JBCore.hpp"

using namespace OMR::JB2;

int32_t numInitializationCalls = 0;

bool
initializeJit() {
    numInitializationCalls++;
    return true;
}

void
shutdownJit() {
    numInitializationCalls--;
}


int
main(int argc, char** argv) {
   ::testing::InitGoogleTest(&argc, argv);
   return RUN_ALL_TESTS();
}

TEST(BasicJB2, basicInitAndShutdown) {
    EXPECT_TRUE(initializeJit()) << "initializeJit()";
    EXPECT_EQ(numInitializationCalls, 1) << "Check 1 compiler initialized";
    shutdownJit();
    EXPECT_EQ(numInitializationCalls, 0) << "Check 1 compiler initialized";
}

TEST(BasicJB2, compilerCreation) {
    {
        Compiler c("test");
        EXPECT_EQ(c.name(), "test") << "Compiler gets the name test";
        EXPECT_FALSE(c.config() == NULL) << "Compiler creates a Config";
        EXPECT_FALSE(c.litdict() == NULL) << "Compiler creates a LiteralDictionary";
        EXPECT_FALSE(c.symdict() == NULL) << "Compiler creates a SymbolDictionary";
        EXPECT_FALSE(c.typedict() == NULL) << "Compiler creates a TypeDictionary";
    }
    {
        Config cfg;
        Compiler c("test2", &cfg);
        EXPECT_EQ(c.name(), "test2") << "Compiler gets the name test2";
        EXPECT_EQ(c.config(), &cfg) << "Comppiler takes the provided config";
    }
}

TEST(BasicJB2, multipleCompilers) {
    Compiler c1("test1");
    {
        Compiler c2("test2");
        {
            Compiler c3("test3");
            {
                Compiler c4("test4");
                {
                    Compiler c5("test5");

                    EXPECT_EQ(c5.name(), "test5") << "Compiler c5 gets the name test";
                    EXPECT_FALSE(c5.config() == NULL) << "Compiler c5 creates a Config";
                    EXPECT_FALSE(c5.litdict() == NULL) << "Compiler c5 creates a LiteralDictionary";
                    EXPECT_FALSE(c5.symdict() == NULL) << "Compiler c5 creates a SymbolDictionary";
                    EXPECT_FALSE(c5.typedict() == NULL) << "Compiler c5 creates a TypeDictionary";

                    EXPECT_EQ(c4.name(), "test4") << "Compiler c4 gets the name test";
                    EXPECT_FALSE(c4.config() == NULL) << "Compiler c4 creates a Config";
                    EXPECT_FALSE(c4.litdict() == NULL) << "Compiler c4 creates a LiteralDictionary";
                    EXPECT_FALSE(c4.symdict() == NULL) << "Compiler c4 creates a SymbolDictionary";
                    EXPECT_FALSE(c4.typedict() == NULL) << "Compiler c4 creates a TypeDictionary";

                    EXPECT_EQ(c3.name(), "test3") << "Compiler c3 gets the name test";
                    EXPECT_FALSE(c3.config() == NULL) << "Compiler c3 creates a Config";
                    EXPECT_FALSE(c3.litdict() == NULL) << "Compiler c3 creates a LiteralDictionary";
                    EXPECT_FALSE(c3.symdict() == NULL) << "Compiler c3 creates a SymbolDictionary";
                    EXPECT_FALSE(c3.typedict() == NULL) << "Compiler c3 creates a TypeDictionary";

                    EXPECT_EQ(c2.name(), "test2") << "Compiler c2 gets the name test";
                    EXPECT_FALSE(c2.config() == NULL) << "Compiler c2 creates a Config";
                    EXPECT_FALSE(c2.litdict() == NULL) << "Compiler c2 creates a LiteralDictionary";
                    EXPECT_FALSE(c2.symdict() == NULL) << "Compiler c2 creates a SymbolDictionary";
                    EXPECT_FALSE(c2.typedict() == NULL) << "Compiler c2 creates a TypeDictionary";

                    EXPECT_EQ(c1.name(), "test1") << "Compiler c1 gets the name test";
                    EXPECT_FALSE(c1.config() == NULL) << "Compiler c1 creates a Config";
                    EXPECT_FALSE(c1.litdict() == NULL) << "Compiler c1 creates a LiteralDictionary";
                    EXPECT_FALSE(c1.symdict() == NULL) << "Compiler c1 creates a SymbolDictionary";
                    EXPECT_FALSE(c1.typedict() == NULL) << "Compiler c1 creates a TypeDictionary";
                }
            }
        }
    }
}
#if 0
TEST(BasicJB2, extensions) {
    Compiler c1("c1");
    Extension e1(&c1, "e1");
    EXPECT_EQ(c1.lookupExtension<Extension>("e1"), &e1), "Extesion e1 is registered on c1";
    Compiler c2("c2");
    Extension e2(&c2, "e2");
    EXPECT_EQ(c2.lookupExtension<Extension>("e2"), &e2), "Extesion e2 is registered on c2";
    EXPECT_TRUE(c1.lookupExtension<Extension>("e2") == NULL), "Extesion e2 is not registered on c1";
    Extension e3(&c1, "e3");
    EXPECT_EQ(c1.lookupExtension<Extension>("e3"), &e3), "Extesion e3 is registered on c1";
    EXPECT_TRUE(c2.lookupExtension<Extension>("e3") == NULL), "Extesion e3 is not registered on c2";
    Extension e4(&c2, "e4");
    EXPECT_EQ(c2.lookupExtension<Extension>("e4"), &e4), "Extesion e4 is registered on c2";
    EXPECT_TRUE(c1.lookupExtension<Extension>("e4") == NULL), "Extesion e4 is not registered on c1";
}
#endif
#if 0
TEST(BasicJB2, extensions) {
    Compiler c1("c1");
    Extension e1(&c1, "e1");
    EXPECT_EQ(c1.lookupExtension<Extension>("e1"), &e1), "Extesion e1 is registered on c1";
    Compiler c2("c2");
    Extension e2(&c2, "e2");
    EXPECT_EQ(c2.lookupExtension<Extension>("e2"), &e2), "Extesion e2 is registered on c2";
    EXPECT_TRUE(c1.lookupExtension<Extension>("e2") == NULL), "Extesion e2 is not registered on c1";
    Extension e3(&c1, "e3");
    EXPECT_EQ(c1.lookupExtension<Extension>("e3"), &e3), "Extesion e3 is registered on c1";
    EXPECT_TRUE(c2.lookupExtension<Extension>("e3") == NULL), "Extesion e3 is not registered on c2";
    Extension e4(&c2, "e4");
    EXPECT_EQ(c2.lookupExtension<Extension>("e4"), &e4), "Extesion e4 is registered on c2";
    EXPECT_TRUE(c1.lookupExtension<Extension>("e4") == NULL), "Extesion e4 is not registered on c1";
}
#endif
