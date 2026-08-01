// SSMLParser.swift - Minimal SSML handling for AVSpeechSynthesisProviderRequest.
// The system hands the utterance to the provider as SSML; Laprdus synthesizes
// plain text, so the prosody attributes are extracted and the tags stripped.

import Foundation

struct SSMLUtterance {
    var text = ""
    /// Rate multiplier (1.0 = normal) parsed from <prosody rate="...">.
    var rate: Float = 1.0
    /// Pitch multiplier (1.0 = normal) parsed from <prosody pitch="...">.
    var pitch: Float = 1.0
}

enum SSMLParser {

    static func parse(_ ssml: String) -> SSMLUtterance {
        var utterance = SSMLUtterance()

        if let rateAttribute = firstAttribute("rate", in: ssml) {
            utterance.rate = rateMultiplier(from: rateAttribute)
        }
        if let pitchAttribute = firstAttribute("pitch", in: ssml) {
            utterance.pitch = pitchMultiplier(from: pitchAttribute)
        }

        // <break> would be swallowed by tag stripping; approximate it with a
        // newline so the engine inserts its newline pause.
        var text = ssml.replacingOccurrences(
            of: "<break[^>]*/?>",
            with: "\n",
            options: [.regularExpression, .caseInsensitive]
        )
        text = text.replacingOccurrences(of: "<[^>]+>", with: "", options: .regularExpression)
        text = decodeEntities(text)
        utterance.text = text.trimmingCharacters(in: .whitespacesAndNewlines)
        return utterance
    }

    private static func firstAttribute(_ name: String, in ssml: String) -> String? {
        let pattern = "\(name)\\s*=\\s*\"([^\"]*)\""
        guard let regex = try? NSRegularExpression(pattern: pattern, options: .caseInsensitive),
              let match = regex.firstMatch(in: ssml, range: NSRange(ssml.startIndex..., in: ssml)),
              let range = Range(match.range(at: 1), in: ssml) else {
            return nil
        }
        return String(ssml[range])
    }

    /// "50%" → 0.5, "1.5" → 1.5, plus the SSML keyword scale.
    private static func rateMultiplier(from value: String) -> Float {
        let lowered = value.lowercased().trimmingCharacters(in: .whitespaces)
        switch lowered {
        case "x-slow": return 0.5
        case "slow": return 0.75
        case "medium", "default": return 1.0
        case "fast": return 1.5
        case "x-fast": return 2.0
        default: break
        }
        if lowered.hasSuffix("%"), let percent = Float(lowered.dropLast()) {
            return clamp(percent / 100.0)
        }
        if let numeric = Float(lowered) {
            return clamp(numeric)
        }
        return 1.0
    }

    /// "+50%" → 1.5, "-25%" → 0.75, "150%" → 1.5, plus SSML keywords.
    private static func pitchMultiplier(from value: String) -> Float {
        let lowered = value.lowercased().trimmingCharacters(in: .whitespaces)
        switch lowered {
        case "x-low": return 0.5
        case "low": return 0.75
        case "medium", "default": return 1.0
        case "high": return 1.5
        case "x-high": return 2.0
        default: break
        }
        if lowered.hasSuffix("%") {
            let body = String(lowered.dropLast())
            if body.hasPrefix("+") || body.hasPrefix("-"), let delta = Float(body) {
                return clamp(1.0 + delta / 100.0)
            }
            if let percent = Float(body) {
                return clamp(percent / 100.0)
            }
        }
        if let numeric = Float(lowered) {
            return clamp(numeric)
        }
        return 1.0
    }

    private static func clamp(_ value: Float) -> Float {
        min(max(value, 0.5), 2.0)
    }

    private static func decodeEntities(_ text: String) -> String {
        var result = text
        let entities: [(String, String)] = [
            ("&lt;", "<"),
            ("&gt;", ">"),
            ("&quot;", "\""),
            ("&apos;", "'"),
            ("&amp;", "&"),
        ]
        for (entity, character) in entities {
            result = result.replacingOccurrences(of: entity, with: character)
        }
        return result
    }
}
