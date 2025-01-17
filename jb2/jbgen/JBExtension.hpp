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

#ifndef JBEXTENSION_INCL
#define JBEXTENSION_INCL

#include "JBCore.hpp"

namespace OMR {
namespace JB2 {
namespace jbgen {

class JBCodeGenerator;
class OMR_JB;

class JBExtension : public Extension {
    JBALLOC_(JBExtension)

public:
    DYNAMIC_ALLOC_ONLY(JBExtension, LOCATION, Compiler *compiler, bool extended=false, String extensionName="");

    static const String NAME;

    virtual const SemanticVersion * semver() const {
        return &version;
    }

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

private:
    OMR_JB *_jb;

protected:
    virtual void notifyNewExtension(Extension *other);

    OMR_JB *jb();

    JBCodeGenerator *_jbcg;

    static const MajorID JBEXT_MAJOR=0;
    static const MinorID JBEXT_MINOR=1;
    static const PatchID JBEXT_PATCH=0;
    static const SemanticVersion version;

    SUBCLASS_KINDSERVICE_DECL(Extensible,JBExtension);
};

} // namespace jbgen
} // namespace JB2
} // namespace OMR

#endif // defined(JBEXTENSION_INCL)
