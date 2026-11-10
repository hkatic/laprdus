// -*- coding: utf-8 -*-
// text_marks.hpp - Marks the number converter leaves in the text for the
// front end and the synthesizers
//
// Both are Unicode noncharacters (U+FDD0..U+FDEF are set aside for a
// program's internal use and are not interchanged), so no text a host sends
// has them: a private use character can come from an icon font in the label
// of a button. Both sit between two spaces or after a word, and the clause
// splitter (InflectionProcessor) passes them on as it does any letter.
//
// The engine's other marks, the Slovenian ones of formant_slovenian.hpp, are
// U+E000 to U+E003 and stand at the end of a clause.

#ifndef LAPRDUS_TEXT_MARKS_HPP
#define LAPRDUS_TEXT_MARKS_HPP

namespace laprdus {

// A short pause inside a clause, before the next word: between the hours,
// minutes and seconds of a clock time (12:30:45), between the day, month and
// year of a numeric date (10.11.2026.) and before the address of a spreadsheet
// cell that follows the cell's contents (Marjan A17). The melody of the
// sentence goes on over it; a comma would end the clause there ("u dvanaest,
// trideset u uredu"). Written as a word of its own, " <mark> ".
constexpr char32_t MINOR_BREAK_MARK = 0xFDD0;
constexpr char MINOR_BREAK_UTF8[] = "\xEF\xB7\x90";

// Directly after the letters of a designation written with capital letters
// glued to a number (A17, F5, S22, AB12): they are letters, read by their
// names or sounds as the spelling mode says, never the words they are spelled
// like (the conjunctions a and i, the prepositions u, o, s and k, the words
// na, do, on).
constexpr char32_t LETTER_CODE_MARK = 0xFDD1;
constexpr char LETTER_CODE_UTF8[] = "\xEF\xB7\x91";

} // namespace laprdus

#endif // LAPRDUS_TEXT_MARKS_HPP
