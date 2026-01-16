package com.newsoftphonea.sip

enum class CallState {
    IDLE,
    INCOMING,
    CALLING,
    EARLY,
    CONNECTED,
    CONFIRMED,
    DISCONNECTED,
    HOLD,
    REMOTE_HOLD;

    companion object {
        fun fromNative(value: Int): CallState = entries.getOrNull(value) ?: IDLE
    }
}
