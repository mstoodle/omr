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

#include "complex.hpp"
#include "ComplexReducer.hpp"
#include "ComplexTypeDictionary.hpp"
#include "Builder.hpp"
#include "FunctionBuilder.hpp"
#include "Operation.hpp"
#include "Symbol.hpp"
#include "Type.hpp"
#include "Value.hpp"

using namespace OMR::JitBuilder;

Value *
ComplexReducer::toReal(Value * cv)
   {
   return _toRealValue[cv];
   }

Value *
ComplexReducer::toImag(Value * cv)
   {
   return _toImagValue[cv];
   }

Value *
ComplexReducer::mapValue(Value * oldv)
   {
   return _remapValue[oldv];
   }

Value *
ComplexReducer::remap(Value * oldv)
   {
   Value *v = _remapValue[oldv];
   return v ? v : oldv;
   }

bool
ComplexReducer::isComplex(const Value * v)
   {
   return *v->type() == *_typeComplex;
   }

bool
ComplexReducer::isComplex(const Type * t)
   {
   return *t == *_typeComplex;
   }

bool
ComplexReducer::isComplex(const Symbol & s)
   {
   return *s.type() == *_typeComplex;
   }

bool
ComplexReducer::isPtrToComplex(Value * v)
   {
   return *v->type() == *_typePtrToComplex;
   }

bool
ComplexReducer::isPtrToComplex(Type * t)
   {
   return *t == *_typePtrToComplex;
   }


FunctionBuilder *
ComplexReducer::transformFunctionBuilder(FunctionBuilder *fb)
   {
   ComplexTypeDictionary * types = static_cast<ComplexTypeDictionary *>(fb->types());
   _typeComplex = types->Complex;
   _typePtrToComplex = types->PointerTo(types->Complex);

   Type * Double = types->Double;
   std::vector<std::string> newLocals;
   for (SymbolIterator localIt=fb->LocalsBegin(); localIt != fb->LocalsEnd(); localIt++)
      {
      const Symbol &local = *localIt;
      if (isComplex(local))
         {
         newLocals.push_back(local.name() + "_real");
         newLocals.push_back(local.name() + "_imag");
         }
      }

   // what about Parameters :(

   for (std::vector<std::string>::iterator lIt = newLocals.begin(); lIt != newLocals.end(); lIt++)
      {
      std::string newName = *lIt;
      fb->DefineLocal(newName, Double);
      }

   return NULL;
   }

