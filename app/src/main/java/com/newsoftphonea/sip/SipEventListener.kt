package com.newsoftphonea.sip

interface SipEventListener {
    fun onIncomingCall(callId: Int, fromUri: String, displayName: String?)
    fun onCallState(callId: Int, state: CallState, statusCode: Int)
    fun onCallMediaState(callId: Int, isActive: Boolean)
    fun onRegistrationState(isRegistered: Boolean, statusCode: Int)
}
