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

#ifndef COREEXTENSIONIRWRITER_INCL
#define COREEXTENSIONIRWRITER_INCL

#include "ExtensionIRWriter.hpp"

namespace OMR {
namespace JB2 {

class CoreExtension;

class CoreExtensionIRWriter : public ExtensionIRWriter {
    JBALLOC_(CoreExtensionIRWriter)

public:
    DYNAMIC_ALLOC_ONLY(CoreExtensionIRWriter, CoreExtension *ext);

    IDSpaceID actionIDSpace()     const { return _spActionID; }
    IDSpaceID typeIDSpace()       const { return _spTypeID; }
    IDSpaceID literalIDSpace()    const { return _spLiteralID; }
    IDSpaceID symbolIDSpace()     const { return _spSymbolID; }
    IDSpaceID entryPointIDSpace() const { return _spEntryPointID; }
    IDSpaceID contextIDSpace()    const { return _spContextID; }
    IDSpaceID scopeIDSpace()      const { return _spScopeID; }
    IDSpaceID builderIDSpace()    const { return _spBuilderID; }
    IDSpaceID valueIDSpace()      const { return _spValueID; }
    IDSpaceID operationIDSpace()  const { return _spOperationID; }

    bool isActionIDSpace(IDSpaceID s)     const { return s == _spActionID; }
    bool isTypeIDSpace(IDSpaceID s)       const { return s == _spTypeID; }
    bool isLiteralIDSpace(IDSpaceID s)    const { return s == _spLiteralID; }
    bool isSymbolIDSpace(IDSpaceID s)     const { return s == _spSymbolID; }
    bool isEntryPointIDSpace(IDSpaceID s) const { return s == _spEntryPointID; }
    bool isContextIDSpace(IDSpaceID s)    const { return s == _spContextID; }
    bool isScopeIDSpace(IDSpaceID s)      const { return s == _spScopeID; }
    bool isBuilderIDSpace(IDSpaceID s)    const { return s == _spBuilderID; }
    bool isValueIDSpace(IDSpaceID s)      const { return s == _spValueID; }
    bool isOperationIDSpace(IDSpaceID s)  const { return s == _spOperationID; }

    void writeAction(IRSerializer & s, ActionID action, const String & name);
    void writeAction(IRSerializer & s, ActionID action);

    virtual void writeDeclaredIDSpaces(IRSerializer & s) override;
    virtual void writeElementsForIDSpace(IRSerializer & s, IDSpaceID space) override;

protected:
    IDSpaceID _spExtID;
    IDSpaceID _spTypeID;
    IDSpaceID _spActionID;
    IDSpaceID _spLiteralID;
    IDSpaceID _spSymbolID;
    IDSpaceID _spEntryPointID;
    IDSpaceID _spContextID;
    IDSpaceID _spScopeID;
    IDSpaceID _spBuilderID;
    IDSpaceID _spValueID;
    IDSpaceID _spOperationID;

    SUBCLASS_KINDSERVICE_DECL(Extensible, CoreExtensionIRWriter);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(COREEXTENSIONIRWRITER_INCL)
