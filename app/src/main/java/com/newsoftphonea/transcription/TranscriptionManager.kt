package com.newsoftphonea.transcription

import android.content.Context
import android.content.Intent
import android.media.MediaRecorder
import android.os.Bundle
import android.os.Handler
import android.os.Looper
import android.speech.RecognitionListener
import android.speech.RecognizerIntent
import android.speech.SpeechRecognizer
import java.util.concurrent.atomic.AtomicBoolean

class TranscriptionManager(
    private val context: Context
) : RecognitionListener {

    interface Listener {
        fun onPartial(segment: TranscriptionSegment)
        fun onFinal(segment: TranscriptionSegment)
        fun onAudioBuffer(buffer: ByteArray)
        fun onError(errorCode: Int)
    }

    private val mainHandler = Handler(Looper.getMainLooper())
    private val isRunning = AtomicBoolean(false)
    private val transcriptBuilder = StringBuilder()
    private val recognizer: SpeechRecognizer? =
        if (SpeechRecognizer.isRecognitionAvailable(context)) {
            SpeechRecognizer.createSpeechRecognizer(context)
        } else {
            null
        }

    private val audioTap = AudioTap { buffer, read ->
        if (!isRunning.get()) return@AudioTap
        val copy = buffer.copyOf(read)
        listener?.onAudioBuffer(copy)
    }

    var listener: Listener? = null
    var lastErrorCode: Int? = null
        private set

    fun start() {
        if (isRunning.getAndSet(true)) return
        transcriptBuilder.clear()
        if (recognizer == null) {
            listener?.onError(SpeechRecognizer.ERROR_CLIENT)
            isRunning.set(false)
            return
        }
        audioTap.start()
        startListening()
    }

    fun stop(): String {
        if (!isRunning.getAndSet(false)) return transcriptBuilder.toString()
        audioTap.stop()
        recognizer?.stopListening()
        recognizer?.cancel()
        return transcriptBuilder.toString()
    }

    fun getTranscriptSnapshot(): String = transcriptBuilder.toString().trim()

    private fun startListening() {
        val intent = Intent(RecognizerIntent.ACTION_RECOGNIZE_SPEECH).apply {
            putExtra(RecognizerIntent.EXTRA_PARTIAL_RESULTS, true)
            putExtra(RecognizerIntent.EXTRA_LANGUAGE_MODEL, RecognizerIntent.LANGUAGE_MODEL_FREE_FORM)
            putExtra(RecognizerIntent.EXTRA_AUDIO_SOURCE, MediaRecorder.AudioSource.VOICE_COMMUNICATION)
        }
        recognizer?.setRecognitionListener(this)
        recognizer?.startListening(intent)
    }

    private fun restartListeningWithDelay(delayMs: Long) {
        if (!isRunning.get()) return
        mainHandler.postDelayed({ startListening() }, delayMs)
    }

    override fun onReadyForSpeech(params: Bundle?) = Unit

    override fun onBeginningOfSpeech() = Unit

    override fun onRmsChanged(rmsdB: Float) = Unit

    override fun onBufferReceived(buffer: ByteArray?) = Unit

    override fun onEndOfSpeech() = Unit

    override fun onError(error: Int) {
        lastErrorCode = error
        listener?.onError(error)
        restartListeningWithDelay(500L)
    }

    override fun onResults(results: Bundle?) {
        val text = results
            ?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)
            ?.firstOrNull()
            ?.trim()
            .orEmpty()

        if (text.isNotEmpty()) {
            val segment = TranscriptionSegment(
                text = text,
                isFinal = true,
                timestampMs = System.currentTimeMillis()
            )
            transcriptBuilder.append(text).append(' ')
            listener?.onFinal(segment)
        }

        restartListeningWithDelay(200L)
    }

    override fun onPartialResults(partialResults: Bundle?) {
        val text = partialResults
            ?.getStringArrayList(SpeechRecognizer.RESULTS_RECOGNITION)
            ?.firstOrNull()
            ?.trim()
            .orEmpty()

        if (text.isNotEmpty()) {
            val segment = TranscriptionSegment(
                text = text,
                isFinal = false,
                timestampMs = System.currentTimeMillis()
            )
            listener?.onPartial(segment)
        }
    }

    override fun onEvent(eventType: Int, params: Bundle?) = Unit
}
