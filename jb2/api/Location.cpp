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

#include "Compilation.hpp"
#include "IRCloner.hpp"
#include "Location.hpp"

namespace OMR {
namespace JitBuilder {

INIT_JBALLOC_ON(Location, IL)

Location::Location(Allocator *a, IR *ir, String fileName, String lineNumber)
    : Allocatable(a)
    , _id(ir->getLocationID())
    , _ir(ir)
    , _fileName(fileName)
    , _lineNumber(lineNumber)
    , _bcIndex(_id-1) { // LocationIDs start at 1 but bcIndex can start at 0

    ir->registerLocation(this);
}

Location::Location(Allocator *a, IR *ir, String fileName, String lineNumber, int32_t bcIndex)
    : Allocatable(a)
    , _id(ir->getLocationID())
    , _ir(ir)
    , _fileName(fileName)
    , _lineNumber(lineNumber)
    , _bcIndex(bcIndex) {

    ir->registerLocation(this);
}

Location::Location(Allocator *a, const Location *source, IRCloner *cloner)
    : Allocatable(a)
    , _id(source->_id)
    , _ir(cloner->clonedIR())
    , _fileName(source->_fileName)
    , _lineNumber(source->_lineNumber)
    , _bcIndex(source->_bcIndex) {

    cloner->clonedIR()->registerLocation(this);
}

Location *
Location::cloneLocation(Allocator *mem, IRCloner *cloner) const {
    return new (mem) Location(mem, this, cloner);
}

Location::~Location() {

}

} // namespace JitBuilder
} // namespace OMR
