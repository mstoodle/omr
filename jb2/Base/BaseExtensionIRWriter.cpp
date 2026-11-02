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

#include "Base/BaseExtension.hpp"
#include "Base/BaseExtensionIRWriter.hpp"
#include "Compiler.hpp"
#include "CoreExtension.hpp"
#include "CoreExtensionIRWriter.hpp"
#include "IRSerializer.hpp"

namespace OMR {
namespace JB2 {
namespace Base {

INIT_JBALLOC_REUSECAT(BaseExtensionIRWriter, Extension)
SUBCLASS_KINDSERVICE_IMPL(BaseExtensionIRWriter, "BaseExtensionIRWriter", ExtensionIRWriter, Extensible)

BaseExtensionIRWriter::BaseExtensionIRWriter(Allocator *a, BaseExtension *ext)
    : ExtensionIRWriter(a, ext, CLASSKIND(BaseExtensionIRWriter, Extensible)) {
}

BaseExtensionIRWriter::~BaseExtensionIRWriter() {
}

void
BaseExtensionIRWriter::writeElementsForIDSpace(IRSerializer & s, IDSpaceID space) {
    CoreExtension *core = _ext->compiler()->coreExt();
    CoreExtensionIRWriter *coreWriter = core->irWriter()->refine<CoreExtensionIRWriter>();
    if (coreWriter != NULL && coreWriter->isActionIDSpace(space)) {
        BaseExtension *bx = static_cast<BaseExtension *>(_ext);
        const ActionID baseActions[] = {
            bx->aConst,
            bx->aAdd,
            bx->aAnd,
            bx->aConvertTo,
            bx->aDiv,
            bx->aEqualTo,
            bx->aMul,
            bx->aNotEqualTo,
            bx->aSub,
            bx->aLoadAt,
            bx->aStoreAt,
            bx->aLoadField,
            bx->aStoreField,
            bx->aLoadFieldAt,
            bx->aStoreFieldAt,
            bx->aCreateLocalArray,
            bx->aCreateLocalStruct,
            bx->aIndexAt,
            bx->aForLoopUp,
            bx->aGoto,
            bx->aIfCmpEqual,
            bx->aIfCmpEqualZero,
            bx->aIfCmpGreaterThan,
            bx->aIfCmpGreaterOrEqual,
            bx->aIfCmpLessThan,
            bx->aIfCmpLessOrEqual,
            bx->aIfCmpNotEqual,
            bx->aIfCmpNotEqualZero,
            bx->aIfCmpUnsignedGreaterThan,
            bx->aIfCmpUnsignedGreaterOrEqual,
            bx->aIfCmpUnsignedLessThan,
            bx->aIfCmpUnsignedLessOrEqual,
            bx->aIfThenElse,
            bx->aSwitch
        };
        uint32_t count = sizeof(baseActions) / sizeof(baseActions[0]);
        IRSerializer::Section sec(s, "actions");
        s.writeCount(count);
        for (uint32_t i = 0; i < count; ++i) {
            coreWriter->writeAction(s, baseActions[i]);
        }
    }
}

} // namespace Base
} // namespace JB2
} // namespace OMR
