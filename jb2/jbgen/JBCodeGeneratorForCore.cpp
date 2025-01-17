/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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

#include "JBCore.hpp"
#include "jbgen/JBCodeGenerator.hpp"
#include "jbgen/JBCodeGeneratorForCore.hpp"
#include "jbgen/JBMethodBuilder.hpp"


namespace OMR {
namespace JB2 {
namespace jbgen {

static const MajorID BASEDON_COREEXT_MAJOR=0;
static const MajorID BASEDON_COREEXT_MINOR=1;
static const MajorID BASEDON_COREEXT_PATCH=0;
const static SemanticVersion minCoreVersion(BASEDON_COREEXT_MAJOR,BASEDON_COREEXT_MINOR,BASEDON_COREEXT_PATCH);

INIT_JBALLOC_REUSECAT(JBCodeGeneratorForCore, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(JBCodeGeneratorForCore,"JBCodeGeneratorForCore",CodeGeneratorForCore,Extensible);

JBCodeGeneratorForCore::JBCodeGeneratorForCore(Allocator *a, JBCodeGenerator *jbcg, CoreExtension *cx)
    : CodeGeneratorForCore(a, jbcg, cx) {

    assert(cx->semver()->isCompatibleWith(minCoreVersion));

    setTraceEnabled(false);
}

JBCodeGeneratorForCore::~JBCodeGeneratorForCore() {
}


JBCodeGenerator *
JBCodeGeneratorForCore::jbcg() const {
    return cg()->refine<JBCodeGenerator>();
}

JBMethodBuilder *
JBCodeGeneratorForCore::jbmb() const {
    return jbcg()->jbmb();
}

bool
JBCodeGeneratorForCore::registerType(const Type *t) {
    assert(t->id() == cx()->tNoType);
    jbmb()->registerNoType(t);
    return true;
}

bool
JBCodeGeneratorForCore::registerBuilder(Builder *b) {
    assert(b->isExactKind<Builder>());
    jbmb()->createBuilder(b);
    return true;
}

Builder *
JBCodeGeneratorForCore::gencodeAppendBuilder(Operation *op) {
    jbmb()->AppendBuilder(op->location(), op->parent(), op->builder());
    return NULL;
}

Builder *
JBCodeGeneratorForCore::gencodeMergeDef(Operation *op) {
    jbmb()->StoreOver(op->location(), op->parent(), op->result(), op->operand());
    return NULL;
}

} // namespace jbgen
} // namespace JB2
} // namespace OMR
