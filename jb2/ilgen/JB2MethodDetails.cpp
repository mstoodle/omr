/*******************************************************************************
 * Copyright (c) 2014, 2016 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#include "api/omrgen/OMRIlGen.hpp"
#include "compile/JB2ResolvedMethod.hpp"
#include "il/ResolvedMethodSymbol.hpp"
#include "ilgen/JB2MethodDetails.hpp"

namespace OMR {
namespace JitBuilder {
namespace omrgen {

TR::ResolvedMethod *
JB2MethodDetails::getMethod() {
    return reinterpret_cast<TR::ResolvedMethod *>(_method); // TODO figure this out
}

TR_ResolvedMethod *
JB2MethodDetails::getResolvedMethod() {
    return reinterpret_cast<TR_ResolvedMethod *>(_method); // TODO figure this out
}


bool
JB2MethodDetails::sameAs(TR::IlGeneratorMethodDetails & other, TR_FrontEnd *fe) {
    return (static_cast<JB2MethodDetails *>(&other) == this);
}

void
JB2MethodDetails::print(TR_FrontEnd *fe, TR::FILE *file) {

}

TR_IlGenerator *
JB2MethodDetails::getIlGenerator(TR::ResolvedMethodSymbol *methodSymbol,
                                 TR_FrontEnd * fe,
                                 TR::Compilation *comp,
                                 TR::SymbolReferenceTable *symRefTab,
                                 bool forceClassLookahead,
                                 TR_InlineBlocks *blocksToInline) {
    TR_IlGenerator *ilgen = _method->getIlGenerator(this, methodSymbol, fe, symRefTab);
    return ilgen;
}

} // namespace omrgen
} // namespace JitBuilder
} // namespace OMR
