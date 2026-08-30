// SSMLParser.swift - Minimal SSML handling for AVSpeechSynthesisProviderRequest.
// The system hands the utterance to the provider as SSML; Laprdus synthesizes
// plain text, so the prosody attributes are extracted and the tags stripped.
//
// Used only by the LaprdusVoices extension, but it lives in Shared so the
// app-hosted test bundle can reach it.

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

        let prosody = prosodyTags(in: ssml)
        if let rateAttribute = firstAttribute("rate", inTags: prosody) {
            utterance.rate = rateMultiplier(from: rateAttribute)
        }
        if let pitchAttribute = firstAttribute("pitch", inTags: prosody) {
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

    /// The opening <prosody> tags, so prosody attributes are never read out of
    /// the spoken text itself — reading markup or source code aloud otherwise
    /// let a literal rate="..." in the content change the speech rate.
    private static func prosodyTags(in ssml: String) -> [String] {
        let pattern = "<prosody\\b[^>]*>"
        guard let regex = try? NSRegularExpression(pattern: pattern, options: .caseInsensitive) else {
            return []
        }
        let range = NSRange(ssml.startIndex..., in: ssml)
        return regex.matches(in: ssml, range: range).compactMap { match in
            Range(match.range, in: ssml).map { String(ssml[$0]) }
        }
    }

    /// First value of `name` across the given tags. Both quote styles are
    /// accepted; the lookbehind keeps "rate" from matching inside another
    /// attribute name such as x-rate.
    private static func firstAttribute(_ name: String, inTags tags: [String]) -> String? {
        let pattern = "(?<![-\\w])\(name)\\s*=\\s*(?:\"([^\"]*)\"|'([^']*)')"
        guard let regex = try? NSRegularExpression(pattern: pattern, options: .caseInsensitive) else {
            return nil
        }
        for tag in tags {
            let range = NSRange(tag.startIndex..., in: tag)
            guard let match = regex.firstMatch(in: tag, range: range) else { continue }
            for group in 1...2 {
                if let valueRange = Range(match.range(at: group), in: tag) {
                    return String(tag[valueRange])
                }
            }
        }
        return nil
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
