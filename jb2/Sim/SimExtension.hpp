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

#ifndef DEBUGEXTENSION_INCL
#define DEBUGEXTENSION_INCL

#include <stdint.h>
#include <map>
#include "JBCore.hpp"


namespace OMR {
namespace JB2 {

class Compilation;
class Context;
class Location;
class Value;

namespace Base { class BaseExtension; }

namespace Debug {

class Debugger;

class DebugExtension : public Extension {

public:
    DebugExtension(LOCATION, Compiler *compiler, bool extended=false, String extensionName="vm");
    virtual ~DebugExtension();

    static const String NAME;

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    Base::BaseExtension *base() const { return _base; }

    //
    // Types
    //

    //
    // Actions
    //

    //
    // CompilerReturnCodes
    //

    //
    // Operations
    //

    // Pseudo operations
    Debugger *createDebugger(LOCATION, Debugger *caller=NULL);

protected:
    Base::BaseExtension *_base;

    static const MajorID DEBUGEXT_MAJOR=0;
    static const MinorID DEBUGEXT_MINOR=1;
    static const PatchID DEBUGEXT_PATCH=0;
    static const SemanticVersion version;

    static const MajorID REQUIRED_BASEEXT_MAJOR=0;
    static const MinorID REQUIRED_BASEEXT_MINOR=1;
    static const PatchID REQUIRED_BASEEXT_PATCH=0;
    static const SemanticVersion requiredBaseVersion;
};

} // namespace Debug
} // namespace JB2
} // namespace OMR

#endif // defined(DEBUGEXTENSION_INCL)
