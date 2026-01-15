package com.newsoftphonea.ui

import android.view.LayoutInflater
import android.view.ViewGroup
import androidx.recyclerview.widget.DiffUtil
import androidx.recyclerview.widget.ListAdapter
import androidx.recyclerview.widget.RecyclerView
import com.newsoftphonea.databinding.ItemTranscriptionBinding
import com.newsoftphonea.transcription.TranscriptionSegment
import java.text.SimpleDateFormat
import java.util.Date
import java.util.Locale

class TranscriptionAdapter :
    ListAdapter<TranscriptionSegment, TranscriptionAdapter.TranscriptionViewHolder>(DiffCallback) {

    override fun onCreateViewHolder(parent: ViewGroup, viewType: Int): TranscriptionViewHolder {
        val binding = ItemTranscriptionBinding.inflate(
            LayoutInflater.from(parent.context),
            parent,
            false
        )
        return TranscriptionViewHolder(binding)
    }

    override fun onBindViewHolder(holder: TranscriptionViewHolder, position: Int) {
        holder.bind(getItem(position))
    }

    class TranscriptionViewHolder(
        private val binding: ItemTranscriptionBinding
    ) : RecyclerView.ViewHolder(binding.root) {

        fun bind(segment: TranscriptionSegment) {
            binding.transcriptionText.text = segment.text
            binding.transcriptionTime.text = timeFormatter.format(Date(segment.timestampMs))
            binding.transcriptionStatus.text = if (segment.isFinal) "Final" else "Partial"
        }
    }

    companion object {
        private val DiffCallback = object : DiffUtil.ItemCallback<TranscriptionSegment>() {
            override fun areItemsTheSame(
                oldItem: TranscriptionSegment,
                newItem: TranscriptionSegment
            ): Boolean = oldItem.timestampMs == newItem.timestampMs

            override fun areContentsTheSame(
                oldItem: TranscriptionSegment,
                newItem: TranscriptionSegment
            ): Boolean = oldItem == newItem
        }

        private val timeFormatter = SimpleDateFormat("HH:mm:ss", Locale.US)
    }
}
