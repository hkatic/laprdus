//
//  LaprdusTests.swift
//  LaprdusTests
//

import Foundation
import Testing
@testable import Laprdus

// MARK: - Engine

@Suite(.serialized)
struct EngineTests {

    @Test func voiceRegistryExposesAllVoices() throws {
        let ids = VoiceCatalog.all.map(\.id)
        #expect(ids == ["josip", "vlado", "detence", "baba", "djed",
                        "zvonko", "stojan", "mirsad",
                        "orguljas", "klapa", "trubac", "harmonikas",
                        "sevdalija", "sazlija", "pjevac", "pevac", "solist",
                        "becarac"])
    }

    @Test func formantVoicesHaveTheirOwnLanguages() throws {
        #expect(VoiceCatalog.voice(withID: "zvonko")?.languageCode == "hr-HR")
        #expect(VoiceCatalog.voice(withID: "stojan")?.languageCode == "sr-RS")
        #expect(VoiceCatalog.voice(withID: "mirsad")?.languageCode == "bs-BA")
        #expect(VoiceCatalog.defaultVoiceID(forLanguage: "bs-BA") == "mirsad")
        // Each language defaults to its formant voice.
        #expect(VoiceCatalog.defaultVoiceID(forLanguage: "hr-HR") == "zvonko")
        #expect(VoiceCatalog.defaultVoiceID(forLanguage: "sr-RS") == "stojan")
    }

    @Test func formantVoicesSynthesizeAndSwitchBack() throws {
        let engine = try LaprdusEngine()
        for id in ["zvonko", "stojan", "mirsad"] {
            try engine.loadVoice(id, dictionaries: .bundledOnly)
            #expect(engine.currentVoice == id)
            let chunk = try engine.synthesize("Dobar dan, kako ste?")
            #expect(chunk.samples.count > 22050 / 2)
            #expect(chunk.sampleRate == 22050)
            let spelled = try engine.synthesize("Č", spelled: true)
            #expect(spelled.samples.count > 0)
        }
        // Back to a recorded voice: its data has to be loaded again.
        try engine.loadVoice("josip", dictionaries: .bundledOnly)
        let chunk = try engine.synthesize("Dobar dan!")
        #expect(chunk.samples.count > 0)
    }

    @Test func derivedVoicesReferenceBasePitch() throws {
        let detence = try #require(VoiceCatalog.voice(withID: "detence"))
        #expect(detence.basePitch == 1.5)
        #expect(!detence.isPhysical)
        let josip = try #require(VoiceCatalog.voice(withID: "josip"))
        #expect(josip.isPhysical)
    }

    @Test func synthesizesAudioForCroatianText() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("josip", dictionaries: .bundledOnly)
        #expect(engine.isInitialized)
        let chunk = try engine.synthesize("Dobar dan!")
        #expect(chunk.samples.count > 0)
        #expect(chunk.sampleRate == 22050)
        #expect(chunk.channels == 1)
    }

    @Test func synthesizesSpelledCharacter() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("josip", dictionaries: .bundledOnly)
        let chunk = try engine.synthesize("Č", spelled: true)
        #expect(chunk.samples.count > 0)
    }

    @Test func switchingToDerivedVoiceWorks() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("baba", dictionaries: .bundledOnly)
        #expect(engine.currentVoice == "baba")
        let chunk = try engine.synthesize("Dobar dan")
        #expect(chunk.samples.count > 0)
    }

    @Test func appliesSettingsWithoutError() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("josip", dictionaries: .bundledOnly)
        var snapshot = SettingsSnapshot()
        snapshot.speed = 1.5
        snapshot.pitch = 1.2
        snapshot.volume = 0.8
        snapshot.numberMode = 1
        snapshot.sentencePause = 250
        engine.apply(snapshot)
        let chunk = try engine.synthesize("Broj 123.")
        #expect(chunk.samples.count > 0)
    }

    @Test func formantSettingsChangeTheFormantVoices() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("zvonko", dictionaries: .bundledOnly)
        var snapshot = SettingsSnapshot()
        engine.apply(snapshot)
        let normal = try engine.synthesize("Dobar dan, kako ste?")

        // Acceleration 2.0 at speed 1.0 is speed 2.0: about half as long.
        snapshot.acceleration = 2.0
        engine.apply(snapshot)
        let fast = try engine.synthesize("Dobar dan, kako ste?")
        #expect(fast.samples.count < normal.samples.count * 7 / 10)

        // A monotone has the same length and different samples.
        snapshot.acceleration = 1.0
        snapshot.inflectionLevel = 0.0
        engine.apply(snapshot)
        let flat = try engine.synthesize("Dobar dan, kako ste?")
        #expect(flat.samples.count == normal.samples.count)
        #expect(flat.samples != normal.samples)

        // The formant voices take the wider rate range.
        snapshot.inflectionLevel = 0.5
        snapshot.speed = 0.25
        engine.apply(snapshot)
        let slowest = try engine.synthesize("Dobar dan, kako ste?")
        #expect(slowest.samples.count > normal.samples.count * 3)

        #expect(VoiceCatalog.voice(withID: "zvonko")?.isFormant == true)
        #expect(VoiceCatalog.voice(withID: "josip")?.isFormant == false)
        #expect(VoiceCatalog.voice(withID: "klapa")?.isFormant == true)
        #expect(VoiceCatalog.voice(withID: "klapa")?.isSinging == true)
        #expect(VoiceCatalog.voice(withID: "zvonko")?.isSinging == false)
        #expect((VoiceCatalog.voice(withID: "zvonko")?.nominalWordsPerMinute ?? 0) > 100)
    }
}

