// -*- coding: utf-8 -*-
// tts_engine.cpp - Main TTS engine implementation

#include "tts_engine.hpp"
#include "spelling_dict.hpp"
#include "emoji_dict.hpp"
#include "phoneme_mapper.hpp"
#include "../formant/formant_synthesizer.hpp"
#include <algorithm>
#include <filesystem>
#include <fstream>
#include <iterator>

namespace laprdus {

// =============================================================================
// Implementation Structure
// =============================================================================

struct TTSEngine::Impl {
    PhonemeData phoneme_data;
    std::unique_ptr<AudioSynthesizer> synthesizer;
    VoiceLanguage concat_language = VoiceLanguage::Croatian;
    CroatianNumbers number_converter;
    InflectionProcessor inflection;
    PronunciationDictionary dictionary;
    SpellingDictionary spelling_dictionary;         // bundled: digits, punctuation, symbols
    SpellingDictionary user_spelling_dictionary;    // the user's: wins over everything
    EmojiDictionary emoji_dictionary;
    VoiceParams voice_params;
    bool initialized = false;

    // Formant voice (replaces phoneme_data/synthesizer when set)
    std::unique_ptr<formant::FormantSynthesizer> formant;

    // Streaming: the clauses are handed over in chunks of this size
    std::function<void(const AudioBuffer&)> stream_callback;
    uint32_t stream_chunk_samples = 0;

    // The user's accent entries, kept across voice changes and handed to
    // every new formant synthesizer
    std::shared_ptr<const formant::UserLexicon> user_lexicon;
    std::string user_lexicon_report;

    Impl() = default;
};

// =============================================================================
// Constructor / Destructor
// =============================================================================

TTSEngine::TTSEngine()
    : m_impl(std::make_unique<Impl>())
{
}

TTSEngine::~TTSEngine() = default;

TTSEngine::TTSEngine(TTSEngine&&) noexcept = default;

TTSEngine& TTSEngine::operator=(TTSEngine&&) noexcept = default;

// =============================================================================
// Initialize from File
// =============================================================================

bool TTSEngine::initialize(const std::string& phoneme_path,
                          span<const uint8_t> key) {
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }

    // A formant voice stays usable if the data of the requested voice cannot
    // be loaded; it does not depend on the phoneme data replaced here.
    if (!m_impl->formant) {
        m_impl->initialized = false;
    }

    // Determine if path is a directory or file
    std::filesystem::path path(phoneme_path);

    bool loaded = false;

    if (std::filesystem::is_directory(path)) {
        // Load from directory of individual WAV files
        loaded = m_impl->phoneme_data.load_from_directory(phoneme_path);
    } else if (std::filesystem::exists(path)) {
        // Load from packed binary file
        loaded = m_impl->phoneme_data.load_from_file(phoneme_path, key);
    } else {
        // Path doesn't exist - try as directory anyway in case it will be created
        loaded = m_impl->phoneme_data.load_from_directory(phoneme_path);
    }

    if (!loaded) {
        return false;
    }

    m_impl->formant.reset();
    m_impl->number_converter.set_dialect(CroatianNumbers::Dialect::Croatian);

    // Create synthesizer (analyses the recordings)
    m_impl->synthesizer = std::make_unique<AudioSynthesizer>(m_impl->phoneme_data,
                                                             m_impl->concat_language);
    m_impl->synthesizer->set_user_lexicon(m_impl->user_lexicon);
    m_impl->synthesizer->set_voice_params(m_impl->voice_params);

    m_impl->initialized = true;
    return true;
}

// =============================================================================
// Initialize from Memory
// =============================================================================

