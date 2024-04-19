/* DO NOT EDIT THIS FILE - it is machine generated */
#include <jni.h>
/* Header for class cn_oureda_coralreefplayer_NativeMethods */

#ifndef _Included_cn_oureda_coralreefplayer_NativeMethods
#define _Included_cn_oureda_coralreefplayer_NativeMethods
#ifdef __cplusplus
extern "C" {
#endif
/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_create
 * Signature: ()J
 */
JNIEXPORT jlong JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1create
  (JNIEnv *, jclass);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_destroy
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1destroy
  (JNIEnv *, jclass, jlong);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_auth
 * Signature: (JLjava/lang/String;Ljava/lang/String;Z)V
 */
JNIEXPORT void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1auth
  (JNIEnv *, jclass, jlong, jstring, jstring, jboolean);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_play
 * Signature: (JLjava/lang/String;Ljava/lang/Object;Ljava/lang/Object;)V
 */
JNIEXPORT void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1play
  (JNIEnv *, jclass, jlong, jstring, jobject, jobject);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_replay
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1replay
  (JNIEnv *, jclass, jlong);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_stop
 * Signature: (J)V
 */
JNIEXPORT void JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1stop
  (JNIEnv *, jclass, jlong);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_version_code
 * Signature: ()I
 */
JNIEXPORT jint JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1version_1code
  (JNIEnv *, jclass);

/*
 * Class:     cn_oureda_coralreefplayer_NativeMethods
 * Method:    crp_version_str
 * Signature: ()Ljava/lang/String;
 */
JNIEXPORT jstring JNICALL Java_cn_oureda_coralreefplayer_NativeMethods_crp_1version_1str
  (JNIEnv *, jclass);

#ifdef __cplusplus
}
#endif
#endif
