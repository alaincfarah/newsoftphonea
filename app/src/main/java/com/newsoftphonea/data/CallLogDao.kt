package com.newsoftphonea.data

import androidx.room.Dao
import androidx.room.Insert
import androidx.room.Query
import androidx.room.Update

@Dao
interface CallLogDao {
    @Insert
    suspend fun insert(callLog: CallLogEntity): Long

    @Update
    suspend fun update(callLog: CallLogEntity)

    @Query("SELECT * FROM call_logs WHERE call_id = :callId ORDER BY id DESC LIMIT 1")
    suspend fun findLatestByCallId(callId: Int): CallLogEntity?
}
