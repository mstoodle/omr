# JBIR: the jb2 IR Serialization Format

Status: **Draft**, revision 1 (2026-11-05)
Format version defined here: **2.0.0** (see [Open points](#15-open-points))

## 1. Introduction

This document specifies JBIR, the format used to serialize a jb2 `IR` object
and later reconstruct an equivalent `IR` object in a possibly different
`Compiler`, possibly in a different process, possibly on a different platform.

### 1.1 Goals

1. **Complete.** A JBIR file holds everything contained by an `IR` object:
   types, literals, symbols, locations, contexts, scopes, entry points,
   builders, values, operations, and every element category contributed by
   extensions. It does not hold the `CompileUnit`, the `Compilation`, or any
   pass-local state.
2. **Coherent.** Deserializing a file produces a fully coherent `IR` object
   that can be injected into a compilation at any point where an `IR` can be
   observed: after `buildIL`, between any two passes, or (in principle) in
   the middle of a pass.
3. **Independent of identifiers.** A reader never assumes that any
   identifier (`ExtensionID`, `ActionID`, `TypeID`, `KindService` kind, ...)
   in the writing `Compiler` has the same value in the reading `Compiler`.
   Every identifier in a file is file-local, and every reference to something
   that lives outside the file is made by globally unique name.
4. **Extensible as a first-class facility.** Extensions can contribute new
   element *kinds* within existing categories (a new `Operation` or `Type`)
   and entirely new element *categories* (e.g. Base's `Case`). The core IR
   elements are defined by `CoreExtension` through exactly the same mechanism,
   with no special status in the format.
5. **Self-describing.** A file carries the schema of every element kind it
   uses, so a file can be validated and printed by a tool that has none of
   the defining extensions loaded, on any platform.
6. **One abstract model, two encodings.** The content of a file is defined
   once, abstractly (sections 3 to 9). A normative binary encoding
   (section 10) and a normative text encoding (section 11) are both
   mechanical renderings of that model. Neither encoding requires per-kind
   code beyond the kind's schema.

### 1.2 Non-goals

- Executability across platforms. A file must be *readable* (decodable,
  validatable and printable) on any platform, but an IR deserialized on a
  platform different from the writer's is not guaranteed to compile or
  behave identically (section 4).
- Serializing `Addon`s. Addons are created and maintained by the extensions
  loaded in the reading `Compiler`, never by the deserializer (section 8.4).
- Serializing compiler-implementation provenance. `CreateLocation` values
  (the jb2 source location that created an element) are never written.
- Preserving numeric IDs. The reader assigns fresh IDs; only relative order
  is preserved, and only where section 8.3 says so.

### 1.3 Conventions

The key words MUST, MUST NOT, SHOULD, SHOULD NOT and MAY are to be
interpreted as in RFC 2119.

*Writer* means the code that serializes an IR from the *writing Compiler*.
*Reader* means the code that deserializes a file into a new IR in the
*reading Compiler*. *File* means any JBIR byte or text stream; nothing in
this specification requires a file system.

Indices written `@n` are file-local element indices (section 3.3).

## 2. Model overview

A JBIR file is, abstractly, the following sequence:

| # | Part | Purpose |
|---|------|---------|
| 1 | Header | Magic number, byte order, format version (section 10.1) |
| 2 | Platform | Properties of the writing platform (section 4) |
| 3 | Extension table | Every extension the file depends on, with version (section 5.1) |
| 4 | Category table | Every element category the file uses (section 5.2) |
| 5 | Kind table | Every element kind the file uses, with its schema (section 5.3) |
| 6 | Element declarations | For each category, every element: its kind and declaration mode (section 6) |
| 7 | Element bodies | Field values for every *constructed* element (section 7) |
| 8 | Root | The IR's top-level scope and context (section 7.4) |

Parts 3 to 5 together form the **dictionary**. The dictionary is everything
a reader needs to map the file's local identifiers onto the live objects of
the reading `Compiler`, and everything a generic tool needs to decode the
remainder of the file.

## 3. Core concepts

### 3.1 Extensions

Every category, kind and named element in a file is owned by exactly one
extension. Extensions are identified by their name (`Extension::NAME`, e.g.
`"core"`, `"jb2base"`, `"jb2func"`). Extension names MUST be globally unique.

### 3.2 Categories

A **category** is a family of IR elements that share an identifier space and
can be referenced uniformly: `Type`, `Operation`, `Builder`, `Case`, and so on.
A category has:

- a globally unique **name**;
- an owning **extension**;
- a **scope**, either `compiler` or `ir`:
  - `compiler`-scoped categories contain entities registered with the
    `Compiler` rather than created in an IR (e.g. `Action`). Their elements
    can only be *bound* (section 6.1), never constructed;
  - `ir`-scoped categories contain elements that belong to an `IR` object;
- a list of **common fields** (section 3.5) that every kind in the category
  carries before its kind-specific fields. The list may be empty.

A category corresponds to one identifier space in the implementation (e.g.
`TypeID`, `OperationID`, `CaseID`). The owning extension is responsible for
allocating IDs in that space in the reading IR.

### 3.3 Elements and file indices

An **element** is one object in a category. Within a file, every element of
category *C* has a **file index** in the range `1..N(C)`, where `N(C)` is the
number of elements of *C* declared in the file. Indices are dense and
per-category. Index `0` is reserved and means "no element" (null) wherever
a reference is allowed to be null.

File indices have no relationship to the element's ID in either the writing
or the reading `Compiler`.

### 3.4 Kinds

A **kind** is a concrete class of element within a category, e.g.
`Op_Add` in `Operation`, `PointerType` in `Type`, `LocalSymbol` in `Symbol`.
A kind has:

- a **name**, which MUST be the class's `KindService` name and MUST be
  globally unique;
- an owning **extension** (which may differ from the category's owner);
- its **category**;
- a **construction schema**: the ordered fields needed to create the
  element;
- a **completion schema**: the ordered fields applied after the element
  exists. These may refer to elements that, directly or indirectly, refer
  back to this element;
- for kinds in the `Type` category only, a **literal schema**: the ordered
  fields that encode the value of a `Literal` of that type (section 7.3).

Each schema is the category's common fields for that phase, followed by the
kind's own fields. Inherited C++ structure (`Op_Add : OperationR1V2 :
OperationR1 : Operation`) is flattened: a kind's schema lists every field
the kind needs, in order.

Abstract classes (`Symbol`, `OperationR1V2`, `BaseType`, ...) are never
kinds. Only classes of which elements actually exist appear in a file.

### 3.5 Fields and field types

A **field** is a `(name, type)` pair. Field names are identifiers matching
`[A-Za-z_][A-Za-z0-9_]*` and MUST be unique within a schema. Field types
are:

| Type | Meaning |
|------|---------|
| `bool` | true or false |
| `i8` `i16` `i32` `i64` | two's-complement signed integer |
| `u8` `u16` `u32` `u64` | unsigned integer |
| `f32` `f64` | IEEE-754 binary32 / binary64 |
| `string` | UTF-8 text; may be empty |
| `bytes` | opaque byte sequence; may be empty |
| `ref<C>` | reference to an element of category *C* by file index; `0` = null |
| `list<T>` | ordered sequence of values of type *T* |
| `record(f1: T1, ...)` | anonymous fixed sequence of named fields |

Types nest arbitrarily (`list<record(name: string, type: ref<Type>, offset: u64)>`).

A `ref<C>` field may additionally be annotated `kind=K`, which constrains
the referenced element to be of kind *K* or (for kinds that model a class
hierarchy) a kind the schema author designates. The reader MUST enforce
`ref<C>` category constraints. Kind annotations are advisory for generic
tools and MUST be enforced by the defining extension's reader code.

### 3.6 Names

Several things in a file are matched by name against the reading `Compiler`:
extension names, category names, kind names, and bound element names
(section 6.1). All of these names MUST be globally unique within their
namespace (extensions, categories, kinds, and the elements of one category,
respectively) across every extension that can be loaded together. Names are
compared as exact byte sequences.

## 4. Platform

### 4.1 Platform record

The platform record describes the writing `Compiler`'s target platform:

| Field | Type | Meaning |
|-------|------|---------|
| `wordSize` | `u8` | `Compiler::platformWordSize()` in bits (e.g. 64) |

Further fields MAY be added in minor format revisions (section 12.1).

A reader MUST decode a file regardless of platform record values. If the
reading `Compiler`'s platform differs from the record, the reader MUST
report a *platform mismatch* to its client, which decides whether the
resulting IR may be compiled. Printing and validation MUST always be
possible.

All multi-byte numeric values in a binary file, *including literal values*,
use the file's byte order (section 10.1). A file is therefore decodable on
a host of either byte order.

### 4.2 Host addresses

This revision has no field type for host addresses and no mechanism for
resolving them in the reading process. Two pieces of IR state are
affected:

- `FunctionSymbol`'s entry point is written as a `bytes` field holding the
  pointer's bytes exactly as they appeared in the writing process. The
  reader installs those bytes unchanged, and they are only meaningful in
  the writing process. Like a `Literal` (section 7.3), the field is a plain
  value with no special resolution behaviour.
- `NativeEntry` elements are not serialized. A `NativeEntry` only exists
  once the IR has been compiled successfully, so it records the result of
  a compilation rather than IR content. A writer MUST silently omit every
  `NativeEntry` element. It MUST also omit every `Scope.entries` record that
  refers to one, and MUST NOT treat their presence as an error. An IR read
  from such a file has no native entries, exactly like an IR that has not
  yet been compiled.

Environment-dependent values are expected to be handled in a future
revision (section 15).

## 5. Dictionary

### 5.1 Extension table

An ordered list of entries. Entry *i* (1-based) defines file extension
index `@i`:

| Field | Type |
|-------|------|
| `name` | `string` |
| `major` `minor` `patch` | `u16` each |
| `preRelease` | `string` |
| `buildMetadata` | `string` |

The writer MUST list every extension that owns at least one category, kind,
or element referenced anywhere in the file. The writer SHOULD list no others.
Entries MUST appear in the order in which the extensions were loaded into
the writing `Compiler`, so that a reader that loads missing extensions in
table order respects their load-time dependencies.

**Resolution.** For each entry, the reader looks up an extension of that
name in the reading `Compiler`. If none is loaded, the reader MAY load it
(`Compiler::loadExtensionByName`) if the client permits loading. The
default is to permit it. The loaded extension's version *L* is compatible
with the entry's version *F* if and only if:

- `L.major == F.major`, and
- `L.minor >= F.minor`, and
- if either version has a non-empty `preRelease`, then `L` and `F` are
  identical in major, minor, patch and preRelease.

`buildMetadata` is ignored for compatibility. If any entry cannot be
resolved to a compatible extension, deserialization fails (section 9).

Every listed extension is required. Because addons are not serialized,
there is no extension-private data a reader could skip.

### 5.2 Category table

An ordered list. Entry *i* defines file category index `#i`:

| Field | Type |
|-------|------|
| `name` | `string` |
| `extension` | file extension index |
| `scope` | `u8`: 0 = `compiler`, 1 = `ir` |
| `commonConstruction` | field list (section 5.4) |
| `commonCompletion` | field list |

The reader resolves each entry by name to a category registered in the
reading `Compiler` by the named extension. A category MUST be listed if it
has elements in the file or if any listed schema contains a `ref` to it.
Otherwise it SHOULD be omitted. The order of the table is the order in which the
reader processes categories (section 8.1). Writers SHOULD order categories
so that construction-phase references mostly point to earlier categories.
The core order in section 13.1 is recommended.

### 5.3 Kind table

An ordered list. Entry *i* defines file kind index `%i`:

| Field | Type |
|-------|------|
| `name` | `string` |
| `extension` | file extension index |
| `category` | file category index |
| `construction` | field list: the kind's own construction fields |
| `completion` | field list: the kind's own completion fields |
| `literal` | field list: present only for `Type`-category kinds; may be empty |

The full construction schema of an element is
`category.commonConstruction ++ kind.construction`. The same applies to
completion.

**Schema compatibility.** The reader compares each kind's file schema with
the schema of the same kind in the reading `Compiler`:

- If the file schema is identical to the reader's, the kind is compatible.
- If the file schema is a *strict prefix* of the reader's, field by field
  in each phase, the kind is compatible. Missing trailing fields take the
  defaults the reading extension defines for them. This is how minor
  versions add fields (section 12.2).
- Any other difference is a *schema mismatch*, and deserialization fails.

A reader whose extension defines a kind unknown to the file needs nothing:
the file simply contains no elements of that kind.

### 5.4 Field lists and type descriptors

A field list is an ordered list of `(name: string, type: typedesc)`. A
`typedesc` is one of the field types of section 3.5. `ref<C>` carries a file
category index. `list<T>` carries one nested `typedesc`. `record(...)`
carries a nested field list. Encodings are in sections 10.4 and 11.

## 6. Element declarations

For each category in the category table, in table order, the file declares
`N(C)` elements. Each declaration has a kind and one of three **modes**:

### 6.1 `bind name`

The element already exists in the reading `Compiler` (for `compiler`-scoped
categories) or in the newly created reading IR (for `ir`-scoped categories)
and is located by its globally unique name within the category. Examples:

- every `Action` element, e.g. `bind "Add"`;
- `Type` elements that every IR receives from the IR prototype, e.g.
  `bind "Int32"`, `bind "NoType"`.

A writer MUST use `bind` for every element of a `compiler`-scoped category.
It MUST use `bind` for an `ir`-scoped element exactly when that element is a
clone of an element of the writing `Compiler`'s IR prototype. A reader that
cannot find a bound element by name, or finds one whose kind differs from
the declared kind, MUST fail.

Bound elements have no body.

### 6.2 `construct`

The element is created by the reader from its body (section 7). This is the
mode for all IR content that was built for this IR.

### 6.3 `derive owner key`

The element is created as a side effect of constructing another element
(its **owner**, a `ref` to an element of any category) and is located
afterwards through the owner using `key` (a `string`). The canonical
example is a `FieldType`, which `StructTypeBuilder` creates while building
its `StructType`; it is declared as `derive @S "fieldName"`.

The owner's kind defines how `key` is interpreted. A derived element has no
body. The owner MUST be `construct`- or `bind`-mode; derivation chains are
not allowed. If the owner does not produce an element for the key, or
produces one of a different kind, the reader MUST fail.

## 7. Element bodies

### 7.1 Structure

Every `construct`-mode element has exactly one body. A body is the
element's construction-phase field values followed by its completion-phase
field values, each encoded in schema order. Bodies appear grouped by
category in category-table order and, within a category, in file-index
order, so a body's position identifies its element. Each body is
length-prefixed in the binary encoding (section 10.5).

### 7.2 Phase semantics

- **Construction fields** are the inputs the owning extension needs to
  create the element through its normal factory path. The graph whose
  edges are the `ref` values occurring in construction fields MUST be
  acyclic across the whole file. Writers MUST NOT produce files that
  violate this, and readers MUST detect a violation and fail.
- **Completion fields** are applied to an element after it exists. They may
  form arbitrary cycles (builder ↔ bound operation, builder ↔ child
  builders, and so on).
- **Binding point.** The reader MUST associate an element's file index with
  its live object no later than the moment the object's completion begins.
  For kinds built through a jb2 builder helper (`StructTypeBuilder`,
  `PointerTypeBuilder`, `FunctionTypeBuilder`), completion takes place
  *inside* the builder's helper callback. The helper binds the
  partially-built object first, then resolves completion references. This
  is how recursive types such as `struct S { S *next; }` are reconstructed
  using the same helper facility clients use to build them.

### 7.3 Literals

`Literal` (core) has construction fields `type: ref<Type>` and `value`.
`value` is not a single field type: it is encoded according to the
`literal` schema of the referenced type's **kind**, which the reader knows
from the declaration table without constructing the type. Examples:
`Int32Type` declares `literal = (v: i32)`. `Float64Type` declares
`(v: f64)`. `AddressType` and `PointerType` declare `(v: u64)`.
`StructType` declares `(bytes: bytes)`. A type kind whose values cannot be
literals declares an empty literal schema, and literals of that type are
invalid.

A `Literal` is a plain constant. Its value is reproduced exactly as
written, and the reader applies no platform-specific interpretation,
adjustment or resolution to it. In particular, an address-typed literal is
just the integer that was written. The reader MUST produce the in-memory
`LiteralBytes` in host byte order from the decoded value, which keeps
literals printable on any platform.

Values that must be adjusted for a different execution environment are
outside the scope of `Literal`. They are expected to be represented in a
future revision by a separate `RelocatableLiteral` kind (section 15).

### 7.4 Root

After all bodies, the root record:

| Field | Type |
|-------|------|
| `scope` | `ref<Scope>` |
| `context` | `ref<Context>` |

Either may be null (e.g. an IR serialized before `buildContext`).

## 8. Reconstruction semantics

### 8.1 Procedure

A conforming reader behaves as if it performed these steps in order:

1. Validate the header (section 10.1). Decode the platform record and
   record any platform mismatch (section 4.1).
2. Resolve the extension table (section 5.1), then the category table and
   the kind table (sections 5.2 and 5.3), checking schema compatibility.
3. Create a new IR with `Compiler::createIR()`, i.e. a clone of the reading
   `Compiler`'s IR prototype. All addons the loaded extensions attach to an
   IR are therefore created by those extensions.
4. Resolve every `bind` declaration.
5. **Construction.** For each category in table order, for each element in
   index order, ensure the element is constructed. To construct an element,
   first resolve every `ref` in its construction fields, constructing the
   referenced elements on demand and recursively. Then create the element
   through the owning extension's factory. After constructing an owner,
   resolve every `derive` declaration that names it.
6. **Completion.** For each category in table order, for each `construct`
   element in index order whose completion has not already happened inside
   a builder helper (section 7.2), apply its completion fields.
7. Install the root scope and context into the IR.
8. Invoke each loaded extension's *post-read* hook, in extension-table
   order. The hook allows the extension to validate cross-element invariants
   it owns and to adjust addon state that its factories could not maintain.
9. Validate the IR (section 9.2). On success, return it.

Any order of work that produces the same result is conforming.

### 8.2 Factory principle

Every element MUST be created through the same factory path the owning
extension uses when a client builds IR (`PointerTypeBuilder::create()`,
`Value::create()`, the operation's constructor plus
`Extension::addOperation()`, `BaseIRAddon::getCaseID()`, and so on). The
reader MUST NOT bypass factories by writing object state directly. This keeps
extension-maintained addon state (pointer and struct type caches, function
type maps, the `CaseID` counter, `IR::_builders`, `IR::_locations`, value
definition lists) correct by construction. Where an extension needs a
factory variant that accepts an already-created referent, it provides one.
Such variants are implementation detail and not part of this format.

### 8.3 Identifiers in the reading IR

Every reconstructed element receives a fresh ID from its category's normal
ID allocator in the reading IR. Within each category, a writer MUST assign
file indices in increasing order of the element's ID in the writing IR.
Elements constructed in step 5 without on-demand recursion therefore receive
IDs in the same relative order as in the writing IR. Relative order is
**not** guaranteed for elements constructed on demand.

### 8.4 Addons

Addons are never written. An IR returned by the reader has exactly the
addons the reading `Compiler`'s loaded extensions attach (section 8.1,
step 3), maintained by the factories (section 8.2) and the post-read hooks
(section 8.1, step 8).

### 8.5 Derived state

The following IR state is not written, because the factory principle
reconstructs it:

- each dictionary's (`TypeDictionary`, `LiteralDictionary`,
  `SymbolDictionary`) membership;
- `IR::_builders` and `IR::_locations`;
- each `Value`'s definition list. The reader constructs `Operation`
  elements in file-index order, and the writer's ID-ordered index
  assignment (section 8.3) guarantees that definitions are registered in the
  same relative order as in the writing IR.

Where a relationship is written on both sides for ordering reasons (e.g.
`Operation.parent` and `Builder.operations`), the reader MUST verify that
both sides agree and fail otherwise.

## 9. Errors and validation

### 9.1 Failure

Deserialization is all-or-nothing. On any failure the reader MUST release
everything it created and return no IR, reporting one of:

| Condition | Typical cause |
|-----------|---------------|
| `BadHeader` | wrong magic, non-zero reserved bits |
| `UnsupportedFormatVersion` | section 12.1 |
| `MissingExtension` / `IncompatibleExtension` | section 5.1 |
| `UnknownCategory` / `UnknownKind` | name not registered by its extension |
| `SchemaMismatch` | section 5.3 |
| `UnboundName` | section 6.1 |
| `BadReference` | index out of range, wrong category, null where not permitted |
| `ConstructionCycle` | section 7.2 |
| `Inconsistent` | section 8.5, or a kind- or extension-specific invariant |
| `Truncated` / `Malformed` | encoding errors |

These conditions SHOULD be surfaced as `CompilerReturnCode`s registered by
`CoreExtension`.

### 9.2 Validation

Before returning, the reader MUST check that every reference resolved to a
live element of the correct category, and that every `construct` element was
both constructed and completed. Extension post-read hooks perform
kind-specific validation.

## 10. Binary encoding

### 10.1 Header

The first 12 bytes:

| Offset | Size | Field |
|--------|------|-------|
| 0 | 4 | magic `'J' 'B' 'I' 'R'` |
| 4 | 1 | `flags0`: bit 0 = 1 if the file is little-endian, 0 if big-endian. Bits 1 to 7 are reserved and MUST be 0 |
| 5 | 1 | `flags1`: reserved, MUST be 0 |
| 6 | 2 | `versionMajor` |
| 8 | 2 | `versionMinor` |
| 10 | 2 | `versionPatch` |

The magic and flags bytes are read as raw bytes. Every multi-byte value
from offset 6 to the end of the file uses the byte order given by
`flags0` bit 0. This header is unchanged from `JBIRFileHeader.hpp`.

### 10.2 Sections

After the header, the file is a sequence of sections, each:

| Field | Encoding |
|-------|----------|
| `tag` | 4 raw bytes |
| `length` | `u64`: number of payload bytes that follow |
| payload | `length` bytes |

The sections MUST appear exactly once each, in this order:

| Tag | Content |
|-----|---------|
| `PLAT` | platform record (section 4.1) |
| `EXTS` | extension table (section 5.1) |
| `CATS` | category table (section 5.2) |
| `KNDS` | kind table (section 5.3) |
| `DECL` | element declarations (section 6) |
| `BODY` | element bodies (section 7) |
| `ROOT` | root record (section 7.4) |
| `END.` | empty payload; end of file |

A reader MUST fail if a section's content does not consume exactly
`length` bytes. Minor format revisions MAY add sections with new tags
immediately before `END.`, and readers MUST skip unknown sections located
there.

### 10.3 Primitive values

| Type | Encoding |
|------|----------|
| `bool` | `u8`, 0 or 1; any other value is `Malformed` |
| integers | fixed width, file byte order |
| `f32` `f64` | IEEE-754 bit pattern, as `u32` / `u64` |
| `string` | `u32` byte length, then UTF-8 bytes; no terminator |
| `bytes` | `u32` length, then bytes |
| `ref<C>` | `u32` file index |
| `list<T>` | `u32` count, then `count` values of `T` |
| `record(...)` | each field's value in order; no framing |
| table counts | `u32` |
| file extension / category / kind index | `u32`, 1-based |

### 10.4 Type descriptors

A `typedesc` is a `u8` code followed by parameters:

| Code | Type | Parameters |
|------|------|------------|
| 0x01 | `bool` | |
| 0x02 to 0x05 | `i8` `i16` `i32` `i64` | |
| 0x06 to 0x09 | `u8` `u16` `u32` `u64` | |
| 0x0A, 0x0B | `f32` `f64` | |
| 0x0C | `string` | |
| 0x0D | `bytes` | |
| 0x0E | reserved | |
| 0x10 | `ref` | `u32` category index; `string` kind annotation (empty = none) |
| 0x11 | `list` | nested `typedesc` |
| 0x12 | `record` | field list |

A field list is a `u32` count followed by `(string name, typedesc type)`
pairs. Other codes are `Malformed`.

### 10.5 Section payloads

- `PLAT`: the platform record fields, in order.
- `EXTS`: count, then entries as in section 5.1.
- `CATS`: count, then entries as in section 5.2.
- `KNDS`: count, then entries as in section 5.3. The `literal` field list
  is written for every kind; it is empty (count 0) for non-`Type` kinds.
- `DECL`: for each category in table order, `u32 N(C)`, then `N(C)`
  declarations, each: `u32` kind index, `u8` mode (0 = `bind`,
  1 = `construct`, 2 = `derive`), then for `bind` a `string` name, or for
  `derive` an owner (`u32` category index, `u32` file index) and a
  `string` key.
- `BODY`: for each category in table order, for each `construct` element
  in index order: `u32` body length in bytes, then the construction fields,
  then the completion fields. The length lets a reader build an index for
  on-demand construction (section 8.1, step 5) without decoding every body.
- `ROOT`: the root record.

## 11. Text encoding

The text encoding renders the same model for human reading, diffing and
hand-written tests. It is normative: a reader MUST produce the same IR from
a text file as from the binary file of the same model. Because both
encodings derive entirely from the schemas in the file, a text writer and
reader need no kind-specific code.

### 11.1 Lexical rules

UTF-8. Whitespace (space, tab, CR, LF) separates tokens and is otherwise
insignificant. `//` starts a comment that runs to end of line. Tokens:
identifiers `[A-Za-z_][A-Za-z0-9_]*`; integers (decimal with optional `-`,
or `0x` hex); floats in C99 `strtod` syntax including `inf` and `nan`, or
`0x...p...` hexadecimal for exact round-tripping. Writers MUST emit floats
as hexadecimal floating point. Strings are `"..."` with escapes `\" \\ \n
\t \xHH`. Bytes are `x"..."` containing an even number of hex digits. A
reference is `@` followed by an integer.

### 11.2 Grammar

```ebnf
file        = header platform extensions categories kinds
              declarations bodies root ;
header      = "JBIR" version ;                      (* byte order is not represented *)
version     = int "." int "." int [ "-" string ] [ "+" string ] ;
platform    = "platform" "{" { ident "=" value } "}" ;
extensions  = "extensions" "{" { int string version } "}" ;
categories  = "categories" "{" { int string "ext" "=" int
                                 "scope" "=" ( "compiler" | "ir" )
                                 [ "common" schema ] } "}" ;
kinds       = "kinds" "{" { int string "ext" "=" int "cat" "=" int schema } "}" ;
schema      = "construct" fields "complete" fields [ "literal" fields ] ;
fields      = "(" [ field { "," field } ] ")" ;
field       = ident ":" type ;
type        = "bool" | "i8" | "i16" | "i32" | "i64" | "u8" | "u16" | "u32" | "u64"
            | "f32" | "f64" | "string" | "bytes"
            | "ref" "<" catname [ "kind" "=" kindname ] ">"
            | "list" "<" type ">" | "record" fields ;
declarations= "declare" "{" { catname "{" { decl } "}" } "}" ;
decl        = "@" int kindname ( "bind" string
                               | "construct"
                               | "derive" catname "@" int string ) ;
bodies      = "bodies" "{" { catname "{" { body } "}" } "}" ;
body        = "@" int "{" { ident "=" value } "}" ;
root        = "root" "{" "scope" "=" ref "context" "=" ref "}" ;
value       = "true" | "false" | int | float | string | bytes | ref | "null"
            | "[" [ value { "," value } ] "]"
            | "{" { ident "=" value } "}" ;
ref         = "@" int ;
catname     = ident ;   (* category name; MUST be in the category table *)
kindname    = ident ;   (* kind name; MUST be in the kind table *)
```

In `body`, every field of the element's full schema MUST appear exactly
once and in schema order. Construction fields come first, followed by
completion fields, with no separator. `null` is the text form of reference
index 0. In the text form of `ref` types, category names replace the
binary encoding's numeric category indices. Numeric indices in the
`extensions`, `categories` and `kinds` tables MUST equal their 1-based
positions.

## 12. Versioning

### 12.1 Format version

The header version governs everything in sections 2 to 11 except the
schemas of individual kinds. A reader supports format `M.m.p` if its own
format major equals `M` and its own minor is at least `m`. Minor revisions
may only add, in a backward-compatible way: platform fields, trailing
sections (section 10.2), new type-descriptor codes (a reader seeing an
unknown code fails with `UnsupportedFormatVersion`), and new error
conditions.

### 12.2 Extension versions

Each extension's semantic version governs its categories' common fields
and its kinds' schemas. Within one major version, an extension MAY:

- add new kinds and new categories;
- append fields to the end of a kind's construction, completion or literal
  schema, or to the end of a category's common construction or completion
  fields, provided it defines a default for each appended field;
- add new bindable names.

Any other change (removing, reordering or retyping a field, changing a
field's meaning, renaming a kind, category or bindable element) requires a
major version increment. A reader older than the writer (lower minor)
rejects the file through the rule of section 5.1. This specification does
not require the reader to handle schemas newer than its own.

## 13. Core definitions

This section defines the categories and kinds owned by `CoreExtension`
(name `"core"`). Field names are normative. All `Location` references are
mandatory content: they are written whenever the element has a location,
and are null only when it has none.

### 13.1 Core categories

In recommended category-table order:

| Category | Scope | Common construction | Common completion |
|----------|-------|---------------------|-------------------|
| `Action` | compiler | | |
| `Type` | ir | | |
| `Location` | ir | | |
| `Literal` | ir | | |
| `Symbol` | ir | `name: string`, `type: ref<Type>` | |
| `Context` | ir | `parent: ref<Context>`, `name: string` | |
| `Scope` | ir | `parent: ref<Scope>`, `name: string` | `builders: list<ref<Builder>>`, `entries: list<record(entryID: u64, entry: ref<EntryPoint>)>` |
| `Builder` | ir | `name: string`, `scope: ref<Scope>` | see `Builder` kind |
| `Value` | ir | `parent: ref<Builder>`, `type: ref<Type>` | |
| `Operation` | ir | `parent: ref<Builder>`, `action: ref<Action>`, `location: ref<Location>`, `debugName: string` | |
| `EntryPoint` | ir | `entryID: u64`, `name: string` | |

Notes:

- `Operation.debugName` is optional debug information. A writer MAY write
  `""`, in which case the reader uses the action's name.
- Extension categories (e.g. Base's `Case`) are placed by the writer
  wherever their construction dependencies suggest, typically between
  `Value` and `Operation`.

### 13.2 Core kinds

| Kind | Category | Construction (own) | Completion (own) | Literal | Notes |
|------|----------|--------------------|------------------|---------|-------|
| `Action` | Action | | | | always `bind` by action name |
| `NoTypeType` | Type | | | () | always `bind "NoType"` |
| `Location` | Location | `fileName: string`, `lineNumber: string`, `bcIndex: i32` | | | |
| `Literal` | Literal | `type: ref<Type>`, *value* | | | value encoded per section 7.3 |
| `Context` | Context | | | | |
| `Scope` | Scope | | | | |
| `Builder` | Builder | | `parent: ref<Builder>`, `children: list<ref<Builder>>`, `context: ref<Context>`, `successor: ref<Builder>`, `boundTo: ref<Operation>`, `operations: list<ref<Operation>>`, `appendPoint: ref<Operation>`, `location: ref<Location>`, `isTarget: bool`, `controlReachesEnd: bool` | | see below |
| `Value` | Value | | | | |
| `Op_AppendBuilder` | Operation | `builder: ref<Builder>` | | | |
| `Op_MergeDef` | Operation | `existingDef: ref<Value>`, `newDef: ref<Value>` | | | |
| `BuilderEntry` | EntryPoint | `builder: ref<Builder>` | | | |

`Builder` completion semantics: the builder is bound to `boundTo` if that is
non-null, and unbound otherwise. `operations` is the builder's complete
operation list, in order. Each listed operation's `parent` MUST be this
builder. The reader appends the operations in order and then sets the
append point to `appendPoint`; null means "at beginning". Each child's
`parent` MUST be this builder. The builder's operation count is derived.

### 13.3 Bindable core names

`Action` names (e.g. `"AppendBuilder"`, `"MergeDef"`), as registered by
`Extension::registerAction()`, and `Type` name `"NoType"`.

## 14. Worked examples (non-normative)

### 14.1 An extension-defined category: Base's `Case`

`BaseExtension` (`"jb2base"`) registers category `Case` (scope `ir`, no
common fields). It owns the `CaseID` space through
`BaseIRAddon::getCaseID()`, and defines one kind:

| Kind | Construction | Completion |
|------|--------------|------------|
| `Case` | `literal: ref<Literal>`, `builder: ref<Builder>`, `fallsThrough: bool` | |

`Op_Switch` then has construction fields `selector: ref<Value>`,
`defaultBuilder: ref<Builder>`, `cases: list<ref<Case>>`. Neither the
format nor `CoreExtension` knows anything about cases.

### 14.2 Recursive types

`struct Node { Node *next; int32 v; }`:

```
declare {
  Type {
    @1 Int32Type  bind "Int32"
    @2 StructType construct
    @3 PointerType construct
    @4 FieldType  derive Type @2 "next"
    @5 FieldType  derive Type @2 "v"
  }
}
bodies {
  Type {
    @2 { name = "Node" size = 128
         fields = [ { name = "next" type = @3 offset = 0 },
                    { name = "v"    type = @1 offset = 64 } ] }
    @3 { baseType = @2 }
  }
}
```

Here `StructType` declares construction `(name: string, size: u64)` and
completion `(fields: list<record(name: string, type: ref<Type>, offset:
u64)>)`, with completion performed inside the `StructTypeBuilder` helper.
`PointerType` declares construction `(baseType: ref<Type>)`. Construction
of `@2` needs nothing. Its completion runs inside the helper, after `@2` is
bound, and resolves `@3` on demand. `@3`'s construction needs `@2`, which
is already bound. The construction graph is acyclic, as section 7.2
requires.

### 14.3 A small IR in text form

```
JBIR 2.0.0
platform { wordSize = 64 }
extensions { 1 "core" 1.0.0   2 "jb2base" 1.0.0 }
categories {
  1 "Action"    ext=1 scope=compiler
  2 "Type"      ext=1 scope=ir
  3 "Location"  ext=1 scope=ir
  4 "Literal"   ext=1 scope=ir
  5 "Scope"     ext=1 scope=ir common construct (parent: ref<Scope>, name: string)
                                         complete (builders: list<ref<Builder>>,
                                                   entries: list<record(entryID: u64, entry: ref<EntryPoint>)>)
  6 "Builder"   ext=1 scope=ir common construct (name: string, scope: ref<Scope>) complete ()
  7 "Value"     ext=1 scope=ir common construct (parent: ref<Builder>, type: ref<Type>) complete ()
  8 "Operation" ext=1 scope=ir common construct (parent: ref<Builder>, action: ref<Action>,
                                                 location: ref<Location>, debugName: string) complete ()
  9 "EntryPoint" ext=1 scope=ir common construct (entryID: u64, name: string) complete ()
 10 "Context"   ext=1 scope=ir common construct (parent: ref<Context>, name: string) complete ()
}
kinds {
  1 "Action"    ext=1 cat=1 construct () complete ()
  2 "Int32Type" ext=2 cat=2 construct () complete () literal (v: i32)
  3 "Location"  ext=1 cat=3 construct (fileName: string, lineNumber: string, bcIndex: i32) complete ()
  4 "Literal"   ext=1 cat=4 construct (type: ref<Type>) complete ()   // + value, per section 7.3
  5 "Scope"     ext=1 cat=5 construct () complete ()
  6 "Builder"   ext=1 cat=6 construct () complete (parent: ref<Builder>, children: list<ref<Builder>>,
                  context: ref<Context>, successor: ref<Builder>, boundTo: ref<Operation>,
                  operations: list<ref<Operation>>, appendPoint: ref<Operation>,
                  location: ref<Location>, isTarget: bool, controlReachesEnd: bool)
  7 "Value"     ext=1 cat=7 construct () complete ()
  8 "Op_Const"  ext=2 cat=8 construct (result: ref<Value>, literal: ref<Literal>) complete ()
  9 "Op_Add"    ext=2 cat=8 construct (result: ref<Value>, left: ref<Value>, right: ref<Value>) complete ()
 10 "BuilderEntry" ext=1 cat=9 construct (builder: ref<Builder>) complete ()
}
// Context has no elements here; it is listed only because Builder refers to it.
declare {
  Action    { @1 Action bind "Const"   @2 Action bind "Add" }
  Type      { @1 Int32Type bind "Int32" }
  Location  { @1 Location construct }
  Literal   { @1 Literal construct   @2 Literal construct }
  Scope     { @1 Scope construct }
  Builder   { @1 Builder construct }
  Value     { @1 Value construct   @2 Value construct   @3 Value construct }
  Operation { @1 Op_Const construct   @2 Op_Const construct   @3 Op_Add construct }
  EntryPoint { @1 BuilderEntry construct }
}
bodies {
  Location  { @1 { fileName = "add.c" lineNumber = "3" bcIndex = -1 } }
  Literal   { @1 { type = @1 v = 3 }   @2 { type = @1 v = 4 } }
  Scope     { @1 { parent = null name = "main" builders = [@1]
                   entries = [ { entryID = 0 entry = @1 } ] } }
  Builder   { @1 { name = "entry" scope = @1 parent = null children = [] context = null
                   successor = null boundTo = null operations = [@1, @2, @3]
                   appendPoint = @3 location = @1 isTarget = false controlReachesEnd = true } }
  Value     { @1 { parent = @1 type = @1 }  @2 { parent = @1 type = @1 }  @3 { parent = @1 type = @1 } }
  Operation {
    @1 { parent = @1 action = @1 location = @1 debugName = "" result = @1 literal = @1 }
    @2 { parent = @1 action = @1 location = @1 debugName = "" result = @2 literal = @2 }
    @3 { parent = @1 action = @2 location = @1 debugName = "" result = @3 left = @1 right = @2 }
  }
  EntryPoint { @1 { entryID = 0 name = "" builder = @1 } }
}
root { scope = @1 context = null }
```

## 15. Open points

1. **Format version number.** This draft keeps 2.0.0, the value already in
   `JBIRFileHeader.hpp`. If 2.0.0 files from the earlier layout exist
   anywhere, this should become 3.0.0.
2. **Host addresses and `NativeEntry`** (section 4.2). `FunctionSymbol`
   entry points are opaque bytes for now, and `NativeEntry` elements are
   omitted. Both should be revisited together with
   `RelocatableLiteral` (point 6).
3. **Extension loading default** (section 5.1): permitted by default.
4. **Text encoding cost.** Because the text encoding is schema-driven, it
   needs no per-kind code. If it nonetheless proves costly to maintain, it
   can be demoted to a non-normative debug rendering without changing
   anything else in this document.
5. **Func and Complex kinds** (`FunctionContext`, `FunctionScope`,
   `LocalSymbol`, `ParameterSymbol`, `FunctionSymbol`, `FunctionType`,
   `ComplexFloatType`, ...) and the remaining Base kinds are to be specified
   by their owning extensions, ideally as companion documents in this
   directory.
6. **`RelocatableLiteral`** (future). This will be a special kind of literal
   carrying the information needed to adjust its value in a different
   execution environment. It is expected to be added in a minor revision as
   a new kind, either in the `Literal` category or alongside it, with its own
   resolution rules. Until then, `Literal` is a plain constant (section 7.3).

## Appendix A. Implementation implications (non-normative)

Recorded here to seed the implementation discussion. None of this is part of
the format.

- **Registries.** `IDSpaceService` (process-global, keyed by string) is
  replaced by a per-`Compiler` registry of categories and kinds, populated
  by each extension at load time. `CoreExtension` registers the section 13
  categories through the same API every other extension uses.
- **Kind descriptors.** Each kind supplies one descriptor: schema,
  construct, complete, and (for types) a literal codec. The writer, the
  binary reader, the text reader and a generic printer are all written once
  against an abstract field visitor, which makes the per-kind cost a single
  descriptor. This replaces `ExtensionIRWriter::writeElementsForIDSpace`.
- **Reader structure.** The on-demand "file index → live object" maps
  mirror `IRCloner`'s `cloned*` arrays, so the two could share a mapper
  abstraction.
- **Factory variants needed.** Builders need a construction path that
  takes no parent and no bound operation (both are set at completion). The
  type builders need a deserializer-supplied helper that binds and then
  completes (section 7.2). `FieldType` needs lookup by owner and key
  (`StructType::LookupField`).
- **Name lookups.** `Compiler::lookupActionFromName()` asserts on an unknown
  name. The reader needs a non-asserting variant so it can report
  `UnboundName`.
- **Global name uniqueness** is not enforced today. Registration should
  reject duplicate extension, category, kind and bindable names.
