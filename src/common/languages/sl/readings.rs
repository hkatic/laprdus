//! Slovenian spellings with more than one reading: names, the head of a
//! clause, the words around a word, verb or noun, compound numerals.
//!
//! Declarations only: the bodies are placeholders until the Slovenian rules
//! and the generated tables are ported from `formant_slovenian.cpp` and
//! `formant_slovenian.inc`. A reading ("pron") is a pronunciation in the
//! notation of [`super::lexicon::Lexicon`].

/// The words of one clause as the rules of readings see them.
#[derive(Clone, Copy, Debug)]
pub struct Clause<'a> {
    /// The clause's words, lowercase, foreign letters rewritten.
    pub words: &'a [Vec<char>],
    /// Which were written with a capital (not in text all in capitals, where
    /// only a single letter counts).
    pub capitalized: &'a [bool],
    /// Which were written all in capitals (JAWS) or are a letter of a
    /// spelled abbreviation (NVDA).
    pub acronym: &'a [bool],
    /// The clause after the comma opens with da, naj, kako ...
    /// ([`super::marks::BEFORE_COMPLEMENT_MARK`]): Meni, da je prav.
    pub before_complement: bool,
    /// The sentence ends in a question mark
    /// ([`super::marks::IN_QUESTION_MARK`]): Lahko delo opravi vsak? is the
    /// modal, Lahko delo opravi vsak. the adjective.
    pub question: bool,
    /// The sentence ends in an exclamation mark
    /// ([`super::marks::IN_EXCLAMATION_MARK`]): Gori na podstrešju! is the
    /// fire, not the adverb gôri of Gori na podstrešju je krama.
    pub exclamation: bool,
}

/// The reading of a word written with a capital letter that is a name (a
/// person, a place, a region) the lexicon reads otherwise, mostly because a
/// common word is spelled like it: Sóbota (Múrska Sóbota) and sobóta
/// "Saturday", Jêlena and jeléna "of the deer". From the hand-accented names
/// of Sloleks and the names of Slovenski pravopis 2001. `w` is lowercase;
/// `clause_head`: the word opens its clause (or follows an opening quote),
/// where any word has a capital, so only a name more frequent than the
/// common word is read. `None` when the lexicon's reading holds.
pub fn name_reading(_w: &[char], _clause_head: bool) -> Option<Vec<char>> {
    // TODO(sl)
    None
}

/// The reading of a surname in -e: after a given name (Vesna Smode) a
/// capitalized word in a consonant and -e the tables of names do not know is
/// stressed on its open final e (Smodè, Starè, as the hand-accented Peršè,
/// Lekšè, Peterlè, Šukljè of Sloleks), whatever common word it is spelled
/// like. Not one of the hand-accented exceptions (Lapájne), not spelled the
/// foreign way (Moore, Hesse). The list of given names leaves out names in -a
/// that are mostly a man's genitive (Miroslava Krleže). `w` and `previous` are
/// lowercase; `None` when the rule does not apply.
pub fn surname_reading(_w: &[char], _previous: &[char], _previous_capitalized: bool) -> Option<Vec<char>> {
    // TODO(sl)
    None
}

/// Whether a word (in small letters) is one of the given names of the
/// generated table (`tools/formant/slovenian_given_names.tsv`).
pub fn is_given_name(_w: &[char]) -> bool {
    // TODO(sl)
    false
}

/// The reading of a spelling at the head of a clause.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct HeadReading {
    pub pron: &'static str,
    /// A verb of saying (govori, odgovori, prosi): before a comma and a
    /// clause of da it is the present (Govori, da bo prišel).
    pub speech: bool,
}

/// The reading of a spelling at the head of a clause, where it is the
/// greeting or the imperative (žívijo, narêdi, govôri, and the commands of
/// the generated table of head imperatives: posodôbi, nadgrádi, namésti,
/// izvŕši); `None` for a word that reads the same everywhere. Elsewhere the
/// lexicon's reading holds (živíjo, naredí "they live", "he makes").
pub fn head_reading(_w: &[char]) -> Option<HeadReading> {
    // TODO(sl)
    None
}

