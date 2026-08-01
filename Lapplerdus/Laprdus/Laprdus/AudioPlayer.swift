// AudioPlayer.swift - Plays 16-bit mono PCM produced by the engine.

import AVFAudio
import Foundation

@MainActor
final class AudioPlayer {
    private let audioEngine = AVAudioEngine()
    private let playerNode = AVAudioPlayerNode()
    private var connectedSampleRate: Double = 0

    /// Plays a synthesized chunk and returns when playback finishes
    /// (or when `stop()` is called).
    func play(_ chunk: LaprdusEngine.AudioChunk) async throws {
        guard !chunk.samples.isEmpty else { return }
        guard let format = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: chunk.sampleRate,
            channels: 1,
            interleaved: false
        ), let buffer = AVAudioPCMBuffer(
            pcmFormat: format,
            frameCapacity: AVAudioFrameCount(chunk.samples.count)
        ), let channel = buffer.floatChannelData?[0] else {
            return
        }

        buffer.frameLength = AVAudioFrameCount(chunk.samples.count)
        for index in 0..<chunk.samples.count {
            channel[index] = Float(chunk.samples[index]) / Float(Int16.max)
        }

        #if !os(macOS)
        let session = AVAudioSession.sharedInstance()
        try? session.setCategory(.playback, mode: .spokenAudio)
        try? session.setActive(true)
        #endif

        if audioEngine.attachedNodes.contains(playerNode) == false {
            audioEngine.attach(playerNode)
        }
        if connectedSampleRate != chunk.sampleRate {
            audioEngine.connect(playerNode, to: audioEngine.mainMixerNode, format: format)
            connectedSampleRate = chunk.sampleRate
        }
        if !audioEngine.isRunning {
            try audioEngine.start()
        }

        playerNode.play()
        await withCheckedContinuation { continuation in
            playerNode.scheduleBuffer(buffer, completionCallbackType: .dataPlayedBack) { _ in
                continuation.resume()
            }
        }
    }

    /// Stops playback; pending buffer completion handlers fire, which resumes
    /// any awaiting `play(_:)` call.
    func stop() {
        playerNode.stop()
    }
}
