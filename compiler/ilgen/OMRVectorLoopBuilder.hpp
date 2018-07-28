/*******************************************************************************
 * Copyright (c) 2018, 2018 IBM Corp. and others
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

#ifndef OMR_VECTORLOOPBUILDER_INCL
#define OMR_VECTORLOOPBUILDER_INCL

#include <map>

#include "ilgen/IlBuilder.hpp"

namespace OMR
{

/**
 * The VectorLoopBuilder class is designed to simplify the construction of vectorized
 * loops, by automatically handling one of the trickier and error prone aspects of
 * vector loops: ensuring that both vector and scalar loops perform the same operations.
 *
 * A VectorLoopBuilder object is typically constructed via IlBuilder::VectorForLoop()
 * which returns the VectorLoopBuilder object to be used for the loop.  VectorLoopBuilder
 * duplicates vectorizable services from IlBuilder, including ForLoop to facilitate
 * outer loop vectorization. Each of these services automatically injects operations
 * into the scalar and (if it is needed) the vector loop body. IlBuilder::VectorForLoop
 * creates the control flow for the vectorized loop but uses the VectorLoopBuilder's
 * vectorBody() and residueBody() IlBuilder objects as the core builder objects
 * representing the loop operations. The VectorLoopBuilder object becomes a proxy to
 * clients for populating those loops consistently with the same basic operations
 * (vector operations in the vector loop, and scalar operations in the residue loop).
 *
 * To make all that work smoothly, VectorLoopBuilder considers the residue loop to be
 * the "master" * loop. VectorLoopBuilder also maintains a mapping between TR::IlValues
 * computed in the residue loop and the corresponding TR::IlValues created for the
 * same operation in the vector loop. In this way, the client works with the residue
 * loop TR::IlValues, but the VectorLoopBuilder can always associate those values to
 * the corresponding values int he vector loop so as to build the same operations there.
 * Another source of confusion is referencing the loop iteration variable, so the
 * VectorLoopBuilder object provides LoadIterationVar() to hide the actual names from
 * the client by returning a TR::IlValue corresponding to the iteration variable's value
 * in the residue loop (which is transparently mapped to the corresponding TR::IlValue
 * in the vector
 * loop).
 */

