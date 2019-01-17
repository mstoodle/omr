/*******************************************************************************
 * Copyright (c) 2017, 2019 IBM Corp. and others
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

#if (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX)
#include <sys/mman.h>
#if defined(__APPLE__) || !defined(MAP_ANONYMOUS)
#define MAP_ANONYMOUS MAP_ANON
#endif
#elif defined(OMR_OS_WINDOWS)
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#else
#include <string.h>
#endif /* defined(OMR_OS_WINDOWS) */

#include <stddef.h>  // for size_t
#include <cstdlib>   // for free, malloc
#include <new>       // for bad_alloc, nothrow, nothrow_t
#include "infra/Assert.hpp"
#include "env/newmemory/DebugAllocator.hpp"

void *
OMR::DebugAllocator::allocate(size_t size, const std::nothrow_t tag,void * hint) const throw()
   {
#if (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX)
   void *p = mmap(NULL, size, PROT_READ | PROT_WRITE, MAP_ANONYMOUS | MAP_PRIVATE, -1, 0);
   return (p == MAP_FAILED) ? NULL : p;
#elif defined(OMR_OS_WINDOWS)
   return  VirtualAlloc(NULL, size, MEM_COMMIT | MEM_RESERVE, PAGE_READWRITE);
#else
   return this->OMR::MallocAllocator::allocate(size, tag, hint);
#endif /* (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX) */
   }

void
OMR::DebugAllocator::deallocate(void* p) const throw()
   {
#if (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX)
   TR_ASSERT(0, "DebugAllocator does not support deallocation without size parameter on this platform");
#elif defined(OMR_OS_WINDOWS)
   VirtualFree(p, 0, MEM_RELEASE);
#else
   this->OMR::MallocAllocator::deallocate(p);
#endif /* (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX) */
   }

void
OMR::DebugAllocator::deallocate(void *p, size_t size) const throw()
   {
#if (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX)
   munmap(p, size);
#else
   deallocate(p); // other platforms can deallocate without size
#endif /* (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX) */
   }

void
OMR::DebugAllocator::noLongerUsed(void *p, size_t size) const throw()
   {
#if (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX)
   void * remap = mmap(p, size, PROT_NONE, MAP_ANONYMOUS | MAP_PRIVATE | MAP_FIXED, -1, 0);
   TR_ASSERT(remap == p, "Remapping of memory failed!");
#elif defined(OMR_OS_WINDOWS)
   VirtualFree(p, size, MEM_DECOMMIT);
   VirtualAlloc(p, size, MEM_COMMIT, PAGE_NOACCESS);
#else
   memset(p, 0xEF, size);
#endif /* (defined(LINUX) && !defined(OMRZTPF)) || defined(__APPLE__) || defined(_AIX) */
   }