/// The reading of word `i` that depends on the other words of its clause,
/// or `None` to keep the lexicon's. Meni is the menu, mení, alone in its
/// clause, after a preposition no pronoun meni follows (pod meni), after v
/// or na with a verb of motion (pojdi v meni), an adjective or a verb that
/// takes a menu (glavni meni, odpri meni) and before a capital (meni Start);
/// after another preposition or before se, je, ni the pronoun mêni;
/// elsewhere the lexicon's méni, the verb. Lepo, težko, slabo and the other
/// spellings of an adverb on -ó and an adjective on its stem are the adverb
/// at the end of the clause (lepó, težkó), the adjective before another word
/// (lépo, têžko). Lahko is the adjective láhko before a noun it qualifies
/// (lahko noč, z lahko nogo, moraš lahko hrano, daj mi lahko nalogo), else
/// the lexicon's modal lahkó. Roka is the genitive róka of the deadline
/// after a preposition or a genitive (do roka, vojaškega roka) or of the
/// name when no hand can be meant (Roka že ni, Roka nisem videl), else the
/// lexicon's hand, rôka; roki and roke are a deadline's by an adjective or a
/// plural verb (plačilni roki, roki so kratki). Afna is the verb's afná next
/// to se, else the lexicon's noun, áfna.
pub fn clause_reading(_clause: &Clause, _i: usize) -> Option<&'static str> {
    // TODO(sl)
    None
}

/// Whether the reading [`clause_reading`] gave is a content word although
/// its spelling is a function word's: the adjective láhko has a full accent.
pub fn clause_reading_is_content(_w: &[char], _reading: &str) -> bool {
    // TODO(sl)
    false
}

/// A spelling read one way as a verb's form and another way as a noun, an
/// adjective or an adverb (veselí "looks forward" and veséli "cheerful",
/// letí "flies" and léti "two years", vôzil "drove" and vozíl "of
/// vehicles"): both readings in the notation, from the generated table. The
/// lexicon gives the more frequent reading; [`context_reading`] says when
/// the words around it ask for the other.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct VerbNounReadings {
    pub verb: Vec<char>,
    pub nominal: Vec<char>,
    /// The verb's reading is the l-participle.
    pub participle: bool,
    /// The share of a genitive plural in -l, in percent of Gigafida.
    pub noun_percent: Option<u8>,
}

/// The two readings of a verb/noun spelling; `None` for any other spelling.
pub fn verb_noun_readings(_w: &[char]) -> Option<VerbNounReadings> {
    // TODO(sl)
    None
}

/// What a verb/noun spelling is in its clause.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VerbOrNoun {
    /// No cue: the more frequent reading, the lexicon's, stays.
    Lexicon,
    Verb,
    Nominal,
}

/// Which reading of the verb/noun spelling at `i` ([`verb_noun_readings`])
/// the clause asks for. A preposition or dve/obe before it make it the noun,
/// ne the verb; veseli and leti have rules of their own. A spelling in -l
/// that is a participle and a noun's genitive plural (`noun_percent`:
/// obvéstil, obvestíl) is told by its auxiliary. Of the clause only the
/// words, `before_complement` and `exclamation` are looked at.
pub fn context_reading(_clause: &Clause, _i: usize, _participle: bool, _noun_percent: Option<u8>) -> VerbOrNoun {
    // TODO(sl)
    VerbOrNoun::Lexicon
}

/// A compound numeral (enaindvajset, štiriinštirideset) split into its ones
/// and tens; `None` for any other word. The front end reads it as three
/// words, ena in dvajset, with an accent on the ones and on the tens (SP
/// 2001: ênaindvájset, pétindvájset) and no pause between them.
pub fn split_compound_numeral(_w: &[char]) -> Option<(Vec<char>, Vec<char>)> {
    // TODO(sl)
    None
}
