//! Slovenian, as the engine asks for it: the front end with the user's
//! accent entries, the number words and the hooks of the number converter.

use super::frontend::{self, Slovenian as Words};
use super::{marks, numerals, words};
use crate::languages::{LanguageFrontend, Rules};
use crate::prosody::Utterance;
use crate::text::Punctuation;
use crate::text::clauses::Clause;
use crate::text::frontend::{LanguageWords, UserLexicon};
use crate::text::numbers::cardinal::{Numerals, ScaleAgreement};
use crate::text::numbers::scan::{Out, Text, Unit, join_parts};
use crate::text::numbers::{Grammar, NumberReader};
use std::sync::Arc;

/// Slovenian, as the engine asks for it.
pub struct Slovenian;

impl Rules for Slovenian {
    fn frontend(&self) -> Box<dyn LanguageFrontend> {
        Box::new(Frontend { user: None })
    }

    fn words(&self) -> &'static dyn LanguageWords {
        &Words
    }

    fn numbers(&self) -> &'static dyn Grammar {
        &Numbers
    }

    fn spelled_name(&self, character: &str) -> Option<&'static str> {
        words::spelled_name(character)
    }

    fn mark_clauses(&self, clauses: &mut [Clause]) {
        marks::mark_clauses(clauses);
    }

    fn check_verb_entry(&self, _entry: &[char]) -> Result<(), &'static str> {
        Err("Slovenian has word entries only")
    }
}

/// The Slovenian front end with the user's accent lexicon.
struct Frontend {
    /// The user's lexicon as given, for its Slovenian entries.
    user: Option<Arc<UserLexicon>>,
}

impl LanguageFrontend for Frontend {
    fn set_user_lexicon(&mut self, lexicon: Option<Arc<UserLexicon>>) {
        self.user = lexicon;
    }
}

impl crate::text::Frontend for Frontend {
    fn process(&self, text: &[char], punct: Punctuation, letter_sounds: bool) -> Utterance {
        // The user's entries for Slovenian, when there are any
        let user = self.user.as_deref().map(|user| &user.slovenian).filter(|lexicon| !lexicon.is_empty());
        frontend::process(text, punct, letter_sounds, user)
    }

    fn letter_sound(&self, letter: &[char]) -> Utterance {
        frontend::letter_sound(letter)
    }
}

/// The Slovenian number words (Slovenski pravopis 2001, § 11 and § 536):
/// nič, ena, dve, tri, štiri, pet, šest, sedem, osem, devet; enajst, dvajset;
/// the ones before the tens as one word (enaindvajset, štiriinštirideset);
/// dvesto, štiristo; the invariable tisoč (dva tisoč, pet tisoč, tristo en
/// tisoč); milijon with the dual (dva milijona), nominative plural (trije
/// milijoni, štirje milijoni) and genitive plural (pet milijonov,
/// enaindvajset milijonov); milijarda, dve milijardi, tri milijarde, pet
/// milijard.
static NUMERALS: Numerals = Numerals {
    // The word of a digit inside a longer number: dva tisoč, sto dva. The
    // number 2 itself is dve. en/ena and dva/dve agree with a scale word, and
    // the compounds have dva- (dvaindvajset)
    digits: ["nič", "ena", "dva", "tri", "štiri", "pet", "šest", "sedem", "osem", "devet"],
    // SP 2001, § 11 and SSKJ² give dve as the name of the number 2 (ena in
    // ena je dve), also in a phone number (Jezikovna svetovalnica, "Uporaba
    // dva in dve", 2014)
    two_alone: "dve",
    teens: [
        "",
        "enajst",
        "dvanajst",
        "trinajst",
        "štirinajst",
        "petnajst",
        "šestnajst",
        "sedemnajst",
        "osemnajst",
        "devetnajst",
    ],
    tens: [
        "",
        "deset",
        "dvajset",
        "trideset",
        "štirideset",
        "petdeset",
        "šestdeset",
        "sedemdeset",
        "osemdeset",
        "devetdeset",
    ],
    hundreds: ["", "sto", "dvesto", "tristo", "štiristo", "petsto", "šeststo", "sedemsto", "osemsto", "devetsto"],
    // tisoč does not change: tisoč, dva tisoč, pet tisoč
    thousand: ["tisoč"; 4],
    // en milijon, dva milijona (dual), trije/štirje milijoni, pet milijonov,
    // and the genitive plural of every compound (enaindvajset milijonov)
    million: ["lijon", "lijona", "lijoni", "lijonov"],
    // ena milijarda, dve milijardi (dual), tri/štiri milijarde, pet milijard
    milliard: ["lijarda", "lijardi", "lijarde", "lijard"],
    feminine_thousand: false,
    // The unit of a compound numeral: enaindvajset, dvaindvajset,
    // triindvajset, štiriindvajset ... (Slovenski pravopis 2001, § 536)
    ones_first: true,
    // One to four agree with the scale word: en tisoč, dva tisoč, tri tisoč,
    // štiri tisoč; en milijon, dva milijona, trije milijoni, štirje
    // milijoni; ena milijarda, dve milijardi, tri milijarde, štiri milijarde
    scale_agreement: ScaleAgreement::UpToFour([
        ["en", "en", "ena"],
        ["dva", "dva", "dve"],
        ["tri", "trije", "tri"],
        ["štiri", "štirje", "štiri"],
    ]),
};

