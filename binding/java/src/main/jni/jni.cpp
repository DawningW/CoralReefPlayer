#include <cstring>
#include <initializer_list>
#include <unordered_map>
#include <jni.h>
#ifdef ANDROID
#include <android/native_window.h>
#include <android/native_window_jni.h>
#endif
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
            jobject buffer = jenv->NewDirectByteBuffer(frame->data[i], frame->stride[i] * (frame->height >> !!i));
            jenv->SetObjectArrayElement(data, i, buffer);
        }
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "data", "[Ljava/nio/ByteBuffer;"), data);
        jintArray stride = jenv->NewIntArray(4);
        jenv->SetIntArrayRegion(stride, 0, 4, (const jint*) frame->stride);
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "stride", "[I"), stride);
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
        jobject buffer = jenv->NewDirectByteBuffer(frame->data[0], frame->stride[0]);
        jenv->SetObjectArrayElement(data, 0, buffer);
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "data", "[Ljava/nio/ByteBuffer;"), data);
        jintArray stride = jenv->NewIntArray(1);
        jenv->SetIntArrayRegion(stride, 0, 1, (const jint*) frame->stride);
        jenv->SetObjectField(jframe, jenv->GetFieldID(cls2, "stride", "[I"), stride);
        jenv->SetLongField(jframe, jenv->GetFieldID(cls2, "pts", "J"), frame->pts);
        jenv->CallVoidMethod(callback, method, true, jframe);
    } else if (event == CRP_EV_VIDEO_EXTRADATA || event == CRP_EV_AUDIO_EXTRADATA) {
        EventData *ed = (EventData *) data;
        jmethodID method = jenv->GetMethodID(cls, "onEvent", "(ILjava/lang/Object;)V");
        jbyteArray data = jenv->NewByteArray(ed->extra_data.size);
        jenv->SetByteArrayRegion(data, 0, ed->extra_data.size, (const jbyte*) ed->extra_data.data);
        jenv->CallVoidMethod(callback, method, event, data);
    } else {
        jmethodID method = jenv->GetMethodID(cls, "onEvent", "(ILjava/lang/Object;)V");
        jclass cls2 = jenv->FindClass("java/lang/Long");
        jobject jdata = jenv->NewObject(cls2, jenv->GetMethodID(cls2, "<init>", "(J)V"), (jlong) data);
        jenv->CallVoidMethod(callback, method, event, jdata);
    }
    if (result == JNI_EDETACHED) {
        g_jvm->DetachCurrentThread();
    }
}

jlong create(JNIEnv *jenv, jclass jcls) {
    return (jlong) crp_create();
}

void destroy(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    crp_handle handle = (crp_handle) jarg1;
    crp_destroy(handle);
    if (g_callbacks.find(handle) != g_callbacks.end()) {
        jenv->DeleteGlobalRef(g_callbacks[handle]);
        g_callbacks.erase(handle);
    }
}

void auth(JNIEnv *jenv, jclass jcls, jlong jarg1, jstring jarg2, jstring jarg3, jboolean jarg4) {
    crp_handle arg1 = (crp_handle) jarg1;
    const char *arg2 = jenv->GetStringUTFChars(jarg2, 0);
    const char *arg3 = jenv->GetStringUTFChars(jarg3, 0);
    bool arg4 = jarg4 ? true : false;

    crp_auth(arg1, arg2, arg3, arg4);

    jenv->ReleaseStringUTFChars(jarg2, arg2);
    jenv->ReleaseStringUTFChars(jarg3, arg3);
}

