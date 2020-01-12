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

#ifndef CASE_INCL
#define CASE_INCL

#include <stdint.h>
#include <vector>
#include <iostream>
#include "FunctionBuilder.hpp"
#include "Object.hpp"
#include "PrettyPrinter.hpp"

namespace OMR
{

namespace JitBuilder
{

class Case : public Object
   {
public:
   static Case * create(int v, Builder * b)
      {
      Case *c = new Case(v, b);
      b->fb()->registerObject(c);
      return c;
      }

   int64_t id() const          { return _id; }
   int32_t value() const       { return _value; }
   Builder * builder() const   { return _builder; }
   virtual size_t size() const { return sizeof(Case); }

protected:
   Case(int v, Builder * b)
      : Object(b->fb())
      , _id(_globalIndex++)
      , _value(v)
      , _builder(b)
      { }

   int64_t _id;
   int32_t _value;
   Builder * _builder;

   static int64_t _globalIndex;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(CASE_INCL)
