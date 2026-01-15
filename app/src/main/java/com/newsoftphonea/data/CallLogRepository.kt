package com.newsoftphonea.data

class CallLogRepository(
    private val callLogDao: CallLogDao
) {
    suspend fun insertCallLog(callLog: CallLogEntity): Long = callLogDao.insert(callLog)

    suspend fun updateCallLog(callLog: CallLogEntity) {
        callLogDao.update(callLog)
    }

    suspend fun findLatestByCallId(callId: Int): CallLogEntity? =
        callLogDao.findLatestByCallId(callId)
}