// MARK: - Dictionary store

struct DictionaryStoreTests {

    private func makeStore() throws -> (DictionaryStore, URL) {
        let dir = FileManager.default.temporaryDirectory
            .appendingPathComponent("LaprdusTests-\(UUID().uuidString)", isDirectory: true)
        try FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return (DictionaryStore(directory: dir), dir)
    }

    @Test func missingFileYieldsEmptyList() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }
        #expect(try store.load(.main).isEmpty)
    }

    @Test func saveAndLoadRoundTrip() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        let entry = DictionaryEntry(
            grapheme: "ZG",
            phoneme: "Ze Ge",
            caseSensitive: true,
            wholeWord: false,
            comment: "Zagreb plates"
        )
        try store.save([entry], type: .main)
        let loaded = try store.load(.main)
        #expect(loaded.count == 1)
        #expect(loaded[0].grapheme == "ZG")
        #expect(loaded[0].phoneme == "Ze Ge")
        #expect(loaded[0].caseSensitive == true)
        #expect(loaded[0].wholeWord == false)
        #expect(loaded[0].comment == "Zagreb plates")
    }

    @Test func writtenFormatMatchesSharedDictionaryFormat() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        try store.save([DictionaryEntry(grapheme: "Facebook", phoneme: "Fejzbuk")], type: .main)
        let data = try Data(contentsOf: store.fileURL(for: .main))
        let root = try #require(try JSONSerialization.jsonObject(with: data) as? [String: Any])
        #expect(root["version"] as? String == "1.0")
        let entries = try #require(root["entries"] as? [[String: Any]])
        #expect(entries.count == 1)
        #expect(entries[0]["grapheme"] as? String == "Facebook")
        // An empty comment is omitted from the written file.
        #expect(entries[0]["comment"] == nil)
    }

    @Test func dictionaryTypesUseSharedFileNames() {
        #expect(DictionaryType.main.fileName == "user.json")
        #expect(DictionaryType.spelling.fileName == "spelling.json")
        #expect(DictionaryType.emoji.fileName == "emoji.json")
        #expect(DictionaryStore.accentLexiconFileName == "accents.json")
    }

    @Test func stateIsEmptyWhenUserDictionariesAreDisabled() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        try store.save([DictionaryEntry(grapheme: "ZG", phoneme: "Ze Ge")], type: .main)
        let state = store.dictionaryState(userDictionariesEnabled: false)
        #expect(state.userDictionaryURL == nil)
        #expect(state.stamp == DictionaryState.bundledOnly.stamp)
    }

    @Test func stateIncludesTheAccentLexicon() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        let before = store.dictionaryState(userDictionariesEnabled: true)
        #expect(before.accentLexiconURL == nil)
        #expect(before.stamp == "absent")

        try #"{ "entries": [ { "word": "balk'o:n*" } ] }"#
            .write(to: store.accentLexiconURL, atomically: true, encoding: .utf8)
        let after = store.dictionaryState(userDictionariesEnabled: true)
        #expect(after.accentLexiconURL == store.accentLexiconURL)
        #expect(after.urls.isEmpty)
        #expect(after.stamp != before.stamp)
        #expect(store.dictionaryState(userDictionariesEnabled: false).accentLexiconURL == nil)
    }

    @Test func engineAcceptsAnAccentLexicon() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }
        try #"{ "entries": [ { "word": "balk'o:n*" }, { "word": "balkon" } ] }"#
            .write(to: store.accentLexiconURL, atomically: true, encoding: .utf8)

        let engine = try LaprdusEngine()
        try engine.loadVoice("zvonko", dictionaries: store.dictionaryState(userDictionariesEnabled: true))
        let withLexicon = try engine.synthesize("balkon").samples
        try engine.loadVoice("zvonko", dictionaries: .bundledOnly)
        let without = try engine.synthesize("balkon").samples
        #expect(!withLexicon.isEmpty)
        #expect(withLexicon != without)
    }

    @Test func stateReportsMissingFile() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        let state = store.dictionaryState(userDictionariesEnabled: true)
        #expect(state.userDictionaryURL == nil)
        #expect(state.stamp == "absent")
    }

    /// The stamp is what tells the running speech extension that entries were
    /// edited in the app, so editing must change it.
    @Test func stampChangesWhenEntriesAreEdited() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        try store.save([DictionaryEntry(grapheme: "ZG", phoneme: "Ze Ge")], type: .main)
        let first = store.dictionaryState(userDictionariesEnabled: true)
        #expect(first.userDictionaryURL != nil)
        #expect(first.stamp != "absent")

        #expect(store.dictionaryState(userDictionariesEnabled: true).stamp == first.stamp)

        try store.save(
            [
                DictionaryEntry(grapheme: "ZG", phoneme: "Ze Ge"),
                DictionaryEntry(grapheme: "Facebook", phoneme: "Fejzbuk"),
            ],
            type: .main
        )
        #expect(store.dictionaryState(userDictionariesEnabled: true).stamp != first.stamp)
    }

    @Test func engineReloadsChangedUserDictionary() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        let engine = try LaprdusEngine()
        try store.save([DictionaryEntry(grapheme: "ZG", phoneme: "Ze Ge")], type: .main)
        try engine.loadVoice("josip", dictionaries: store.dictionaryState(userDictionariesEnabled: true))
        #expect(engine.currentVoice == "josip")

        // A changed dictionary is reloaded without losing the current voice,
        // and an unchanged one is a no-op.
        try store.save([DictionaryEntry(grapheme: "ZG", phoneme: "Zagreb")], type: .main)
        engine.syncDictionaries(store.dictionaryState(userDictionariesEnabled: true))
        #expect(engine.currentVoice == "josip")
        #expect(engine.isInitialized)
        #expect(try engine.synthesize("ZG").samples.count > 0)
    }

    /// Entries added in the spelling dictionary editor have to reach the
    /// engine; they are saved in the same format as pronunciation entries.
    @Test func userSpellingDictionaryIsApplied() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        let engine = try LaprdusEngine()
        try engine.loadVoice("zvonko", dictionaries: store.dictionaryState(userDictionariesEnabled: true))
        let bundled = try engine.synthesize("Q", spelled: true).samples.count

        try store.save(
            [DictionaryEntry(grapheme: "Q", phoneme: "ovo je vrlo dugacak naziv jednog slova")],
            type: .spelling
        )
        engine.syncDictionaries(store.dictionaryState(userDictionariesEnabled: true))
        let custom = try engine.synthesize("Q", spelled: true).samples.count
        #expect(custom > bundled * 2)
    }
}