void play(JNIEnv *jenv, jclass jcls, jlong jarg1, jstring jarg2, jobject jarg3, jobject jarg4) {
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

void replay(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    crp_replay((crp_handle) jarg1);
}

void stop(JNIEnv *jenv, jclass jcls, jlong jarg1) {
    crp_stop((crp_handle) jarg1);
}

jint version_code(JNIEnv *jenv, jclass jcls) {
    return (jint) crp_version_code();
}

jstring version_str(JNIEnv *jenv, jclass jcls) {
    return jenv->NewStringUTF(crp_version_str());
}

#ifdef ANDROID
void render(JNIEnv *env, jclass cls, jobject surface, jobject frame) {
    jclass cls2 = g_frame_class;
    int width = env->GetIntField(frame, env->GetFieldID(cls2, "width", "I"));
    int height = env->GetIntField(frame, env->GetFieldID(cls2, "height", "I"));
    int format = env->GetIntField(frame, env->GetFieldID(cls2, "format", "I"));
    jobjectArray data = static_cast<jobjectArray>(env->GetObjectField(
            frame, env->GetFieldID(cls2, "data", "[Ljava/nio/ByteBuffer;")));
    jobject buffer = env->GetObjectArrayElement(data, 0);
    uint8_t *pData = static_cast<uint8_t *>(env->GetDirectBufferAddress(buffer));
    jintArray stride = static_cast<jintArray>(env->GetObjectField(
            frame, env->GetFieldID(cls2, "stride", "[I")));
    jint *pStride = env->GetIntArrayElements(stride, NULL);

    ANativeWindow *nativeWindow = ANativeWindow_fromSurface(env, surface);

    ANativeWindow_setBuffersGeometry(nativeWindow, width, height, WINDOW_FORMAT_RGBA_8888);

    ANativeWindow_Buffer nativeWindowBuffer;
    ANativeWindow_lock(nativeWindow, &nativeWindowBuffer, nullptr);
    uint8_t *dstBuffer = static_cast<uint8_t *>(nativeWindowBuffer.bits);

    int srcLineSize = pStride[0];
    int dstLineSize = nativeWindowBuffer.stride * 4;
    for (int i = 0; i < height; ++i) {
        memcpy(dstBuffer + i * dstLineSize, pData + i * srcLineSize, srcLineSize);
    }

    ANativeWindow_unlockAndPost(nativeWindow);

    ANativeWindow_release(nativeWindow);
}
#endif

static int registerNativeMethods(JNIEnv *env, const char *className, std::initializer_list<JNINativeMethod> methods) {
    jclass cls = env->FindClass(className);
    if (cls == NULL) {
        return JNI_ERR;
    }
    if (env->RegisterNatives(cls, methods.begin(), methods.size()) < 0) {
        return JNI_ERR;
    }
    return JNI_OK;
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *jvm, void *reserved) {
    JNIEnv *jenv = NULL;
    if (jvm->GetEnv((void **)&jenv, JNI_VERSION_1_4) != JNI_OK) {
        return JNI_ERR;
    }

    if (registerNativeMethods(jenv, "cn/oureda/coralreefplayer/NativeMethods", {
        {"crp_create", "()J", (void *)create},
        {"crp_destroy", "(J)V", (void *)destroy},
        {"crp_auth", "(JLjava/lang/String;Ljava/lang/String;Z)V", (void *)auth},
        {"crp_play", "(JLjava/lang/String;Ljava/lang/Object;Ljava/lang/Object;)V", (void *)play},
        {"crp_replay", "(J)V", (void *)replay},
        {"crp_stop", "(J)V", (void *)stop},
        {"crp_version_code", "()I", (void *)version_code},
        {"crp_version_str", "()Ljava/lang/String;", (void *)version_str},
    }) != JNI_OK) {
        return JNI_ERR;
    }

#ifdef ANDROID
    if (registerNativeMethods(jenv, "cn/oureda/coralreefplayer/PlayerView", {
        {"renderYUVOnSurface", "(Landroid/view/Surface;Lcn/oureda/coralreefplayer/Frame;)V", (void *)render},
    }) != JNI_OK) {
        return JNI_ERR;
    }
#endif

    g_jvm = jvm;
    // See https://developer.android.google.cn/training/articles/perf-jni?hl=zh-cn#faq:-why-didnt-findclass-find-my-class
    jclass cls = jenv->FindClass("cn/oureda/coralreefplayer/Frame");
    g_frame_class = (jclass)jenv->NewGlobalRef(cls);

    return JNI_VERSION_1_4;
}
