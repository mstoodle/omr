<!--
Copyright (c) 2017, 2018 IBM Corp. and others

This program and the accompanying materials are made available under
the terms of the Eclipse Public License 2.0 which accompanies this
distribution and is available at https://www.eclipse.org/legal/epl-2.0/
or the Apache License, Version 2.0 which accompanies this distribution and
is available at https://www.apache.org/licenses/LICENSE-2.0.

This Source Code may also be made available under the following
Secondary Licenses when the conditions for such availability set
forth in the Eclipse Public License, v. 2.0 are satisfied: GNU
General Public License, version 2 with the GNU Classpath
Exception [1] and GNU General Public License, version 2 with the
OpenJDK Assembly Exception [2].

[1] https://www.gnu.org/software/classpath/license.html
[2] http://openjdk.java.net/legal/assembly-exception.html

SPDX-License-Identifier: EPL-2.0 OR Apache-2.0 OR GPL-2.0 WITH Classpath-exception-2.0 OR LicenseRef-GPL-2.0 WITH Assembly-exception
-->

# OMR Compiler Memory Manager

The OMR Compiler doesn't necessarily operate in a vacuum, but rather 
as one component of a Managed runtime. Additionally,  communications 
between components could be using C linkage. Therefore, one can't simply 
use something like `::operator new` since it can throw. Additionally,
any generic common allocator would have poor interaction between 
components, with little to no insight into usage.

Using an allocator specific to a Managed Runtime, such as 
`omrmem_allocate_memory` in the Port Library, is expensive 
(both in terms of memory footprint and execution time) 
for many of the types of allocations performed 
by the Compiler; there are too many participants for informal trust, 
book-keeping is expensive, and optimizing for divergent component 
use-cases is difficult to impossible.

Therefore, the compiler manages its own memory.

## Compiler Memory Manager Hierarchy

The compiler memory management implementation is divided into two
distinct levels: the low level allocators that allocate (and return)
typically "large" TR::MemorySegments, and the high level allocators
that provide allocation support for STL containers as well as bulk
free and region based allocation.

