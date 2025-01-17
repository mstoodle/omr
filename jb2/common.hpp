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

#ifndef COMMON_INCL
#define COMMON_INCL

#include <stdint.h>
#include <cassert>
#include <exception>
#include <iostream>
#include "Allocatable.hpp"
#include "IDs.hpp"
#include "Array.hpp"
#include "Iterator.hpp"
#include "List.hpp"

#define TOSTR(x) #x
#define LINETOSTR(x) TOSTR(x)


namespace OMR {
namespace JB2 {

typedef uint64_t EyeCatcher;

class Builder;
typedef ForwardSimpleIterator<Builder *> BuilderIterator;
typedef Array<Builder *> BuilderArray;
typedef BuilderArray::ForwardIterator BuilderArrayIterator;
typedef List<Builder *> BuilderList;
typedef BuilderList::Iterator BuilderListIterator;

class Case;
typedef ForwardSimpleIterator<Case *> CaseIterator;

class Literal;
typedef uint8_t LiteralBytes;
typedef ForwardSimpleIterator<Literal *> LiteralIterator;
typedef Array<Literal *> LiteralArray;
typedef LiteralArray::ForwardIterator LiteralArrayIterator;
typedef List<Literal *> LiteralList;
typedef LiteralList::Iterator LiteralListIterator;

class Symbol;
typedef ForwardSimpleIterator<Symbol *> SymbolIterator;
typedef List<Symbol *> SymbolList;
typedef SymbolList::Iterator SymbolListIterator;
typedef Array<Symbol *> SymbolArray;
typedef SymbolArray::ForwardIterator SymbolArrayIterator;

class Type;
typedef ForwardSimpleIterator<const Type *> TypeIterator;
typedef List<const Type *> TypeList;
typedef TypeList::Iterator TypeListIterator;
typedef Array<const Type *> TypeArray;
typedef TypeArray::ForwardIterator TypeArrayIterator;

class Value;
typedef ForwardSimpleIterator<Value *> ValueIterator;

} // namespace JB2
} // namespace OMR

#endif // defined(COMMON_INCL)
