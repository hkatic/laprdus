// SSMLParser.swift - Minimal SSML handling for AVSpeechSynthesisProviderRequest.
// The system hands the utterance to the provider as SSML; Laprdus synthesizes
// plain text, so the request is cut into the parts that are spoken separately,
// each with its own prosody.
//
// Used only by the LaprdusVoices extension, but it lives in Shared so the
// app-hosted test bundle can reach it.

import Foundation

/// A stretch of text that is spoken in one go, with one rate and one pitch.
struct SSMLSpeech: Equatable {
    var text = ""
    /// Rate multiplier (1.0 = normal) from the <prosody rate="..."> around it.
    var rate: Float = 1.0
    /// Pitch multiplier (1.0 = normal) from the <prosody pitch="..."> around it.
    var pitch: Float = 1.0
    /// The host asked for this text to be spelled out
    /// (<say-as interpret-as="characters">).
    var spellOut = false
}

enum SSMLPart: Equatable {
    case speech(SSMLSpeech)
    /// <break>: silence of the given length.
    case pause(seconds: Double)
    /// Two elements that are separate phrases (<voice>, <p>, <s>) follow each
    /// other with no <break> between them.
    case phraseBoundary
}

/// A request in the order it has to be heard.
///
/// One request is not one phrase with one set of attributes. VoiceOver
/// describes an item as a row of <voice> elements (name, value, type), puts
/// <break> between some of them, and marks only a part of it as characters to
/// spell: a badge count next to a button name is such a part. Each part
/// therefore carries its own attributes.
struct SSMLUtterance {
    var parts: [SSMLPart] = []

    var speech: [SSMLSpeech] {
        parts.compactMap { part in
            if case .speech(let speech) = part { return speech }
            return nil
        }
    }

    /// The parts with the spoken text left out, for logging.
    var summary: String {
        parts.map { part in
            switch part {
            case .speech(let speech):
                return "\(speech.spellOut ? "spell" : "text") \(speech.text.count)"
                    + " rate \(speech.rate) pitch \(speech.pitch)"
            case .pause(let seconds):
                return "pause \(Int((seconds * 1000).rounded())) ms"
            case .phraseBoundary:
                return "boundary"
            }
        }.joined(separator: " | ")
    }
}

enum SSMLParser {

    /// Elements whose content is a phrase of its own.
    private static let phraseElements: Set<String> = ["speak", "voice", "p", "s"]

    /// Longest silence a single <break> may ask for.
    private static let longestBreak = 10.0

    static func parse(_ ssml: String) -> SSMLUtterance {
        var utterance = SSMLUtterance()
        guard let regex = try? NSRegularExpression(pattern: "<[^>]+>") else { return utterance }

        // <prosody> elements nest, and each one is relative to the one around
        // it: VoiceOver wraps everything in a neutral `pitch="+0.0%"` and marks
        // a capital letter with an inner `pitch="+50.0%"`. Attributes are only
        // ever read from tags, never from the spoken text itself — reading
        // markup or source code aloud otherwise let a literal rate="..." in
        // the content change the speech rate.
        var prosody = [SSMLSpeech()]
        var sayAs: [Bool] = []
        var run = ""
        var runStyle = SSMLSpeech()
        var phraseEnded = false

        func style() -> SSMLSpeech {
            var style = prosody.last ?? SSMLSpeech()
            style.spellOut = sayAs.contains(true)
            return style
        }

        func flush() {
            var speech = runStyle
            speech.text = decodeEntities(run).trimmingCharacters(in: .whitespacesAndNewlines)
            run = ""
            guard !speech.text.isEmpty else { return }
            if phraseEnded, case .speech = utterance.parts.last {
                utterance.parts.append(.phraseBoundary)
            }
            phraseEnded = false
            utterance.parts.append(.speech(speech))
        }

        func add(text: Substring) {
            guard !text.isEmpty else { return }
            // Text under other attributes than the text before it is a part
            // of its own. Inline elements that change nothing (<mark>,
            // <emphasis>, <lang>) leave the sentence in one piece.
            let current = style()
            if current != runStyle {
                flush()
                runStyle = current
            }
            run += text
        }

        var position = ssml.startIndex
        for match in regex.matches(in: ssml, range: NSRange(ssml.startIndex..., in: ssml)) {
            guard let tagRange = Range(match.range, in: ssml) else { continue }
            add(text: ssml[position..<tagRange.lowerBound])
            position = tagRange.upperBound

            let tag = String(ssml[tagRange])
            let closing = tag.hasPrefix("</")
            let selfClosing = tag.hasSuffix("/>")
            let name = tag.dropFirst(closing ? 2 : 1)
                .prefix { $0.isLetter || $0.isNumber || $0 == "-" || $0 == ":" || $0 == "_" }
                .lowercased()

            switch name {
            case "prosody":
                if closing {
                    if prosody.count > 1 { prosody.removeLast() }
                } else if !selfClosing {
                    var level = prosody.last ?? SSMLSpeech()
                    if let rate = firstAttribute("rate", inTags: [tag]) {
                        level.rate = clamp(level.rate * rateMultiplier(from: rate))
                    }
                    if let pitch = firstAttribute("pitch", inTags: [tag]) {
                        level.pitch = clamp(level.pitch * pitchMultiplier(from: pitch))
                    }
                    prosody.append(level)
                }
            case "say-as":
                if closing {
                    if !sayAs.isEmpty { sayAs.removeLast() }
                } else if !selfClosing {
                    sayAs.append(firstAttribute("interpret-as", inTags: [tag])?.lowercased() == "characters")
                }
            case "break":
                if !closing {
                    flush()
                    utterance.parts.append(.pause(seconds: breakLength(of: tag)))
                }
            case _ where phraseElements.contains(name):
                flush()
                phraseEnded = true
            default:
                break
            }
        }
        add(text: ssml[position...])
        flush()
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

    /// Length of a <break>: `time="500ms"` / `time="1.5s"`, or the SSML
    /// `strength` scale. A bare <break/> is a medium one.
    private static func breakLength(of tag: String) -> Double {
        if let time = firstAttribute("time", inTags: [tag])?.lowercased().trimmingCharacters(in: .whitespaces) {
            var seconds: Double?
            if time.hasSuffix("ms") {
                seconds = Double(time.dropLast(2)).map { $0 / 1000 }
            } else if time.hasSuffix("s") {
                seconds = Double(time.dropLast())
            }
            if let seconds, seconds.isFinite {
                return min(max(seconds, 0), longestBreak)
            }
        }
        switch firstAttribute("strength", inTags: [tag])?.lowercased() {
        case "none": return 0
        case "x-weak": return 0.05
        case "weak": return 0.1
        case "strong": return 0.5
        case "x-strong": return 1.0
        default: return 0.25
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
