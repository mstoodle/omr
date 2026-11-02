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

#ifndef TYPEREPLACERADDON_INCL
#define TYPEREPLACERADDON_INCL

#include "common.hpp"
#include "KindService.hpp"
#include "Extensible.hpp"
#include "Extension.hpp"
#include "Addon.hpp"

namespace OMR {
namespace JB2 {

class Builder;
class Operation;
class OperationReplacer;
class TypeReplacer;

class TypeReplacerAddon : public Addon {
    friend TypeReplacer;
    JBALLOC_(TypeReplacerAddon)

public:
    TypeReplacerAddon(Allocator *a, Extension *ext, TypeReplacer *repl, KINDTYPE(Extensible) kind);

protected:
    virtual void collectTypes(IR *ir) const;
    virtual bool hasExpander(Operation *op) const;
    virtual bool expand(Builder *b, Operation *op, OperationReplacer *r) const;

    SUBCLASS_KINDSERVICE_DECL(Extensible,TypeReplacerAddon);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(TYPEREPLACERADDON_INCL)