class VectorLoopBuilder : public TR::IlBuilder
   {
public:
   TR_ALLOC(TR_Memory::IlGenerator)

   VectorLoopBuilder(TR::Compilation *comp, TR::MethodBuilder *methodBuilder, TR::TypeDictionary *types, TR::IlType *vectorElementType);
   virtual ~VectorLoopBuilder() { }

   TR::IlBuilder *vectorBody() { return &_vectorLoopBody; }
   char *vectorIteratorVariable() { return _vectorIteratorName; }
   TR::IlBuilder *residueBody() { return &_residueLoopBody; }
   char *residueIteratorVariable() { return _residueIteratorName; }
   char *residueConditionVariable() { return _residueConditionName; }

   /**
    * @brief returns the vector length being used for this loop, which can be 1 if there is only
    *        a scalar loop
    */
   uint32_t VectorLength();

   /**
    * @brief returns the value of the loop iteration variable
    */
   TR::IlValue *LoadIterationVar();

   // constants

   /**
    * @brief creates a TR::IlValue for the given 8-bit constant in both residue and vector loops
    */
   TR::IlValue *ConstInt8(int8_t value);

   /**
    * @brief creates a TR::IlValue for the given 16-bit constant in both residue and vector loops
    */
   TR::IlValue *ConstInt16(int16_t value);

   /**
    * @brief creates a TR::IlValue for the given 32-bit constant in both residue and vector loops
    */
   TR::IlValue *ConstInt32(int32_t value);

   /**
    * @brief creates a TR::IlValue for the given 64-bit constant in both residue and vector loops
    */
   TR::IlValue *ConstInt64(int64_t value);

   /**
    * @brief creates a TR::IlValue for the given 32-bit floating point constant in both residue
    *        and vector loops
    */
   TR::IlValue *ConstFloat(float value);

   /**
    * @brief creates a TR::IlValue for the given 64-bit floating point constant in both residue
    *        and vector loops
    */
   TR::IlValue *ConstDouble(double value);

   /**
    * @brief creates a TR::IlValue for the given 8-bit constant in both residue and vector loops
    */
   TR::IlValue *Const(int8_t value)             { return ConstInt8(value); }

   /**
    * @brief creates a TR::IlValue for the given 16-bit constant in both residue and vector loops
    */
   TR::IlValue *Const(int16_t value)            { return ConstInt16(value); }

   /**
    * @brief creates a TR::IlValue for the given 32-bit constant in both residue and vector loops
    */
   TR::IlValue *Const(int32_t value)            { return ConstInt32(value); }

   /**
    * @brief creates a TR::IlValue for the given 64-bit constant in both residue and vector loops
    */
   TR::IlValue *Const(int64_t value)            { return ConstInt64(value); }

   /**
    * @brief creates a TR::IlValue for the given 32-bit floating point constant in both residue
    *        and vector loops
    */
   TR::IlValue *Const(float value)              { return ConstFloat(value); }

   /**
    * @brief creates a TR::IlValue for the given 64-bit floating point constant in both residue
    *        and vector loops
    */
   TR::IlValue *Const(double value)             { return ConstDouble(value); }

   /**
    * @brief creates a TR::IlValue according to the provided type for the given integer constant
    *        in both residue and vector loops
    * @param intType the type of constant desired
    * @param value the integer value which will be coerced to the appropriate type
    */
   TR::IlValue *ConstInteger(TR::IlType *intType, int64_t value);

   /**
    * @brief returns a value for the sum of left and right, performed in both residue and vector
    *        loops
    */
   TR::IlValue *Add(TR::IlValue *left, TR::IlValue *right);

   /**
    * @brief returns a value for the difference left minus right, performed as scalar in residue
    *        loop and as vector in vector loop
    */
   TR::IlValue *Sub(TR::IlValue *left, TR::IlValue *right);

   /**
    * @brief returns a value for the product of left and right, performed as scalar in residue
    *        loop and as vector in vector loop
    */
   TR::IlValue *Mul(TR::IlValue *left, TR::IlValue *right);

   /**
    * @brief Loads the named local variable, performed in both residue and vector loops
    */
   TR::IlValue *Load(const char *name);

   /**
    * @brief Stores value to the named local variable, performed in both residue and vector loops
    */
   void Store(const char *name, TR::IlValue *value);

   /**
    * @brief Loads the named local variable, performed as scalar load in residue loop and as
    *        vector load in vector loop
    */
   TR::IlValue *VectorLoad(const char *name);

   /**
    * @brief Stores the value to the named local variable, performed as scalar store in residue
    *        loop and as vector store in vector loop
    */
   void VectorStore(const char *name, TR::IlValue *value);

   /**
    * @brief Loads scalar value from base[index], performed in both residue and vector loops
    */
   TR::IlValue *ArrayLoad(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index);

   /**
    * @brief Stores scalar value to base[index] as type dt, performed in both residue and
    *        vector loops
    */
   void ArrayStore(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index, TR::IlValue *value);

   /**
    * @brief Loads value from base[index], performed as scalar load in residue loop and as
    *        vector load in vector loop
    */
   TR::IlValue *VectorArrayLoad(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index);

   /**
    * @brief Stores value to base[index] as type pdt, performed as scalar store in residue loop
    *        and as vector store in vector loop
    */
   void VectorArrayStore(TR::IlType *dt, TR::IlValue *base, TR::IlValue *index, TR::IlValue *value);

   /**
    * @brief Creates a simple ForLoop iterating up from initial to end by increment. The loop is
    *        created in both scalar and vector loops.
    * @returns VectorLoopBuilder object representing the body of the inner ForLoop
    */
   TR::VectorLoopBuilder *ForLoop(TR::IlValue *initial, TR::IlValue *end, TR::IlValue *increment);

   void initialize(TR::IlGeneratorMethodDetails * details,
                   TR::ResolvedMethodSymbol     * methodSymbol,
                   TR::FrontEnd                 * fe,
                   TR::SymbolReferenceTable     * symRefTab);

   void setupForBuildIL();

protected:
   void setParentVectorLoopBuilder(TR::VectorLoopBuilder *parent) { _parent = parent; }
   TR::VectorLoopBuilder *parentVectorLoopBuilder() { return _parent; }

   TR::IlValue *getVectorValue(TR::IlValue *value);
   char * getVectorName(const char *name);

   /**
    * @brief if this VectorLoopBuilder was created inside another VectorLoopBuilder, record it here
    *        so that residue-to-vector mappings can be looked up in parent loops
    */
   TR::VectorLoopBuilder                  * _parent;

   /**
    * @brief records where this loop has a vector loop (if vector length > 1)
    */
   bool                                     _hasVectorLoop;

   /**
    * @brief the IlType for each vector element (determines vector length)
    */
   TR::IlType                             * _vectorElementType;

   /**
    * @brief map used to map IlValue's in the vector loop to those in the residue loop
    */
   std::map<TR::IlValue *, TR::IlValue *>   _vectorValueMap;

   /**
    * @brief map used to map local names in the vector loop to those in the residue loop
    */
   std::map<const char *, char *>           _vectorNameMap;

   /**
    * @brief the IlBuilder used for the vectorized loop body
    */
   TR::IlBuilder                            _vectorLoopBody;

   /**
    * @brief the name of the iteration variable used by the vector loop
    */
   char *                                   _vectorIteratorName;

   /**
    * @brief the IlBuilder used for the residue loop body
    */
   TR::IlBuilder                            _residueLoopBody;

   /**
    * @brief the name of the iteration variable used by the residue loop
    */
   char *                                   _residueIteratorName;

   /**
    * @brief the name of the condition variable used to exit the residue loop
    */
   char *                                   _residueConditionName;

   /**
    * @brief a unique identifier for this VectorLoopBuilder
    */
   uint32_t                                 _loopID;
   };

} // namespace OMR

#endif // !defined(OMR_VECTORLOOPBUILDER_INCL)