bool TTSEngine::initialize_from_memory(const uint8_t* data, size_t size,
                                       span<const uint8_t> key) {
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }

    if (!m_impl->formant) {
        m_impl->initialized = false;
    }

    if (!data || size == 0) {
        return false;
    }

    if (!m_impl->phoneme_data.load_from_memory(data, size, key)) {
        return false;
    }

    m_impl->formant.reset();
    m_impl->number_converter.set_dialect(CroatianNumbers::Dialect::Croatian);

    // Create synthesizer (analyses the recordings)
    m_impl->synthesizer = std::make_unique<AudioSynthesizer>(m_impl->phoneme_data,
                                                             m_impl->concat_language);
    m_impl->synthesizer->set_user_lexicon(m_impl->user_lexicon);
    m_impl->synthesizer->set_voice_params(m_impl->voice_params);

    m_impl->initialized = true;
    return true;
}

// =============================================================================
// Initialize Formant Voice
// =============================================================================

bool TTSEngine::initialize_formant(const char* voice_id) {
    if (!m_impl) {
        m_impl = std::make_unique<Impl>();
    }

    const formant::FormantVoice* voice = formant::find_formant_voice(voice_id);
    if (!voice) {
        return false;
    }

    m_impl->formant = std::make_unique<formant::FormantSynthesizer>(*voice);
    m_impl->formant->set_user_lexicon(m_impl->user_lexicon);
    m_impl->synthesizer.reset();
    m_impl->phoneme_data.clear();   // a formant voice needs no recordings

    // Number words follow the voice's language (tisuća / hiljada, ...)
    switch (voice->language) {
        case VoiceLanguage::Serbian:
            m_impl->number_converter.set_dialect(CroatianNumbers::Dialect::Serbian);
            break;
        case VoiceLanguage::Bosnian:
            m_impl->number_converter.set_dialect(CroatianNumbers::Dialect::Bosnian);
            break;
        case VoiceLanguage::Croatian:
        default:
            m_impl->number_converter.set_dialect(CroatianNumbers::Dialect::Croatian);
            break;
    }

    m_impl->initialized = true;
    return true;
}

void TTSEngine::set_language(VoiceLanguage language) {
    if (!m_impl) return;
    m_impl->concat_language = language;
    if (m_impl->synthesizer) {
        m_impl->synthesizer->set_language(language);
    }
}

bool TTSEngine::is_formant() const {
    return m_impl && m_impl->formant != nullptr;
}

void TTSEngine::begin_utterance() {
    // A singing preset starts its song over with every utterance.
    if (m_impl && m_impl->formant) {
        m_impl->formant->rewind_song();
    }
}

// =============================================================================
// Is Initialized
// =============================================================================

bool TTSEngine::is_initialized() const {
    return m_impl && m_impl->initialized;
}

// =============================================================================
// Synthesize Text
// =============================================================================

namespace {

// A screen reader announces a capital letter as a word or two and the
// letter: NVDA "veliko N" (Croatian, Serbian, Bosnian) and "cap N",
// TalkBack "veliko slovo N" (Croatian), "велико Н" (Serbian), "veliko N"
// (Serbian Latin, Bosnian) and "capital N", VoiceOver "veliko početno
// slovo N" (Croatian) and "cap N". They send it as text, not as a spelled
// character, so without this the letter is read by its name in every
// spelling mode, and at the speech rate. Returns the words and the letter.
bool split_capital_announcement(const std::string& text, std::string& words,
                                std::string& letter) {
    std::u32string cps = PhonemeMapper::utf8_to_utf32(text);
    auto is_space = [](char32_t c) { return c == U' ' || c == U'\t' || c == U'\n' || c == U'\r'; };
    while (!cps.empty() && (is_space(cps.back()) || cps.back() == U'.')) cps.pop_back();
    size_t start = 0;
    while (start < cps.size() && is_space(cps[start])) ++start;

    std::vector<std::u32string> tokens;
    for (size_t i = start; i < cps.size();) {
        size_t end = i;
        while (end < cps.size() && !is_space(cps[end])) ++end;
        tokens.push_back(cps.substr(i, end - i));
        i = end;
        while (i < cps.size() && is_space(cps[i])) ++i;
    }
    if (tokens.size() < 2 || tokens.size() > 4) return false;
    if (tokens.back().size() != 1 || formant::spelling_letter(tokens.back()[0]).empty()) {
        return false;
    }

    // The words lowercased, Cyrillic in Latin letters
    auto plain = [](const std::u32string& word) {
        std::u32string out;
        for (char32_t c : word) {
            std::u32string letter = formant::spelling_letter(c);
            if (letter.empty()) return std::u32string();
            out += letter;
        }
        return out;
    };
    static const std::u32string first[] = { U"veliko", U"cap", U"capital" };
    static const std::u32string more[] = { U"slovo", U"početno", U"letter" };
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        const std::u32string word = plain(tokens[i]);
        const auto begin = i == 0 ? std::begin(first) : std::begin(more);
        const auto end = i == 0 ? std::end(first) : std::end(more);
        if (std::find(begin, end, word) == end) return false;
    }

