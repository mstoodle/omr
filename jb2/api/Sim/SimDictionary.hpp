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

#ifndef DEBUGDICTIONARY_INCL
#define DEBUGDICTIONARY_INCL

#include <map>
#include "Sim/SimExtension.hpp"

namespace OMR {
namespace JitBuilder {

namespace Base { class BaseExtension; };
namespace Base { class FieldType; }
namespace Base { class FunctionCompilation; }
namespace Base { class PointerType; }
namespace Base { class StructType; }

namespace Debug {

class DebugDictionary : public TypeDictionary {
public:
    DebugDictionary(Compiler *compiler);
    DebugDictionary(Compiler *compiler, DebugDictionary *baseDict);

    Base::BaseExtension *_base;

    const Base::StructType *_DebugValue;
    const Base::PointerType *_pDebugValue;
    const Base::FieldType *_DebugValue_type;
    std::map<const Type *, const Base::FieldType *> _DebugValue_fields;

    const Base::StructType *_DebugFrame;
    const Base::PointerType *_pDebugFrame;
    const Base::FieldType *_DebugFrame_info;
    const Base::FieldType *_DebugFrame_debugger;
    const Base::FieldType *_DebugFrame_locals;
    const Base::FieldType *_DebugFrame_values;
    const Base::FieldType *_DebugFrame_returnValues;
    const Base::FieldType *_DebugFrame_fromBuilder;
    const Base::FieldType *_DebugFrame_returning;
    const Base::FieldType *_DebugFrame_builderToDebug;

protected:
    void createTypes(Base::FunctionCompilation *compToDebug);
    void initTypes(DebugDictionary *baseDict);
};

} // namespace Debug
} // namespace JitBuilder
} // namespace OMR

#endif // defined(DEBUGDICTIONARY_INCL)