// MARK: - SSML

/// The system hands the speech extension SSML, so these cover what Spoken
/// Content and VoiceOver actually send, plus the prosody-scoping rules.
struct SSMLParserTests {

    private func text(_ text: String, rate: Float = 1.0, pitch: Float = 1.0, spellOut: Bool = false) -> SSMLPart {
        .speech(SSMLSpeech(text: text, rate: rate, pitch: pitch, spellOut: spellOut))
    }

    @Test func readsProsodyRateAndPitch() {
        let utterance = SSMLParser.parse(
            "<speak><prosody rate=\"1.5\" pitch=\"0.75\">Dobar dan</prosody></speak>"
        )
        #expect(utterance.parts == [text("Dobar dan", rate: 1.5, pitch: 0.75)])
    }

    /// Reading markup or source code aloud must not let a literal rate="..."
    /// in the spoken text change how fast the text is read.
    @Test func ignoresAttributesInSpokenText() {
        let escaped = SSMLParser.parse(
            "<speak><prosody rate=\"1.5\">Atribut rate=&quot;2.0&quot; u tekstu</prosody></speak>"
        )
        #expect(escaped.parts == [text("Atribut rate=\"2.0\" u tekstu", rate: 1.5)])

        let unescaped = SSMLParser.parse("<speak>citam rate=\"2.0\" i pitch=\"2.0\" naglas</speak>")
        #expect(unescaped.parts == [text("citam rate=\"2.0\" i pitch=\"2.0\" naglas")])
    }