    letter = PhonemeMapper::utf32_to_utf8(tokens.back());
    std::u32string spoken;
    for (size_t i = 0; i + 1 < tokens.size(); ++i) {
        if (i) spoken.push_back(U' ');
        spoken += tokens[i];
    }
    words = PhonemeMapper::utf32_to_utf8(spoken);
    return true;
}

} // namespace

SynthesisResult TTSEngine::synthesize_capital(const std::string& words,
                                              const std::string& letter) {
    SynthesisResult result = synthesize(words);
    if (!result.success) return result;
    SynthesisResult spelled = synthesize_spelled(letter);
    if (!spelled.success) return spelled;
    result.audio.samples.insert(result.audio.samples.end(), spelled.audio.samples.begin(),
                                spelled.audio.samples.end());
    return result;
}

SynthesisResult TTSEngine::synthesize(const std::string& text) {
    SynthesisResult result;
    begin_utterance();

    if (!is_initialized()) {
        result.success = false;
        result.error_message = "Engine not initialized";
        return result;
    }

    if (text.empty()) {
        result.success = true;  // Empty text is valid, just produces no audio
        return result;
    }

    std::string words, letter;
    if (split_capital_announcement(text, words, letter)) {
        return synthesize_capital(words, letter);
    }

    try {
        // Step 1: Preprocess text (expand numbers, normalize)
        std::string processed = preprocess_text(text);

        // Step 2: Segment text by punctuation
        std::vector<TextSegment> segments = segment_text(processed);

        // Step 3: Synthesize each segment with inflection
        result.audio = synthesize_segments(segments);

        result.success = true;
    } catch (const std::exception& e) {
        result.success = false;
        result.error_message = e.what();
    }

    return result;
}

// =============================================================================
// Synthesize with Streaming
// =============================================================================

