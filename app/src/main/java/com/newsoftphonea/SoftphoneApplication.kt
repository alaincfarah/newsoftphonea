package com.newsoftphonea

import android.app.Application
import com.newsoftphonea.data.AppDatabase

class SoftphoneApplication : Application() {
    val database: AppDatabase by lazy {
        AppDatabase.build(this)
    }
}
