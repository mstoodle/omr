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
#include "Base/Base.hpp"
#include "Debug/DebuggerFrame.hpp"
#include "Debug/DebugValue.hpp"
#include "Debug/FunctionDebugInfo.hpp"

namespace OMR {
namespace JB2 {
namespace Debug {

DebugValue *
DebuggerFrame::getValueInArray(uint8_t *base, uint64_t idx) {
    uint8_t *valueBase = base + idx * _info->_valueSizeInBytes;
    return reinterpret_cast<DebugValue *>(valueBase);
}

DebugValue *
DebuggerFrame::getValue(uint64_t idx) {
    uint8_t *p = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(_values));
    return getValueInArray(p, idx);
}

DebugValue *
DebuggerFrame::getLocal(uint64_t idx) {
    uint8_t *p = reinterpret_cast<uint8_t *>(reinterpret_cast<uintptr_t>(_locals));
    return getValueInArray(p, idx);
}

} // namespace Debug
} // namespace JB2
} // namespace OMR
