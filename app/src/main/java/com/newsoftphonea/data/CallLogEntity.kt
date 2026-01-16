package com.newsoftphonea.data

import androidx.room.ColumnInfo
import androidx.room.Entity
import androidx.room.PrimaryKey

@Entity(tableName = "call_logs")
data class CallLogEntity(
    @PrimaryKey(autoGenerate = true)
    val id: Long = 0,
    @ColumnInfo(name = "call_id")
    val callId: Int,
    @ColumnInfo(name = "remote_uri")
    val remoteUri: String,
    @ColumnInfo(name = "direction")
    val direction: String,
    @ColumnInfo(name = "started_at_epoch_ms")
    val startedAtEpochMs: Long,
    @ColumnInfo(name = "ended_at_epoch_ms")
    val endedAtEpochMs: Long? = null,
    @ColumnInfo(name = "transcription")
    val transcription: String? = null
)