All the classes described in this document reside within the
`compiler/env` directory of the project. Many of the classes
are implemented to be *extensible* meaning that downstream
projects can either reuse the OMR implementations of these classes
(e.g. the `jitbuilder` project reuses the OMR implementations
completely), or can extend the OMR project's implementations
(don't have a current example of this, but not hard to imagine),
or can provide their own to replace the OMR implementations
(Eclipse OpenJ9 examplifies this approach).

The remainder of this document describes the OMR implemtation of the
two levels of memory allocator management in the compiler.

### Low Level Allocators
Low level allocators interface directly with whatever memory provider
is available outside of the JIT. In a language runtime environment,
that may mean allocating memory from services that track memory allocated
by the virtual machine. Or, the memory provider can simply be "malloc".
At its lowest level, the OMR memory allocator currently employs the
system equivalent of malloc/mmap.

There are three fundamental pieces to the lowest memory allocation
layer: `TR::RawAllocator`, `TR::BackingMemoryAllocator`, and
`TR::SegmentAllocator`. Each of these classes builds upon the
earlier classes: a `TR::BackingMemoryAllocator` object is constructed
by providing a `TR::RawAllocator` for it to use, and a
`TR::SegmentAllocator` object is constructed by providing a
`TR::BackingMemoryAllocator` for the segment allocator to use.

This diagram shows how these three classes fit together:
```
              +---------------------------------+
              | +-----------------------------+ |
              | |    +-------------------+    | |
              | |    |                   |    | |
              | |    | &TR::RawAllocator |    | |
              | |    |                   |    | |
              | |    +-------------------+    | |
              | |                             | |
              | | &TR::BackingMemoryAllocator | |
              | |                             | |
              | ------------------------------+ |
              |                                 |
              |      TR::SegmentAllocator       |
              |                                 |
              +---------------------------------+
```

As you can see, a `TR::SegmentAllocator` object contains a reference
to a `TR::BackingMemoryAllocator` object, which itself contains a
reference to a `TR::RawAllocator` object. The lifetimes of these objects
are therefore clear: the raw allocator must outlive the other two, and
the backing memory allocator must outlive the segment allocator. Only
one segment allocator can use a backing memory allocator at a time,
though it is technically possible for a second segment allocator to
be allocated that reuses a backing memory allocator that had been used
by an earlier segment allocator, so long as the earlier segment allocator
was properly destroyed so that all of the backing memory segments it
allocated would have been deallocated.

For diagnosing certain kinds of memory bugs, it is useful to know when
a memory area is accessed after it was freed by the compiler. For this
kind of diagnosis, there are `Debug` subclasses of `TR::RawAllocator`
and `TR::SegmentAllocator` called, respectively, `TR::DebugRawAllocator`
and `TR::DebugSegmentAllocator`. These classes fit together in the 
same way as the default classes:

```
              +---------------------------------+
              | +-----------------------------+ |
              | |  +------------------------+ | |
              | |  |                        | | |
              | |  | &TR::DebugRawAllocator | | |
              | |  |                        | | |
              | |  +------------------------+ | |
              | |                             | |
              | | &TR::BackingMemoryAllocator | |
              | |                             | |
              | +-----------------------------+ |
              |                                 |
              |    TR::DebugSegmentAllocator    |
              |                                 |
              +---------------------------------+
```

The full set of classes used for lower level memory allocation, along
with their relationships, is shown below:


```
RawAllocator classes:
+----------------------------+         +-----------------------------+
|                            |  using  |                             |
|     TR::RawAllocator       +<--------+     OMR::RawAllocator       |
|                            |         |                             |
+-------------+--------------+         +-----------------------------+
              ^                extends                
              +---------------------------------------+
                                                      |
+----------------------------+         +--------------+--------------+
|                            |  using  |                             |
|   TR::DebugRawAllocator    +<--------+   OMR::DebugRawAllocator    |
|                            |         |                             |
+----------------------------+         +-----------------------------+


BackingMemoryAllocator classes:
+----------------------------+         +-----------------------------+
|                            |  using  |                             |
| TR::BackingMemoryAllocator +<--------+ OMR::BackingMemoryAllocator |
|                            |         |                             |
+----------------------------+         +-----------------------------+


SegmentAllocator classes:
+----------------------------+         +-----------------------------+
|                            |  using  |                             |
|    TR::SegmentAllocator    +<--------+ OMR::SegmentAllocator       |
|                            |         |                             |
+-------------+--------------+         +-----------------------------+
              ^                extends
              +---------------------------------------+
                                                      |               
+----------------------------+         +--------------+--------------+
|                            |  using  |                             |
| TR::DebugSegmentAllocator  <---------+ OMR::DebugSegmentAllocator  |
|                            |         |                             |
+----------------------------+         +-----------------------------+
```

More detailed documentation for these classes follows.

#### `TR::RawAllocator`

The lowest level allocator is referred to as a RawAllocator and
is an extensible class. `OMR::RawAllocator` is defined in
`compiler/env/mem/RawAllocator.hpp` and is mapped directly into
the `TR` namespace. `RawAllocator` is expected to directly allocate
memory of the exact size needed and to free that same memory without
caching.  The main public API to `RawAllocator` are these 3 calls:

* `allocate(size_t requestedSize)` allocates memory that can hold
  `requestedSize` bytes. The return value is of type
  `RawAllocator::RawSegment` which is really `void *`
* `deallocate(RawSegment &segment)` frees memory previously
  allocated by `allocate()`
* `protect(RawSegment &segment)` is intended to remove read access
  to the segment previously allocated by `allocate()`. By default,
  this function does nothing, but see `TR::DebugRawAllocator` to
  better understand the purpose of this function. It is implemented
  at the `TR::RawAllocator` level as a virtual call so that
  `TR::DebugRawAllocator` can override its behaviour.

#### `TR::DebugRawAllocator`

This class, found in `compiler/env/mem/DebugRawAllocator.hpp`,
extends `TR::RawAllocator` to provide a diagnostic
memory allocator that does not reuse allocated memory regions
and provides an implementation of the `protect()` function call
so that the debug segment allocator can remove read access to
allocated memory regions once the compiler claims to be freeing
the region. It is used to detect later reuses of freed memory.
Rather than using `malloc` and `free` like the base raw allocator,
this allocator uses the platform equivalent of `mmap` to allocate
memory in order to be able to adjust its access bits.

Details can be found in `compiler/env/mem/DebugRawAllocator.hpp`,
which would probably better be implemented via the port library.
Platforms that do not provide this facility simply delegate
all functions to the base `RawAllocator` functions which means
that the compiler will still operate correctly, but no memory
reuse-after-free will be detected via faults.

This class also overrides the `protect()` function:

* `potect(RawSegment &segment)` removes the "read" access to a
  memory segment previously returned by `allocate()` so that
  any subsequent access will fault.


#### `TR::BackingMemoryAllocator`

The primary purpose of the backing memory allocator is to 
cause the compiler to allocate relatively large and consistently
sized memory regions from its surrounding environment.
Controlling allocations in this way can help to reduce fragmentation
and simplifies core memory reuse without managing free lists (the
higher level memory allocators do employ free lists to manage
smaller chunks of memory).

Each backing memory allocator uses a `TR::RawAllocator` to allocate
memory when needed. Each such memory chunk is represented by
a `TR::MemorySegment`. This `TR::MemorySegment` object is allocated
directly using the raw allocator which must also be freed when
the segment is deallocated. To ensure no memory leaks, the
backing memory allocator maintains an `std::deque` with references
to the memory segments that have been returned from `allocate()`.

All requested backing memory allocations (i.e. calls to
`allocate(size_t requestedSize)`) will round up the allocation size
to a multiple of the default segment allocation size to reduce
fragmentation from the raw allocator. For example, the default
method compilation requests will construct a backing memory allocator
with a default segment size of 64KB (`1 << 16`), which means all 
allocations from the backing memory allocator will rounded up to
a multiple of 64KB.

The backing memory allocator always returns memory to the raw
allocator when `deallocate()` is called. There is no attempt
to reuse segments.

When the backing memory object is destructed, it will first free
all the `TR::MemorySegment`s as well as the raw memory each of
those segments represents. That memory can also be returned
immediately by calling `deallocateSegments()`.

The backing memory allocator tracks how much memory has been allocated
via `_bytesAllocated` which can be queried via `bytesAllocated()`. The
backing memory allocator can also be directed to enforce an
allocation limit via a constructor option which sets the `_allocationLimit`
field. By default, the backing memory allocator will not limit
consumption. If a limit has been set (_allocationLimit > 0) then any
allocation that would cause the total allocated memory to exceed
that limit will be rejected (i.e. `allocate()` will throw
`std::bad_alloc()` ).

The backing memory allocator has a diagnostic logging facility that
uses macros like `MEMLOG`. Because the lowest level memory allocator
layers are so fundamental in the compiler, one cannot trust that
the higher level logging and options processing functions are
available yet, so this facility relies only on `printf` and
environment variables. The options availble for this facility
are documented at the top of
`compiler/env/mem/OMRBackingMemoryAllocator.cpp`. The level of
detail can be set via the environment variable
`OMRDebug_BackingMemory=<number>`. Turning on the debug facility
will send trace output to `stdout` prefixed by the string `BckMem %p`
where `%p` is the address of the `TR::BackingMemoryAllocator` object.

#### `TR::SegmentAllocator`

The segment allocator is the interface used by the higher level memory
allocation functions. Like the backing memory allocator, the
segment allocator returns memory wrapped in `TR::MemorySegment`s, but
the segment allocator can allocate and will try to reuse smaller
segment sizes. The backing memory allocator can be configured to
allocate memory in 1MB segments, for example, but the segment allocator
can then carve up those larger segments into smaller (64KB, say)
segments to satisfy allocation requests made by the higher level
memory allocation layer.

So one key new feature of the segment allocator is that it can
carve up the larger backing memory segments into smaller segments.
The segment allocator maintains its own allocation block size
(`_allocationBlockSize`).

A second new feature is that the segment allocator will try to
reuse segments that have been passed to `deallocate()` without
returning them to the backing memory allocator. To avoid the
overhead of OS level memory allocators, the segment allocator
tries hard not to allocate more memory (with the consequence
that it will hold onto allocated memory longer than strictly
required by the compiler).

This reuse has one restriction to simplify the reuse algorithm:
only segments smaller than the default segment size (which will
be rounded up to a multiple of the default segment size) can
be reused. That means that larger "one-off" allocations will be
satisfied by requesting a dedicated segment from the backing
memory allocator, and such a segment will be immediately sent
to the backing memory allocator to be deallocated when the
compiler asks the segment allocator to deallocate it.

To facilitate reuse, the segment allocator maintains a list of
free segments, which are `TR::MemorySegment`s that have already
been cut up into the default allocation size. Because all
allocation requests are rounded up to this size anyway (except
for larger memory requests), any segment on this free list can
be immediately returned to satisfy any "regular" request, and
the segment allocator prefers to use such a free segment if
one is available.

If no such free segment is available, the next possible source
of memory is _currentBackingMemoryChunk, which is the last
`TR::MemorySegment` allocated by the backing memory allocator
that has not been fully consumed yet. If the request can
be satisfied from this memory segment, then a new
`TR::MemorySegment` will be created (using the raw allocator)
to represent this new memory segment, and it will be returned
to the compiler. Note that this memory request can be larger
than the default allocation size, but must be smaller than
the default allocation size of the backing memory chunk(in order
to be allocated from the backing memory chunk). This newly
allocated segment will also be added to the list of allocated
segments (`_allocatedSegments`) so that the segment allocator
can always return these `TR::MemorySegment` memory blocks to
the raw allocator if the segment allocator object is destroyed.

If the current backing memory chunk cannot satisfy the request,
then a new backing memory allocation request will be needed
to provide this memory to the compiler. There are two scenarios
here: 1) the memory request is a large memory request that could
never be satisfied by a default-sized backing segment, or 2) the
memory request is a "regular" one but the current backing memory
chunk has simply been exhausted.

