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
#include "Base/BaseExtension.hpp"
#include "BasicBinaryDeserializer.hpp"
#include "BasicBinarySerializer.hpp"
#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "IR.hpp"
#include "TextLogger.hpp"
#include "TextSerializer.hpp"

using namespace OMR::JB2;

int main(int argc, char **argv) {
    ::testing::InitGoogleTest(&argc, argv);
    return RUN_ALL_TESTS();
}

TEST(IRSerializationTest, RoundTripCoreExtension) {
    Compiler comp1("sourceCompiler");
    CoreExtension *core1 = comp1.coreExt();
    ASSERT_NE(core1, nullptr);

    IR *ir1 = comp1.irPrototype();
    ASSERT_NE(ir1, nullptr);

    BasicBinarySerializer serializer(comp1.mem(), core1);
    ir1->write(serializer);

    EXPECT_GT(serializer.size(), 0u);

    // Read back in a fresh compiler
    Compiler comp2("targetCompiler");
    BasicBinaryDeserializer deserializer(comp2.mem(), comp2.coreExt(), serializer.data(), serializer.size());

    IR *ir2 = IR::read(comp2.mem(), &comp2, deserializer);
    ASSERT_NE(ir2, nullptr);

    EXPECT_EQ(deserializer.extensions().length(), 2u); // 0 (reserved) + 1 (core)
    EXPECT_NE(deserializer.extension(1), nullptr);
    EXPECT_STREQ(deserializer.extension(1)->name().c_str(), "core");

    CoreExtension *core2 = comp2.coreExt();
    EXPECT_EQ(deserializer.action(core1->aAppendBuilder), core2->aAppendBuilder);
    EXPECT_EQ(deserializer.action(core1->aMergeDef), core2->aMergeDef);
}

TEST(IRSerializationTest, RoundTripMultipleExtensionsWithOffsetActionIDs) {
    Compiler comp1("sourceCompiler");
    CoreExtension *core1 = comp1.coreExt();
    ASSERT_NE(core1, nullptr);

    Base::BaseExtension *base1 = comp1.loadExtension<Base::BaseExtension>(LOC);
    ASSERT_NE(base1, nullptr);

    IR *ir1 = comp1.irPrototype();
    ASSERT_NE(ir1, nullptr);

    BasicBinarySerializer serializer(comp1.mem(), core1);
    ir1->write(serializer);

    EXPECT_GT(serializer.size(), 0u);

    // In target compiler, register several custom actions first to offset ActionIDs
    Compiler comp2("targetCompiler");
    CoreExtension *core2 = comp2.coreExt();
    ASSERT_NE(core2, nullptr);

    // Artificially offset the next ActionID in comp2
    ActionID dummy1 = core2->registerAction("DummyAction1");
    ActionID dummy2 = core2->registerAction("DummyAction2");
    ActionID dummy3 = core2->registerAction("DummyAction3");
    (void)dummy1; (void)dummy2; (void)dummy3;

    BasicBinaryDeserializer deserializer(comp2.mem(), core2, serializer.data(), serializer.size());

    IR *ir2 = IR::read(comp2.mem(), &comp2, deserializer);
    ASSERT_NE(ir2, nullptr);

    Base::BaseExtension *base2 = comp2.lookupExtension<Base::BaseExtension>();
    ASSERT_NE(base2, nullptr);

    // Verify that BaseExtension action IDs are different between comp1 and comp2
    EXPECT_NE(base1->aAdd, base2->aAdd);

    // Verify deserializer mapped file-local IDs (matching base1's numeric IDs) to comp2's live IDs
    EXPECT_EQ(deserializer.extension(1), core2);
    EXPECT_EQ(deserializer.extension(2), base2);

    EXPECT_EQ(deserializer.action(base1->aConst), base2->aConst);
    EXPECT_EQ(deserializer.action(base1->aAdd), base2->aAdd);
    EXPECT_EQ(deserializer.action(base1->aSub), base2->aSub);
    EXPECT_EQ(deserializer.action(base1->aGoto), base2->aGoto);
    EXPECT_EQ(deserializer.action(base1->aSwitch), base2->aSwitch);
}

TEST(IRSerializationTest, TextSerializerProducesOutput) {
    Compiler compiler("testCompiler");
    CoreExtension *core = compiler.coreExt();
    ASSERT_NE(core, nullptr);

    Base::BaseExtension *base = compiler.loadExtension<Base::BaseExtension>(LOC);
    ASSERT_NE(base, nullptr);

    IR *ir = compiler.irPrototype();
    ASSERT_NE(ir, nullptr);

    Allocator *mem = compiler.mem();
    TextLogger logger(mem);
    TextSerializer serializer(mem, core, logger);

    ir->write(serializer);

    EXPECT_GT(logger.length(), 0u);
}
