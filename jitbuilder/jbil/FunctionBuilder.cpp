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

#include <string>
#include <vector>
#include "FunctionBuilder.hpp"
#include "Operation.hpp"
#include "PrettyPrinter.hpp"

using namespace OMR::JitBuilder;


FunctionBuilder::~FunctionBuilder()
   {
   if (config()->reportMemory() && logger())
      logger()->indent() << "FunctionBuilder " << (Builder *)this << " : memory allocated is " << _memoryAllocated << " bytes" << logger()->endl();

   // do accounting for objects that will be freed to see if anything left
   for (auto objIt = _objects.begin(); objIt != _objects.end(); objIt++)
      {
      Object *obj = *objIt;
      _memoryAllocated -= obj->size();
      delete obj;
      }

   if (_memoryAllocated != 0 && config()->reportMemory() && logger())
      logger()->indent() << "Error: unaccounted memory: " << _memoryAllocated << logger()->endl();
   }

void
FunctionBuilder::registerObject(Object *obj)
   {
   if (obj != this)
      {
      _memoryAllocated += obj->size();
      _objects.push_back(obj);
      }
   }

void
FunctionBuilder::DefineName(std::string name)
   {
   _givenName = name;
   }

void
FunctionBuilder::DefineFile(std::string file)
   {
   _fileName = file;
   }

void
FunctionBuilder::DefineLine(std::string line)
   {
   _lineNumber = line;
   }

void
FunctionBuilder::DefineParameter(std::string name, Type * type)
   {
   ParameterSymbol parm(name, type, _parameters.size());
   _parameters.push_back(parm);
   }

void
FunctionBuilder::DefineReturnType(Type * type)
   {
   _returnType = type;
   }

void
FunctionBuilder::DefineLocal(std::string name, Type * type)
   {
   Symbol local(name, type);
   _locals.push_back(local);
   }

Type *
FunctionBuilder::getLocalType(std::string name)
   {
   Type *type = NULL;
   for (SymbolIterator lIt = LocalsBegin(); lIt != LocalsEnd(); lIt++)
      {
      const Symbol & local = *lIt;
      if (local.name() == name)
         {
         type = local.type();
         break;
         }
      }

   if (type)
      return type;

   for (ParameterSymbolIterator pIt = ParametersBegin(); pIt != ParametersEnd(); pIt++)
      {
      const ParameterSymbol & parameter = *pIt;
      if (parameter.name() == name)
         {
         type = parameter.type();
         break;
         }
      }

   return type;
   }
