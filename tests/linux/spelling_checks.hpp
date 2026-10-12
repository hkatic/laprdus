#pragma once

#include <algorithm>
#include <vector>
#include <laprdus/laprdus_api.h>

// Shared PCM regression checks for formant and recorded voices. In particular,
// a longer buffer alone must not let slowed-down speech pass as a longer pause.
inline void check_spelling_pauses(LaprdusHandle engine) {
    auto render = [&](const char* text, bool spelled) {
        int16_t* samples = nullptr;
        LaprdusAudioFormat format;
        const int32_t count = spelled
            ? laprdus_synthesize_spelled(engine, text, &samples, &format)
            : laprdus_synthesize(engine, text, &samples, &format);
        REQUIRE(count > 0);
        std::vector<int16_t> audio(samples, samples + count);
        laprdus_free_buffer(samples);
        return audio;
    };
    REQUIRE(laprdus_add_spelling_entry(engine, "*", "moja zvjezdica") == LAPRDUS_OK);
    REQUIRE(laprdus_set_acceleration(engine, 1.7f) == LAPRDUS_OK);
    REQUIRE(laprdus_set_user_pitch(engine, 1.2f) == LAPRDUS_OK);

    for (auto mode : {LAPRDUS_SPELLING_LETTER_NAMES, LAPRDUS_SPELLING_LETTER_SOUNDS}) {
        REQUIRE(laprdus_set_spelling_mode(engine, mode) == LAPRDUS_OK);
        for (float rate : {0.25f, 1.0f, 4.0f}) {
            REQUIRE(laprdus_set_speed(engine, rate) == LAPRDUS_OK);
            const auto normal = render("Običan govor.", false);
            for (const char* character : {"b", u8"Č", "7", "*"}) {
                INFO("mode " << mode << ", rate " << rate << ", character " << character);
                REQUIRE(laprdus_set_spelling_pause(engine, 0) == LAPRDUS_OK);
                REQUIRE(laprdus_set_spelling_speed(engine, 100) == LAPRDUS_OK);
                const auto full = render(character, true);
                if (mode == LAPRDUS_SPELLING_LETTER_NAMES && character[0] == 'b') {
                    REQUIRE(full == render("be", false));
                }
                size_t previous_size = full.size();
                for (int speed : {50, 0}) {
                    REQUIRE(laprdus_set_spelling_speed(engine, speed) == LAPRDUS_OK);
                    const auto spaced = render(character, true);
                    REQUIRE(spaced.size() > previous_size);
                    REQUIRE(std::equal(full.begin(), full.end(), spaced.begin()));
                    REQUIRE(std::all_of(spaced.begin() + full.size(), spaced.end(),
                                        [](int16_t sample) { return sample == 0; }));
                    previous_size = spaced.size();
                }
            }
            // The configured pause adds to the speed-dependent silence after
            // every character, including the last. UTF-8 letters count once.
            for (int speed : {100, 50, 0}) {
                REQUIRE(laprdus_set_spelling_speed(engine, speed) == LAPRDUS_OK);
                REQUIRE(laprdus_set_spelling_pause(engine, 0) == LAPRDUS_OK);
                const auto no_base_pause = render(u8"bČ7*", true);
                REQUIRE(laprdus_set_spelling_pause(engine, 200) == LAPRDUS_OK);
                const auto together = render(u8"bČ7*", true);
                std::vector<int16_t> separate;
                for (const char* character : {"b", u8"Č", "7", "*"}) {
                    const auto audio = render(character, true);
                    separate.insert(separate.end(), audio.begin(), audio.end());
                }
                REQUIRE(together == separate);
                REQUIRE(together.size() == no_base_pause.size() + 4 * 4410);
                auto capital = render("veliko", false);
                const auto letter = render("N", true);
                capital.insert(capital.end(), letter.begin(), letter.end());
                REQUIRE(render("veliko N", false) == capital);
            }
            REQUIRE(render("Običan govor.", false) == normal);
        }
    }
}
