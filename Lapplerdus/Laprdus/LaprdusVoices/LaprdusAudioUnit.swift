// LaprdusAudioUnit.swift - AVSpeechSynthesisProvider audio unit.
// Exposes the five Laprdus voices to the system (VoiceOver, Spoken Content,
// AVSpeechSynthesizer).

import AVFAudio
import AudioToolbox
import Foundation
import os

public class LaprdusAudioUnitFactory: NSObject, AUAudioUnitFactory {
    public func beginRequest(with context: NSExtensionContext) {}

    public func createAudioUnit(with componentDescription: AudioComponentDescription) throws -> AUAudioUnit {
        try LaprdusAudioUnit(componentDescription: componentDescription, options: [])
    }
}

public class LaprdusAudioUnit: AVSpeechSynthesisProviderAudioUnit {
    private let outputBus: AUAudioUnitBus
    private let audioFormat: AVAudioFormat
    private var _outputBusses: AUAudioUnitBusArray!

    private var engine: LaprdusEngine?

    private let renderState = RenderState()

    public override init(
        componentDescription: AudioComponentDescription,
        options: AudioComponentInstantiationOptions = []
    ) throws {
        guard let format = AVAudioFormat(
            commonFormat: .pcmFormatFloat32,
            sampleRate: 22050,
            channels: 1,
            interleaved: false
        ) else {
            throw NSError(domain: NSOSStatusErrorDomain, code: Int(kAudioUnitErr_FormatNotSupported))
        }
        audioFormat = format
        outputBus = try AUAudioUnitBus(format: format)
        try super.init(componentDescription: componentDescription, options: options)
        _outputBusses = AUAudioUnitBusArray(audioUnit: self, busType: .output, busses: [outputBus])
    }

    public override var outputBusses: AUAudioUnitBusArray {
        _outputBusses
    }

    // MARK: - Voices

    public override var speechVoices: [AVSpeechSynthesisProviderVoice] {
        get {
            VoiceCatalog.all.map { voice in
                AVSpeechSynthesisProviderVoice(
                    name: voice.localizedName,
                    identifier: voice.providerIdentifier,
                    primaryLanguages: [voice.languageCode],
                    supportedLanguages: ["hr-HR", "sr-RS"]
                )
            }
        }
        set {}
    }

    // MARK: - Synthesis

    public override func synthesizeSpeechRequest(_ speechRequest: AVSpeechSynthesisProviderRequest) {
        let settings = SettingsSnapshot.load()
        let utterance = SSMLParser.parse(speechRequest.ssmlRepresentation)

        guard !utterance.text.isEmpty else {
            finishWith(samples: [])
            return
        }

        do {
            let engine = try ensureEngine()

            // The host always picks a concrete voice on Apple platforms
            // (Spoken Content / VoiceOver), so honor its request.
            let voiceID = voiceID(fromIdentifier: speechRequest.voice.identifier)
            let dictionaries = DictionaryStore()
                .dictionaryState(userDictionariesEnabled: settings.userDictionariesEnabled)
            if engine.currentVoice != voiceID || !engine.isInitialized {
                try engine.loadVoice(voiceID, dictionaries: dictionaries)
            } else {
                // This process outlives individual utterances, so entries the
                // user edits in the app have to be picked up here rather than
                // only when the voice changes.
                engine.syncDictionaries(dictionaries)
            }

            engine.apply(settings)
            // Rate/pitch/volume: honor the host request unless forced.
            // Without force, volume is pinned to 1.0 and the system output
            // volume governs.
            engine.setTransientParameters(
                speed: settings.forceSpeed ? settings.speed : utterance.rate,
                userPitch: settings.forcePitch ? settings.pitch : utterance.pitch,
                volume: settings.forceVolume ? settings.volume : 1.0
            )

            // Single grapheme cluster → spelling mode, so character-by-character
            // screen reader navigation names characters ("Č" → "Če").
            let spelled = utterance.text.count == 1
            let chunk = try engine.synthesize(utterance.text, spelled: spelled)
            finishWith(samples: chunk.samples.map { Float($0) / Float(Int16.max) })
        } catch {
            finishWith(samples: [])
        }
    }

    public override func cancelSpeechRequest() {
        engine?.cancel()
        renderState.clear()
    }

    private func finishWith(samples: [Float]) {
        renderState.publish(samples)
    }

