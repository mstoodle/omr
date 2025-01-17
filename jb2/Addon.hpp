/*******************************************************************************
 * Copyright (c) 2021, 2022 IBM Corp. and others
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

/*
 * There are different ways to extend the functionality of classes:
 *    - a class can extend another class when it's important to create objets of the subclass rather than the super class (inheritance)
 *    - objects can be embedded into other classes when that functionality is not optional (the main object cannot operate without the contained object) and does not need to exist independently of the parent object (composition)
 *    - classes can derive from Extensible which allows other Extensions to add extended functionality (called "addon"s) into objects of classes created by other Extensions
 *    - Extension objects collect together a set of capabilities that can be loaded into a Compiler as a group. These objects are responsible for installing
 *      addons where needed. Extensions can register to be notified when particular kinds of Extensible objects are created by its parent Extension
 *
 * For an example consider the Compilation class which is a core Extensible class of the Compiler. Compilation objects are created inherently by the CoreExtension object
 * and are passed to the Compiler's compile function to manage the process of compiling a particular CompileUnit. The FunctionExtension extension object provides a subclass
 * of Compilation called Func::FunctionCompilation which adds a specific CompileUnit called a Func::Function, a specific Context called a Func::FunctionContext, etc. When
 * the Func Extension is loaded, Compilation objects should be created by the Func::FunctionExtension rather than by CoreExtension. Func::FunctionCompilation is an example
 * of inheritance. The Base extension, on the other hand, provides an example of addons: the Base::BaseExtension object, when loaded, registers itself against the CoreExtension
 * to be notified when a Compilation object's constructor is called. Since Func::FunctionCompilation extends Compilation, the creation of a Func::FunctionCompilation object
 * will cause the Base::BaseExtension object to be notified of the object's creation. Upon notification, the Base::BaseExtension object will create a Base::BaseCompilation
 * object and use Extensible's attach API to store this Base::BaseCompilation object inside the created Func::FunctionCompilation object. Any other class can then access
 * the additional functionality of Base::BaseCompilation by simply asking a Compilation object for its Base::BaseCompilation addon via comp->addon<Base::BaseCompilation>().
 * The Base extension uses this feature to record Pointer and Struct Types (which are provided by the Base extension) on the Compilation object. Whenever a Compilation object
 * is passed to the Base::BaseExtension object, it can find and access the Base::BaseCompilation addon to manage these Types without other extensions or the core compiler
 * needing to be aware of them. 
 *
 * Extensible is a base class used to implement this "addon" facility, leveraging the Extensible Kind category used throughout the compiler classes.
 */
#ifndef ADDON_INCL
#define ADDON_INCL

#include "common.hpp"
//#include "Extensible.hpp"
#include "KindService.hpp"

namespace OMR {
namespace JB2 {

class Extension;

// This class simply collects all classes that represent Addons to some "root" Extensible object
class Addon : public Extensible {
    JBALLOC_NO_DESTRUCTOR_(Addon)

public:
    DYNAMIC_ALLOC_ONLY(Addon, Extension *ext, Extensible *root, KINDTYPE(Extensible) kind);
    Extensible *root() const { return _root; }

private:
    Extensible *_root;

    SUBCLASS_KINDSERVICE_DECL(Extensible, Addon);
};

} // namespace JB2
} // namespace OMR

#endif // !defined(ADDON_INCL)

