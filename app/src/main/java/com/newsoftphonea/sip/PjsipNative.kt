package com.newsoftphonea.sip

object PjsipNative {
    private var listener: SipEventListener? = null

    init {
        System.loadLibrary("pjsip_jni")
    }

    fun setListener(listener: SipEventListener?) {
        this.listener = listener
    }

    external fun nativeInit(
        logPath: String,
        sipPort: Int,
        audioPort: Int,
        transportType: Int
    ): Boolean

    external fun nativeCreateAccount(
        username: String,
        password: String,
        domain: String,
        proxy: String?
    ): Int

    external fun nativeRegisterAccount(accountId: Int, enable: Boolean): Boolean

    external fun nativeMakeCall(accountId: Int, targetUri: String): Int

    external fun nativeAnswerCall(callId: Int, statusCode: Int): Boolean

    external fun nativeHangupCall(callId: Int): Boolean

    external fun nativeHoldCall(callId: Int, hold: Boolean): Boolean

    external fun nativeMuteCall(callId: Int, mute: Boolean): Boolean

    external fun nativeBlindTransfer(callId: Int, targetUri: String): Boolean

    external fun nativeAttendedTransfer(callId: Int, replaceCallId: Int): Boolean

    external fun nativeStartRecording(callId: Int, filePath: String): Boolean

    external fun nativeStopRecording(callId: Int): Boolean

    external fun nativeSetCodecPriorities(
        g729: Int,
        pcmu: Int,
        pcma: Int
    ): Boolean

    external fun nativeHandleNetworkChange(): Boolean

    @JvmStatic
    fun onIncomingCall(callId: Int, fromUri: String, displayName: String?) {
        listener?.onIncomingCall(callId, fromUri, displayName)
    }

    @JvmStatic
    fun onCallState(callId: Int, state: Int, statusCode: Int) {
        listener?.onCallState(callId, CallState.fromNative(state), statusCode)
    }

    @JvmStatic
    fun onCallMediaState(callId: Int, isActive: Boolean) {
        listener?.onCallMediaState(callId, isActive)
    }

    @JvmStatic
    fun onRegistrationState(isRegistered: Boolean, statusCode: Int) {
        listener?.onRegistrationState(isRegistered, statusCode)
    }
}
