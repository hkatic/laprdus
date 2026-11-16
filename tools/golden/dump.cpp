// dump.cpp - what the C++ engine makes of the golden corpus below the C API:
// the phones of the front end, the audio of the formant synthesizer for
// exactly those phones, and the text of the number converter. Temporary: it
// goes with the C++ sources once the Rust engine reproduces all of it.
//
//   dump <corpus.txt> <frontend.tsv> <numbers.tsv> [<recorded.tsv> <voice data dir>]
//
// corpus.txt has one text per line with \n, \t, \r and \\ escaped
// (tools/golden/corpus.py --plain writes it from tests/golden/corpus.jsonl).

#include "audio/audio_synthesizer.hpp"
#include "audio/phoneme_data.hpp"
#include "core/croatian_numbers.hpp"
#include "core/phoneme_mapper.hpp"
#include "formant/formant_frontend.hpp"
#include "formant/formant_synthesizer.hpp"
#include <cstdint>
#include <cstdio>
#include <fstream>
#include <string>
#include <vector>

using namespace laprdus;
using namespace laprdus::formant;

static std::string unescape(const std::string& line) {
    std::string out;
    for (size_t i = 0; i < line.size(); ++i) {
        if (line[i] == '\\' && i + 1 < line.size()) {
            const char c = line[++i];
            out += c == 'n' ? '\n' : c == 't' ? '\t' : c == 'r' ? '\r' : c;
        } else {
            out += line[i];
        }
    }
    return out;
}

static std::string escape(const std::string& text) {
    std::string out;
    for (char c : text) {
        if (c == '\n') out += "\\n";
        else if (c == '\t') out += "\\t";
        else if (c == '\r') out += "\\r";
        else if (c == '\\') out += "\\\\";
        else out += c;
    }
    return out;
}

static uint64_t fnv(const void* data, size_t size) {
    const unsigned char* p = static_cast<const unsigned char*>(data);
    uint64_t h = 1469598103934665603ull;
    for (size_t i = 0; i < size; ++i) h = (h ^ p[i]) * 1099511628211ull;
    return h;
}

int main(int argc, char** argv) {
    if (argc < 4) return 2;
    std::vector<std::string> texts;
    {
        std::ifstream in(argv[1], std::ios::binary);
        std::string line;
        while (std::getline(in, line)) texts.push_back(unescape(line));
    }
    static const char* const LANGS[] = {"hr", "sr", "bs", "sl"};

    std::FILE* out = std::fopen(argv[2], "wb");
    const FormantVoice* zvonko = find_formant_voice("zvonko");
    for (int lang = 0; lang < 4; ++lang) {
        Frontend frontend(static_cast<VoiceLanguage>(lang));
        FormantSynthesizer synth(*zvonko);
        synth.set_language(static_cast<VoiceLanguage>(lang));
        for (size_t i = 0; i < texts.size(); ++i) {
            const std::u32string text = PhonemeMapper::utf8_to_utf32(texts[i]);
            for (int sounds = 1; sounds >= 0; --sounds) {
                const Utterance utt = frontend.process(text, Punctuation::PERIOD, sounds != 0);
                std::fprintf(out, "%s\t%d\t%zu\t%d %d %d %d %d\t", LANGS[lang], sounds, i + 1,
                             static_cast<int>(utt.kind), utt.syllable_count, utt.focus_word,
                             utt.isolated_sound ? 1 : 0, utt.slovenian ? 1 : 0);
                for (const Phone& p : utt.phones) {
                    const int flags = p.nucleus | p.nucleus_tail << 1 | p.stressed << 2 | p.is_long << 3 |
                                      p.short_glide << 4 | p.word_start << 5 | p.vowel_word << 6 |
                                      p.break_before << 7 | p.item_break << 8;
                    std::fprintf(out, "%d:%d:%d:%d:%d:%d:%d ", static_cast<int>(p.ph), flags,
                                 static_cast<int>(p.accent), p.prominence, p.word, p.syllable, p.word_syllables);
                }
                if (sounds) {
                    VoiceParams params;
                    const AudioBuffer audio = synth.synthesize_clause(text, Punctuation::PERIOD, params);
                    std::fprintf(out, "\t%zu\t%016llx", audio.samples.size(),
                                 static_cast<unsigned long long>(
                                     fnv(audio.samples.data(), audio.samples.size() * sizeof(AudioSample))));
                }
                std::fputc('\n', out);
            }
        }
    }
    std::fclose(out);

    // The recorded voices: Josip for the Croatian phones, Vlado for the Serbian
    if (argc >= 6) {
        out = std::fopen(argv[4], "wb");
        static const char* const FILES[] = {"Josip.bin", "Vlado.bin"};
        for (int lang = 0; lang < 2; ++lang) {
            PhonemeData data;
            if (!data.load_from_file(std::string(argv[5]) + "/" + FILES[lang])) return 3;
            AudioSynthesizer synth(data, static_cast<VoiceLanguage>(lang));
            for (size_t i = 0; i < texts.size(); ++i) {
                const AudioBuffer audio =
                    synth.synthesize_clause(PhonemeMapper::utf8_to_utf32(texts[i]), Punctuation::PERIOD, true, true);
                std::fprintf(out, "%s\t%zu\t%zu\t%016llx\n", LANGS[lang], i + 1, audio.samples.size(),
                             static_cast<unsigned long long>(
                                 fnv(audio.samples.data(), audio.samples.size() * sizeof(AudioSample))));
            }
        }
        std::fclose(out);
    }

    out = std::fopen(argv[3], "wb");
    for (int lang = 0; lang < 4; ++lang) {
        CroatianNumbers numbers;
        numbers.set_dialect(static_cast<CroatianNumbers::Dialect>(lang));
        for (size_t i = 0; i < texts.size(); ++i) {
            std::fprintf(out, "%s\tw\t%zu\t%s\n", LANGS[lang], i + 1,
                         escape(numbers.convert_numbers_in_text(texts[i])).c_str());
            std::fprintf(out, "%s\td\t%zu\t%s\n", LANGS[lang], i + 1,
                         escape(numbers.convert_digits_in_text(texts[i])).c_str());
        }
    }
    std::fclose(out);
    return 0;
}
