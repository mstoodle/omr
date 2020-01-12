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

#ifndef METHODBUILDER_INCL
#define METHODBUILDER_INCL


#include <string>
#include <vector>

#include "Builder.hpp"
#include "Config.hpp"
#include "Operation.hpp"
#include "Symbol.hpp"
#include "Type.hpp"

namespace OMR
{

namespace JitBuilder
{

class Operation;
class PrettyPrinter;
class TypeDictionary;

class FunctionBuilder : public Builder
   {
   friend class Builder;
public:
   virtual ~FunctionBuilder();

   void DefineName(std::string name);
   void DefineFile(std::string file);
   void DefineLine(std::string line);
   void DefineParameter(std::string name, Type * type);
   void DefineReturnType(Type * type);
   void DefineLocal(std::string name, Type * type);

   std::string name() const                        { return _givenName; }
   std::string fileName() const                    { return _fileName; }
   std::string lineNumber() const                  { return _lineNumber; }
   ParameterSymbolIterator ParametersBegin() const { return _parameters.begin(); }
   ParameterSymbolIterator ParametersEnd() const   { return _parameters.end(); }
   SymbolIterator LocalsBegin() const              { return _locals.begin(); }
   SymbolIterator LocalsEnd() const                { return _locals.end(); }
   Type * getReturnType() const                    { return _returnType; }
   TypeDictionary * types() const                  { return _types; }
   Config * config()                               { return &_config; }
   virtual size_t size() const                     { return sizeof(FunctionBuilder); }

   bool constructIL()
      {
      bool rc = buildIL();
      _ilBuilt = true;
      return rc;
      }

   virtual bool buildIL()
      {
      return false;
      }

   bool ilBuilt() const                   { return _ilBuilt; }

   Type * getLocalType(std::string name);

   int32_t incrementLocations()           { return _numLocations++; }
   void addLocation(Location *loc)        { _locations.push_back(loc); }

   int64_t incrementTransformation()      { return _numTransformations++; }

   void setLogger(PrettyPrinter * logger) { _logger = logger; }
   PrettyPrinter * logger()               { return _logger; }

   virtual size_t size() { return sizeof(FunctionBuilder); }
   void registerObject(Object *obj);

   protected:
   FunctionBuilder(TypeDictionary * types)
      : Builder(this, this)
      , _types(types)
      , _returnType(Type::NoType)
      , _ilBuilt(false)
      , _numLocations(0)
      , _memoryAllocated(0)
      , _numTransformations(0)
      , _logger(NULL)
      {
      SourceLocation(); // make sure everything has a locaion; by default BCIndex == 0
      }

   TypeDictionary         * _types;
   Config                   _config;
   std::string              _givenName;
   std::string              _fileName;
   std::string              _lineNumber;
   ParameterSymbolVector    _parameters;
   SymbolVector             _locals;
   Type                   * _returnType;

   int64_t                  _numLocations;
   std::vector<Location *>  _locations;
   bool                     _ilBuilt;
   uint64_t                 _memoryAllocated;
   std::vector<Object *>    _objects;
   int64_t                  _numTransformations;
   PrettyPrinter          * _logger;
   };

} // namespace JitBuilder

} // namespace OMR

#endif // defined(METHODBUILDER_INCL)
