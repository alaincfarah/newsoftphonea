package com.newsoftphonea.network

import android.content.Context
import android.net.ConnectivityManager
import android.net.LinkAddress
import android.net.Network
import android.net.NetworkCapabilities
import android.net.NetworkRequest
import android.util.Log

class VpnNetworkMonitor(
    context: Context,
    private val onNetworkChanged: (isVpn: Boolean, address: String?) -> Unit
) {
    private val connectivityManager =
        context.getSystemService(Context.CONNECTIVITY_SERVICE) as ConnectivityManager
    private var lastAddress: String? = null
    private var lastIsVpn: Boolean? = null

    private val callback = object : ConnectivityManager.NetworkCallback() {
        override fun onAvailable(network: Network) = handleNetwork(network)

        override fun onCapabilitiesChanged(network: Network, caps: NetworkCapabilities) =
            handleNetwork(network, caps)

        override fun onLinkPropertiesChanged(network: Network, linkProperties: android.net.LinkProperties) =
            handleNetwork(network)
    }

    fun start() {
        val request = NetworkRequest.Builder().build()
        connectivityManager.registerNetworkCallback(request, callback)
        connectivityManager.activeNetwork?.let { handleNetwork(it) }
    }

    fun stop() {
        try {
            connectivityManager.unregisterNetworkCallback(callback)
        } catch (error: Exception) {
            Log.w(TAG, "Network callback already unregistered", error)
        }
    }

    private fun handleNetwork(network: Network, caps: NetworkCapabilities? = null) {
        val capabilities = caps ?: connectivityManager.getNetworkCapabilities(network)
        val isVpn = capabilities?.hasTransport(NetworkCapabilities.TRANSPORT_VPN) == true
        val linkProperties = connectivityManager.getLinkProperties(network)
        val address = linkProperties?.linkAddresses
            ?.firstOrNull { it is LinkAddress }
            ?.address
            ?.hostAddress

        if (address != lastAddress || isVpn != lastIsVpn) {
            lastAddress = address
            lastIsVpn = isVpn
            onNetworkChanged(isVpn, address)
        }
    }

    companion object {
        private const val TAG = "VpnNetworkMonitor"
    }
}
