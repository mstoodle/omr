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

#ifndef TYPE_INCL
#define TYPE_INCL

#include <stdint.h>
#include <iostream>
#include <string>

namespace OMR
{

namespace JitBuilder
{

class Type
   {
   friend class TypeDictionary;
public:
   static Type * NoType;
   std::string name() const { return _name; }
   int64_t id() const       { return _id; }

   bool operator!=(const Type & other) const
      {
      return _id != other._id;
      }
   bool operator==(const Type & other) const
      {
      return _id == other._id;
      }
   virtual bool isPointer() const
      {
      return false;
      }

protected:
   static Type * create(std::string name) { return new Type(name); }
   Type(std::string & name)
      : _id(globalIndex++)
      , _name(name)
      { }
   int64_t        _id;
   std::string    _name;

   static int64_t globalIndex;
   };


class PointerType : public Type
   {
public:
   static PointerType * create(Type * baseType, std::string name)
      {
      return new PointerType(baseType, name);
      }

   Type * BaseType() const { return _baseType; }
   virtual bool isPointer() const { return true; }

protected:
   PointerType(Type * baseType, std::string & name)
      : Type(name)
      , _baseType(baseType)
      { }
   Type * _baseType;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(TYPE_INCL)