SynthesisResult TTSEngine::synthesize_streaming(
    const std::string& text,
    std::function<void(const AudioBuffer&)> callback,
    uint32_t chunk_ms) {

    SynthesisResult result;
    begin_utterance();

    if (!is_initialized()) {
        result.success = false;
        result.error_message = "Engine not initialized";
        return result;
    }

    if (!callback) {
        result.success = false;
        result.error_message = "No callback provided";
        return result;
    }

    if (text.empty()) {
        result.success = true;
        return result;
    }

    std::string words, letter;
    if (split_capital_announcement(text, words, letter)) {
        // Short: synthesized whole, then handed over in chunks
        SynthesisResult whole = synthesize_capital(words, letter);
        if (!whole.success) return whole;
        const size_t chunk_size = chunk_ms > 0 ? (SAMPLE_RATE * chunk_ms) / 1000
                                               : whole.audio.samples.size();
        try {
            for (size_t pos = 0; pos < whole.audio.samples.size(); pos += chunk_size) {
                AudioBuffer chunk;
                chunk.sample_rate = whole.audio.sample_rate;
                chunk.bits_per_sample = whole.audio.bits_per_sample;
                chunk.channels = whole.audio.channels;
                const size_t end = std::min(pos + chunk_size, whole.audio.samples.size());
                chunk.samples.assign(whole.audio.samples.begin() + static_cast<std::ptrdiff_t>(pos),
                                     whole.audio.samples.begin() + static_cast<std::ptrdiff_t>(end));
                callback(chunk);
            }
        } catch (const std::exception& e) {
            result.success = false;
            result.error_message = e.what();
            return result;
        } catch (...) {
            result.success = false;
            result.error_message = "Unknown error during synthesis";
            return result;
        }
        result.success = true;
        return result;
    }

    auto set_callback = [&](std::function<void(const AudioBuffer&)> cb) {
        m_impl->stream_chunk_samples = cb ? (SAMPLE_RATE * chunk_ms) / 1000 : 0;
        m_impl->stream_callback = std::move(cb);
    };

    try {
        // Set up streaming callback
        set_callback(callback);

        // Step 1: Preprocess text
        std::string processed = preprocess_text(text);

        // Step 2: Segment text
        std::vector<TextSegment> segments = segment_text(processed);

        // Step 3: Synthesize (will stream via callback)
        result.audio = synthesize_segments(segments);

        // Clear callback
        set_callback(nullptr);

        result.success = true;
    } catch (const std::exception& e) {
        set_callback(nullptr);
        result.success = false;
        result.error_message = e.what();
    } catch (...) {
        // Whatever the caller's callback threw, it must not stay installed.
        set_callback(nullptr);
        result.success = false;
        result.error_message = "Unknown error during synthesis";
    }

    return result;
}

// =============================================================================
// Voice Parameters
// =============================================================================

void TTSEngine::set_voice_params(const VoiceParams& params) {
    if (m_impl) {
        m_impl->voice_params = params;
        m_impl->voice_params.clamp();

        // Sync emoji dictionary enabled state with voice params
        // This ensures emoji replacement works when enabled via set_voice_params()
        m_impl->emoji_dictionary.set_enabled(m_impl->voice_params.emoji_enabled);

        if (m_impl->synthesizer) {
            m_impl->synthesizer->set_voice_params(m_impl->voice_params);
        }
    }
}

VoiceParams TTSEngine::voice_params() const {
    if (m_impl) {
        return m_impl->voice_params;
    }
    return VoiceParams{};
}

// =============================================================================
// Utility Functions
// =============================================================================

const char* TTSEngine::version() {
    return LAPRDUS_VERSION_STRING;
}

uint32_t TTSEngine::sample_rate() const {
    if (m_impl && !m_impl->formant && m_impl->phoneme_data.is_loaded()) {
        return m_impl->phoneme_data.sample_rate();
    }
    return SAMPLE_RATE;
}

size_t TTSEngine::memory_usage() const {
    if (m_impl) {
        return m_impl->phoneme_data.memory_usage();
    }
    return 0;
}

// =============================================================================
// Preprocess Text
// =============================================================================

std::string TTSEngine::preprocess_text(const std::string& text) {
    std::string result = text;

    // Step 1: Apply emoji dictionary (if enabled)
    if (m_impl->voice_params.emoji_enabled && !m_impl->emoji_dictionary.empty()) {
        result = m_impl->emoji_dictionary.replace_emojis(result);
    }

    // Step 2: Apply pronunciation dictionary (word-level replacements)
    if (!m_impl->dictionary.empty()) {
        result = m_impl->dictionary.apply(result);
    }

    // Step 3: Process numbers based on mode
    if (m_impl->voice_params.number_mode == NumberMode::WholeNumbers) {
        // Expand numbers to words (default behavior)
        result = m_impl->number_converter.convert_numbers_in_text(result);
    } else {
        // Digit-by-digit mode - convert each digit to its word form separately
        result = m_impl->number_converter.convert_digits_in_text(result);
    }

    return result;
}

// =============================================================================
// Segment Text
// =============================================================================

std::vector<TextSegment> TTSEngine::segment_text(const std::string& processed_text) {
    // Use inflection processor to analyze and segment text
    return m_impl->inflection.analyze_text(processed_text);
}

