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
#include "jb/CoreCodeGenerator.hpp"
#include "jb/JBMethodBuilder.hpp"


namespace OMR {
namespace JitBuilder {
namespace JB {

static const MajorID BASEDON_COREEXT_MAJOR=0;
static const MajorID BASEDON_COREEXT_MINOR=1;
static const MajorID BASEDON_COREEXT_PATCH=0;
const static SemanticVersion correctCoreVersion(BASEDON_COREEXT_MAJOR,BASEDON_COREEXT_MINOR,BASEDON_COREEXT_PATCH);

INIT_JBALLOC_REUSECAT(CoreJBCodeGenerator, CodeGeneration)
SUBCLASS_KINDSERVICE_IMPL(CoreJBCodeGenerator,"CoreJBCodeGenerator",JBCodeGenerator,Extensible);

CoreJBCodeGenerator::CoreJBCodeGenerator(Allocator *a, CoreExtension *cx)
    : JBCodeGenerator(a, cx)
    , _cx(cx) {

    assert(cx->semver()->isCompatibleWith(correctCoreVersion));

    setTraceEnabled(false);
}

CoreJBCodeGenerator::~CoreJBCodeGenerator() {
}


bool
CoreJBCodeGenerator::registerType(JBMethodBuilder *jbmb, const Type *t) {
    assert(t == _cx->NoType);
    jbmb->registerNoType(t);
    return true;
}

bool
CoreJBCodeGenerator::registerBuilder(JBMethodBuilder *jbmb, Builder *b) {
    assert(b->isExactKind<Builder>());
    jbmb->createBuilder(b);
    return true;
}

void
CoreJBCodeGenerator::gencode(JBMethodBuilder *jbmb, Operation *op) {
    if (op->action() == _cx->aAppendBuilder)
        jbmb->AppendBuilder(op->location(), op->parent(), op->builder());
    else if (op->action() == _cx->aMergeDef)
        jbmb->StoreOver(op->location(), op->parent(), op->result(), op->operand());
    else
        assert(0);
}

} // namespace JB
} // namespace JitBuilder
} // namespace OMR