Builder *
ComplexReducer::transformOperation(Operation * op)
   {
   Type * Double = _fb->types()->Double;
   Type * pDouble = _fb->types()->PointerTo(Double);

   Builder *b = op->parent()->OrphanBuilder();
   switch (op->action())
      {
      case aConstComplex :
         {
         Value * rv = op->result();
         Value * rv_r = b->ConstDouble(op->getLiteralComplex().real);
         Value * rv_i = b->ConstDouble(op->getLiteralComplex().imag);

         setReal(rv, rv_r);
         setImag(rv, rv_i);

         return b;
         }

      case aMagnitude :
         {
         break;
         #if 0 // until fully supported
         Value * v = op->operand();
         Value * v_r = toReal(v);
         Value * v_i = toImag(v);
  
         Value * new_rv = b->Sqrt(
                          b->   Add(
                          b->      Mul(v_r, v_r),
                          b->      Mul(v_i, v_i)));
         Value * rv = op->result();
         setMap(rv, new_rv);

         return b;
         #endif
         }

      case aConjugate :
         {
         Value * v = op->operand();
         Value * v_r = toReal(v);
         Value * v_i = toImag(v);
  
         Value * rv_r = v_r;
         Value * rv_i = b->Sub(       //b->Neg(v_i);
                        b->   ConstDouble(0.0),
                              v_i);

         Value * rv = op->result();
         setReal(rv, rv_r);
         setImag(rv, rv_i);

         return b;
         }

      case aLoad :
         {
         std::string varName = op->getLiteralString();
         Type * varType = _fb->getLocalType(varName);
         assert(varType);
         if (isComplex(varType))
            {
            Value * rv_r = b->Load(varName + "_real");
            Value * rv_i = b->Load(varName + "_imag");

            Value * rv = op->result();
            setReal(rv, rv_r);
            setImag(rv, rv_i);

            return b;
            }
         else if (isPtrToComplex(varType))
            {
            Value * rv = op->result();
            Value * old_v =  b->Load(varName);
            Value * new_rv = b->CoercePointer(pDouble, old_v);
            setMap(rv, new_rv);

            return b;
            }
         }
         break;

      case aStore :
         {
         std::string varName = op->getLiteralString();
         if (isComplex(_fb->getLocalType(varName)))
            {
            Value * operand = op->operand();
            Value * operand_r = toReal(operand);
            Value * operand_i = toImag(operand);

            b->Store(varName + "_real", operand_r);
            b->Store(varName + "_imag", operand_i);

            return b;
            }
         }
         break;

      case aIndexAt :
         {
         if (isPtrToComplex(op->type()))
            {
            Value * base = op->operand(0);
            assert(isPtrToComplex(base));
            Value * index = op->operand(1);

            Value *new_base = remap(base);
            Value *x2Index = b->Mul(
                                   index,
                             b->   ConstInt64(2));
            Value * rv_r = b->IndexAt(pDouble,
                           b->   Add(
                                    new_base,
                           b->      ConstInt64(offsetof(complex<double>, real))),
                                 x2Index);
            Value * rv_i = b->IndexAt(pDouble,
                           b->   Add(
                                    new_base,
                           b->      ConstInt64(offsetof(complex<double>, imag))),
                                 x2Index);

            Value * rv = op->result();
            setReal(rv, rv_r);
            setImag(rv, rv_i);

            return b;
            }
         }
         break;

      case aLoadAt :
         {
         if (isPtrToComplex(op->type()))
            {
            Value * address = op->operand();
            assert(isPtrToComplex(address));
            Value * address_r = toReal(address);
            Value * address_i = toImag(address);

            Value * rv = op->result();
            Value * rv_r = b->LoadAt(pDouble, address_r);
            Value * rv_i = b->LoadAt(pDouble, address_i);

            setReal(rv, rv_r);
            setImag(rv, rv_i);

            return b;
            }
         }
         break;

      case aStoreAt :
         {
         if (isComplex(op->operand(1)))
            {
            Value * address = op->operand(0);
            assert(isPtrToComplex(address));
            Value * address_r = toReal(address);
            Value * address_i = toImag(address);

            Value * v = op->operand(1);
            assert(isComplex(v));
            Value * v_r = toReal(v);
            Value * v_i = toImag(v);

            b->StoreAt(address_r, v_r);
            b->StoreAt(address_i, v_i);

            return b;
            }
         }
         break;

      // skip anything else for now, we'll look more closely at them later
      default :
         break;
      }

   // if we got here, then there are no complex-specific operations
   // but there may be generic operations that involve complex Value operands or results
   bool foundComplex = false;
   for (ValueIterator opIt = op->OperandsBegin(); opIt != op->OperandsEnd(); opIt++)
      {
      Value * operand = *opIt;
      if (isComplex(operand->type()) || _remapValue[operand] != NULL)
         foundComplex = true;
      }

   for (ValueIterator rIt = op->ResultsBegin(); rIt != op->ResultsEnd(); rIt++)
      {
      Value * result = *rIt;
      if (isComplex(result->type()))
         foundComplex = true;
      }

   // if no complex operands or results, then nothing to do: delegate to base class DialectReducer
   if (!foundComplex)
      return this->DialectReducer::transformOperation(op);

   // handling whatever complex values there are will depend on the action
   switch(op->action())
      {
      case aAdd :
         {
         Value * left = op->operand(0);
         Value * right = op->operand(1);

         bool leftComplex = isComplex(left->type());
         Value * left_r = leftComplex ? toReal(left) : remap(left);
         Value * left_i = leftComplex ? toImag(left) : b->ConstDouble(0.0);

         bool rightComplex = isComplex(right->type());
         Value * right_r = rightComplex ? toReal(right) : remap(right);
         Value * right_i = rightComplex ? toImag(right) : b->ConstDouble(0.0);

         Value * rv = op->result();
         Value * rv_r = b->Add(left_r, right_r);
         Value * rv_i = b->Add(left_i, left_i);

         setReal(rv, rv_r);
         setImag(rv, rv_i);

         return b;
         }

      case aSub :
         {
         Value * left = op->operand(0);
         Value * right = op->operand(1);

         bool leftComplex = isComplex(left->type());
         Value * left_r = leftComplex ? toReal(left) : remap(left);
         Value * left_i = leftComplex ? toImag(left) : b->ConstDouble(0.0);

         bool rightComplex = isComplex(right->type());
         Value * right_r = rightComplex ? toReal(right) : remap(right);
         Value * right_i = rightComplex ? toImag(right) : b->ConstDouble(0.0);

         Value * rv = op->result();
         Value * rv_r = b->Sub(left_r, right_r);
         Value * rv_i = b->Sub(left_i, left_i);

         setReal(rv, rv_r);
         setImag(rv, rv_i);

         return b;
         }

      case aMul :
         {
         Value * left = op->operand(0);
         Value * right = op->operand(1);

         bool leftComplex = isComplex(left->type());
         Value * left_r = leftComplex ? toReal(left) : remap(left);
         Value * left_i = leftComplex ? toImag(left) : b->ConstDouble(0.0);

         bool rightComplex = isComplex(right->type());
         Value * right_r = rightComplex ? toReal(right) : remap(right);
         Value * right_i = rightComplex ? toImag(right) : b->ConstDouble(0.0);

         Value * rv = op->result();
         Value * rv_r = b->Sub(
                        b->   Mul(left_r, right_r),
                        b->   Mul(left_i, right_i));
         Value * rv_i = b->Add(
                        b->   Mul(left_r, right_i),
                        b->   Mul(left_i, right_r));

         setReal(rv, rv_r);
         setImag(rv, rv_i);

         return b;
         }

      default :
         // ignore everything else for now, though technically need to remap
         // any operation that could consume a Double value
         break;
      }

   return NULL;
   }
