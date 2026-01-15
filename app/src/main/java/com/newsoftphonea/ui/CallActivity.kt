package com.newsoftphonea.ui

import android.os.Bundle
import android.widget.CompoundButton
import androidx.activity.viewModels
import androidx.appcompat.app.AppCompatActivity
import androidx.recyclerview.widget.LinearLayoutManager
import com.newsoftphonea.databinding.ActivityCallBinding
import com.newsoftphonea.network.VpnNetworkMonitor
import com.newsoftphonea.util.PermissionHelper

class CallActivity : AppCompatActivity() {
    private lateinit var binding: ActivityCallBinding
    private val viewModel: CallViewModel by viewModels()
    private lateinit var transcriptionAdapter: TranscriptionAdapter
    private var transcriptionListener: CompoundButton.OnCheckedChangeListener? = null
    private var networkMonitor: VpnNetworkMonitor? = null

    override fun onCreate(savedInstanceState: Bundle?) {
        super.onCreate(savedInstanceState)
        binding = ActivityCallBinding.inflate(layoutInflater)
        setContentView(binding.root)

        PermissionHelper.requestMissingPermissions(this)
        viewModel.initialize()

        setupTranscriptionList()
        setupControls()
        observeViewModel()

        networkMonitor = VpnNetworkMonitor(this) { _, _ ->
            viewModel.handleNetworkChange()
        }
    }

    override fun onStart() {
        super.onStart()
        networkMonitor?.start()
    }

    override fun onRequestPermissionsResult(
        requestCode: Int,
        permissions: Array<out String>,
        grantResults: IntArray
    ) {
        super.onRequestPermissionsResult(requestCode, permissions, grantResults)
        if (requestCode == PermissionHelper.REQUEST_CODE) {
            val missing = PermissionHelper.missingPermissions(this)
            if (missing.isNotEmpty()) {
                binding.statusText.text = "Missing permissions: ${missing.joinToString()}"
            }
        }
    }

    override fun onStop() {
        super.onStop()
        networkMonitor?.stop()
    }

    private fun setupTranscriptionList() {
        transcriptionAdapter = TranscriptionAdapter()
        binding.transcriptionList.apply {
            adapter = transcriptionAdapter
            layoutManager = LinearLayoutManager(this@CallActivity)
        }
    }

    private fun setupControls() = with(binding) {
        registerButton.setOnClickListener {
            viewModel.createAccount(
                username = usernameInput.text?.toString().orEmpty(),
                password = passwordInput.text?.toString().orEmpty(),
                domain = domainInput.text?.toString().orEmpty(),
                proxy = proxyInput.text?.toString()?.ifBlank { null }
            )
        }

        callButton.setOnClickListener {
            viewModel.makeCall(targetUriInput.text?.toString().orEmpty())
        }

        answerButton.setOnClickListener { viewModel.answerCall() }
        hangupButton.setOnClickListener { viewModel.hangupCall() }

        holdSwitch.setOnCheckedChangeListener { _, isChecked ->
            viewModel.toggleHold(isChecked)
        }
        muteSwitch.setOnCheckedChangeListener { _, isChecked ->
            viewModel.toggleMute(isChecked)
        }

        recordButton.setOnClickListener { viewModel.toggleRecording() }

        blindTransferButton.setOnClickListener {
            viewModel.blindTransfer(transferTargetInput.text?.toString().orEmpty())
        }

        warmTransferButton.setOnClickListener {
            val attendedId = attendedCallIdInput.text?.toString()?.toIntOrNull() ?: return@setOnClickListener
            viewModel.attendedTransfer(attendedId)
        }

        transcriptionListener = CompoundButton.OnCheckedChangeListener { _, isChecked ->
            viewModel.toggleTranscription(isChecked)
        }
        transcriptionToggle.setOnCheckedChangeListener(transcriptionListener)
    }

    private fun observeViewModel() {
        viewModel.statusText.observe(this) { status ->
            binding.statusText.text = status
        }
        viewModel.callId.observe(this) { callId ->
            binding.callIdText.text = "Call ID: ${callId ?: "--"}"
        }
        viewModel.transcription.observe(this) { segments ->
            transcriptionAdapter.submitList(segments)
            if (segments.isNotEmpty()) {
                binding.transcriptionList.scrollToPosition(segments.lastIndex)
            }
        }
        viewModel.recordingEnabled.observe(this) { isRecording ->
            binding.recordButton.text = if (isRecording) "Stop Recording" else "Record"
        }
        viewModel.transcriptionEnabled.observe(this) { enabled ->
            binding.transcriptionToggle.setOnCheckedChangeListener(null)
            binding.transcriptionToggle.isChecked = enabled
            binding.transcriptionToggle.setOnCheckedChangeListener(transcriptionListener)
        }
    }
}
