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

    @Test func voiceRegistryExposesAllFiveVoices() throws {
        let ids = VoiceCatalog.all.map(\.id)
        #expect(ids == ["josip", "vlado", "detence", "baba", "djed"])
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
        try engine.loadVoice("josip")
        #expect(engine.isInitialized)
        let chunk = try engine.synthesize("Dobar dan!")
        #expect(chunk.samples.count > 0)
        #expect(chunk.sampleRate == 22050)
        #expect(chunk.channels == 1)
    }

    @Test func synthesizesSpelledCharacter() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("josip")
        let chunk = try engine.synthesize("Č", spelled: true)
        #expect(chunk.samples.count > 0)
    }

    @Test func switchingToDerivedVoiceWorks() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("baba")
        #expect(engine.currentVoice == "baba")
        let chunk = try engine.synthesize("Dobar dan")
        #expect(chunk.samples.count > 0)
    }

    @Test func appliesSettingsWithoutError() throws {
        let engine = try LaprdusEngine()
        try engine.loadVoice("josip")
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

    @Test func writtenFormatMatchesAndroid() throws {
        let (store, dir) = try makeStore()
        defer { try? FileManager.default.removeItem(at: dir) }

        try store.save([DictionaryEntry(grapheme: "Facebook", phoneme: "Fejzbuk")], type: .main)
        let data = try Data(contentsOf: store.fileURL(for: .main))
        let root = try #require(try JSONSerialization.jsonObject(with: data) as? [String: Any])
        #expect(root["version"] as? String == "1.0")
        let entries = try #require(root["entries"] as? [[String: Any]])
        #expect(entries.count == 1)
        #expect(entries[0]["grapheme"] as? String == "Facebook")
        // Empty comment is omitted, same as the Android writer.
        #expect(entries[0]["comment"] == nil)
    }

    @Test func dictionaryTypesUseAndroidFileNames() {
        #expect(DictionaryType.main.fileName == "user.json")
        #expect(DictionaryType.spelling.fileName == "spelling.json")
        #expect(DictionaryType.emoji.fileName == "emoji.json")
    }
}

// MARK: - Settings

struct SettingsTests {

    @Test func defaultsMatchAndroid() throws {
        let suiteName = "LaprdusTests-\(UUID().uuidString)"
        let defaults = try #require(UserDefaults(suiteName: suiteName))
        defer { defaults.removePersistentDomain(forName: suiteName) }

        let snapshot = SettingsSnapshot.load(from: defaults)
        #expect(snapshot.defaultVoice == "josip")
        #expect(snapshot.speed == 1.0)
        #expect(snapshot.pitch == 1.0)
        #expect(snapshot.volume == 1.0)
        #expect(snapshot.forceSpeed == false)
        #expect(snapshot.emojiEnabled == false)
        #expect(snapshot.inflectionEnabled == true)
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