    private func ensureEngine() throws -> LaprdusEngine {
        if let engine {
            return engine
        }
        let created = try LaprdusEngine()
        engine = created
        return created
    }

    private func voiceID(fromIdentifier identifier: String) -> String {
        if let voice = VoiceCatalog.all.first(where: { $0.providerIdentifier == identifier }) {
            return voice.id
        }
        // Never refuse an unknown voice: fall back to the persisted default.
        return SettingsSnapshot.load().defaultVoice
    }

    // MARK: - Rendering

    public override var internalRenderBlock: AUInternalRenderBlock {
        // The state object is captured strongly and retained once, here: the
        // render callback itself must stay free of ARC traffic, and a weak
        // capture of self would take a runtime lock on every cycle.
        let state = renderState
        return { actionFlags, _, frameCount, _, outputAudioBufferList, _, _ in
            let buffers = UnsafeMutableAudioBufferListPointer(outputAudioBufferList)
            guard let rawData = buffers.first?.mData else {
                return kAudioUnitErr_NoConnection
            }
            let frames = Int(frameCount)
            let finished = state.render(
                into: rawData.assumingMemoryBound(to: Float.self),
                frames: frames
            )
            buffers[0].mDataByteSize = UInt32(frames * MemoryLayout<Float>.size)
            if finished {
                actionFlags.pointee = .offlineUnitRenderAction_Complete
            }
            return noErr
        }
    }
}

/// Audio handed from the synthesis thread to the render thread.
///
/// The render callback runs under a real-time deadline, so it must not block,
/// allocate, or touch ARC. Samples therefore live in a raw buffer rather than a
/// Swift array, buffers are always freed on the synthesis thread, and the
/// render thread only *tries* the lock: if the synthesis thread happens to be
/// mid-handoff it emits one buffer of silence and picks the samples up on the
/// next cycle instead of waiting on a lower-priority thread.
private final class RenderState {
    private let lock: UnsafeMutablePointer<os_unfair_lock>
    private var samples: UnsafeMutablePointer<Float>?
    private var count = 0
    private var readIndex = 0
    private var pending = false

    init() {
        lock = UnsafeMutablePointer<os_unfair_lock>.allocate(capacity: 1)
        lock.initialize(to: os_unfair_lock())
    }

    deinit {
        samples?.deallocate()
        lock.deinitialize(count: 1)
        lock.deallocate()
    }

    /// Synthesis thread. Publishing an empty buffer completes the request
    /// immediately, which is how a failed or empty utterance is reported.
    func publish(_ newSamples: [Float]) {
        var buffer: UnsafeMutablePointer<Float>?
        if !newSamples.isEmpty {
            let allocated = UnsafeMutablePointer<Float>.allocate(capacity: newSamples.count)
            newSamples.withUnsafeBufferPointer { source in
                if let base = source.baseAddress {
                    allocated.update(from: base, count: newSamples.count)
                }
            }
            buffer = allocated
        }
        os_unfair_lock_lock(lock)
        let previous = samples
        samples = buffer
        count = newSamples.count
        readIndex = 0
        pending = true
        os_unfair_lock_unlock(lock)
        // Safe to free here: any render already copying held the lock we just
        // took, and every later render sees the new buffer.
        previous?.deallocate()
    }

    /// Synthesis thread.
    func clear() {
        os_unfair_lock_lock(lock)
        let previous = samples
        samples = nil
        count = 0
        readIndex = 0
        pending = false
        os_unfair_lock_unlock(lock)
        previous?.deallocate()
    }

    /// Render thread. Returns true when the current request has been fully
    /// delivered. Never blocks and never allocates.
    func render(into output: UnsafeMutablePointer<Float>, frames: Int) -> Bool {
        guard frames > 0 else { return false }
        guard os_unfair_lock_trylock(lock) else {
            output.update(repeating: 0, count: frames)
            return false
        }
        var copied = 0
        if pending, let samples {
            copied = max(0, min(frames, count - readIndex))
            if copied > 0 {
                output.update(from: samples + readIndex, count: copied)
                readIndex += copied
            }
        }
        if copied < frames {
            (output + copied).update(repeating: 0, count: frames - copied)
        }
        let finished = pending && readIndex >= count
        if finished {
            pending = false
        }
        os_unfair_lock_unlock(lock)
        return finished
    }
}