In the first case, a dedicated memory segment will be allocated
and directly returned to the compiler.

In the second case, a new backing memory chunk will need to
be allocated and will replace the one stored in
`_currentBackingMemoryChunk`. If the current backing segment has
more free memory than the default allocation block size (which
can happen if a request for a multiple of the allocation block
size is made), then the remaining free memory will be first carved
up into default sized segments that are placed onto the free
list. Once a new backing memory chunk has been allocated, the
memory request will be directly fulfilled by allocating a
`TR::MemorySegment` for the needed size and consuming that much
memory from the new (now current) backing memory chunk.

When the compiler calls `deallocate()` to hand a segment back
to the segment allocator, the segment will be placed on the
free list if possible. If the segment is not a default allocation
size (according to the segment allocator), then either it will be
returned directly to the backing memory allocator (if the size is
larger even than the default allocation size of the backing memory
allocator) or it will be carved up into default sized segments
and placed onto the free list for later reuse.

The segment allocator also  has a diagnostic logging facility that
uses macros like `MEMLOG`. Because the lowest level memory allocator
layers are so fundamental in the compiler, one cannot trust that
the higher level logging and options processing functions are
available yet, so this facility relies only on `printf` and
environment variables. The options availble for this facility
are documented at the top of
`compiler/env/mem/OMRSegmentAllocator.cpp`. The level of
detail can be set via the environment variable
`OMRDebug_SegmentAllocator=<number>`. Turning on the debug facility
will send trace output to `stdout` prefixed by the string `SegAll %p`
where `%p` is the address of the `TR::SegmentAllocator` object.

