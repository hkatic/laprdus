// LaprdusAudioUnit.swift - AVSpeechSynthesisProvider audio unit.
// Exposes the Laprdus voices to the system (VoiceOver, Spoken Content,
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

    private static let log = Logger(subsystem: "com.hrvojekatic.laprdus", category: "speech")

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

    /// One system voice per voice and language (18 x 4 = 72): every voice
    /// is listed under Croatian, Serbian, Bosnian and Slovenian, and the entry the host
    /// picks names both. The system shows the language itself, so the name
    /// is the bare display name from the registry.
    public override var speechVoices: [AVSpeechSynthesisProviderVoice] {
        get {
            VoiceCatalog.providerVoices.map { entry in
                let voice = AVSpeechSynthesisProviderVoice(
                    name: entry.voice.displayName,
                    identifier: entry.identifier,
                    primaryLanguages: [entry.language.code],
                    supportedLanguages: [entry.language.code]
                )
                voice.gender = entry.voice.gender == "Female" ? .female : .male
                switch entry.voice.age {
                case "Child": voice.age = 8
                case "Senior": voice.age = 70
                default: voice.age = 35
                }
                return voice
            }
        }
        set {}
    }

    // MARK: - Synthesis

    public override func synthesizeSpeechRequest(_ speechRequest: AVSpeechSynthesisProviderRequest) {
        // A request that is cancelled while it is still being synthesized
        // must not be heard afterwards.
        let request = renderState.beginRequest()
        let settings = SettingsSnapshot.load()
        let utterance = SSMLParser.parse(speechRequest.ssmlRepresentation)
        // What the host asked for, without the text itself. Read it with
        // `log show --info --predicate 'subsystem == "com.hrvojekatic.laprdus"'`.
        Self.log.info("""
            request: \(SSMLParser.markup(in: speechRequest.ssmlRepresentation), privacy: .public) \
            parts: \(utterance.summary, privacy: .public)
            """)

        let speech = utterance.speech
        guard !speech.isEmpty else {
            finishWith(samples: [], request: request)
            return
        }

        do {
            let engine = try ensureEngine()

            // The host always picks a concrete voice on Apple platforms
            // (Spoken Content / VoiceOver), and its identifier names the
            // voice and the language, so honor both.
            let (voiceID, languageCode) = resolve(identifier: speechRequest.voice.identifier, settings: settings)
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
            // The chosen language survives voice changes inside the engine,
            // and the wrapper asks for it again only when it differs.
            try engine.setLanguage(languageCode)

            engine.apply(settings)

            // Spelling mode where the host asks for it, and for a request
            // that is a single grapheme cluster, so character-by-character
            // screen reader navigation names characters ("Č" → "Če").
            let singleCharacter = speech.count == 1 && speech[0].text.count == 1
            var samples: [Float] = []
            // The silence a spelled part owes to what follows it (seconds).
            // The engine leaves the pause after the last spelled character
            // out; text that follows with nothing between gets it, a break
            // or a phrase boundary replaces it. VoiceOver reads a button with
            // a badge as three phrases, "Notifications 1 Tab", with the count
            // spelled: the spelling pause on top of the comma pause held the
            // type of the control back.
            var owedPause = 0.0
            for part in utterance.parts {
                guard renderState.isCurrent(request) else { return }
                switch part {
                case .speech(let speech):
                    // Rate/pitch: the host's value scales the Laprdus setting
                    // (its normal) unless the setting is forced; the
                    // acceleration multiplies the rate on top of that.
                    // Without force, the volume is the host's: VoiceOver
                    // puts its own volume setting in the request's
                    // <prosody volume>, and the system output volume
                    // governs on top of that. It used to be pinned to 1.0,
                    // so VoiceOver's volume did nothing.
                    engine.setTransientParameters(
                        speed: VoiceCatalog.requestValue(
                            setting: settings.speed, requested: speech.rate,
                            forced: settings.forceSpeed, voice: voiceID),
                        userPitch: VoiceCatalog.requestValue(
                            setting: settings.pitch, requested: speech.pitch,
                            forced: settings.forcePitch, voice: voiceID),
                        volume: settings.forceVolume ? settings.volume : speech.volume
                    )
                    // A part the engine has nothing to say for (a lone
                    // symbol) must not silence the parts around it.
                    guard let chunk = try? engine.synthesize(
                        speech.text,
                        spelled: speech.spellOut || singleCharacter,
                        finalPause: false
                    ) else { continue }
                    samples.append(contentsOf: silence(seconds: owedPause))
                    samples.append(contentsOf: chunk.samples.lazy.map { Float($0) / Float(Int16.max) })
                    owedPause = chunk.finalPause
                case .pause(let seconds):
                    // The break alone sets the gap after a spelled character.
                    owedPause = 0
                    samples.append(contentsOf: silence(seconds: seconds))
                case .phraseBoundary:
                    // The engine ends an unpunctuated phrase without a pause,
                    // so phrases the host keeps apart get the one the user
                    // chose for commas; after a spelled part it is the whole
                    // gap.
                    owedPause = 0
                    samples.append(contentsOf: silence(seconds: Double(max(0, min(settings.commaPause, 2000))) / 1000))
                }
            }
            // A request that ends with a spelled character keeps that
            // character's pause, so the characters VoiceOver sends one by one
            // (typing, moving by character) keep the spelling speed's cadence.
            samples.append(contentsOf: silence(seconds: owedPause))
            finishWith(samples: samples, request: request)
        } catch {
            finishWith(samples: [], request: request)
        }
    }

    public override func cancelSpeechRequest() {
        engine?.cancel()
        renderState.clear()
    }

    private func finishWith(samples: [Float], request: Int) {
        renderState.publish(samples, request: request)
    }

    private func silence(seconds: Double) -> Repeated<Float> {
        repeatElement(0, count: Int(seconds * audioFormat.sampleRate))
    }

    private func ensureEngine() throws -> LaprdusEngine {
        if let engine {
            return engine
        }
        let created = try LaprdusEngine()
        engine = created
        return created
    }

    /// The voice and language a request's voice identifier names:
    /// "com.hrvojekatic.laprdus.zvonko.sr" is Zvonko speaking Serbian, a
    /// pre-2.0 identifier without the language part is the voice in its
    /// native language, and an old voice id is the renamed voice. An
    /// identifier that is none of ours is never refused: the voice and the
    /// language from the settings speak instead.
    private func resolve(identifier: String, settings: SettingsSnapshot) -> (voiceID: String, languageCode: String) {
        if let entry = VoiceCatalog.providerVoice(forIdentifier: identifier) {
            return (entry.voice.id, entry.language.code)
        }
        return (settings.defaultVoice, settings.language)
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
    private var request = 0

    init() {
        lock = UnsafeMutablePointer<os_unfair_lock>.allocate(capacity: 1)
        lock.initialize(to: os_unfair_lock())
    }

    deinit {
        samples?.deallocate()
        lock.deinitialize(count: 1)
        lock.deallocate()
    }

    /// Synthesis thread. Starts a request and returns the number that its
    /// samples have to be published with.
    func beginRequest() -> Int {
        os_unfair_lock_lock(lock)
        request += 1
        let current = request
        os_unfair_lock_unlock(lock)
        return current
    }

    /// Synthesis thread. False once the request was cancelled or followed by
    /// another one, so the rest of it need not be synthesized.
    func isCurrent(_ asking: Int) -> Bool {
        os_unfair_lock_lock(lock)
        defer { os_unfair_lock_unlock(lock) }
        return asking == request
    }

    /// Synthesis thread. Publishing an empty buffer completes the request
    /// immediately, which is how a failed or empty utterance is reported.
    /// Samples of a request that was cancelled in the meantime are dropped.
    func publish(_ newSamples: [Float], request publishing: Int) {
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
        guard publishing == request else {
            os_unfair_lock_unlock(lock)
            buffer?.deallocate()
            return
        }
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
        request += 1
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
