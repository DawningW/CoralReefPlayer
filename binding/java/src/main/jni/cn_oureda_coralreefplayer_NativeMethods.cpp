#include <cstring>
#include <unordered_map>
#include "cn_oureda_coralreefplayer_NativeMethods.h"
#include "coralreefplayer.h"

#ifdef ANDROID
#define JENV_TYPE JNIEnv
#else
#define JENV_TYPE void
#endif

JavaVM *g_jvm = NULL;
jclass g_frame_class = NULL;
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
        jmethodID method = jenv->GetMethodID(cls, "onFrame", "(ZLcn/oureda/coralreefplayer/Frame;)V");
        jclass cls2 = g_frame_class;
        jobject jframe = jenv->NewObject(cls2, jenv->GetMethodID(cls2, "<init>", "()V"));
        jenv->SetIntField(jframe, jenv->GetFieldID(cls2, "width", "I"), frame->width);
        jenv->SetIntField(jframe, jenv->GetFieldID(cls2, "height", "I"), frame->height);
        jenv->SetIntField(jframe, jenv->GetFieldID(cls2, "format", "I"), frame->format);
        jobjectArray data = jenv->NewObjectArray(4, jenv->FindClass("java/nio/ByteBuffer"), NULL);
        for (int i = 0; i < 4; i++) {
            if (frame->data[i] == NULL) {
                break;
            }
            jobject buffer = jenv->NewDirectByteBuffer(frame->data[i], frame->linesize[i] * (frame->height >> !!i));
            jenv->SetObjectArrayElement(data, i, buffer);
        }
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "data", "[Ljava/nio/ByteBuffer;"), data);
        jintArray linesize = jenv->NewIntArray(4);
        jenv->SetIntArrayRegion(linesize, 0, 4, (const jint*) frame->linesize);
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "linesize", "[I"), linesize);
        jenv->SetLongField(jframe, jenv->GetFieldID(cls2, "pts", "J"), frame->pts);
        jenv->CallVoidMethod(callback, method, false, jframe);
    } else if (event == CRP_EV_NEW_AUDIO) {
        Frame *frame = (Frame *) data;
        jmethodID method = jenv->GetMethodID(cls, "onFrame", "(ZLcn/oureda/coralreefplayer/Frame;)V");
        jclass cls2 = g_frame_class;
        jobject jframe = jenv->NewObject(cls2, jenv->GetMethodID(cls2, "<init>", "()V"));
        jenv->SetIntField(jframe, jenv->GetFieldID(cls2, "sampleRate", "I"), frame->sample_rate);
        jenv->SetIntField(jframe, jenv->GetFieldID(cls2, "channels", "I"), frame->channels);
        jenv->SetIntField(jframe, jenv->GetFieldID(cls2, "format", "I"), frame->format);
        jobjectArray data = jenv->NewObjectArray(1, jenv->FindClass("java/nio/ByteBuffer"), NULL);
        jobject buffer = jenv->NewDirectByteBuffer(frame->data[0], frame->linesize[0]);
        jenv->SetObjectArrayElement(data, 0, buffer);
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "data", "[Ljava/nio/ByteBuffer;"), data);
        jintArray linesize = jenv->NewIntArray(1);
        jenv->SetIntArrayRegion(linesize, 0, 1, (const jint*) frame->linesize);
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "linesize", "[I"), linesize);
        jenv->SetLongField(jframe, jenv->GetFieldID(cls2, "pts", "J"), frame->pts);
        jenv->CallVoidMethod(callback, method, true, jframe);
    } else {
        jmethodID method = jenv->GetMethodID(cls, "onEvent", "(IJ)V");
        jenv->CallVoidMethod(callback, method, event, (jlong) data);
    }
    if (result == JNI_EDETACHED) {
        g_jvm->DetachCurrentThread();
    }
}

jlong JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1create(JNIEnv *jenv, jclass jcls) {
    if (g_jvm == NULL) {
        jenv->GetJavaVM(&g_jvm);
    }
    // See https://developer.android.google.cn/training/articles/perf-jni?hl=zh-cn#faq:-why-didnt-findclass-find-my-class
    if (g_frame_class == NULL) {
        jclass cls = jenv->FindClass("cn/oureda/coralreefplayer/Frame");
        g_frame_class = (jclass)jenv->NewGlobalRef(cls);
    }
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

void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1play(JNIEnv *jenv, jclass jcls, jlong jarg1, jstring jarg2, jobject jarg3, jobject jarg4) {
    crp_handle arg1 = (crp_handle) jarg1;
    const char *arg2 = jenv->GetStringUTFChars(jarg2, 0);

    jclass cls = jenv->GetObjectClass(jarg3);
    Option option{};
    option.transport = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "transport", "I"));
    option.video.width = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "width", "I"));
    option.video.height = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "height", "I"));
    option.video.format = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "videoFormat", "I"));
    jstring str = (jstring) jenv->GetObjectField(jarg3, jenv->GetFieldID(cls, "hwDevice", "Ljava/lang/String;"));
    if (str != NULL) {
        const char* hwDevice = jenv->GetStringUTFChars(str, NULL);
        strncpy(option.video.hw_device, hwDevice, sizeof(option.video.hw_device));
        option.video.hw_device[sizeof(option.video.hw_device) - 1] = '\0';
        jenv->ReleaseStringUTFChars(str, hwDevice);
    }
    option.enable_audio = jenv->GetBooleanField(jarg3, jenv->GetFieldID(cls, "enableAudio", "Z"));
    option.audio.sample_rate = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "sampleRate", "I"));
    option.audio.channels = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "channels", "I"));
    option.audio.format = jenv->GetIntField(jarg3, jenv->GetFieldID(cls, "audioFormat", "I"));
    option.timeout = jenv->GetLongField(jarg3, jenv->GetFieldID(cls, "timeout", "J"));

    if (g_callbacks.find(arg1) != g_callbacks.end()) {
        jenv->DeleteGlobalRef(g_callbacks[arg1]);
    }
    g_callbacks[arg1] = jenv->NewGlobalRef(jarg4);

    crp_play(arg1, arg2, &option, java_callback, arg1);

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