// =============================================================================
// Synthesize Segments
// =============================================================================

AudioBuffer TTSEngine::synthesize_segments(const std::vector<TextSegment>& segments) {
    AudioBuffer result;
    result.sample_rate = SAMPLE_RATE;
    result.bits_per_sample = BITS_PER_SAMPLE;
    result.channels = NUM_CHANNELS;

    // A clause after a comma, semicolon or colon goes on the sentence and
    // starts lower than one that opens a sentence.
    bool sentence_initial = true;
    for (const auto& segment : segments) {
        if (segment.text.empty()) {
            continue;
        }

        // The clause is synthesized by rule (formant voice) or from the
        // recordings (PSOLA); rate, pitch, volume and intonation are all
        // applied at the source by either.
        AudioBuffer clause;
        if (m_impl->formant) {
            clause = m_impl->formant->synthesize_clause(
                segment.text, segment.trailing_punct, m_impl->voice_params, sentence_initial);
        } else if (m_impl->synthesizer) {
            clause = m_impl->synthesizer->synthesize_clause(segment.text, segment.trailing_punct,
                                                            sentence_initial);
        }
        sentence_initial = segment.trailing_punct != Punctuation::COMMA &&
                           segment.trailing_punct != Punctuation::SEMICOLON &&
                           segment.trailing_punct != Punctuation::COLON;
        if (clause.empty()) {
            continue;
        }
        if (segment.trailing_punct != Punctuation::NONE) {
            clause.append_silence(
                m_impl->inflection.get_pause_duration(segment.trailing_punct));
        }

        if (m_impl->stream_callback) {
            // Streaming: hand the clause over in chunks, keep nothing.
            // A chunk size of 0 means "do not split".
            const size_t chunk_size = m_impl->stream_chunk_samples > 0
                ? m_impl->stream_chunk_samples
                : clause.samples.size();
            for (size_t pos = 0; pos < clause.samples.size(); pos += chunk_size) {
                AudioBuffer chunk;
                chunk.sample_rate = clause.sample_rate;
                chunk.bits_per_sample = clause.bits_per_sample;
                chunk.channels = clause.channels;
                size_t end = std::min(pos + chunk_size, clause.samples.size());
                chunk.samples.assign(clause.samples.begin() + static_cast<std::ptrdiff_t>(pos),
                                     clause.samples.begin() + static_cast<std::ptrdiff_t>(end));
                m_impl->stream_callback(chunk);
            }
        } else {
            result.append(clause);
        }
    }

    return result;
}

// =============================================================================
// Pronunciation Dictionary
// =============================================================================

bool TTSEngine::load_dictionary(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    return m_impl->dictionary.load_from_file(path);
}

bool TTSEngine::load_dictionary_from_memory(const char* json_content, size_t length) {
    if (!m_impl) {
        return false;
    }
    return m_impl->dictionary.load_from_memory(json_content, length);
}

bool TTSEngine::append_dictionary(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    return m_impl->dictionary.append_from_file(path);
}

void TTSEngine::add_pronunciation(const std::string& grapheme, const std::string& phoneme,
                                  bool case_sensitive, bool whole_word) {
    if (m_impl) {
        m_impl->dictionary.add_entry(DictionaryEntry(grapheme, phoneme, case_sensitive, whole_word));
    }
}

void TTSEngine::add_spelling_entry(const std::string& character, const std::string& pronunciation) {
    if (m_impl) {
        m_impl->user_spelling_dictionary.add_entry(character, pronunciation);
    }
}

void TTSEngine::add_emoji_entry(const std::string& emoji, const std::string& text) {
    if (m_impl) {
        m_impl->emoji_dictionary.add_entry(emoji, text);
    }
}

void TTSEngine::clear_dictionary() {
    if (m_impl) {
        m_impl->dictionary.clear();
    }
}

// =============================================================================
// Spelling Dictionary
// =============================================================================

