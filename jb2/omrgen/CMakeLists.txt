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

omr_assert(
	TEST OMR_JB2
	MESSAGE "OMR_JB2 must be set."
)

include(OmrCompilerSupport)

SET(OMRGEN_OBJECTS
	${omr_SOURCE_DIR}/compiler/control/SimpleJit.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCodeGenerator.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRIlGen.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCompiler.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/Jit.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCodeGeneratorExtensionAddon.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCodeGeneratorForBase.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCodeGeneratorForCore.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCodeGeneratorForFunc.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRCodeGeneratorForVM.cpp
	${omr_SOURCE_DIR}/compiler/jb2cg/OMRExtension.cpp
)

add_library(jb2omrgen SHARED ${OMRGEN_OBJECTS})

if(OMR_ARCH_POWER)
	list(APPEND OMRCOMPILER_OBJECTS
		${omr_SOURCE_DIR}/compiler/jb2cg/p/CodeSync.cpp
		${omr_SOURCE_DIR}/compiler/jb2cg/p/AsmUtil.spp
		${omr_SOURCE_DIR}/compiler/jb2cg/p/CodeDispatch.spp
		${omr_SOURCE_DIR}/compiler/p/env/OMRDebugEnv.cpp
	)
endif()

if(OMR_ARCH_X86 OR OMR_ARCH_S390)
	set(OMR_ENHANCED_WARNINGS OFF)
else()
	set(OMR_WARNINGS_AS_ERRORS OFF)
	set(OMR_ENHANCED_WARNINGS OFF)
endif()

# Workaround for deprecated sprintf
if(OMR_OS_OSX)
	set(OMR_WARNINGS_AS_ERRORS OFF)
endif()

# Create omrgen library with "minimal" optimizer to reduce library size
create_omr_compiler_library(
	NAME     jb2omrcompiler
	OBJECTS  ${OMRCOMPILER_OBJECTS}
	DEFINES  PROD_WITH_ASSUMES JITTEST OMR_OPTIMIZER_SMALL
	INCLUDES ${JB2_CORE_DIR}
)

# Export paths for dependent objects
make_compiler_target(jb2omrcompiler INTERFACE COMPILER jb2omrcompiler)

target_link_libraries(jb2omrgen PUBLIC jb2vm jb2 jb2omrcompiler)

add_subdirectory(test)