#### `TR::DebugSegmentAllocator`

The debug segment allocator is a subclass of the segment allocator
that overrides just the `allocate()` and `deallocate()` functions
so that no segment caching or actual deallocation occurs.

That means that `allocate()` will always allocate a segment of a
size rounded up to a multiple of the default allocation size (for
consistency with how the non-debug version works). When the compiler
calls `deallocate()` on a segment, rather than freeing the segment
it simply uses the debug raw allocator to `protect()` the segment
so that any subsequent access to the segment will fault.

Since the debug segment allocator simply reuses segments returned
by the backing memory allocator directly, it does not need to
track any segments or create new `TR::MemorySegment` objects; it
really becomes just a wrapper around the
`TR::BackingMemoryAllocator`.


### High level Allocators
High Level Allocators can be used by STL containers by wrapping them
in a `TR::typed_allocator` (see below). They generally use a Low 
Level Allocator as their underlying provider of memory.

#### TR::Region
`TR::Region` facilitates Region Semantics. It is used to allocate 
memory that exists for its lifetime. It frees all of its memory when 
it is destroyed. It uses `TR::SegmentProvider` as its backing provider 
of memory. It also contains an automatic conversion (which wraps it 
in a `TR::typed_allocator`) for ease of use with STL containers.

#### TR::StackMemoryRegion
`TR::StackMemoryRegion` extends `TR::Region`. It is used to facilitate 
Stack Mark / Stack Release semantics. A Stack Mark is implemented by 
constructing the object, at which point it registers itself with the 
`TR_Memory` object (see below), and saves the previous 
`TR::StackMemoryRegion` object. A Stack Release is implemented by 
destroying the object, at which point it unregisters itself with the 
`TR_Memory` object, restoring the previous `TR::StackMemoryRegion` 
object. `TR_Memory` is then used to allocate memory from the region.
The use of this object **must** be consistent; it must either 
be used for Stack Mark / Stack Release via `TR_Memory`, OR, it must 
be used as a Region (though at that point, `TR::Region` should be used). 
Interleaving Stack Mark / Stack Release and Region semantics **will** 
result in invalid data.

