// -*- coding: utf-8 -*-
// inflection.hpp - Clause segmentation at punctuation and pause settings

#ifndef LAPRDUS_INFLECTION_HPP
#define LAPRDUS_INFLECTION_HPP

#include "laprdus/types.hpp"
#include <vector>
#include <string>
#include <string_view>

namespace laprdus {

/**
 * InflectionProcessor - Splits text into clauses at punctuation and keeps
 * the pause settings.
 *
 * The pitch contour itself is drawn per clause by the intonation model in
 * src/formant/formant_intonation.* for both kinds of voices.
 */
class InflectionProcessor {
public:
    InflectionProcessor();
    ~InflectionProcessor() = default;

    /**
     * Set pause settings for sentences, commas, and newlines.
     * @param settings Pause settings structure.
     */
    void set_pause_settings(const PauseSettings& settings);

    /**
     * Get current pause settings.
     * @return Current pause settings.
     */
    PauseSettings pause_settings() const;

    /**
     * Analyze text and detect inflection points.
     * @param text UTF-8 text to analyze.
     * @return Vector of text segments with inflection markers.
     */
    std::vector<TextSegment> analyze_text(const std::string& text);

    /**
     * Get pause duration for punctuation type.
     * Uses custom pause settings if set.
     * @param punct Punctuation type.
     * @return Pause duration in milliseconds.
     */
    uint32_t get_pause_duration(Punctuation punct) const;

    /**
     * Get default pause duration for punctuation type (static version).
     * @param punct Punctuation type.
     * @return Default pause duration in milliseconds.
     */
    static uint32_t get_default_pause_duration(Punctuation punct);

    /**
     * Detect inflection type from punctuation.
     * @param punct Punctuation type.
     * @return Corresponding inflection type.
     */
    static InflectionType punct_to_inflection(Punctuation punct);

private:
    PauseSettings m_pause_settings;
};

} // namespace laprdus

#endif // LAPRDUS_INFLECTION_HPP