    @Test func acceptsSingleQuotedAttributes() {
        let utterance = SSMLParser.parse("<speak><prosody rate='fast' pitch='low'>Test</prosody></speak>")
        #expect(utterance.parts == [text("Test", rate: 1.5, pitch: 0.75)])
    }

    @Test func readsRelativePitch() {
        #expect(SSMLParser.parse("<speak><prosody pitch=\"+50%\">Test</prosody></speak>").speech.first?.pitch == 1.5)
        #expect(SSMLParser.parse("<speak><prosody pitch=\"-25%\">Test</prosody></speak>").speech.first?.pitch == 0.75)
    }

    /// VoiceOver wraps every request in a neutral <prosody> and marks a
    /// capital letter with a second one inside it.
    @Test func innerProsodyIsRelativeToTheOuterOne() {
        let capital = SSMLParser.parse(
            "<speak><prosody pitch=\"+0.0%\" rate=\"100.0%\" volume=\"+0.0dB\"><lang xml:lang=\"hr\">"
            + "<voice name=\"\"><prosody pitch=\"+50.0%\"><say-as interpret-as=\"characters\">A</say-as>"
            + "</prosody></voice></lang></prosody></speak>"
        )
        #expect(capital.parts == [text("A", pitch: 1.5, spellOut: true)])

        let nested = SSMLParser.parse(
            "<speak><prosody pitch=\"+20%\" rate=\"150%\"><prosody pitch=\"-50%\">Test</prosody></prosody></speak>"
        )
        #expect(abs((nested.speech.first?.pitch ?? 0) - 0.6) < 0.001)
        #expect(nested.speech.first?.rate == 1.5)
    }

    /// Text under another pitch is a part of its own and keeps that pitch.
    @Test func eachPartKeepsItsOwnPitch() {
        let utterance = SSMLParser.parse(
            "<speak><prosody pitch=\"+0%\">Dugacka recenica <prosody pitch=\"+50%\">A</prosody></prosody></speak>"
        )
        #expect(utterance.parts == [text("Dugacka recenica"), text("A", pitch: 1.5)])
    }