bool TTSEngine::load_spelling_dictionary(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    m_impl->user_spelling_dictionary.clear();
    return m_impl->spelling_dictionary.load_from_file(path);
}

bool TTSEngine::load_spelling_dictionary_from_memory(const char* json_content, size_t length) {
    if (!m_impl) {
        return false;
    }
    m_impl->user_spelling_dictionary.clear();
    return m_impl->spelling_dictionary.load_from_memory(json_content, length);
}

bool TTSEngine::append_spelling_dictionary(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    return m_impl->user_spelling_dictionary.append_from_file(path);
}

void TTSEngine::clear_spelling_dictionary() {
    if (m_impl) {
        m_impl->spelling_dictionary.clear();
        m_impl->user_spelling_dictionary.clear();
    }
}

void TTSEngine::set_spelling_mode(SpellingMode mode) {
    if (m_impl) {
        m_impl->voice_params.spelling_mode =
            mode == SpellingMode::LetterSounds ? SpellingMode::LetterSounds : SpellingMode::LetterNames;
    }
}

SpellingMode TTSEngine::spelling_mode() const {
    return m_impl ? m_impl->voice_params.spelling_mode : SpellingMode::LetterNames;
}

void TTSEngine::set_spelling_speed(int percent) {
    if (m_impl) {
        m_impl->voice_params.spelling_speed =
            std::clamp(percent, SPELLING_SPEED_MIN, SPELLING_SPEED_MAX);
    }
}

int TTSEngine::spelling_speed() const {
    return m_impl ? m_impl->voice_params.spelling_speed : SPELLING_SPEED_DEFAULT;
}

namespace {

// Spelled characters are rendered at a fraction of the speech rate: the
// speed is scaled for the duration of one character and put back after.
class SpellingRate {
public:
    SpellingRate(VoiceParams& params, AudioSynthesizer* synthesizer)
        : m_params(params), m_synthesizer(synthesizer), m_speed(params.speed) {
        m_params.speed = std::max(m_speed * spelling_rate_factor(m_params.spelling_speed),
                                  FORMANT_SPEED_MIN);
        if (m_synthesizer) m_synthesizer->set_voice_params(m_params);
    }
    ~SpellingRate() {
        m_params.speed = m_speed;
        if (m_synthesizer) m_synthesizer->set_voice_params(m_params);
    }
    SpellingRate(const SpellingRate&) = delete;
    SpellingRate& operator=(const SpellingRate&) = delete;

private:
    VoiceParams& m_params;
    AudioSynthesizer* m_synthesizer;
    float m_speed;
};

} // namespace

VoiceLanguage TTSEngine::current_language() const {
    if (m_impl && m_impl->formant) return m_impl->formant->language();
    return m_impl ? m_impl->concat_language : VoiceLanguage::Croatian;
}

SynthesisResult TTSEngine::spell_character(const std::string& character) {
    const std::u32string cps = PhonemeMapper::utf8_to_utf32(character);
    const std::u32string letter = cps.size() == 1 ? formant::spelling_letter(cps[0])
                                                   : std::u32string();
    const std::string* user = m_impl->user_spelling_dictionary.find(character);
    // q, w, x and y are not letters of the language: named in both modes
    const bool sounds = m_impl->voice_params.spelling_mode == SpellingMode::LetterSounds &&
                        !formant::is_foreign_letter(letter);

    std::string text;
    bool sound = false;
    if (!letter.empty()) {
        if (sounds) {
            // The user's entry wins unless it is just a letter's name
            if (user && !formant::is_letter_name(letter, PhonemeMapper::utf8_to_utf32(*user))) {
                text = *user;
            } else {
                sound = true;
            }
        } else if (user) {
            text = *user;
        } else {
            text = PhonemeMapper::utf32_to_utf8(formant::letter_name(letter, current_language()));
        }
    } else if (user) {
        text = *user;
    } else if (const std::string* bundled = m_impl->spelling_dictionary.find(character)) {
        text = *bundled;
    } else {
        text = character;
    }

    SpellingRate rate(m_impl->voice_params, m_impl->synthesizer.get());
    if (!sound) {
        return synthesize(text);
    }

    SynthesisResult result;
    begin_utterance();
    if (m_impl->formant) {
        result.audio = m_impl->formant->synthesize_letter_sound(letter, m_impl->voice_params);
    } else if (m_impl->synthesizer) {
        result.audio = m_impl->synthesizer->synthesize_letter_sound(letter);
    }
    result.success = true;
    return result;
}

