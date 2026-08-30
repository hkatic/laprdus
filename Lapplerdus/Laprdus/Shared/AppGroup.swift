// AppGroup.swift - Shared container between the app and the LaprdusVoices extension.

import Foundation
import os

enum AppGroup {
    /// App group shared by the app and the speech synthesis extension so that
    /// settings and user dictionaries written by the app are visible to the
    /// extension process.
    static let identifier = "group.com.hrvojekatic.laprdus"

    private static let log = Logger(subsystem: "com.hrvojekatic.laprdus", category: "AppGroup")

    /// Whether the app group container is actually available. When it is not,
    /// the app and the extension each fall back to their own private storage
    /// and stop agreeing on settings and dictionaries, so this is surfaced in
    /// the About screen instead of failing silently.
    static var isShared: Bool {
        FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: identifier) != nil
            && UserDefaults(suiteName: identifier) != nil
    }

    /// Shared defaults suite; falls back to standard defaults when the app
    /// group is not provisioned (e.g. local development builds).
    static var defaults: UserDefaults {
        guard let suite = UserDefaults(suiteName: identifier) else {
            log.fault("App group \(identifier, privacy: .public) unavailable; settings will not be shared with the speech extension")
            return .standard
        }
        return suite
    }

    /// Directory where user dictionaries are stored. Prefers the app group
    /// container so the extension can read them; falls back to Application
    /// Support inside the current container.
    static var dictionariesDirectory: URL {
        let base: URL
        if let container = FileManager.default.containerURL(forSecurityApplicationGroupIdentifier: identifier) {
            base = container
        } else {
            log.fault("App group container \(identifier, privacy: .public) unavailable; user dictionaries will not reach the speech extension")
            base = FileManager.default.urls(for: .applicationSupportDirectory, in: .userDomainMask).first
                ?? FileManager.default.temporaryDirectory
        }
        let dir = base.appendingPathComponent("Dictionaries", isDirectory: true)
        try? FileManager.default.createDirectory(at: dir, withIntermediateDirectories: true)
        return dir
    }
}
