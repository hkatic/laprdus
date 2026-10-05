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
    /// The host asked for the text to be spelled out
    /// (<say-as interpret-as="characters">).
    var spellOut = false
}

enum SSMLParser {

    static func parse(_ ssml: String) -> SSMLUtterance {
        var utterance = SSMLUtterance()

        (utterance.rate, utterance.pitch) = prosody(in: ssml)
        utterance.spellOut = ssml.range(
            of: "<say-as\\b[^>]*interpret-as\\s*=\\s*[\"']characters[\"']",
            options: [.regularExpression, .caseInsensitive]
        ) != nil

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

    /// The tags of an SSML string with the spoken text left out, for logging.
    static func markup(in ssml: String) -> String {
        guard let regex = try? NSRegularExpression(pattern: "<[^>]+>") else { return "" }
        let range = NSRange(ssml.startIndex..., in: ssml)
        return regex.matches(in: ssml, range: range)
            .compactMap { Range($0.range, in: ssml).map { String(ssml[$0]) } }
            .joined()
    }

    /// Rate and pitch of the spoken text.
    ///
    /// <prosody> elements nest, and each one is relative to the one around
    /// it: VoiceOver wraps everything in a neutral `pitch="+0.0%"` and marks a
    /// capital letter with an inner `pitch="+50.0%"`. The engine speaks a
    /// request with one rate and one pitch, so the values in force over the
    /// largest share of the text win. Attributes are only ever read from
    /// <prosody> tags, never from the spoken text itself — reading markup or
    /// source code aloud otherwise let a literal rate="..." in the content
    /// change the speech rate.
    private static func prosody(in ssml: String) -> (rate: Float, pitch: Float) {
        guard let regex = try? NSRegularExpression(pattern: "<(/?)([A-Za-z][\\w:-]*)[^>]*>") else {
            return (1.0, 1.0)
        }
        struct Level: Hashable {
            var rate: Float = 1.0
            var pitch: Float = 1.0
        }
        var stack = [Level()]
        var weights: [Level: Int] = [:]
        var order: [Level] = []
        var position = ssml.startIndex

        func count(textUpTo end: String.Index) {
            let spoken = ssml[position..<end].filter { !$0.isWhitespace }.count
            guard spoken > 0, let level = stack.last else { return }
            if weights[level] == nil { order.append(level) }
            weights[level, default: 0] += spoken
        }

        for match in regex.matches(in: ssml, range: NSRange(ssml.startIndex..., in: ssml)) {
            guard let tagRange = Range(match.range, in: ssml),
                  let nameRange = Range(match.range(at: 2), in: ssml) else { continue }
            count(textUpTo: tagRange.lowerBound)
            position = tagRange.upperBound

            guard ssml[nameRange].lowercased() == "prosody" else { continue }
            let tag = String(ssml[tagRange])
            if match.range(at: 1).length > 0 {
                if stack.count > 1 { stack.removeLast() }
            } else if !tag.hasSuffix("/>") {
                var level = stack.last ?? Level()
                if let rate = firstAttribute("rate", inTags: [tag]) {
                    level.rate = clamp(level.rate * rateMultiplier(from: rate))
                }
                if let pitch = firstAttribute("pitch", inTags: [tag]) {
                    level.pitch = clamp(level.pitch * pitchMultiplier(from: pitch))
                }
                stack.append(level)
            }
        }
        count(textUpTo: ssml.endIndex)

        // The first level wins a tie.
        var best = Level()
        var bestWeight = 0
        for level in order where weights[level, default: 0] > bestWeight {
            best = level
            bestWeight = weights[level, default: 0]
        }
        return (best.rate, best.pitch)
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

    /// "50%" → 0.5, "+10%" → 1.1, "1.5" → 1.5, plus the SSML keyword scale.
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
