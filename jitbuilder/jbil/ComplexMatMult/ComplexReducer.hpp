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

#include <map>
#include "DialectReducer.hpp"

using namespace OMR::JitBuilder;

namespace OMR { namespace JitBuilder { class Builder; } }
namespace OMR { namespace JitBuilder { class FunctionBuilder; } }
namespace OMR { namespace JitBuilder { class Operation; } }
namespace OMR { namespace JitBuilder { class Symbol; } }
namespace OMR { namespace JitBuilder { class Type; } }
namespace OMR { namespace JitBuilder { class Value; } }

class ComplexReducer : public DialectReducer
   {
public:
   ComplexReducer(FunctionBuilder *fb, Dialect target)
      : DialectReducer(fb, target)
      , _typeComplex(NULL)
      , _typePtrToComplex(NULL)
      { }

protected:
   virtual FunctionBuilder * transformFunctionBuilder(FunctionBuilder * fb);
   virtual Builder * transformOperation(Operation * op);

   // helpful functions
   void setReal(Value * cv, Value * rv)    { _toRealValue[cv] = rv; }
   void setImag(Value * cv, Value * iv)    { _toImagValue[cv] = iv; }
   void setMap(Value * oldv, Value * newv) { _remapValue[oldv] = newv; }

   Value * toReal(Value * cv);
   Value * toImag(Value * cv);
   Value * mapValue(Value * oldv);
   Value * remap(Value * oldv);

   bool isComplex(const Value * v);
   bool isComplex(const Type * t);
   bool isComplex(const Symbol & s);

   bool isPtrToComplex(Value * v);
   bool isPtrToComplex(Type * t);

private:
   std::map<Value *, Value *> _toRealValue;
   std::map<Value *, Value *> _toImagValue;
   std::map<Value *, Value *> _remapValue;
   Type * _typeComplex;
   Type * _typePtrToComplex;
   };