#### TR::PersistentAllocator
`TR::PersistentAllocator` is used to allocate memory that persists 
for the lifetime of the Managed Runtime. It uses `TR::RawAllocator` 
to allocate memory. It receives this allocator via the 
`TR::PersistentAllocatorKit` (see below). It also contains 
an automatic conversion (which wraps it in a `TR::typed_allocator`) 
for ease of use with STL containers.

#### TR_TypedPersistentAllocator
`TR_TypedPersistentAllocator` is a wrapper around `TR_PersistentMemory`.
Its purpose is to allow the compiler to track memory per type (as 
defined in `TR_MemoryBase::ObjectType`) when using STL containers for
persistent data types; it contains an automatic conversion (which 
wraps it in a `TR::typed_allocator`) for ease of use with STL containers.

For example, given:
```
class Test
   {
public:
   Test(uintptr_t f1, uintptr_t f2)
      : _field1(f1),
        _field2(f2)
      { }

   uintptr_t _field1;
   uintptr_t _field2;
   };
```
1. Add (or replace the existing `TR_ALLOC` with) `TR_ALLOC_SPECIALIZED` 
with the appropriate `TR_MemoryBase::ObjectType` enum value to the 
class definition
2. Declare and use the container, eg:
```
typedef TR::list<Test, Test::TrackedPersistentAllocator> TestList;
TestList myList(getTypedAllocator<Test, Test::TrackedPersistentAllocator>(Test::getPersistentAllocator()));
```
If the desire is to allocate the container using persistent memory 
(ie not a local on the stack), then:
```
typedef TR::list<Test, Test::TrackedPersistentAllocator> TestList;
void *storage = TR_Memory::jitPersistentAlloc(sizeof(TestList));
TestList *myList = new (storage) TestList(getTypedAllocator<Test, Test::TrackedPersistentAllocator>(Test::getPersistentAllocator()));
```

#### TR::RawAllocator
`TR::RawAllocator` uses `malloc` (or equivalent) to allocate memory. 
It contains an automatic conversion (which wraps it in a 
`TR::typed_allocator`) for ease of use with STL containers.

#### TR::Allocator
`TR::Allocator` is an allocator provided by the CS2 library. 
Use of `TR:Allocator` is strongly discouraged; it only exists for 
legacy purposes. It will quite likley be removed in the 
future. It contains an automatic conversion (which wraps it in a 
`TR::typed_allocator`) for ease of use with STL containers.

## Other Compiler Memory Manager Related Concepts

#### TR::typed_allocator
`TR::typed_allocator` is used to wrap High Level 
Allocators such as `TR::Region` in order to allow them to 
be used by STL containers.

#### TR::RegionProfiler
`TR::RegionProfiler` is used to profile a `TR::Region`. 
It does so via Debug Counters.

