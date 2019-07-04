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

class MethodBuilderHelper extends
{
   MethodBuilderHelper(String name, Class<?> returnClass, LinkedList<Class<?>> parameterClasses) {
      _name = name;
      _returnClass = returnClass;
      _parameterClasses = parameterTypes;
   }

   MethodHandle createMethodHandle(long address) throws Throwable {
      Field unsafeField = Unsafe.class.getDeclaredField("theUnsafe");
      unsafeField.setAccessible(true);
      Unsafe unsafe = (Unsafe) unsafeField.get(null);

      MethodType signature = MethodType.methodType(_returnClass, _parameterClasses);
      byte[] generatedClass = generateClass(signature.toMethodDescriptorString());
      Class<?> clazz = unsafe.defineAnonymousClass(CompiledMethodClassTemplate.class, generatedClass, null);
      registerNative(clazz, _name, signature, address);

      Method getLookup = clazz.getDeclaredMethod("getLookup");
      MethodHandles.Lookup lookup = (MethodHandles.Lookup)getLookup.invoke(null);
      MethodHandle mh = lookup.findStatic(clazz, _name, signature);

      return mh;
   }

   byte[] generateClass(String descriptorString) {
      ClassWriter cw = new ClassWriter(ClassWriter.COMPUTE_FRAMES + ClassWriter.COMPUTE_MAXS);
      cw.visit(52, ACC_PUBLIC + ACC_SUPER, "CompiledMethodClassTemplate", null, "java/lang/Object", null);
      cw.visitInnerClass("java/lang/invoke/MethodHandles$Lookup", "java/lang/invoke/MethodHandles", "Lookup", ACC_PUBLIC + ACC_FINAL + ACC_STATIC);

      MethodVisitor mv;
      {
         mv = cw.visitMethod(ACC_PUBLIC, "<init>", "()V", null, null);
         mv.visitCode();
         mv.visitVarInsn(ALOAD, 0);
         mv.visitMethodInsn(INVOKESPECIAL, "java/lang/Object", "<init>", "()V", false);
         mv.visitInsn(RETURN);
         mv.visitMaxs(1, 1);
         mv.visitEnd();
      }

      {
         mv = cw.visitMethod(ACC_PUBLIC + ACC_STATIC + ACC_NATIVE, _name, descriptorString, null, null);
         mv.visitEnd();
      }

      {
         mv = cw.visitMethod(ACC_PUBLIC + ACC_STATIC, "getLookup", "()Ljava/lang/invoke/MethodHandles$Lookup;", null, null);
         mv.visitCode();
         mv.visitMethodInsn(INVOKESTATIC, "java/lang/invoke/MethodHandles", "lookup", "()Ljava/lang/invoke/MethodHandles$Lookup;", false);
         mv.visitInsn(ARETURN);
         mv.visitMaxs(1, 0);
         mv.visitEnd();

      }
      cw.visitEnd();

      return cw.toByteArray();
   }

   private void registerNative(Class<?> clazz, String name, MethodType type, long address) {
      //System.out.println(clazz +"." + name + type + "::" + address);
      //for (Method m : clazz.getDeclaredMethods()) {
      //   System.out.println(m);
      //}
      if (!registerNativeImpl(clazz, name, type.toMethodDescriptorString(), address)) {
         throw new Error("Failed to register native");
      }
   }

   private static native boolean registerNativeImpl(Class<?> clazz, String name, String type, long address);

   private String         _name;
   private Class<?>       _returnClass;
   private List<Class<?>> _parameterClasses;
};
