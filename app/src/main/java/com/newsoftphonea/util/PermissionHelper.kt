package com.newsoftphonea.util

import android.Manifest
import android.app.Activity
import android.content.pm.PackageManager
import androidx.core.app.ActivityCompat
import androidx.core.content.ContextCompat

object PermissionHelper {
    const val REQUEST_CODE = 2001

    private val requiredPermissions = listOf(
        Manifest.permission.RECORD_AUDIO,
        Manifest.permission.MANAGE_OWN_CALLS,
        Manifest.permission.INTERNET
    )

    fun missingPermissions(activity: Activity): List<String> =
        requiredPermissions.filter {
            ContextCompat.checkSelfPermission(activity, it) != PackageManager.PERMISSION_GRANTED
        }

    fun requestMissingPermissions(activity: Activity) {
        val missing = missingPermissions(activity)
        if (missing.isNotEmpty()) {
            ActivityCompat.requestPermissions(activity, missing.toTypedArray(), REQUEST_CODE)
        }
    }
}
