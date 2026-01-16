package com.newsoftphonea.transcription

data class TranscriptionSegment(
    val text: String,
    val isFinal: Boolean,
    val timestampMs: Long
)
