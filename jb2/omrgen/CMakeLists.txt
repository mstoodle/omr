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

# Build jb2omrcompiler
add_subdirectory(compiler)

add_library(jb2omrgen SHARED
	OMRCodeGeneratorExtensionAddon.cpp
	OMRCodeGeneratorForBase.cpp
	OMRCodeGeneratorForCore.cpp
	OMRCodeGeneratorForFunc.cpp
	OMRCodeGeneratorForVM.cpp
	OMRExtension.cpp
)

target_include_directories(jb2omrgen PRIVATE
	${JB2_CORE_DIR}
	${JB2_CORE_DIR}/omrgen/compiler
	${omr_SOURCE_DIR}/compiler
)

# Need this so OMRGen can identify VM extension
target_link_libraries(jb2omrgen jb2vm jb2 jb2omrcompiler)

add_subdirectory(test)