/// How Slovenian reads numbers, as the number converter asks.
struct Numbers;

impl Grammar for Numbers {
    fn numerals(&self) -> &'static Numerals {
        &NUMERALS
    }

    fn units(&self) -> &'static [Unit; 4] {
        &numerals::UNITS
    }

    fn home_country(&self) -> &'static str {
        "386"
    }

    /// The decimal comma read by name, where the number is not read as a
    /// decimal (tri cele štirinajst).
    fn decimal_comma(&self) -> &'static str {
        " vejica "
    }

    fn number_at(&self, r: &NumberReader, t: Text, i: usize, out: &mut Out) -> Option<usize> {
        numerals::number_at(r, t, i, out, false)
    }

    fn agree(&self, t: Text, start: usize, end: usize, words: &mut String) {
        numerals::agree(t, start, end, words);
    }

    fn ordinal_before(&self, r: &NumberReader, t: Text, start: usize, end: usize, after: usize) -> String {
        numerals::ordinal_before(r, t, start, end, after)
    }

    /// Slovenian reads a date in the genitive, which does not depend on the
    /// sentence: sedmega desetega dva tisoč šestindvajset (Jezikovna
    /// svetovalnica, ZRC SAZU: "šestega julija dva tisoč deset").
    fn date_to_words(&self, r: &NumberReader, groups: &[&str; 3]) -> String {
        join_parts([
            numerals::ordinal(r, groups[0], "ega"),
            numerals::ordinal(r, groups[1], "ega"),
            r.number_to_words(groups[2]),
        ])
    }

    /// The hour and the minute: dve trideset.
    fn clock_part(&self, part: &mut String) {
        numerals::feminine_two(part);
    }

    /// 132.457 is sto dvaintrideset tisoč štiristo sedeminpetdeset
    /// (Slovenski pravopis 2001, § 253).
    fn groups_thousands_by_period(&self) -> bool {
        true
    }

    /// tri cele štirinajst.
    fn names_decimals(&self) -> bool {
        true
    }

    fn currency_amount(&self, r: &NumberReader, t: Text, start: usize, whole: &str, unit: &Unit) -> String {
        let case = numerals::unit_case(t, &t.prev_word(start));
        numerals::count(r, whole, unit, "", case, false)
    }

    fn decimal(&self, r: &NumberReader, whole: &str, fraction: &str) -> String {
        numerals::decimal(r, whole, fraction)
    }
}