    /// VoiceOver sends the name, the badge count and the type of a button as
    /// one request and marks only the count as characters. Spelling the whole
    /// request read "Ažuriraj sve 5 Button" letter by letter.
    @Test func onlyTheMarkedPartIsSpelled() {
        let utterance = SSMLParser.parse(
            "<speak><prosody pitch=\"+0.0%\" rate=\"100.0%\" volume=\"+0.0dB\"><lang xml:lang=\"hr\">"
            + "<voice name=\"\">Ažuriraj sve</voice>"
            + "<voice name=\"\"><say-as interpret-as=\"characters\">5</say-as></voice>"
            + "<voice name=\"\">Button</voice></lang></prosody></speak>"
        )
        #expect(utterance.parts == [
            text("Ažuriraj sve"),
            .phraseBoundary,
            text("5", spellOut: true),
            .phraseBoundary,
            text("Button"),
        ])
    }

    @Test func otherSayAsKindsAreNotSpelled() {
        let utterance = SSMLParser.parse(
            "<speak>Imam <say-as interpret-as=\"cardinal\">12</say-as> jabuka</speak>"
        )
        #expect(utterance.parts == [text("Imam 12 jabuka")])
    }

    /// The name and the type of a control are separate <voice> elements and
    /// must not run together into one phrase.
    @Test func voiceElementsAreSeparatePhrases() {
        let utterance = SSMLParser.parse(
            "<speak><voice name=\"\">Zrakoplovni mod</voice><voice name=\"\">Tipka za izmjenu</voice>"
            + "<voice name=\"\"><break time=\"500.0ms\"/>isključeno</voice></speak>"
        )
        #expect(utterance.parts == [
            text("Zrakoplovni mod"),
            .phraseBoundary,
            text("Tipka za izmjenu"),
            .pause(seconds: 0.5),
            text("isključeno"),
        ])
    }

    /// Inline elements that change nothing leave a sentence in one piece.
    @Test func inlineElementsDoNotSplitASentence() {
        let utterance = SSMLParser.parse(
            "<speak>Prvi <mark name=\"a\"/>drugi <emphasis>treći</emphasis> četvrti</speak>"
        )
        #expect(utterance.parts == [text("Prvi drugi treći četvrti")])
    }

    @Test func clampsOutOfRangeValues() {
        // The formant voices' range; the engine narrows it for the recorded voices.
        #expect(SSMLParser.parse("<speak><prosody rate=\"9.0\">Test</prosody></speak>").speech.first?.rate == 4.0)
        #expect(SSMLParser.parse("<speak><prosody rate=\"0.01\">Test</prosody></speak>").speech.first?.rate == 0.25)
        #expect(SSMLParser.parse("<speak><prosody pitch=\"+900%\">Test</prosody></speak>").speech.first?.pitch == 4.0)
    }

    @Test func breakIsAPauseOfItsLength() {
        let timed = SSMLParser.parse(
            "<speak><prosody rate=\"1.0\">Prvi<break time=\"300ms\"/>drugi<break time=\"1.5s\"/></prosody></speak>"
        )
        #expect(timed.parts == [text("Prvi"), .pause(seconds: 0.3), text("drugi"), .pause(seconds: 1.5)])

        let strength = SSMLParser.parse("<speak>Prvi<break strength=\"strong\"/>drugi<break/>treći</speak>")
        #expect(strength.parts == [
            text("Prvi"), .pause(seconds: 0.5), text("drugi"), .pause(seconds: 0.25), text("treći"),
        ])

        let endless = SSMLParser.parse("<speak>Prvi<break time=\"3600s\"/></speak>")
        #expect(endless.parts == [text("Prvi"), .pause(seconds: 10)])
    }

    @Test func plainTextKeepsDefaults() {
        let utterance = SSMLParser.parse("<speak>Samo tekst</speak>")
        #expect(utterance.parts == [text("Samo tekst")])
    }

    @Test func decodesEntitiesWithoutDoubleDecoding() {
        let utterance = SSMLParser.parse("<speak>Ivan &amp;lt; Marko &amp; Ana</speak>")
        #expect(utterance.parts == [text("Ivan &lt; Marko & Ana")])
    }

    /// The log line names what a request was made of, never what it said.
    @Test func summaryLeavesTheTextOut() {
        let utterance = SSMLParser.parse(
            "<speak><voice name=\"\">Tajna</voice><voice name=\"\"><say-as interpret-as=\"characters\">5</say-as>"
            + "<break time=\"60ms\"/></voice></speak>"
        )
        #expect(utterance.summary
            == "text 5 rate 1.0 pitch 1.0 | boundary | spell 1 rate 1.0 pitch 1.0 | pause 60 ms")
    }
}

// MARK: - Settings

struct SettingsTests {

    @Test func snapshotHasExpectedDefaults() throws {
        let suiteName = "LaprdusTests-\(UUID().uuidString)"
        let defaults = try #require(UserDefaults(suiteName: suiteName))
        defer { defaults.removePersistentDomain(forName: suiteName) }

        let snapshot = SettingsSnapshot.load(from: defaults)
        #expect(snapshot.defaultVoice == VoiceCatalog.defaultVoiceID)
        #expect(snapshot.speed == 1.0)
        #expect(snapshot.pitch == 1.0)
        #expect(snapshot.volume == 1.0)
        #expect(snapshot.forceSpeed == false)
        #expect(snapshot.emojiEnabled == false)
        #expect(snapshot.inflectionEnabled == true)
        #expect(snapshot.inflectionLevel == 0.5)
        #expect(snapshot.acceleration == 1.0)
        #expect(snapshot.sentencePause == 100)
        #expect(snapshot.commaPause == 100)
        #expect(snapshot.newlinePause == 100)
        #expect(snapshot.numberMode == 0)
        #expect(snapshot.userDictionariesEnabled == true)
    }

    @Test func storePersistsChanges() throws {
        let suiteName = "LaprdusTests-\(UUID().uuidString)"
        let defaults = try #require(UserDefaults(suiteName: suiteName))
        defer { defaults.removePersistentDomain(forName: suiteName) }

        let store = SettingsStore(defaults: defaults)
        store.speed = 1.7
        store.inflectionEnabled = false
        store.defaultVoice = "vlado"

        let reloaded = SettingsSnapshot.load(from: defaults)
        #expect(reloaded.speed == 1.7)
        #expect(reloaded.inflectionEnabled == false)
        #expect(reloaded.defaultVoice == "vlado")
    }
}
