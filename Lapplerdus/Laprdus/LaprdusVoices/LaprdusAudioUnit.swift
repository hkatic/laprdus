// LaprdusAudioUnit.swift - AVSpeechSynthesisProvider audio unit.
// Apple counterpart of the Android LaprdusTTSService: exposes the five
// Laprdus voices to the system (VoiceOver, Spoken Content, AVSpeechSynthesizer).

import AVFAudio
import AudioToolbox
import Foundation

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

    // Rendering state, guarded by `renderLock` (written on the synthesis
    // thread, read on the render thread).
    private let renderLock = NSLock()
    private var pendingSamples: [Float] = []
    private var readIndex = 0
    private var hasPendingRequest = false

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

            // Voice resolution: the host's requested voice, unless
            // "Force language" pins the user's default voice (Android parity).
            var voiceID = voiceID(fromIdentifier: speechRequest.voice.identifier)
            if settings.forceLanguage {
                voiceID = settings.defaultVoice
            }
            if engine.currentVoice != voiceID || !engine.isInitialized {
                try engine.loadVoice(voiceID)
                loadUserDictionaries(into: engine, settings: settings)
            }

            engine.apply(settings)
            // Rate/pitch/volume: honor the host request unless forced.
            // Without force, volume is pinned to 1.0 and the system output
            // volume governs — same as the Android service.
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
        renderLock.lock()
        pendingSamples = []
        readIndex = 0
        hasPendingRequest = false
        renderLock.unlock()
    }

    private func finishWith(samples: [Float]) {
        renderLock.lock()
        pendingSamples = samples
        readIndex = 0
        hasPendingRequest = true
        renderLock.unlock()
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
        // Never refuse an unknown voice: fall back to the persisted default,
        // matching the Android service's permissive language policy.
        return SettingsSnapshot.load().defaultVoice
    }

    private func loadUserDictionaries(into engine: LaprdusEngine, settings: SettingsSnapshot) {
        guard settings.userDictionariesEnabled else { return }
        let store = DictionaryStore()
        if let url = store.userDictionaryURLIfPresent {
            engine.appendUserDictionary(at: url)
        }
    }

    // MARK: - Rendering

    public override var internalRenderBlock: AUInternalRenderBlock {
        { [weak self] actionFlags, _, frameCount, _, outputAudioBufferList, _, _ in
            let buffers = UnsafeMutableAudioBufferListPointer(outputAudioBufferList)
            guard let self,
                  let rawData = buffers.first?.mData else {
                return kAudioUnitErr_NoConnection
            }
            let output = rawData.assumingMemoryBound(to: Float.self)
            let requested = Int(frameCount)

            self.renderLock.lock()
            let available = self.pendingSamples.count - self.readIndex
            let toCopy = max(0, min(requested, available))
            if toCopy > 0 {
                self.pendingSamples.withUnsafeBufferPointer { source in
                    output.update(from: source.baseAddress! + self.readIndex, count: toCopy)
                }
                self.readIndex += toCopy
            }
            for index in toCopy..<requested {
                output[index] = 0
            }
            let finished = self.hasPendingRequest && self.readIndex >= self.pendingSamples.count
            if finished {
                self.hasPendingRequest = false
            }
            self.renderLock.unlock()

            buffers[0].mDataByteSize = UInt32(requested * MemoryLayout<Float>.size)
            if finished {
                actionFlags.pointee = .offlineUnitRenderAction_Complete
            }
            return noErr
        }
    }
}
