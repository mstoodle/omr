/*******************************************************************************
 * Copyright (c) 2014, 2016 IBM Corp. and others
 *
 * This program and the accompanying materials are made available under
 * the terms of the Eclipse Public License 2.0 which accompanies this
 * distribution and is available at https://www.eclipse.org/legal/epl-2.0/
 * or the Apache License, Version 2.0 which accompanies this distribution and
 * is available at https://www.apache.org/licenses/LICENSE-2.0.
 *
 * This Source Code may also be made available under the following
 * Secondary Licenses when the conditions for such availability set
 * forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
 * General Public License, version 2 with the GNU Classpath
 * Exception [1] and GNU General Public License, version 2 with the
 * OpenJDK Assembly Exception [2].
 *
 * [1] https://www.gnu.org/software/classpath/license.html
 * [2] http://openjdk.java.net/legal/assembly-exception.html
 *
 * SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
 *******************************************************************************/

#ifndef JB2METHODDETAILS_INCL
#define JB2METHODDETAILS_INCL

#include "infra/Annotations.hpp"
#include "ilgen/IlGeneratorMethodDetails.hpp"
#include "env/IO.hpp"

class TR_InlineBlocks;
class TR_ResolvedMethod;
class TR_IlGenerator;
namespace TR { class Compilation; }
namespace TR { class ResolvedMethod; }
namespace TR { class ResolvedMethodSymbol; }
namespace TR { class SymbolReferenceTable; }

namespace OMR {
namespace JitBuilder {
namespace omrgen {

class JB2ResolvedMethod;

class JB2MethodDetails : public TR::IlGeneratorMethodDetails
   {

public:
   JB2MethodDetails(JB2ResolvedMethod *method) :
      TR::IlGeneratorMethodDetails(method),
      _method(method)
   { }


   TR::ResolvedMethod * getMethod();
   TR_ResolvedMethod * getResolvedMethod();

   bool sameAs(TR::IlGeneratorMethodDetails & other, TR_FrontEnd *fe);

   void print(TR_FrontEnd *fe, TR::FILE *file);

   virtual TR_IlGenerator *getIlGenerator(TR::ResolvedMethodSymbol *methodSymbol,
                                          TR_FrontEnd * fe,
                                          TR::Compilation *comp,
                                          TR::SymbolReferenceTable *symRefTab,
                                          bool forceClassLookahead,
                                          TR_InlineBlocks *blocksToInline);

protected:
   JB2ResolvedMethod * _method;
   };

} // namespace omrgen
} // namespace JitBuilder
} // namespace OMR

#endif // defined(JB2METHODDETAILS_INCL)
