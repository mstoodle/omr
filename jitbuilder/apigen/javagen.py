#! /usr/bin/env python

###############################################################################
# Copyright (c) 2018, 2019 IBM Corp. and others
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

"""
A module for generating the JitBuilder Java client API.
"""

import os
import shutil
import argparse
import cgen
from genutils import *

class JavaGenerator(cgen.CGenerator):

    def __init__(self, api, jnidir):
        cgen.CGenerator.__init__(self, api, jnidir)
        # Mapping between API type descriptions and Java data types
        self.builtin_type_map = { "none": "void"
                                , "boolean": "boolean"
                                , "integer": "long"
                                , "int8": "byte"
                                , "int16": "short"
                                , "int32": "int"
                                , "int64": "long"
                                , "uint32": "int"
                                , "float": "float"
                                , "double": "double"
                                , "pointer": "long"
                                , "ppointer": "long"
                                , "unsignedInteger": "long"
                                , "constString": "String"
                                , "string": "String"
                                }
        # Mapping between API type descriptions and JNI data types
        self.jni_native_type_map = { "none": "void"
                                   , "boolean": "jboolean"
                                   , "integer": "jlong"
                                   , "int8": "jbyte"
                                   , "int16": "jshort"
                                   , "int32": "jint"
                                   , "int64": "jlong"
                                   , "uint32": "jint"
                                   , "float": "jfloat"
                                   , "double": "jdouble"
                                   , "pointer": "jlong"
                                   , "ppointer": "jlong"
                                   , "unsignedInteger": "jlong"
                                   , "constString": "jstring"
                                   , "string": "jstring"
                                   }
        self.jni_type_api_name_map = { "none": "void"
                                     , "boolean": "Boolean"
                                     , "integer": "Int"
                                     , "int8": "Byte"
                                     , "int16": "Short"
                                     , "int32": "Int"
                                     , "int64": "Long"
                                     , "uint32": "Int"
                                     , "float": "Float"
                                     , "double": "Double"
                                     , "pointer": "Object"
                                     , "ppointer": "Object"
                                     , "unsignedInteger": "Long"
                                     , "constString": "Object"
                                     , "string": "Object"
                                     }
        self.jni_type_signature_string_map = { "none": "V"
                                             , "boolean": "Z"
                                             , "integer": "I"
                                             , "int8": "B"
                                             , "int16": "S"
                                             , "int32": "I"
                                             , "int64": "J"
                                             , "uint32": "I"
                                             , "float": "F"
                                             , "double": "D"
                                             , "pointer": "Ljava/lang/Object;"
                                             , "ppointer": "Ljava/lang/Object;"
                                             , "unsignedInteger": "J"
                                             , "constString": "L/java/lang/String;"
                                             , "string": "L/java/lang/String;"
                                             }


    def gen_api_impl_includes(self, classes_desc, api_headers_dir):
        """
        Generates a list of files to be included in the client
        API implementation.

        Two headers are included for each top-level (non-nested) class
        in the API description: one is the implementation header in `ilgen/`,
        the other is the header produced by this generator. In addition,
        a `Macros.hpp` is included as it contains some utilities used
        in the generated code.
        """
        files = [os.path.join("ilgen", c.name() + ".hpp") for c in classes_desc]
        #files += [os.path.join(api_headers_dir, "Macros.hpp")]
        #files += [os.path.join(api_headers_dir, c.name() + ".h") for c in classes_desc]
        return files

    def get_full_class_name(self, c):
        return "$".join(c.containing_classes() + [c.name()])

    def get_client_class_name(self, c):
        """
        Returns the name of a given class in the client API implementation,
        prefixed with the name of all contianing classes.
        """
        return c.name() #"".join(c.containing_classes() + [c.name()])

    def get_client_type(self, t, ns=""):
        """
        Returns the C type to be used in the client API implementation
        for a given type name, prefixing with a given namespace if needed.

        Because C does not support the notion of namespaces, the namespace
        argument to this function is forced to be an empty string.
        """
        return "struct " + cgen.CGenerator.get_client_type(self, t) if t.is_class() else cgen.CGenerator.get_client_type(self, t)

    def get_impl_type(self, c):
        """
        Returns the C++ type to be used in the JitBuilder implementation
        for a given type name, prefixing with a given namespace if needed.
        """
        return "long" if c.is_class() else self.builtin_type_map[c.name()]

    def get_service_call_name(self, service, owner=None):
        """
        Produces the full name of a service usable as a call.
        """
        #owner = owner if owner is not None else service.owning_class()
        #prefix = "{}_".format(owner.short_name()) if owner is not None else ""
        return service.name()

    def get_service_decl_name(self, service, owner=None):
        """
        Produces the full name of a service for use in a function declarations.
        """
        # in C, we can use the same name as in a function call
        return self.get_service_call_name(service, owner)

    def get_service_impl_name(self, service, owner=None):
        """
        Produces the full name of a service for use in a function definition.
        """
        # in C, we can use the same name as in a function call
        return self.get_service_call_name(service, owner)

    def get_ctor_name(self, ctor_desc):
        """
        Produces the name of a client API class constructor from the class's description.
        """
        return ctor_desc.owning_class()

    def get_impl_ctor_name(self, class_desc):
        return class_desc.name()

    def get_dtor_name(self, class_desc):
        """
        Produces the name of a client API class destructor from the class's desctipion.
        """
        return "Delete{}".format(self.get_client_class_name(class_desc))

    def get_self_parm_for_class(self, class_desc):
        return "{} self".format(self.get_client_type(class_desc.as_type()))

    def get_self_parm(self, service):
        return self.get_self_parm_for_class(service.owning_class())

    def get_self_arg(self, service):
        return "self"

    def generate_java_parm(self, parm_desc, is_client=True):
        """
        Produces a parameter declaration from a given parameter description.
        The parameter declaration is usable in a function declaration.
        """
        t = parm_desc.type()
        t = self.get_client_type(t) if is_client else self.get_impl_type(t)
        fmt = "{t}[] {n}" if parm_desc.is_array() else "{t} {n}"
        return fmt.format(t=t,n=parm_desc.name())

    def generate_java_parm_list(self, parms_desc, namespace="", is_client=True):
        """
        Produces a stringified comma seperated list of parameter
        declarations from a list of parameter descriptions. The list
        is usable as the parameter listing of a function declaration.
        """
        return ", ".join([ self.generate_java_parm(p, is_client=is_client) for p in parms_desc ])

    def generate_java_service_parms(self, service, is_client=True):
        """
        Produces a stringified, comma separated list of parameter
        declarations for a service.
        """
        return self.generate_java_parm_list(service.parameters(), is_client=is_client)

    def generate_service_parms(self, service):
        """
        Produces a stringified, comma separated list of parameter
        declarations for a service.
        """
        return self.generate_parm_list(service.parameters())

    def generate_service_vararg_parms(self, service):
        """
        Produces a stringified, comma separated list of parameter
        declarations for a service.
        """
        return list_str_prepend(self.get_self_parm(service), self.generate_vararg_parm_list(service.parameters()))

    def generate_parm_callback(self, callback_desc):
        """
        Produces the declaration of a function parameter for accepting a
        callback as a function pointer.
        """
        rtype = self.get_client_type(callback_desc.return_type())
        name = callback_desc.overload_name()
        arg_types = ", ".join([self.get_client_type(p.type()) for p in callback_desc.parameters()])
        return "{rtype} (*{name}_)({arg_types})".format(rtype=rtype, name=name, arg_types=arg_types)

    def generate_arg_callback(self, callback_desc):
        """
        Produces an expression representing a callback function pointer passed in as an argument.
        """
        return "{}_".format(callback_desc.overload_name())

    def generate_ctor_parms(self, ctor_desc):
        parms = [self.generate_java_parm(p, is_client=True) for p in ctor_desc.parameters()]
        return ", ".join(parms)

    def generate_native_parm(self, parm_desc):
        """
        Produces a native parameter declaration from a given parameter description.
        The parameter declaration is usable in a function declaration.
        """
        t = parm_desc.type()
        t = "long[]" if parm_desc.is_array() else "long" if t.is_class() else self.get_client_type(t)
        return "{t} {n}".format(t=t,n=parm_desc.name())

    def generate_native_parms(self, service_desc):
        parms = [self.generate_native_parm(p) for p in service_desc.parameters()]
        return ", ".join(parms)

    def generate_get_client_field(self, name):
        """
        Produces a string that "gets" a field with name 'name' from a client class.
        """
        return "self->{}".format(name)

    def get_class_inititializer(self, class_desc):
        """
        Produces a description of a service to initialize an instance of a client API class.
        """
        init_service = {
            "name":"initializeFromImpl",
            "overloadsuffix": "",
            "flags": "",
            "return": "none",
            "parms": [ {"name":"impl","type":"pointer"} ]
        }
        return APIService(init_service, class_desc, class_desc.api)

    # header utilities ###################################################

    def get_class_name(self, c):
        """
        Returns the name of a given class in the client API implementation.

        If the class is a nested class, then the name is prefixed with name
        of all containing classes, separated by the scope resolution operator.
        """
        return ".".join(c.containing_classes() + [c.name()])

    def get_client_type(self, t, namespace=""):
        """
        Returns the C++ type to be used in the client API implementation
        for a given type name, prefixing with a given namespace if needed.
        """
        return self.get_client_class_name(t.as_class()) if t.is_class() else self.builtin_type_map[t.name()]

    def generate_xetClientObj_fname(self, x, class_desc):
        if class_desc.containing_classes():
            name = x + "et_{}_ClientObj".format("_".join(class_desc.containing_classes() + [class_desc.name()]))
        else:
            name = x + "etClientObj"
        return name

    def generate_getClientObj_fname(self, class_desc):
        return self.generate_xetClientObj_fname("g", class_desc)

    def generate_setClientObj_fname(self, class_desc):
        return self.generate_xetClientObj_fname("s", class_desc)

    def generate_java_call_xetClientObj(self, x, class_desc):
        top_class = class_desc.containing_classes()[-1] if class_desc.containing_classes() else class_desc.name()
        return top_class + "." + self.generate_xetClientObj_fname(x, class_desc)

    def generate_java_call_getClientObj(self, class_desc):
        return self.generate_java_call_xetClientObj("g", class_desc)

    def generate_java_call_setClientObj(self, class_desc):
        return self.generate_java_call_xetClientObj("s", class_desc)

    def generate_jni_class_name(self, name):
        return "OMR::JitBuilder::" + name

    def generate_jni_call_xetClientObj(self, x, class_desc):
        top_class = class_desc.containing_classes()[-1] if class_desc.containing_classes() else class_desc.name()
        return self.generate_jni_class_name(top_class) + "::" + self.generate_xetClientObj_fname(x, class_desc)

    def generate_jni_call_getClientObj(self, class_desc):
        return self.generate_jni_xetClientObj("g", class_desc)

    def generate_jni_call_getClientObj(self, class_desc):
        return self.generate_jni_xetClientObj("s", class_desc)

    def generate_field_decl(self, field, with_visibility = False):
        """
        Produces the declaration of a client API field from
        its description, specifying its visibility as required.
        """
        t = self.get_client_type(field.type())
        n = field.name()
        return "protected {type} {name};\n".format(type=t, name=n)

    def generate_service_args(self, service):
        """
        Produces a stringified, comma separated list of parameter
        declarations for a service.
        """
        return list_str_prepend(self.get_self_parm(service), self.generate_parm_list(service.parameters()))

    def generate_class_service_decl(self, service,is_callback=False):
        """
        Produces the declaration for a client API class service
        from its description.
        """
        vis = service.visibility() + ": "
        ret = self.get_client_type(service.return_type())
        class_prefix = service.owning_class().short_name()
        name = self.get_service_decl_name(service)
        parms = self.generate_service_parms(service)

        decl = "{rtype} {name}({parms});\n".format(rtype=ret, name=name, parms=parms)

        if service.is_vararg():
            parms = self.generate_service_vararg_parms(service)
            decl = decl + "{rtype} {name}_v({parms});\n".format(rtype=ret, name=name, parms=parms)

        return decl

    def generate_ctor_decl(self, ctor_desc):
        """
        Produces the declaration of a client API class constructor
        from its description and the name of its class.
        """
        rtype = self.get_client_type(ctor_desc.owning_class().as_type())
        name = self.get_ctor_name(ctor_desc)
        parms = self.generate_ctor_parms(ctor_desc)
        return "{rtype} {name}({parms});\n".format(rtype=rtype, name=name, parms=parms)

    def generate_impl_ctor_decl(self, class_desc):
        """
        Produces the declaration of the client API class constructor
        that takes an impl object as argument.
        """
        ctype = self.get_client_type(class_desc.as_type())
        name = self.get_impl_ctor_name(class_desc)
        return "{ctype} {name}(void * impl);\n".format(ctype=ctype, name=name)

    def generate_dtor_decl(self, class_desc):
        """
        Produces the declaration of client API class destructor
        from the class's name.
        """
        return "void {name}({parm});\n".format(name=self.get_dtor_name(class_desc), parm=self.get_self_parm_for_class(class_desc))
    
    def generate_buildil_callback_decl(self, class_desc):
        """
        Produces the declaration of the buildIl callback for classes that 
        extend IlBuilder.
        """
        short_name = class_desc.short_name()
        return "void {short_name}_setBuildIlCallback(struct {name} * self, bool(* buildIl_Callback)(struct IlBuilder *));\n".format(short_name=short_name, name=class_desc.name())

    def generate_allocator_decl(self, class_desc):
        """Produces the declaration of a client API object allocator."""
        return 'extern void * {alloc}(void * impl);\n'.format(alloc=self.get_allocator_name(class_desc))

    def grab_impl(self, v, t):
        """
        Constructs an expression that grabs the implementation object
        from a client API object `v` and with type name `t`.
        """
        return "({v} != null ? {v}._impl : 0L)".format(v=v) if t.is_class() else v

    def impl_getter_name(self, class_desc):
        """
        Produces the name of the callback that, given a client object,
        returns the corresponding JitBuilder implementation of the object.
        """
        if not isinstance(class_desc, APIClass):
            class_desc = class_desc.as_class()
        return "getImpl_{}".format(self.get_client_class_name(class_desc))

    def write_class_def(self, writer, class_desc):
        """Write the definition of a client API class from its description."""

        name = class_desc.name()

        containing_classes = class_desc.containing_classes()
        if containing_classes:
            name = "".join(containing_classes) + name

        has_extras = name in self.classes_with_extras

        writer.write("struct {name} {{\n".format(name=name))
        writer.indent()

        if class_desc.has_parent():
            # "inheritance" is represented using a data member called `super` that
            # points to the instance of the "parent" class
            writer.write("{} super;\n".format(self.get_client_type(class_desc.parent().as_type())))

        # write fields
        for field in class_desc.fields():
            decl = self.generate_field_decl(field)
            writer.write(decl)

        # write needed impl field
        if not class_desc.has_parent():
            writer.write("void * _impl;\n")

        if has_extras:
            writer.write(self.generate_include('{}ExtrasInsideClass.hpp'.format(class_desc.name())))

        writer.outdent()
        writer.write("};\n")

        # write nested classes
        for c in class_desc.inner_classes():
            self.write_class_def(writer, c)

        for ctor in class_desc.constructors():
            decls = self.generate_ctor_decl(ctor)
            writer.write(decls)

        #write get impl declaration
        writer.write('void * {}(void * client);\n'.format(self.impl_getter_name(class_desc)))

        # write impl constructor
        writer.write(self.generate_impl_ctor_decl(class_desc))

        # write impl init service declaration
        init_service = self.get_class_inititializer(class_desc)
        writer.write(self.generate_class_service_decl(init_service))

        #write buildIl callback declaration
        if class_desc.has_parent() and class_desc.parent().name() == 'IlBuilder':
            writer.write(self.generate_buildil_callback_decl(class_desc))

        dtor_decl = self.generate_dtor_decl(class_desc)
        writer.write(dtor_decl)

        for callback in class_desc.callbacks():
            decl = self.generate_class_service_decl(callback, is_callback=True)
            writer.write(decl)

        service_names = [s.name() for s in class_desc.services()]
        for service in class_desc.services():
            if service.suffix() == "" or service.overload_name() not in service_names:
                # if a service already exists with the same overload name, don't declare the current one
                decl = self.generate_class_service_decl(service)
                writer.write(decl)


    def write_class_header(self, writer, class_desc, namespaces, class_names):
        """Writes the header for a client API class from the class description."""

        has_extras = class_desc.name() in self.classes_with_extras

        writer.write(self.get_copyright_header())
        writer.write("\n")

        writer.write("#ifndef {}_INCL\n".format(class_desc.name()))
        writer.write("#define {}_INCL\n".format(class_desc.name()))
        writer.write("#ifdef __cplusplus\n")
        writer.write('extern "C" {\n')
        writer.write("#endif // __cplusplus")

        if class_desc.has_parent():
            writer.write(self.generate_include("{}.h".format(class_desc.parent().name())))

        if has_extras:
            writer.write(self.generate_include('{}ExtrasOutsideClass.h'.format(class_desc.name())))
        writer.write("\n")

        writer.write("// forward declarations for all API classes\n")
        for c in class_names:
            writer.write("struct {};\n".format(c))
        writer.write("\n")

        self.write_class_def(writer, class_desc)
        writer.write("\n")

        self.write_allocator_decl(writer, class_desc)
        writer.write("\n")

        writer.write("#ifdef __cplusplus\n")
        writer.write('} // extern "C" \n')
        writer.write("#endif // __cplusplus\n")
        writer.write("#endif // {}_INCL\n".format(class_desc.name()))

    # source utilities ###################################################

    def generate_java_arg(self, parm_desc):
        """
        Produces an argument variable from a parameter description.
        The produced value is usable as the "variable" for accessing
        the specified parameter.
        """
        n = parm_desc.name()
        t = parm_desc.type()
        return "transformArray({n})".format(n=n) if parm_desc.is_array() else self.grab_impl(n,t)

    def generate_java_arg_list(self, parms_desc):
        """
        Produces a stringified comma separated list of argument
        variables, given a list of parameter descriptions. The
        produced list can be used to forward the arguments from
        a caller to a callee function.
        """
        return ", ".join([ self.generate_java_arg(a) for a in parms_desc ])

    def generate_native_ctor_name(self, ctor_desc):
        class_name = ctor_desc.overload_name()
        return "new" +  class_name

    def generate_native_service_name(self, service):
        return service.overload_name()

    def generate_native_ctor_name_and_parms(self, ctor_desc):
        ctor_name = self.generate_native_ctor_name(ctor_desc)
        parms = self.generate_native_parms(ctor_desc)
        return "{ctor_name}({parms})".format(ctor_name=ctor_name, parms=parms)

    def generate_native_service_name_and_parms(self, service_desc):
        name = self.generate_native_service_name(service)
        parms = self.generate_native_parms(service_desc)
        return "{name}({parms})".format(name=name,  parms=parms)

    def write_native_ctor(self, writer, ctor_desc):
        """
        Write the client API Java class native constructor from
        its description and its class description.
        """
        ctor_name_and_parms = self.generate_native_ctor_name_and_parms(ctor_desc)
        writer.write("private static native long {name_and_parms};\n".format(name_and_parms=ctor_name_and_parms))

    def write_java_ctor(self, writer, class_desc, ctor_desc):
        """
        Write the client API Java class constructor from
        its description and its class description.
        """

        class_name = ctor_desc.name()
        parms = self.generate_ctor_parms(ctor_desc)
        ctor_name_and_parms = self.generate_native_ctor_name_and_parms(ctor_desc)

        class_name = ctor_desc.name()
        writer.write("public {class_name}({parms}) {{\n".format(class_name=class_name,parms=parms))
        writer.indent()

        # allocate implementation object
        args = self.generate_java_arg_list(ctor_desc.parameters())
        if class_desc.has_parent():
           writer.write("super( new{cname}({args}) );\n".format(cname=ctor_desc.overload_name(), args=args))
        else:
            writer.write("_impl = new{cname}({args});\n".format(cname=ctor_desc.overload_name(), args=args))

        init_service = self.get_class_inititializer(class_desc)
        writer.write("impl_{}(_impl);\n".format(self.get_service_call_name(init_service)))

        writer.write(self.generate_java_call_setClientObj(class_desc) + "(this, _impl);\n")

        writer.outdent()
        writer.write("}\n")

    def write_impl_ctor_impl(self, writer, class_desc):
        """
        Writes the implementation of the special client API class
        constructor that takes a pointer to an implementation object.

        The special constructor is package public and only has one
        parameter: an opaque pointer to an implementation object. It
        simply sets itself as the client object corresponding to the
        passed-in implementation object and calls the common initialization
        function. This is the constructor that constructors of derived clienti
        API classes should invoke.
        """
        name = self.get_impl_ctor_name(class_desc)
        rtype = self.get_client_type(class_desc.as_type())

        writer.write("{}(long impl) {{\n".format(name))
        writer.indent()

        if class_desc.has_parent():
            # construct parent
            writer.write("super(impl);\n")

        # call initializer
        init_service = self.get_class_inititializer(class_desc)
        writer.write("impl_{}(impl);\n".format(self.get_service_call_name(init_service)))

        writer.outdent()
        writer.write("}\n")

    def write_impl_initializer(self, writer, class_desc):
        """
        Writes the implementation of the client API class common
        initialization function from the description of a class.

        The common initialization function is called by all client
        API class constructors to initialize the class's fields,
        including the private pointer to the corresponding
        implementation object. It is also used to set any callbacks
        to the client on the implementation object.
        """

        init_service = self.get_class_inititializer(class_desc)
        writer.write("void {name}({parms}) {{\n".format(name=self.get_service_impl_name(init_service), parms=self.generate_service_parms(init_service)))
        writer.indent()

        full_name = self.get_class_name(class_desc)
        impl_cast = self.to_impl_cast(class_desc,"impl")

        if class_desc.has_parent():
            name = self.get_service_call_name(self.get_class_inititializer(class_desc.parent()))
            parms = list_str_prepend(self.generate_get_client_field("super"), "impl")
            writer.write("{name}({parms});\n".format(name=name, parms=parms))
        else:
            writer.write("{} = impl;\n".format(self.generate_get_client_field("_impl")))

        for field in class_desc.fields():
            fmt = "GET_CLIENT_OBJECT(clientObj_{fname}, {ftype}, {impl_cast}->{fname});\n"
            writer.write(fmt.format(fname=field.name(), ftype=field.type().name(), impl_cast=impl_cast))
            writer.write("{field} = clientObj_{fname};\n".format(field=self.generate_get_client_field(field.name()), fname=field.name()))

        # write setting of the impl getter
        impl_getter = self.impl_getter_name(class_desc)
        writer.write("{impl_cast}->setGetImpl(&{impl_getter});\n".format(impl_cast=impl_cast,impl_getter=impl_getter))

        writer.outdent()
        writer.write("}\n")

    def write_array_transformer(self, writer, parm_class):
        assert parm_class.is_class()
        t = parm_class.name()
        writer.write("private static long[] transformArray({t}[] clientArray) {{\n".format(t=t))
        writer.indent();
        writer.write("long implArray[] = new long[clientArray.length];\n")
        writer.write("for (int i=0;i < clientArray.length; i++) {\n")
        writer.indent()
        writer.write("{t} o = clientArray[i];\n".format(t=t))
        writer.write("implArray[i] = (o != null) ? (o._impl) : 0L;\n")
        writer.outdent()
        writer.write("}\n")
        writer.write("return implArray;\n")
        writer.outdent()
        writer.write("}\n")

    def write_method_array_arg_transformers(self, writer, m, transformed):
        for parm in m.parameters():
            if parm.is_array():
                name=parm.type().name()
                if name not in transformed:
                    self.write_array_transformer(writer, parm.type())
                    transformed[name] = True

    def write_array_arg_transformers(self, writer, desc):
        transformed = {} 
        for ctor in desc.constructors():
            self.write_method_array_arg_transformers(writer, ctor, transformed)
        for c in desc.inner_classes():
            for ctor in c.constructors():
                self.write_method_array_arg_transformers(writer, ctor, transformed)
        for s in class_desc.services():
            self.write_method_array_arg_transformers(writer, s, transformed)
        for c in desc.inner_classes():
            for s in c.services():
                self.write_method_array_arg_transformers(writer, s, transformed)

    def write_dtor_impl(self, writer, class_desc):
        writer.write("void {}({}) {{\n".format(self.get_dtor_name(class_desc), self.get_self_parm_for_class(class_desc)))
        writer.indent()
        # delete parent object if present
        if class_desc.has_parent():
            writer.write("{}({});\n".format(self.get_dtor_name(class_desc.parent()), self.generate_get_client_field("super")))

        writer.write("free(self);\n")
        writer.outdent()
        writer.write("}\n")

    def get_java_package_text(self):
        return """\
package org.eclipse.omr.jitbuilder;
import java.lang.invoke.MethodHandle;
import java.lang.invoke.MethodHandles;
import java.lang.invoke.MethodType;
import java.lang.reflect.Field;
import java.lang.reflect.Method;
import java.util.*;
import sun.misc.Unsafe;
import jdk.internal.org.objectweb.asm.*;
import static jdk.internal.org.objectweb.asm.Opcodes.*;
""";

    def generate_arg_list(self, parms_desc):
        """
        Produces a stringified comma separated list of argument
        variables, given a list of parameter descriptions. The
        produced list can be used to forward the arguments from
        a caller to a callee function.
        """
        return ", ".join([ self.generate_arg(a) for a in parms_desc ])
        
    def write_get_client_object(self, writer, client_var_name, return_type, impl_object):
        """
        Gets the client object from the provided implementation object
        or null if the provided object is 0L. Allocates a new client
        object if needed. The provided client object variable will be
        declared and used to hold the client object.
        """
        client_type = self.get_client_type(return_type)
        writer.write("{} {}=null;\n".format(client_type, client_var_name))
        writer.write("if ({} != 0L) {{\n".format(impl_object))
        writer.indent()
        getClientObj = self.generate_java_call_getClientObj(return_type.as_class())
        writer.write("{} = {}({});\n".format(client_var_name, getClientObj, impl_object))
        writer.write("if ({} == null) {{\n".format(client_var_name))
        writer.indent();
        writer.write("{} = new {}({});\n".format(client_var_name, client_type, impl_object))
        writer.outdent()
        writer.write("}\n")
        writer.outdent()
        writer.write("}\n")

    def write_java_class_service_impl(self, writer, desc, class_desc):
        """
        Writes the implementation of a client API class service.

        Services simply forward their arguments to the corresponding
        functions on the implementation object and forward returned
        values. Special setup for the arguments that require it is
        done before the implementation function  is called and the
        corresponding reconstruction is done after.
        """
        rtype = self.get_client_type(desc.return_type())
        service_name = self.get_service_impl_name(desc)
        parms = self.generate_java_service_parms(desc)
        class_prefix = class_desc.short_name()
        writer.write("public {rtype} {name}({parms}) {{\n".format(rtype=rtype, name=service_name, parms=parms))
        writer.indent()
        impl_name = desc.name()
        class_name = self.get_class_name(class_desc)

        if desc.is_impl_default():
            writer.write("return 0;\n")
        else:
            args = self.generate_java_arg_list(desc.parameters())
            impl_call = "impl_{name}{suffix}({args})".format(name=impl_name,suffix=desc.suffix(),args=args)
            
            if "none" == desc.return_type().name():
                writer.write(impl_call + ";\n")
                #for parm in desc.parameters():
                #    self.write_arg_return(writer, parm)
            elif desc.return_type().is_class():
                writer.write("long implRet = {};\n".format(impl_call))
                #for parm in desc.parameters():
                #    self.write_arg_return(writer, parm)

                self.write_get_client_object(writer, "clientRet", desc.return_type(), "implRet")
                writer.write("return clientRet;\n")
            else:
                writer.write("{} ret = {};\n".format(rtype, impl_call))
                #for parm in desc.parameters():
                #    self.write_arg_return(writer, parm)
                writer.write("return ret;\n")
        
        writer.outdent()
        writer.write("}\n")
        
        #if desc.is_vararg():
        #    writer.write("\n")
        #    self.write_vararg_service_impl(writer, desc, class_name)

    def write_vararg_service_impl(self, writer, desc, class_name):
        """
        Writes the implementation of a client API class
        vararg service.
        Vararg functions are expected to have equivalent functions
        that take an array instead of the vararg. As such,
        their implementation simply re-packages the vararg into
        an array and calls the array version of the function.
        """
        rtype = self.get_client_type(desc.return_type())
        service_name = self.get_service_impl_name(desc)
        vararg = desc.parameters()[-1]
        vararg_type = self.get_client_type(vararg.type())

        parms = self.generate_vararg_parm_list(desc.parameters())
        writer.write("{rtype} {name}({parms}) {{\n".format(rtype=rtype,name=service_name,parms=parms))

        args = ", ".join([p.name() for p in desc.parameters()])
        writer.write("{t}* {arg} = new {t}[{num}];\n".format(t=vararg_type,arg=vararg.name(),num=vararg.array_len()))
        writer.write("va_list vararg;\n")
        writer.write("va_start(vararg, {num});\n".format(num=vararg.array_len()))
        writer.write("for (int i = 0; i < {num}; ++i) {{ {arg}[i] = va_arg(vararg, {t}); }}\n".format(num=vararg.array_len(),arg=vararg.name(),t=vararg_type))
        writer.write("va_end(vararg);\n")

        get_ret = "" if "none" == desc.return_type().name() else "{rtype} ret = ".format(rtype=rtype)
        callee_name = self.get_service_call_name(desc, desc.owning_class())
        writer.write("{get_ret}{name}({args});\n".format(get_ret=get_ret,name=callee_name,args=args))

        writer.write("delete[] {arg};\n".format(arg=vararg.name()))
        if "none" != desc.return_type().name():
            writer.write("return ret;\n")
        writer.write("}\n")

    def write_callback_thunk(self, writer, class_desc, callback_desc):
        """
        Writes the implementation of a callback thunk.

        A callback is implemented as a thunk that forwards the call
        to a member function on the object that the the callback is
        called on (the first argument of the callback).
        An abstract example:

        ```
            callback_thunk(obj, ...) {
                return obj->callback(...);
            }
        ```
        """
        rtype = self.get_client_type(callback_desc.return_type())
        thunk = self.callback_thunk_name(class_desc, callback_desc)
        ctype = self.get_client_type(class_desc.as_type())
        callback = callback_desc.name()
        args = self.generate_callback_arg_list(callback_desc.parameters())
        parms = self.generate_callback_parm_list(callback_desc.parameters())

        writer.write('extern "C" {rtype} {thunk}({parms}) {{\n'.format(rtype=rtype,thunk=thunk,parms=parms))
        writer.indent()
        writer.write("{ctype} client = {clientObj};\n".format(ctype=ctype,clientObj=self.to_client_cast(class_desc,"clientObj")))
        writer.write("return {shortname}_{callback}(client{comma}{args});\n".format(shortname=class_desc.short_name(), callback=callback, args=args, comma=", " if len(args) > 0 else ""))
        writer.outdent()
        writer.write("}\n")

    def write_java_native_service_decl(self, writer, desc, class_desc):
        rtype = self.get_impl_type(desc.return_type())
        service_name = self.get_service_impl_name(desc)
        parms = self.generate_java_service_parms(desc, is_client=False)
        class_prefix = class_desc.short_name()
        writer.write("private native {rtype} impl_{name}{suffix}({parms});".format(rtype=rtype, name=service_name, suffix=desc.suffix(), parms=parms))

    def write_java_native_getClientObj(self, writer, desc):
        client_type = self.get_client_class_name(desc);
        fname = self.generate_getClientObj_fname(desc)
        writer.write("native static {client_type} {fname}(long implObj);\n".format(client_type=client_type, fname=fname))

    def write_java_native_setClientObj(self, writer, desc):
        client_type = self.get_client_class_name(desc)
        fname = self.generate_setClientObj_fname(desc)
        writer.write("native static void {fname}({client_type} clientObj, long implObj);\n".format(fname=fname, client_type=client_type))

    def write_java_class_impl(self, writer, class_desc):
        """Write the implementation of a client API Java class."""

        name = class_desc.name()
        full_name = self.get_class_name(class_desc)

        super = " extends " + self.get_class_name(class_desc.parent()) if class_desc.has_parent() else ""
        writer.write("public class {cname}{super}\n{{\n\n".format(cname=name, super=super))

        # write source for inner classes first
        writer.indent()
        for c in class_desc.inner_classes():
            self.write_java_class_impl(writer, c)

        # write constructor definitions
        self.write_impl_ctor_impl(writer, class_desc)
        writer.write("\n")

        for ctor in class_desc.constructors():
            self.write_java_ctor(writer, class_desc, ctor)
        writer.write("\n")

        # write impl getter definition
        #self.write_java_impl_getter_impl(writer, class_desc) # from C generator
        #writer.write("\n")

        # write service definitions
        for s in class_desc.services():
            if s.suffix() != "New":
                self.write_java_class_service_impl(writer, s, class_desc)
                writer.write("\n")

        # write any array argument transformers required
        # transformers for inner classes are created by outermost class
        if not class_desc.containing_classes():
            self.write_array_arg_transformers(writer, class_desc)

        # write native declarations
        init_service = self.get_class_inititializer(class_desc)
        self.write_java_native_service_decl(writer, init_service, class_desc)
        writer.write("\n")

        if not class_desc.containing_classes():
            for ctor in class_desc.constructors():
                self.write_native_ctor(writer, ctor)
        for c in class_desc.inner_classes():
            for ctor in c.constructors():
                self.write_native_ctor(writer, ctor)

        if not class_desc.containing_classes():
            self.write_java_native_getClientObj(writer, class_desc)
            self.write_java_native_setClientObj(writer, class_desc)
            for c in class_desc.inner_classes():
                self.write_java_native_getClientObj(writer, c)
                self.write_java_native_setClientObj(writer, c)

        for s in class_desc.services():
            if s.suffix() != "New":
                self.write_java_native_service_decl(writer, s, class_desc)
                writer.write("\n")

        # write out fields
        for field in class_desc.fields():
            decl = self.generate_field_decl(field)
            writer.write(decl);

        # write needed impl field
        if not class_desc.has_parent():
            writer.write("long _impl;\n")

        writer.outdent()
        writer.write("}\n\n");

    def jni_receiver_name(self):
        return "receiver"

    def generate_jni_function_name(self, class_desc, cname, fname):
        # handle JNI special characters _ and $
        mod_fname=fname.replace("_", "_1")
        mod_cname=cname.replace("$", "_00024")

        # by design not using overloaded native methods which means argument signature should not be  needed here
        # if overloading is ever enabled, need to include signature and then need additional remappings for ; and [ in signatures
        # mod_args=args.replace(";", "_2").replace("[","_3")

        namespace = "_".join(class_desc.as_type().api.namespaces())
        return "Java_org_eclipse_{ns}_{cname}_{fname}".format(ns=namespace, cname=mod_cname,fname=mod_fname)

    def generate_jni_parm(self, parm_desc):
        """
        Produces a JNI parameter declaration from a given parameter description.
        The JNI parameter declaration is usable in a JNI function declaration.
        """
        fmt = "{t}* {n}" if parm_desc.is_array() else "{t} {n}"
        t = parm_desc.type()
        t = "jlong" if t.is_class() else self.jni_native_type_map[t.name()]
        return fmt.format(t=t,n=parm_desc.name())

    def generate_jni_ctor_parms(self, ctor_desc):
        parms = ["JNIEnv *env"] + [self.generate_jni_parm(p) for p in ctor_desc.parameters()]
        return ", ".join(parms)

    def generate_jni_parms(self, ctor_desc):
        parms = ["jlong " + self.jni_receiver_name()] + [self.generate_jni_parm(p) for p in ctor_desc.parameters()]
        return ", ".join(parms)

    def generate_cpp_namespace(self, class_desc):
        return "::".join(class_desc.as_type().api.namespaces())

    def get_impl_class_name(self, c, fully_qualified=False):
        """
        Returns the name of a given class for the JNI implementation.

        If the class is a nested class, then the name is prefixed with name
        of all containing classes, separated by the scope resolution operator.
        """
        namespace = self.generate_cpp_namespace(c) + "::" if fully_qualified else ""
        return namespace + "::".join(c.containing_classes() + [c.name()])

    def generate_jni_arg(self, fclass, parm_desc):
        """
        Produces a JNI argument from a given parameter description.
        The JNI argument is usable in a JNI function call
        """
        fmt = "static_cast<{t}*>({n})" if parm_desc.is_array() else "static_cast<{t}>({n})"
        t = parm_desc.type()
        t = self.generate_cpp_namespace(fclass) + "::" + t.as_class().name() if t.is_class() else self.get_impl_type(t)
        return fmt.format(t=t,n=parm_desc.name())

    def generate_jni_args(self, class_desc, func_desc):
        parms = [self.generate_jni_arg(class_desc, p) for p in func_desc.parameters()]
        return ", ".join(parms)

    def write_jni_constructor_function(self, writer, class_desc, ctor_class, ctor):
        # Java: private static native long new<inner class>(constructor args)
        # JNI: JNIEXPORT jlong JNICALL Java_org_eclipse_omr_jitbuilder_MethodBuilder_newMethodBuilder (JNIEnv *, jclass, jlong);
        class_name = class_desc.name()
        fname = self.generate_jni_function_name(class_desc, class_name, self.generate_native_ctor_name(ctor))
        parms = self.generate_native_parms(ctor)
        writer.write("JNIEXPORT jlong JNICALL {fname}({parms}) {{\n".format(fname=fname, parms=parms))
        writer.indent()
        impl_class_name = self.get_impl_class_name(ctor_class)
        args = self.generate_jni_args(ctor_class, ctor)
        namespace = self.generate_cpp_namespace(ctor_class)
        writer.write("return static_cast<jlong>( ::new {ns}::{cname}({args}) );\n".format(ns=namespace, cname=impl_class_name, args=args))
        writer.outdent()
        writer.write("}\n")

    def write_jni_class_service_impl(self, writer, service, class_desc):
        # JNI: JNIEXPORT <return type> JNICALL Java_org_eclipse_<ns>_<cname>_<service name> (JNIEnv *, [jobject receiver | jclass owningClass], <args>);
        class_name = class_desc.name()
        raw_rtype = service.return_type()
        rtype = "jlong" if raw_rtype.is_class() else self.jni_native_type_map[raw_rtype.name()]
        fname = self.generate_jni_function_name(class_desc, class_name, self.generate_native_service_name(service))
        parms = self.generate_jni_parms(service)
        writer.write("JNIEXPORT {rtype} JNICALL {fname}({parms}) {{\n".format(rtype=rtype, fname=fname, parms=parms))
        writer.indent()
        impl_rtype = self.get_impl_type(raw_rtype)
        namespace = self.generate_cpp_namespace(class_desc)
        impl_class_name = namespace + "::" + self.get_impl_class_name(class_desc)
        args = self.generate_jni_args(class_desc, service)
        if service.is_static():
            call = "{cname}::{fname}({args})".format(cname=impl_class_name, fname=service.name(), args=args)
        else:
            call = "static_cast<{cname} *>(receiver)->{fname}({args})".format(cname=impl_class_name, fname=service.name(), args=args)
        if "none" == service.return_type().name():
            writer.write(call + ";\n")
        else:
            fmt = "{t} * rc = {v};\n" if raw_rtype.is_class() else "{t} rc = {v};\n"
            t = raw_rtype
            t = impl_class_name if t.is_class() else self.get_impl_type(t)
            writer.write(fmt.format(t=t,v=call))
            writer.write("return static_cast<{rtype}>(rc);\n".format(rtype=rtype))
        writer.outdent()
        writer.write("}\n")

    def generate_java_namespace(self, class_desc):
        return "/".join(class_desc.as_type().api.namespaces())

    def get_java_class_name(self, class_desc):
        namespace = self.generate_java_namespace(class_desc)
        cname = "/".join(class_desc.containing_classes() + [class_desc.name()])
        return "org/eclipse/{ns}/{cname}".format(ns=namespace, cname=cname)

    def get_jni_type_signature(self, type):
        return "L"+self.get_java_class_name(type.as_class())+";" if type.is_class() else self.jni_type_signature_string_map[type.name()]

    def get_jni_type_name(self, type):
        # HACK!
        return "Object" if type.is_class() else self.jni_type_api_name_map[type.name()]

    def write_jni_field_init(self, writer, class_desc, field_desc):
        writer.write("\n")
        # static jfieldID valId = env->GetFieldID(jcls, <field name>, <field type signature>);
        # env->Set<field type>Field(jobj, valId, <value>);
        field_name = field_desc.name()
        field_signature =  self.get_jni_type_signature(field_desc.type())
        writer.write("static jfieldID {field_name}Id = env->GetFieldID(implClass, \"{field_name}\", \"{field_signature}\");\n".format(field_name=field_name, field_signature=field_signature))
        field_type = self.get_jni_type_name(field_desc.type())
        field_value = "impl->{field}".format(field=field_name)
        if field_desc.type().is_class():
            field_value = field_value + "->client()"
        jni_type = "jobject" if field_desc.type().is_class() else self.jni_native_type_map[field_desc.type().name()]
        field_value = "static_cast<{jni_type}>( {field} )".format(jni_type=jni_type, field=field_value)
        writer.write("env->Set{field_type}Field(clientObj, {field_name}Id, {value} );\n".format(field_type=field_type, field_name=field_name, value=field_value))

    def write_jni_initializeFromImpl(self, writer, class_desc):
        # Java: private native void impl_initializeFromImpl(long impl)
        # JNI: JNIEXPORT void JNICALL Java_org_eclipse_omr_jitbuilder_IlBuilder_00024JBCase_impl_1initializeFromImpl
        class_name = self.get_full_class_name(class_desc)
        init_service = self.get_class_inititializer(class_desc)
        fname = "impl_" + self.get_service_call_name(init_service)
        full_fname = self.generate_jni_function_name(class_desc, class_name, fname)
        writer.write("JNIEXPORT void JNICALL {fname}(JNIEnv *env, jobject clientObj, jlong implObj) {{\n".format(fname=full_fname))
        writer.indent()
        impl_type = self.get_impl_class_name(class_desc, fully_qualified=True)
        writer.write("{type} *impl = static_cast<{type} *>(implObj);\n".format(type=impl_type))

        if class_desc.fields():
            # static jclass implClass=env->FindClass(<class name using slashes>);
            full_class_name=self.get_java_class_name(class_desc)
            writer.write("static  jclass implClass=env->FindClass(\"{full_class_name}\");\n".format(full_class_name=full_class_name))

            for field in class_desc.fields():
                self.write_jni_field_init(writer, class_desc, field)

            writer.write("\n")

        writer.outdent()
        writer.write("}\n")
        return

    def write_jni_getClientObj(self, writer, class_desc):
        # JNI: JNIEXPORT jobject JNICALL Java_org_eclipse_omr_jitbuilder_<top class>_get<class_list>ClientObj (JNIEnv *, jclass, jlong);
        class_name = self.get_full_class_name(class_desc)
        full_fname = self.generate_jni_function_name(class_desc, class_name, self.generate_getClientObj_fname(class_desc))
        writer.write("JNIEXPORT void JNICALL {fname}(JNIEnv *env, jclass cls, jlong implObj) {{\n".format(fname=full_fname))
        writer.indent()
        impl_type = self.get_impl_class_name(class_desc, fully_qualified=True)
        writer.write("{type} *impl = static_cast<{type} *>(implObj);\n".format(type=impl_type))
        writer.write("return impl->client();\n")
        writer.outdent()
        writer.write("}\n")

    def write_jni_setClientObj(self, writer, class_desc):
        # JNI: JNIEXPORT jobject JNICALL Java_org_eclipse_omr_jitbuilder_<top class>_set<class_list>ClientObj (JNIEnv *, jclass, jobject, jlong);
        class_name = self.get_full_class_name(class_desc)
        full_fname = self.generate_jni_function_name(class_desc, class_name, self.generate_setClientObj_fname(class_desc))
        writer.write("JNIEXPORT void JNICALL {fname}(JNIEnv *env, jclass cls, jobject clientObj, jlong implObj) {{\n".format(fname=full_fname))
        writer.indent()
        writer.write("void *clientPtr = static_cast<void *>( env->NewGlobalRef(clientObj) );\n")
        impl_type = self.get_impl_class_name(class_desc, fully_qualified=True)
        writer.write("{type} *impl = static_cast<{type} *>(implObj);\n".format(type=impl_type))
        writer.write("impl->setClient(clientPtr);\n")
        writer.outdent()
        writer.write("}\n")

    def write_jni_callback(self, writer, class_desc, callback):
        return

    def write_jni_class(self, writer, class_desc):
        self.write_jni_initializeFromImpl(writer, class_desc)
        writer.write("\n")

        self.write_jni_getClientObj(writer, class_desc)
        writer.write("\n")

        self.write_jni_setClientObj(writer, class_desc)
        writer.write("\n")

        for ctor in class_desc.constructors():
            self.write_jni_constructor_function(writer, class_desc, class_desc, ctor)
            writer.write("\n")

        for s in class_desc.services():
            self.write_jni_class_service_impl(writer, s, class_desc)
            writer.write("\n")

        for callback in class_desc.callbacks():
            self.write_jni_callback(writer, class_desc, callback)
            writer.write("\n")

    def write_jni_impl(self, writer, class_desc):
        """Write the implementation of a client API class."""

        writer.write('extern "C" {\n')

        # handle inner classes first
        for c in class_desc.inner_classes():
            self.write_jni_class(writer, c)

        # now outermost class
        self.write_jni_class(writer, class_desc)

        writer.write("}\n")

    def write_jni_source(self, writer, class_desc, namespaces, class_names):
        """
        Writes the implementation (source) for a client API class
        from the class description.
        """
        writer.write(self.get_copyright_header())
        writer.write("\n")

        # don't bother checking what headers are needed, include everything
        for h in self.impl_include_files:
            writer.write(self.generate_include(h))
        writer.write('#include "jni.h"\n')
        writer.write("\n")

        self.write_jni_impl(writer, class_desc)

    def write_java_source(self, writer, class_desc, namespaces, class_names):
        """
        Writes the Java implementation (source) for a client API class
        from the class description.
        """
        writer.write(self.get_copyright_header())
        writer.write("\n")

        writer.write(self.get_java_package_text())
        writer.write("\n");

        self.write_java_class_impl(writer, class_desc)

    # common utilities ###################################################
    def generate_allocator_setting(self, class_desc):
        """
        Given a class description, generates a list of calls that set
        the client API object allocators for the class itself and any
        nested classes.

        The generated code allows the implementation to allocated objects
        by invoking these allocators as callbacks.
        """
        registrations = []
        for c in class_desc.inner_classes():
            registrations += self.generate_allocator_setting(c)
        registrations += "{iname}::setClientAllocator({alloc});\n".format(iname=self.get_impl_class_name(class_desc),cname=self.get_class_name(class_desc),alloc=self.get_allocator_name(class_desc))
        return registrations

    def write_class(self, jni_dir, java_dir, class_desc, namespaces, class_names):
        """Generates and writes a client API class from its description."""

        cname = class_desc.name()
        java_source_path = os.path.join(java_dir, cname + ".java")
        with open(java_source_path, "w") as writer:
            self.write_java_source(PrettyPrinter(writer), class_desc, namespaces, class_names)

        jni_source_path = os.path.join(jni_dir, cname + ".cpp")
        with open(jni_source_path, "w") as writer:
            self.write_jni_source(PrettyPrinter(writer), class_desc, namespaces, class_names)

    def write_service_impl(self, writer, desc, namespace=""):
        """
        Writes the implementation of client API (non-class) service.
        The same approach is used to implement non-class services
        as is used for class services.
        """
        rtype = self.get_client_type(desc.return_type())
        name = desc.name()
        parms = self.generate_parm_list(desc.parameters(), namespace)
        writer.write("extern \"C\" {rtype} {name}({parms}) {{\n".format(rtype=rtype, name=name, parms=parms))
        writer.indent()

        if desc.sets_allocators():
            writer.write("{}();\n".format(self.allocator_setter_name))

        for parm in desc.parameters():
            self.write_arg_setup(writer, parm)

        args = self.generate_arg_list(desc.parameters())
        impl_call = "{sname}({args})".format(sname= cgen.get_impl_service_name(desc),args=args)
        if "none" == desc.return_type().name():
            writer.write(impl_call + ";\n")
            for parm in desc.parameters():
                self.write_arg_return(writer, parm)
        elif desc.return_type().is_class():
            writer.write("{rtype} implRet = {call};\n".format(rtype=self.get_impl_type(desc.return_type()), call=impl_call))
            for parm in desc.parameters():
                self.write_arg_return(writer, parm)
            writer.write("GET_CLIENT_OBJECT(clientObj, {t}, implRet);\n".format(t=desc.return_type().name()))
            writer.write("return clientObj;\n")
        else:
            writer.write("auto ret = " + impl_call + ";\n")
            for parm in desc.parameters():
                self.write_arg_return(writer, parm)
            writer.write("return ret;\n")

        writer.outdent()
        writer.write("}\n")

        if desc.is_vararg():
            writer.write("\n")
            self.write_vararg_service_impl(writer, desc, class_name)

    def write_common_impl(self, writer, api_desc):
        """Writes the implementation of all client API (non-class) services."""

        writer.write(self.get_copyright_header())
        writer.write("\n")

        for h in self.impl_include_files:
            writer.write(self.generate_include(h))
        writer.write("\n")

        for service in api_desc.services():
            writer.write(self.generate_impl_service_import(service))
        writer.write("\n")

        self.write_allocators_setter(writer, api_desc)
        writer.write("\n")

        ns = "::".join(api_desc.namespaces()) + "::"
        for service in api_desc.services():
            self.write_service_impl(writer, service, ns)
            writer.write("\n")

        writer.write("#ifdef __cplusplus\n")
        writer.write('extern "C" {\n')
        writer.write("#endif // __cplusplus\n\n")

        # write some needed includes and defines
        writer.write("#include <stdint.h>\n")
        writer.write("#include <stdbool.h>\n\n")
        writer.write("#define TOSTR(x) #x\n")
        writer.write("#define LINETOSTR(x) TOSTR(x)\n\n")

        # include headers for each defined class
        for c in api_desc.get_class_names():
            writer.write(self.generate_include(c + ".h"))
        writer.write("\n")

        # write declarations for all services
        ns = "::".join(namespaces) + "::"
        for service in api_desc.services():
            decl = self.generate_service_decl(service, namespace=ns)
            writer.write(decl)
        writer.write("\n")

        writer.write("#ifdef __cplusplus\n")
        writer.write('} // extern "C" \n')
        writer.write("#endif // __cplusplus\n")
        writer.write("#endif // {}_INCL\n".format(api_desc.project()))

# main generator #####################################################

if __name__ == "__main__":
    default_dest = os.path.join(os.getcwd(), "client")
    parser = argparse.ArgumentParser()
    parser.add_argument("--jnidir", type=str, default=default_dest,
                        help="destination directory for the generated JNI source files")
    parser.add_argument("--javadir", type=str, default=default_dest,
                        help="destination directory for the generated Java source files")
    parser.add_argument("description", help="path to the API description file")
    args = parser.parse_args()

    with open(args.description) as api_src:
        api_description = APIDescription.load_json_file(api_src)

    generator = JavaGenerator(api_description, args.jnidir)

    namespaces = api_description.namespaces()
    class_names = api_description.get_class_names()

    for class_desc in api_description.classes():
        generator.write_class(args.jnidir, args.javadir, class_desc, namespaces, class_names)
    with open(os.path.join(args.javadir, "JitBuilder.cpp"), "w") as writer:
        generator.write_common_impl(PrettyPrinter(writer), api_description)

    extras_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), "extras", "java")
    names = os.listdir(extras_dir)
    for name in names:
        if name.endswith(".h"):
            shutil.copy(os.path.join(extras_dir,name), os.path.join(args.jnidir,name))
