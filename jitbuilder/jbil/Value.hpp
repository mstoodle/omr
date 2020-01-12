/*******************************************************************************
 * Copyright (c) 2020, 2020 IBM Corp. and others
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

#ifndef VALUE_INCL
#define VALUE_INCL

#include <stdint.h>
#include <vector>
#include <iostream>
#include "FunctionBuilder.hpp"
#include "Object.hpp"

namespace OMR
{

namespace JitBuilder
{

class Type;
class Builder;

class Value : public Object
   {
   friend class Builder;
   friend class BuilderBase;

public:
   uint64_t id() const           { return _id; }
   Type * type() const           { return _type; }

   virtual size_t size() const   { return sizeof(Value); }

protected:
   static Value * create(Builder * parent, Type * type)
      { Value *value = new Value(parent, type);
      parent->fb()->registerObject(value);
      return value;
      }
   Value(Builder * parent, Type * type)
      : Object(parent->fb())
      , _id(globalIndex++)
      , _parent(parent)
      , _type(type)
      { }

   int64_t   _id;
   Builder * _parent;
   Type    * _type;

   static int64_t globalIndex;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(VALUE_INCL)
