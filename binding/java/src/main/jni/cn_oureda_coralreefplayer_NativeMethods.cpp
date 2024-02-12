#include <unordered_map>
#include "cn_oureda_coralreefplayer_NativeMethods.h"
#include "coralreefplayer.h"

#ifdef ANDROID
#define JENV_TYPE JNIEnv
#else
#define JENV_TYPE void
#endif

JavaVM *g_jvm = NULL;
std::unordered_map<crp_handle, jobject> g_callbacks;

static void ThrowJavaException(JNIEnv* jenv, const char* exc, const char* msg) {
    jenv->ExceptionClear();
    jclass cls = jenv->FindClass(exc);
    if (cls)
        jenv->ThrowNew(cls, msg);
}

void java_callback(int event, void *data, void *user_data) {
    jobject callback = g_callbacks[(crp_handle) user_data];
    JNIEnv *jenv;
    jint result = g_jvm->GetEnv((void **) &jenv, JNI_VERSION_1_6);
    if (result == JNI_EDETACHED) {
        if (g_jvm->AttachCurrentThread((JENV_TYPE **) &jenv, NULL) != JNI_OK) {
            return;
        }
    }
    jclass cls = jenv->GetObjectClass(callback);
    if (event == CRP_EV_NEW_FRAME) {
        Frame *frame = (Frame *) data;
        jmethodID method = jenv->GetMethodID(cls, "onFrame", "(III[BJ)V");
        int size = frame->linesize[0] * frame->height;
        jbyteArray array = jenv->NewByteArray(size);
        jenv->SetByteArrayRegion(array, 0, size, (jbyte *) frame->data[0]);
        jenv->CallVoidMethod(callback, method, frame->width, frame->height, frame->format, array, (jlong) frame->pts);
        jenv->DeleteLocalRef(array);
    } else {
        jmethodID method = jenv->GetMethodID(cls, "onEvent", "(IJ)V");
        jenv->CallVoidMethod(callback, method, event, (jlong) data);
    }
    if (result == JNI_EDETACHED) {
        g_jvm->DetachCurrentThread();
    }
}

jlong JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1create(JNIEnv *jenv, jclass jcls) {
    return (jlong) crp_create();
}

void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1destroy(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    crp_handle handle = (crp_handle) jarg1;
    crp_destroy(handle);
    if (g_callbacks.find(handle) != g_callbacks.end()) {
        jenv->DeleteGlobalRef(g_callbacks[handle]);
        g_callbacks.erase(handle);
    }
}

void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1auth(JNIEnv *jenv, jclass jcls, jlong jarg1, jstring jarg2, jstring jarg3, jboolean jarg4) {
    crp_handle arg1 = (crp_handle) jarg1;
    const char *arg2 = jenv->GetStringUTFChars(jarg2, 0);
    const char *arg3 = jenv->GetStringUTFChars(jarg3, 0);
    bool arg4 = jarg4 ? true : false;

    crp_auth(arg1, arg2, arg3, arg4);

    jenv->ReleaseStringUTFChars(jarg2, arg2);
    jenv->ReleaseStringUTFChars(jarg3, arg3);
}

void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1play(JNIEnv *jenv, jclass jcls, jlong jarg1, jstring jarg2, jint jarg3, jint jarg4, jint jarg5, jint jarg6, jobject jarg7) {
    crp_handle arg1 = (crp_handle) jarg1;
    const char *arg2 = jenv->GetStringUTFChars(jarg2, 0);
    int arg3 = (int) jarg3;
    int arg4 = (int) jarg4;
    int arg5 = (int) jarg5;
    int arg6 = (int) jarg6;

    if (g_jvm == NULL) {
        jenv->GetJavaVM(&g_jvm);
    }
    if (g_callbacks.find(arg1) != g_callbacks.end()) {
        jenv->DeleteGlobalRef(g_callbacks[arg1]);
    }
    g_callbacks[arg1] = jenv->NewGlobalRef(jarg7);
    crp_play(arg1, arg2, arg3, arg4, arg5, arg6, java_callback, arg1);

    jenv->ReleaseStringUTFChars(jarg2, arg2);
}

void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1replay(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    crp_replay((crp_handle) jarg1);
}

void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1stop(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    crp_stop((crp_handle) jarg1);
}

jint JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1version_1code(JNIEnv *jenv, jclass jcls) {
    return (jint) crp_version_code();
}

jstring JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1version_1str(JNIEnv *jenv, jclass jcls) {
    return jenv->NewStringUTF(crp_version_str());
}
