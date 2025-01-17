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

#include "JBCore.hpp"
#include "Base/BaseExtension.hpp" // ### only needed if depends on Base extension
#include "NewExtension.hpp" // ### include your extension's header here

namespace OMR {
namespace JB2 {
namespace New { // ### use your extension's namespace here

INIT_JBALLOC_REUSECAT(NewExtension, Extension)

// ###  globally rename NewExtension according to your extension's name
const SemanticVersion NewExtension::version(VMEXT_MAJOR,VMEXT_MINOR,VMEXT_PATCH);
const SemanticVersion NewExtension::requiredBaseVersion(REQUIRED_BASEEXT_MAJOR,REQUIRED_BASEEXT_MINOR,REQUIRED_BASEEXT_PATCH);
const String NewExtension::NAME("jb2new"); // ### give your extension a unique library name

extern "C" {
    Extension *create(LOCATION, Compiler *compiler) {
        Allocator *mem = compiler->mem();
        return new (mem) NewExtension(MEM_PASSLOC(mem), compiler);
    }
}

NewExtension::NewExtension(MEM_LOCATION(a), Compiler *compiler, bool extended, String extensionName)
    : Extension(a, compiler, (extended ? extensionName : NAME)) {

    // ### only needed if depends on Base extension
    _base = compiler->loadExtension<Base::BaseExtension>(PASSLOC, &requiredBaseVersion);

    // ### load any other dependent extensions here
}

NewExtension::~NewExtension() {
}

} // namespace New
} // namespace JB2
} // namespace OMR
