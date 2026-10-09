// english_stress.cpp - Helper of tools/formant/english_words.py.
//
// Reads one lowercase word per line from standard input and prints, per word,
// what the front end of the formant voices does with it, built with whatever
// src/formant/formant_english.inc holds (english_words.py builds it with an
// empty table):
//
//     word  shape  letters  hr  sr  bs
//
// shape is 1 for a word spelled the English way (english_spelling()),
// letters the letter of the spelling each syllable nucleus comes from
// (comma-separated), and hr, sr, bs the stressed nucleus and its accent
// (N, F, R) in each language, "-" for none.
//
// Built by english_words.py:
//     c++ -std=c++17 -I include -I src tools/formant/english_stress.cpp \
//         src/formant/formant_lexicon.cpp src/formant/formant_phonemes.cpp \
//         src/core/phoneme_mapper.cpp -o english_stress

#include "formant/formant_frontend.cpp"
#include <iostream>

using namespace laprdus;
using namespace laprdus::formant;

int main() {
    const VoiceLanguage languages[] = {
        VoiceLanguage::Croatian, VoiceLanguage::Serbian, VoiceLanguage::Bosnian,
    };
    std::vector<Frontend> frontends;
    for (VoiceLanguage language : languages) frontends.emplace_back(language);

    std::string line;
    while (std::getline(std::cin, line)) {
        if (line.empty()) continue;
        const std::u32string text = PhonemeMapper::utf8_to_utf32(line);
        Word word(text);
        phonemize(word);
        std::cout << line << '\t' << (english_spelling(word.spelled) ? 1 : 0) << '\t';
        for (size_t k = 0; k < word.nuclei.size(); ++k) {
            const int letter = word.letter[static_cast<size_t>(word.nuclei[k])];
            std::cout << (k ? "," : "") << word.origin[static_cast<size_t>(letter)];
        }
        for (const Frontend& frontend : frontends) {
            const Utterance utt = frontend.process(text, Punctuation::PERIOD);
            int syllable = -1;
            char accent = '-';
            for (const Phone& phone : utt.phones) {
                if (phone.nucleus && phone.stressed && phone.word == 0) {
                    syllable = phone.syllable;
                    accent = phone.accent == Accent::Falling ? 'F'
                           : phone.accent == Accent::Rising ? 'R' : 'N';
                    break;
                }
            }
            const bool one_word = !utt.phones.empty() && utt.phones.back().word == 0;
            std::cout << '\t' << (one_word && syllable >= 0 ? std::to_string(syllable) : "-")
                      << accent;
        }
        std::cout << '\n';
    }
    return 0;
}
