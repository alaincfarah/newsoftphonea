package com.newsoftphonea.ui

import android.app.Application
import android.util.Log
import androidx.lifecycle.AndroidViewModel
import androidx.lifecycle.LiveData
import androidx.lifecycle.MutableLiveData
import androidx.lifecycle.viewModelScope
import com.newsoftphonea.R
import com.newsoftphonea.SoftphoneApplication
import com.newsoftphonea.data.CallLogEntity
import com.newsoftphonea.data.CallLogRepository
import com.newsoftphonea.sip.CallState
import com.newsoftphonea.sip.PjsipManager
import com.newsoftphonea.sip.SipEventListener
import com.newsoftphonea.transcription.TranscriptionManager
import com.newsoftphonea.transcription.TranscriptionSegment
import kotlinx.coroutines.Dispatchers
import kotlinx.coroutines.launch

class CallViewModel(application: Application) : AndroidViewModel(application),
    SipEventListener,
    TranscriptionManager.Listener {

    private val app = application as SoftphoneApplication
    private val callLogRepository = CallLogRepository(app.database.callLogDao())

    private val pjsipManager = PjsipManager(
        context = application,
        urlTemplate = application.getString(R.string.cname_url_template)
    )

    private val transcriptionManager = TranscriptionManager(application)

    private val _callState = MutableLiveData(CallState.IDLE)
    val callState: LiveData<CallState> = _callState

    private val _callId = MutableLiveData<Int?>(null)
    val callId: LiveData<Int?> = _callId

    private val _statusText = MutableLiveData("Idle")
    val statusText: LiveData<String> = _statusText

    private val _transcription = MutableLiveData<List<TranscriptionSegment>>(emptyList())
    val transcription: LiveData<List<TranscriptionSegment>> = _transcription

    private val _transcriptionEnabled = MutableLiveData(false)
    val transcriptionEnabled: LiveData<Boolean> = _transcriptionEnabled

    private val _recordingEnabled = MutableLiveData(false)
    val recordingEnabled: LiveData<Boolean> = _recordingEnabled

    private var activeCallLogId: Long? = null
    private var activeCallRemoteUri: String? = null
    private var activeCallDirection: String? = null

    init {
        pjsipManager.setEventListener(this)
        transcriptionManager.listener = this
    }

    fun initialize() {
        val logPath = "${app.filesDir.absolutePath}/pjsip.log"
        pjsipManager.initialize(logPath)
    }

    fun createAccount(username: String, password: String, domain: String, proxy: String?) {
        val accountId = pjsipManager.createAccount(username, password, domain, proxy)
        if (accountId >= 0) {
            pjsipManager.registerAccount(true)
        }
    }

    fun makeCall(targetUri: String) {
        val newCallId = pjsipManager.makeCall(targetUri)
        if (newCallId >= 0) {
            setActiveCall(newCallId, targetUri, "OUT")
        }
    }

    fun answerCall() {
        val id = _callId.value ?: return
        pjsipManager.answerCall(id, 200)
    }

    fun hangupCall() {
        val id = _callId.value ?: return
        pjsipManager.hangupCall(id)
    }

    fun toggleHold(hold: Boolean) {
        val id = _callId.value ?: return
        pjsipManager.holdCall(id, hold)
    }

    fun toggleMute(mute: Boolean) {
        val id = _callId.value ?: return
        pjsipManager.muteCall(id, mute)
    }

    fun blindTransfer(targetUri: String) {
        val id = _callId.value ?: return
        pjsipManager.blindTransfer(id, targetUri)
    }

    fun attendedTransfer(replaceCallId: Int) {
        val id = _callId.value ?: return
        pjsipManager.attendedTransfer(id, replaceCallId)
    }

    fun toggleRecording() {
        val id = _callId.value ?: return
        val isRecording = _recordingEnabled.value == true
        if (isRecording) {
            if (pjsipManager.stopRecording(id)) {
                _recordingEnabled.value = false
            }
        } else {
            val filePath = "${app.filesDir.absolutePath}/call-$id.wav"
            if (pjsipManager.startRecording(id, filePath)) {
                _recordingEnabled.value = true
            }
        }
    }

    fun toggleTranscription(enable: Boolean) {
        _transcriptionEnabled.value = enable
        if (enable) {
            transcriptionManager.start()
        } else {
            val finalTranscript = transcriptionManager.stop()
            saveTranscript(finalTranscript)
        }
    }

    fun handleNetworkChange() {
        pjsipManager.handleNetworkChange()
    }

    private fun setActiveCall(callId: Int, remoteUri: String, direction: String) {
        _callId.value = callId
        activeCallRemoteUri = remoteUri
        activeCallDirection = direction
        _transcription.postValue(emptyList())
        viewModelScope.launch(Dispatchers.IO) {
            val callLogId = callLogRepository.insertCallLog(
                CallLogEntity(
                    callId = callId,
                    remoteUri = remoteUri,
                    direction = direction,
                    startedAtEpochMs = System.currentTimeMillis()
                )
            )
            activeCallLogId = callLogId
        }
    }

    private fun endActiveCall() {
        val currentCallId = _callId.value ?: return
        val finalTranscript = transcriptionManager.stop()
        saveTranscript(finalTranscript)
        viewModelScope.launch(Dispatchers.IO) {
            val existing = callLogRepository.findLatestByCallId(currentCallId)
            if (existing != null) {
                callLogRepository.updateCallLog(
                    existing.copy(
                        endedAtEpochMs = System.currentTimeMillis(),
                        transcription = finalTranscript.ifBlank { existing.transcription }
                    )
                )
            }
        }
        _callId.postValue(null)
        _recordingEnabled.postValue(false)
        _transcriptionEnabled.postValue(false)
    }

    private fun saveTranscript(finalTranscript: String) {
        val callId = _callId.value ?: return
        viewModelScope.launch(Dispatchers.IO) {
            val existing = callLogRepository.findLatestByCallId(callId)
            if (existing != null) {
                callLogRepository.updateCallLog(
                    existing.copy(transcription = finalTranscript.ifBlank { existing.transcription })
                )
            }
        }
    }

    override fun onIncomingCall(callId: Int, fromUri: String, displayName: String?) {
        setActiveCall(callId, fromUri, "IN")
        _callState.postValue(CallState.INCOMING)
        _statusText.postValue("Incoming call")
    }

    override fun onCallState(callId: Int, state: CallState, statusCode: Int) {
        _callState.postValue(state)
        _statusText.postValue("Call state: $state ($statusCode)")
        if (state == CallState.DISCONNECTED) {
            endActiveCall()
        }
    }

    override fun onCallMediaState(callId: Int, isActive: Boolean) {
        _statusText.postValue(
            if (isActive) "Media active" else "Media inactive"
        )
    }

    override fun onRegistrationState(isRegistered: Boolean, statusCode: Int) {
        _statusText.postValue(
            if (isRegistered) "Registered ($statusCode)" else "Unregistered ($statusCode)"
        )
    }

    override fun onPartial(segment: TranscriptionSegment) {
        val current = _transcription.value.orEmpty().toMutableList()
        if (current.lastOrNull()?.isFinal == false) {
            current[current.lastIndex] = segment
        } else {
            current.add(segment)
        }
        _transcription.postValue(current)
    }

    override fun onFinal(segment: TranscriptionSegment) {
        val current = _transcription.value.orEmpty().toMutableList()
        current.add(segment)
        _transcription.postValue(current)
    }

    override fun onAudioBuffer(buffer: ByteArray) {
        // Audio buffer captured from VOICE_COMMUNICATION stream for analysis or custom engines.
    }

    override fun onError(errorCode: Int) {
        Log.w(TAG, "Transcription error: $errorCode")
    }

    companion object {
        private const val TAG = "CallViewModel"
    }
}
