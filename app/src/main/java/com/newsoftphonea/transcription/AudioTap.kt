package com.newsoftphonea.transcription

import android.media.AudioFormat
import android.media.AudioRecord
import android.media.MediaRecorder
import java.util.concurrent.atomic.AtomicBoolean

class AudioTap(
    private val sampleRate: Int = 16_000,
    private val onAudioBuffer: (ByteArray, Int) -> Unit
) {
    private val isRunning = AtomicBoolean(false)
    private var audioRecord: AudioRecord? = null
    private var audioThread: Thread? = null

    fun start() {
        if (isRunning.getAndSet(true)) return
        val bufferSize = AudioRecord.getMinBufferSize(
            sampleRate,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT
        ).coerceAtLeast(sampleRate / 10)

        audioRecord = AudioRecord(
            MediaRecorder.AudioSource.VOICE_COMMUNICATION,
            sampleRate,
            AudioFormat.CHANNEL_IN_MONO,
            AudioFormat.ENCODING_PCM_16BIT,
            bufferSize
        ).apply { startRecording() }

        audioThread = Thread {
            val buffer = ByteArray(bufferSize)
            while (isRunning.get()) {
                val read = audioRecord?.read(buffer, 0, buffer.size) ?: 0
                if (read > 0) {
                    onAudioBuffer(buffer, read)
                }
            }
        }.also { it.start() }
    }

    fun stop() {
        if (!isRunning.getAndSet(false)) return
        audioRecord?.stop()
        audioRecord?.release()
        audioRecord = null
        audioThread?.join(200)
        audioThread = null
    }
}
