#include "pjsip_engine.h"

#include <jni.h>
#include <string>
#include <vector>

namespace {
    JavaVM *g_vm = nullptr;
    jclass g_pjsipNativeClass = nullptr;
    jmethodID g_onIncomingCall = nullptr;
    jmethodID g_onCallState = nullptr;
    jmethodID g_onCallMediaState = nullptr;
    jmethodID g_onRegistrationState = nullptr;

    PjsipEngine g_engine;

    std::string JStringToString(JNIEnv *env, jstring value) {
        if (!value) return "";
        const char *chars = env->GetStringUTFChars(value, nullptr);
        std::string result(chars ? chars : "");
        env->ReleaseStringUTFChars(value, chars);
        return result;
    }

    void DispatchIncomingCall(int callId, const std::string &fromUri, const std::string &displayName) {
        if (!g_vm || !g_pjsipNativeClass || !g_onIncomingCall) return;
        JNIEnv *env = nullptr;
        g_vm->AttachCurrentThread(&env, nullptr);
        jstring jFrom = env->NewStringUTF(fromUri.c_str());
        jstring jDisplayName = displayName.empty() ? nullptr : env->NewStringUTF(displayName.c_str());
        env->CallStaticVoidMethod(g_pjsipNativeClass, g_onIncomingCall, callId, jFrom, jDisplayName);
        env->DeleteLocalRef(jFrom);
        if (jDisplayName) {
            env->DeleteLocalRef(jDisplayName);
        }
    }

    void DispatchCallState(int callId, int state, int statusCode) {
        if (!g_vm || !g_pjsipNativeClass || !g_onCallState) return;
        JNIEnv *env = nullptr;
        g_vm->AttachCurrentThread(&env, nullptr);
        env->CallStaticVoidMethod(g_pjsipNativeClass, g_onCallState, callId, state, statusCode);
    }

    void DispatchCallMedia(int callId, bool isActive) {
        if (!g_vm || !g_pjsipNativeClass || !g_onCallMediaState) return;
        JNIEnv *env = nullptr;
        g_vm->AttachCurrentThread(&env, nullptr);
        env->CallStaticVoidMethod(
            g_pjsipNativeClass,
            g_onCallMediaState,
            callId,
            static_cast<jboolean>(isActive)
        );
    }

    void DispatchRegistration(bool registered, int statusCode) {
        if (!g_vm || !g_pjsipNativeClass || !g_onRegistrationState) return;
        JNIEnv *env = nullptr;
        g_vm->AttachCurrentThread(&env, nullptr);
        env->CallStaticVoidMethod(
            g_pjsipNativeClass,
            g_onRegistrationState,
            static_cast<jboolean>(registered),
            statusCode
        );
    }
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeInit(
    JNIEnv *env,
    jobject,
    jstring logPath,
    jint sipPort,
    jint audioPort,
    jint transportType
) {
    g_engine.SetIncomingCallCallback(DispatchIncomingCall);
    g_engine.SetCallStateCallback(DispatchCallState);
    g_engine.SetCallMediaCallback(DispatchCallMedia);
    g_engine.SetRegistrationCallback(DispatchRegistration);
    return g_engine.Init(JStringToString(env, logPath), sipPort, audioPort, transportType);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeCreateAccount(
    JNIEnv *env,
    jobject,
    jstring username,
    jstring password,
    jstring domain,
    jstring proxy
) {
    return g_engine.CreateAccount(
        JStringToString(env, username),
        JStringToString(env, password),
        JStringToString(env, domain),
        JStringToString(env, proxy)
    );
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeRegisterAccount(
    JNIEnv *,
    jobject,
    jint accountId,
    jboolean enable
) {
    return g_engine.RegisterAccount(accountId, enable);
}

extern "C" JNIEXPORT jint JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeMakeCall(
    JNIEnv *env,
    jobject,
    jint accountId,
    jstring targetUri
) {
    return g_engine.MakeCall(accountId, JStringToString(env, targetUri));
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeAnswerCall(
    JNIEnv *,
    jobject,
    jint callId,
    jint statusCode
) {
    return g_engine.AnswerCall(callId, statusCode);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeHangupCall(
    JNIEnv *,
    jobject,
    jint callId
) {
    return g_engine.HangupCall(callId);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeHoldCall(
    JNIEnv *,
    jobject,
    jint callId,
    jboolean hold
) {
    return g_engine.HoldCall(callId, hold);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeMuteCall(
    JNIEnv *,
    jobject,
    jint callId,
    jboolean mute
) {
    return g_engine.MuteCall(callId, mute);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeBlindTransfer(
    JNIEnv *env,
    jobject,
    jint callId,
    jstring targetUri
) {
    return g_engine.BlindTransfer(callId, JStringToString(env, targetUri));
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeAttendedTransfer(
    JNIEnv *,
    jobject,
    jint callId,
    jint replaceCallId
) {
    return g_engine.AttendedTransfer(callId, replaceCallId);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeStartRecording(
    JNIEnv *env,
    jobject,
    jint callId,
    jstring filePath
) {
    return g_engine.StartRecording(callId, JStringToString(env, filePath));
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeStopRecording(
    JNIEnv *,
    jobject,
    jint callId
) {
    return g_engine.StopRecording(callId);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeSetCodecPriorities(
    JNIEnv *,
    jobject,
    jint g729,
    jint pcmu,
    jint pcma
) {
    std::vector<CodecPriority> codecs = {
        {"G729/8000", g729},
        {"PCMU/8000", pcmu},
        {"PCMA/8000", pcma}
    };
    return g_engine.SetCodecPriorities(codecs);
}

extern "C" JNIEXPORT jboolean JNICALL
Java_com_newsoftphonea_sip_PjsipNative_nativeHandleNetworkChange(
    JNIEnv *,
    jobject
) {
    return g_engine.HandleNetworkChange();
}

JNIEXPORT jint JNICALL JNI_OnLoad(JavaVM *vm, void *) {
    g_vm = vm;
    JNIEnv *env = nullptr;
    if (vm->GetEnv(reinterpret_cast<void **>(&env), JNI_VERSION_1_6) != JNI_OK) {
        return JNI_ERR;
    }

    jclass localClass = env->FindClass("com/newsoftphonea/sip/PjsipNative");
    g_pjsipNativeClass = static_cast<jclass>(env->NewGlobalRef(localClass));
    env->DeleteLocalRef(localClass);

    g_onIncomingCall = env->GetStaticMethodID(
        g_pjsipNativeClass,
        "onIncomingCall",
        "(ILjava/lang/String;Ljava/lang/String;)V"
    );
    g_onCallState = env->GetStaticMethodID(
        g_pjsipNativeClass,
        "onCallState",
        "(III)V"
    );
    g_onCallMediaState = env->GetStaticMethodID(
        g_pjsipNativeClass,
        "onCallMediaState",
        "(IZ)V"
    );
    g_onRegistrationState = env->GetStaticMethodID(
        g_pjsipNativeClass,
        "onRegistrationState",
        "(ZI)V"
    );

    return JNI_VERSION_1_6;
}
