###############################################################################
# Copyright (c) 2017, 2021 IBM Corp. and others
#
# This program and the accompanying materials are made available under
# the terms of the Eclipse Public License 2.0 which accompanies this
# distribution and is available at https://www.eclipse.org/legal/epl-2.0/
# or the Apache License, Version 2.0 which accompanies this distribution and
# is available at https://www.apache.org/licenses/LICENSE-2.0.
#
# This Source Code may also be made available under the following
# Secondary Licenses when the conditions for such availability set
# forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
# General Public License, version 2 with the GNU Classpath
# Exception [1] and GNU General Public License, version 2 with the
# OpenJDK Assembly Exception [2].
#
# [1] https://www.gnu.org/software/classpath/license.html
# [2] http://openjdk.java.net/legal/assembly-exception.html
#
# SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
###############################################################################

include(OmrCompilerSupport)

omr_assert(
	TEST OMR_COMPILER
	MESSAGE "OMR_COMPILER must be set."
)

# JitBuilder Files
set(JB2_OBJECTS
	env/FrontEnd.cpp
	compile/ResolvedMethod.cpp
	control/Jit.cpp
	ilgen/JBIlGeneratorMethodDetails.cpp
	optimizer/JBOptimizer.hpp
	optimizer/JBOptimizer.cpp
	optimizer/Optimizer.hpp
	runtime/JBCodeCacheManager.cpp
	runtime/JBJitConfig.cpp
	api/jb/JBCodeGenerator.cpp
	api/jb/JBMethodBuilder.cpp
	api/jb/OMRJB.cpp
)

if(OMR_ARCH_X86)
	list(APPEND JB2_OBJECTS
		x/codegen/Evaluator.cpp
	)
elseif(OMR_ARCH_S390)
	list(APPEND JB2_OBJECTS
		z/codegen/Evaluator.cpp
	)
elseif(OMR_ARCH_POWER)
	list(APPEND JB2_OBJECTS
		p/codegen/Evaluator.cpp
		p/runtime/CodeSync.cpp
		p/runtime/AsmUtil.spp
		p/runtime/CodeDispatch.spp
		${omr_SOURCE_DIR}/compiler/p/env/OMRDebugEnv.cpp
	)
endif()


if(OMR_ARCH_X86 OR OMR_ARCH_S390)
	set(OMR_ENHANCED_WARNINGS OFF)
else()
	set(OMR_WARNINGS_AS_ERRORS OFF)
	set(OMR_ENHANCED_WARNINGS OFF)
endif()

# Create jitbuilder library.
create_omr_compiler_library(
	NAME    jb2
	OBJECTS ${JB2_OBJECTS}
	DEFINES PROD_WITH_ASSUMES JITTEST
)

target_include_directories(jb2 PRIVATE ${omr_SOURCE_DIR}/jb2/api/ )

add_subdirectory(api)