#### TR::PersistentAllocatorKit
`TR::PersistentAllocatorKit` contains data that is used 
by the `TR::PersistentAllocator`.

#### TR::MemorySegment
`TR::MemorySegment` is used to describe a chunk, or segment, 
of memory. It is also used to carve out that segment into smaller 
pieces of memory. Both Low and High Level Allocators above describe and 
subdivide the memory they manage using `TR::MemorySegment`s.

#### TR_PersistentMemory
`TR_PersistentMemory` is an object that is used to allocate memory 
that persists throughout the lifetime of the runtime. It uses 
`TR::PersistentAllocator` to allocate memory. For the most part, 
it is used in conjunction with `TR_ALLOC` (and related macros 
described below). It is also used as a substitute for `malloc` in 
legacy code where memory is still allocated and freed explicitly. 
Generally, newer code should favour using `TR::PersistentAllocator` 
instead of `jitPersistentAlloc`/`jitPersistentFree`.

#### TR_Memory
`TR_Memory` is a legacy object that was used as the interface by 
which memory could be allocated. Each `TR::Compilation` contains 
a `TR_Memory` object. `TR_Memory` is similar to `TR_PersistentMemory` 
in that it is used in conjunction with `TR_ALLOC` (and related 
macros described below), as well as is used as the means to allocate 
and free memory explicity. Generally, newer code should favour 
using `TR::Region`.

#### TR_ALLOC, TR_PERSISTENT_ALLOC, TR_PERSISTENT_ALLOC_THROW, TR_ALLOC_WITHOUT_NEW
These are macros that are placed inside a class' definition. 
They are used to define a set of `new`/`delete` operators, as 
well as to add the `jitPersistentAlloc`/`jitPersistentFree` methods 
into the class. They exist to help keep track of the memory 
allocated by various objects, as well as to ensure that the 
`new`/`delete` operators are used in a manner that is consistent 
with the Compiler memory management.

## How the Compiler Allocates Memory

The Compiler deals with two categories of allocations:
1. Allocations that are only useful during a compilation
2. Allocations that need to persist throughout the lifetime of the Managed Runtime

### Allocations only useful during a compilation
The Compiler initializes a `OMR::SystemSegmentProvider` 
(or `OMR::DebugSegmentProvider`) local object at the start of a 
compilation. It then initializes a local `TR::Region` object, and a 
local `TR_Memory` object which uses the `TR::Region` for general 
allocations (and sub Regions), as well as for the first 
`TR::StackMemoryRegion`. At the end of the compilation, the 
`TR::Region` and `OMR::SystemtSegmentProvider` 
(or `OMR::DebugSegmentProvider`) go out of scope, invoking their 
destructors and freeing all the memory.

There are a lot of places (thanks to `TR_ALLOC` and related macros) 
where memory is explicity allocated. However, `TR::Region` 
should be the allocator used for all new code as much as possible.

### Allocations that persist for the lifetime of the Managed Runtime
The Compiler initializes a `TR::PeristentAllocator` object when 
it is first initialized (close to bootstrap time). For the most 
part it allocates persistent memory either directly using the global 
`jitPersistentAlloc`/`jitPersistentFree` or via the object methods 
added through `TR_ALLOC` (and related macros). Again, 
`TR::PersistentAllocator` or `TR_TypedPersistentAllocator` should 
be the allocator used for all new code as much as possible.


## Subtleties and Miscellaneous Information

:rotating_light:`TR::Region` allocations are untyped raw memory. In order to have a region
destroy an object allocated within it, the object would need to be created
using `TR::Region::create`. This requires passing in a reference to an
existing object to copy, which requires that the object be
copy-constructable. The objects die in LIFO order when the Region is destroyed.:rotating_light:

`TR::PersistentAllocator` is not a `TR::Region`. That said, if one wanted
to create a `TR::Region` that used `TR::PersistentAllocator` as its backing
provider, they would simply need to extend `TR::SegmentProvider`, perhaps
calling it `TR::PersistentSegmentProvider`, and have it use
`TR::PersistentAllocator`. The `TR::Region` would then be provided this 
`TR::PersistentSegmentProvider` object.


