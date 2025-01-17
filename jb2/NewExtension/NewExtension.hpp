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

// Search for ### to find locations you need to change when creating your own Extension

#ifndef NEWEXTENSION_INCL // ### replace NEWEXTENSION with new macro name specific to this extension
#define NEWEXTENSION_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace New {      // ### replace New with a unique namespace you'll use for all code in this Extension

class NewExtension : public Extension { // ### replace NewExtension (globally) with the name of your extension, usually still suffixed with "Extension"
    JBALLOC_(NewExtension)

public:
    NewExtension(LOCATION, Compiler *compiler, bool extended=false, String extensionName="vm");

    static const String NAME;

    virtual const SemanticVersion * semver() const {
        return &version;
    }

    // Only needed if depending on BaseExtension
    Base::BaseExtension *base() const { return _base; }

    // ### Add other extension objects here as needed

    //
    // Types
    //
    // ### Add any Types your extension defines

    //
    // Actions
    //
    // ### Add any Actions your extension defines (used by Operations)

    //
    // CompilerReturnCodes
    //
    // ### Add any CompilerReturnCodes your extension defines

    //
    // Operations
    //
    // ### Add functions to create new Operations, see Base/BaseExtension.hpp for example

    // Pseudo operations
    // ### Add any other helpers here

protected:

    // ### Only needed if NewExtension depends on BaseExtension
    Base::BaseExtension *_base;

    // ### Add any other dependent extension objects here

    // ### Update (and maintain) your new extension's semantic version in these consts, typically rename "NEW" for your extension name
    static const MajorID NEWEXT_MAJOR=0;
    static const MinorID NEWEXT_MINOR=1;
    static const PatchID NEWEXT_PATCH=0;
    static const SemanticVersion version;

    // ### Update (and maintain) any minimum requirement on the BaseExtension dependency in these consts
    static const MajorID REQUIRED_BASEEXT_MAJOR=0;
    static const MinorID REQUIRED_BASEEXT_MINOR=1;
    static const PatchID REQUIRED_BASEEXT_PATCH=0;
    static const SemanticVersion requiredBaseVersion;

    // ### Add any other extension dependency minimum version requirements here
};

} // namespace New
} // namespace JB2
} // namespace OMR

#endif // defined(NEWEXTENSION_INCL)
