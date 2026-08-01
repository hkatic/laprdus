// AppGroup.swift - Shared container between the app and the LaprdusVoices extension.

import Foundation

enum AppGroup {
    /// App group shared by the app and the speech synthesis extension so that
    /// settings and user dictionaries written by the app are visible to the
    /// extension process.
    static let identifier = "group.com.hrvojekatic.laprdus"

    /// Shared defaults suite; falls back to standard defaults when the app
    /// group is not provisioned (e.g. local development builds).
    static var defaults: UserDefaults {
        UserDefaults(suiteName: identifier) ?? .standard
    }

    /// Directory where user dictionaries are stored. Prefers the app group
    /// container so the extension can read them; falls back to Application
    /// Support inside the current container.
    static var dictionariesDirectory: URL {
        let base: URL
        if let container = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: identifier) {
            base = container
        } else {
            base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
                ?? FileManager.default.temporaryDirectory
        }
        let dir = base.appendingPathComponent("Dictionaries", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir
    }
}
