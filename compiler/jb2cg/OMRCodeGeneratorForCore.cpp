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

#include "jb2/JBCore.hpp"
#include "jb2cg/OMRCodeGenerator.hpp"
#include "jb2cg/OMRCodeGeneratorForCore.hpp"
#include "jb2cg/OMRIlGen.hpp"


namespace OMR {
namespace JB2 {
namespace omrgen {

static const MajorID BASEDON_COREEXT_MAJOR=0;
static const MajorID BASEDON_COREEXT_MINOR=1;
static const MajorID BASEDON_COREEXT_PATCH=0;
const static SemanticVersion minCoreVersion(BASEDON_COREEXT_MAJOR,BASEDON_COREEXT_MINOR,BASEDON_COREEXT_PATCH);

INIT_JBALLOC_REUSECAT(OMRCodeGeneratorForCore, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(OMRCodeGeneratorForCore,"OMRCodeGeneratorForCore",CodeGeneratorForCore,Extensible);

OMRCodeGeneratorForCore::OMRCodeGeneratorForCore(Allocator *a, OMRCodeGenerator *omrcg, CoreExtension *cx)
    : CodeGeneratorForCore(a, omrcg, cx) {

    assert(cx->semver()->isCompatibleWith(minCoreVersion));

    setTraceEnabled(false);
}

OMRCodeGeneratorForCore::~OMRCodeGeneratorForCore() {
}


OMRCodeGenerator *
OMRCodeGeneratorForCore::omrcg() const {
    return cg()->refine<OMRCodeGenerator>();
}

OMRIlGen *
OMRCodeGeneratorForCore::ilgen() const {
    return omrcg()->ilgen();
}

bool
OMRCodeGeneratorForCore::registerType(const Type *t) {
    assert(t->id() == cx()->tNoType);
    ilgen()->registerNoType(t);
    return true;
}

bool
OMRCodeGeneratorForCore::registerBuilder(Builder *b) {
    assert(b->isExactKind<Builder>());
    ilgen()->registerBuilder(b);
    return true;
}

Builder *
OMRCodeGeneratorForCore::gencodeAppendBuilder(Operation *op) {
    return NULL;
}

Builder *
OMRCodeGeneratorForCore::gencodeMergeDef(Operation *op) {
    return NULL;
}

} // namespace omrgen
} // namespace JB2
} // namespace OMR