SynthesisResult TTSEngine::synthesize_spelled(const std::string& text) {
    SynthesisResult result;
    result.success = false;
    begin_utterance();

    if (!m_impl || !m_impl->initialized) {
        result.error_message = "Engine not initialized";
        return result;
    }

    result.audio.sample_rate = SAMPLE_RATE;
    result.audio.bits_per_sample = BITS_PER_SAMPLE;
    result.audio.channels = NUM_CHANNELS;
    if (text.empty()) {
        result.success = true;
        return result;
    }

    // The pause between the characters; a lone character gets it after
    // itself, for the spacing between one spell call and the next.
    const uint32_t spelling_pause_ms = m_impl->voice_params.pause_settings.spelling_pause_ms;
    const size_t pause_samples = static_cast<size_t>(SAMPLE_RATE * spelling_pause_ms / 1000);

    size_t pos = 0;
    size_t count = 0;
    while (pos < text.size()) {
        unsigned char c = static_cast<unsigned char>(text[pos]);
        size_t char_len = 1;
        if ((c & 0xE0) == 0xC0) char_len = 2;
        else if ((c & 0xF0) == 0xE0) char_len = 3;
        else if ((c & 0xF8) == 0xF0) char_len = 4;
        if (pos + char_len > text.size()) break;
        std::string character = text.substr(pos, char_len);
        pos += char_len;

        SynthesisResult char_result = spell_character(character);
        if (!char_result.success) {
            continue;   // skip a character that could not be synthesized
        }
        if (count > 0) {
            result.audio.samples.insert(result.audio.samples.end(), pause_samples, 0);
        }
        result.audio.samples.insert(result.audio.samples.end(),
                                    char_result.audio.samples.begin(),
                                    char_result.audio.samples.end());
        ++count;
    }
    if (count == 1) {
        result.audio.samples.insert(result.audio.samples.end(), pause_samples, 0);
    }

    result.success = !result.audio.samples.empty();
    return result;
}

// =============================================================================
// Emoji Dictionary
// =============================================================================

bool TTSEngine::load_emoji_dictionary(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    return m_impl->emoji_dictionary.load_from_file(path);
}

bool TTSEngine::load_emoji_dictionary_from_memory(const char* json_content, size_t length) {
    if (!m_impl) {
        return false;
    }
    return m_impl->emoji_dictionary.load_from_memory(json_content, length);
}

bool TTSEngine::append_emoji_dictionary(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    return m_impl->emoji_dictionary.append_from_file(path);
}

void TTSEngine::clear_emoji_dictionary() {
    if (m_impl) {
        m_impl->emoji_dictionary.clear();
    }
}

// =============================================================================
// Accent Lexicon
// =============================================================================

