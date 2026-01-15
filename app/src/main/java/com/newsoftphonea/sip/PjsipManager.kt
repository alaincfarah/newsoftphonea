package com.newsoftphonea.sip

import android.content.Context
import android.content.Intent
import android.net.Uri
import android.util.Log
import com.newsoftphonea.util.SipHeaderParser
import java.util.Locale

class PjsipManager(
    private val context: Context,
    private val urlTemplate: String
) : SipEventListener {

    private var eventListener: SipEventListener? = null
    private var accountId: Int = -1

    fun setEventListener(listener: SipEventListener?) {
        eventListener = listener
    }

    fun initialize(
        logPath: String,
        sipPort: Int = 5060,
        audioPort: Int = 4000,
        transportType: Int = 0
    ): Boolean {
        PjsipNative.setListener(this)
        val initialized = PjsipNative.nativeInit(logPath, sipPort, audioPort, transportType)
        if (initialized) {
            PjsipNative.nativeSetCodecPriorities(
                g729 = 255,
                pcmu = 240,
                pcma = 230
            )
        }
        return initialized
    }

    fun createAccount(
        username: String,
        password: String,
        domain: String,
        proxy: String? = null
    ): Int {
        accountId = PjsipNative.nativeCreateAccount(username, password, domain, proxy)
        return accountId
    }

    fun registerAccount(enable: Boolean): Boolean {
        if (accountId < 0) return false
        return PjsipNative.nativeRegisterAccount(accountId, enable)
    }

    fun makeCall(targetUri: String): Int {
        if (accountId < 0) return -1
        return PjsipNative.nativeMakeCall(accountId, targetUri)
    }

    fun answerCall(callId: Int, statusCode: Int = 200): Boolean =
        PjsipNative.nativeAnswerCall(callId, statusCode)

    fun hangupCall(callId: Int): Boolean = PjsipNative.nativeHangupCall(callId)

    fun holdCall(callId: Int, hold: Boolean): Boolean =
        PjsipNative.nativeHoldCall(callId, hold)

    fun muteCall(callId: Int, mute: Boolean): Boolean =
        PjsipNative.nativeMuteCall(callId, mute)

    fun blindTransfer(callId: Int, targetUri: String): Boolean =
        PjsipNative.nativeBlindTransfer(callId, targetUri)

    fun attendedTransfer(callId: Int, replaceCallId: Int): Boolean =
        PjsipNative.nativeAttendedTransfer(callId, replaceCallId)

    fun startRecording(callId: Int, filePath: String): Boolean =
        PjsipNative.nativeStartRecording(callId, filePath)

    fun stopRecording(callId: Int): Boolean =
        PjsipNative.nativeStopRecording(callId)

    fun handleNetworkChange(): Boolean =
        PjsipNative.nativeHandleNetworkChange()

    override fun onIncomingCall(callId: Int, fromUri: String, displayName: String?) {
        val cname = displayName?.ifBlank { null }
            ?: SipHeaderParser.extractDisplayName(fromUri)
        if (!cname.isNullOrBlank()) {
            val encoded = Uri.encode(cname)
            val url = String.format(Locale.US, urlTemplate, encoded)
            val intent = Intent(Intent.ACTION_VIEW, Uri.parse(url)).apply {
                addFlags(Intent.FLAG_ACTIVITY_NEW_TASK)
            }
            try {
                context.startActivity(intent)
            } catch (error: Exception) {
                Log.w(TAG, "Unable to launch CNAME URL", error)
            }
        }
        eventListener?.onIncomingCall(callId, fromUri, displayName)
    }

    override fun onCallState(callId: Int, state: CallState, statusCode: Int) {
        eventListener?.onCallState(callId, state, statusCode)
    }

    override fun onCallMediaState(callId: Int, isActive: Boolean) {
        eventListener?.onCallMediaState(callId, isActive)
    }

    override fun onRegistrationState(isRegistered: Boolean, statusCode: Int) {
        eventListener?.onRegistrationState(isRegistered, statusCode)
    }

    companion object {
        private const val TAG = "PjsipManager"
    }
}
