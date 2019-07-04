#include <jni.h>

JNIEXPORT jboolean JNICALL Java_org_eclipse_omr_jitbuilder_MethodBuilderHelper_registerNativeImpl
  (JNIEnv *env, jclass, jclass clazz, jstring name, jstring sig, jlong address)
{
        JNINativeMethod native = {0};
        native.name = (char *) env->GetStringUTFChars(name, NULL);
        if (NULL == native.name) {
                return JNI_FALSE;
        }
        //printf("NAME=%s\n", native.name);
        native.signature = (char *) env->GetStringUTFChars(sig, NULL);
        if (NULL == native.signature) {
                return JNI_FALSE;
        }
        //printf("SIG=%s\n", native.signature);
        native.fnPtr = (void *)address;
        env->RegisterNatives(clazz, &native, 1);
        env->ReleaseStringUTFChars(name, native.name);
        env->ReleaseStringUTFChars(sig, native.signature);

        return JNI_TRUE;
}