bool TTSEngine::load_accent_lexicon(const std::string& path) {
    if (!m_impl) {
        return false;
    }
    std::ifstream file(path, std::ios::binary);
    if (!file) {
        m_impl->user_lexicon_report = "cannot open " + path;
        return false;
    }
    std::string json((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    return load_accent_lexicon_from_memory(json.data(), json.size());
}

bool TTSEngine::load_accent_lexicon_from_memory(const char* json_content, size_t length) {
    if (!m_impl || !json_content) {
        return false;
    }
    std::string json = length > 0 ? std::string(json_content, length) : std::string(json_content);
    formant::UserLexicon::Report report;
    std::shared_ptr<const formant::UserLexicon> lexicon = formant::UserLexicon::parse(json, &report);

    m_impl->user_lexicon_report = std::to_string(report.words) + " words, " +
                                  std::to_string(report.verbs) + " verbs";
    if (report.rejected > 0) {
        m_impl->user_lexicon_report += "; " + std::to_string(report.rejected) +
                                       " rejected (first: " + report.first_error + ")";
    }

    m_impl->user_lexicon = lexicon->empty() ? nullptr : lexicon;
    if (m_impl->synthesizer) {
        m_impl->synthesizer->set_user_lexicon(m_impl->user_lexicon);
    }
    if (m_impl->formant) {
        m_impl->formant->set_user_lexicon(m_impl->user_lexicon);
    }
    return !lexicon->empty();
}

void TTSEngine::clear_accent_lexicon() {
    if (!m_impl) {
        return;
    }
    m_impl->user_lexicon.reset();
    m_impl->user_lexicon_report.clear();
    if (m_impl->formant) {
        m_impl->formant->set_user_lexicon(nullptr);
    }
    if (m_impl->synthesizer) {
        m_impl->synthesizer->set_user_lexicon(nullptr);
    }
}

std::string TTSEngine::accent_lexicon_report() const {
    return m_impl ? m_impl->user_lexicon_report : std::string();
}

void TTSEngine::set_emoji_enabled(bool enabled) {
    if (m_impl) {
        m_impl->voice_params.emoji_enabled = enabled;
        m_impl->emoji_dictionary.set_enabled(enabled);
    }
}

bool TTSEngine::is_emoji_enabled() const {
    if (m_impl) {
        return m_impl->voice_params.emoji_enabled;
    }
    return false;
}

// =============================================================================
// Pause Settings
// =============================================================================

void TTSEngine::set_pause_settings(const PauseSettings& settings) {
    if (m_impl) {
        m_impl->voice_params.pause_settings = settings;
        m_impl->voice_params.pause_settings.clamp();
        // Update inflection processor with new pause settings
        m_impl->inflection.set_pause_settings(m_impl->voice_params.pause_settings);
    }
}

PauseSettings TTSEngine::pause_settings() const {
    if (m_impl) {
        return m_impl->voice_params.pause_settings;
    }
    return PauseSettings{};
}

void TTSEngine::set_sentence_pause(uint32_t pause_ms) {
    if (m_impl) {
        m_impl->voice_params.pause_settings.sentence_pause_ms = std::min(pause_ms, 2000u);
        m_impl->inflection.set_pause_settings(m_impl->voice_params.pause_settings);
    }
}

void TTSEngine::set_comma_pause(uint32_t pause_ms) {
    if (m_impl) {
        m_impl->voice_params.pause_settings.comma_pause_ms = std::min(pause_ms, 2000u);
        m_impl->inflection.set_pause_settings(m_impl->voice_params.pause_settings);
    }
}

void TTSEngine::set_newline_pause(uint32_t pause_ms) {
    if (m_impl) {
        m_impl->voice_params.pause_settings.newline_pause_ms = std::min(pause_ms, 2000u);
        m_impl->inflection.set_pause_settings(m_impl->voice_params.pause_settings);
    }
}

void TTSEngine::set_spelling_pause(uint32_t pause_ms) {
    if (m_impl) {
        m_impl->voice_params.pause_settings.spelling_pause_ms = std::min(pause_ms, 2000u);
    }
}

uint32_t TTSEngine::spelling_pause() const {
    if (m_impl) {
        return m_impl->voice_params.pause_settings.spelling_pause_ms;
    }
    return 200;  // Default
}

// =============================================================================
// Number Processing Mode
// =============================================================================

void TTSEngine::set_number_mode(NumberMode mode) {
    if (m_impl) {
        m_impl->voice_params.number_mode = mode;
    }
}

NumberMode TTSEngine::number_mode() const {
    if (m_impl) {
        return m_impl->voice_params.number_mode;
    }
    return NumberMode::WholeNumbers;
}

} // namespace laprdus
