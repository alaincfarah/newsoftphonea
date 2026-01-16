package com.newsoftphonea.util

object SipHeaderParser {
    fun extractDisplayName(fromHeader: String): String? {
        val trimmed = fromHeader.trim()
        val quotedMatch = "\"([^\"]+)\"".toRegex().find(trimmed)
        if (quotedMatch != null) {
            return quotedMatch.groupValues[1].trim().ifBlank { null }
        }

        val angleIndex = trimmed.indexOf('<')
        if (angleIndex > 0) {
            return trimmed.substring(0, angleIndex).trim().ifBlank { null }
        }

        return null
    }
}
