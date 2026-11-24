//! Slovenian spellings with more than one reading: names, the head of a
//! clause, verb or noun, compound numerals. The readings that go by the
//! other words of the clause (meni, lahko, roka, glasu, je ...) are in
//! [`super::clause`]. A reading ("pron") is a pronunciation in the notation
//! of [`super::lexicon::Lexicon`].

use super::lexicon::find_in_table;
use crate::text::frontend::letters::{chars, ends_with, ends_with_any, eq, is_one_of, is_vowel_letter, starts_with};
use crate::text::table;

pub use super::clause::{clause_reading, clause_reading_is_content};

/// Names (persons, places, regions) read so only when written with a
/// capital, anywhere in a clause; front-coded like the lexicon's tables.
static NAMES: &str = include_str!("tables/names.txt");

/// Names read so only inside a clause, not at its head.
static NAMES_INSIDE: &str = include_str!("tables/names_inside.txt");

/// The imperatives of the verbs of commands (`slovenian_commands.tsv`), read
/// at the head of a clause where their spelling is the present elsewhere
/// (nadgrádi, namésti, posodôbi; nadgradí, namestí, posodobí).
static HEAD_IMPERATIVES: &str = include_str!("tables/head_imperatives.txt");

/// The second person imperatives of every verb of Sloleks (hand-accented)
/// where the lexicon reads the spelling otherwise, mostly as the present
/// (izgovôri and izgovorí, sprôsti and sprostí, vêži and véži): read where
/// the clause says it asks for something, see [`imperative_reading`].
static IMPERATIVES: &str = include_str!("tables/imperatives.txt");

/// Given names, lowercase (`slovenian_given_names.tsv`).
static GIVEN_NAMES: &str = include_str!("tables/given_names.txt");

/// Masculine definite adjectives in -i, of frequency 10 or more, without
/// those known by their ending (`is_adjective_ending()`).
static DEFINITE_ADJECTIVES: &str = include_str!("tables/definite_adjectives.txt");

/// Food nouns (`slovenian_food.tsv`): every form of Sloleks with its cases.
static FOOD: &str = include_str!("tables/food.txt");

/// Spellings read as a verb's form or as another word: the verb's reading,
/// the other, `l` when the verb's is the l-participle, and the share in
/// percent of a genitive plural in -l.
static VERB_NOUN: &str = include_str!("tables/verb_noun.txt");

/// The present, the imperative and the neuter l-participle of the main verbs
/// of Sloleks, of frequency 100 or more, without the spellings a noun or an
/// adjective in the feminine accusative or the neuter has as often (prodajo,
/// vozilo): the forms that make lepo, težko ... before them the adverb.
static FINITE_VERBS: &str = include_str!("tables/finite_verbs.txt");

/// The feminine accusative and instrumental and the neuter nominative and
/// accusative singular of the participles of Sloleks (urejeno, povezano,
/// dišeče), of frequency 100 or more, without the spellings a noun has as
/// often in those cases: the forms in -o and -e that make lepo, težko ...
/// before them the adverb, which no ending tells from a noun's.
static PARTICIPLES: &str = include_str!("tables/participles.txt");

/// A word as the key of a generated table.
fn key(w: &[char]) -> String {
    w.iter().collect()
}

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
    /// The clause stands next to prosim between commas
    /// ([`super::marks::REQUEST_MARK`]): Izgovori, prosim, pravilno.
    pub request: bool,
}

impl<'a> Clause<'a> {
    /// The word at `k`; `None` past the end.
    pub(super) fn word(&self, k: usize) -> Option<&'a [char]> {
        self.words.get(k).map(Vec::as_slice)
    }

    /// The word before `k`.
    pub(super) fn before(&self, k: usize) -> Option<&'a [char]> {
        self.word(k.checked_sub(1)?)
    }

    /// Whether there is a word at `k` and it is `s`.
    pub(super) fn is(&self, k: usize, s: &str) -> bool {
        self.word(k).is_some_and(|w| eq(w, s))
    }

    /// Whether there is a word at `k` and it is one of the list.
    pub(super) fn one_of(&self, k: usize, list: &[&str]) -> bool {
        self.word(k).is_some_and(|w| is_one_of(w, list))
    }

    pub(super) fn capital(&self, k: usize) -> bool {
        self.capitalized.get(k).copied().unwrap_or(false)
    }

    pub(super) fn is_acronym(&self, k: usize) -> bool {
        self.acronym.get(k).copied().unwrap_or(false)
    }
}

/// The reading of a word written with a capital letter that is a name (a
/// person, a place, a region) the lexicon reads otherwise, mostly because a
/// common word is spelled like it: Sóbota (Múrska Sóbota) and sobóta
/// "Saturday", Jêlena and jeléna "of the deer". From the hand-accented names
/// of Sloleks and the names of Slovenski pravopis 2001. `w` is lowercase;
/// `clause_head`: the word opens its clause (or follows an opening quote),
/// where any word has a capital, so only a name more frequent than the
/// common word is read. `None` when the lexicon's reading holds.
pub fn name_reading(w: &[char], clause_head: bool) -> Option<Vec<char>> {
    find_in_table(NAMES, w).or_else(|| if clause_head { None } else { find_in_table(NAMES_INSIDE, w) })
}

/// Whether a word (in small letters) is one of the given names of the
/// generated table (`tools/formant/slovenian_given_names.tsv`).
pub fn is_given_name(w: &[char]) -> bool {
    table::contains(GIVEN_NAMES, &key(w))
}

/// The reading of a surname in -e: after a given name (Vesna Smode) a
/// capitalized word in a consonant and -e the tables of names do not know is
/// stressed on its open final e (Smodè, Starè, as the hand-accented Peršè,
/// Lekšè, Peterlè, Šukljè of Sloleks), whatever common word it is spelled
/// like. Not one of the hand-accented exceptions (Lapájne), not spelled the
/// foreign way (Moore, Hesse). The list of given names leaves out names in -a
/// that are mostly a man's genitive (Miroslava Krleže). `w` and `previous` are
/// lowercase; `None` when the rule does not apply.
pub fn surname_reading(w: &[char], previous: &[char], previous_capitalized: bool) -> Option<Vec<char>> {
    // The names in -e with another accent in Sloleks, hand-accented: surnames
    // and given names that may follow a given name
    const NOT_FINAL: &[&str] = &[
        "lapajne", "barle", "strmole", "kobe", "habe", "maze", "mate", "kette", "mengele", "verne", "inge", "arne",
        "stipe", "felipe", "dante", "monte", "ante", "palme", "pavle", "pepe", "polde", "nande", "nace", "rade",
        "dane", "dare", "bine", "brane", "cene", "france", "frane", "jože", "jure", "lojze", "stane", "tine", "tone",
        "zvone", "žare", "čare", "čile",
    ];
    let [stem @ .., before, 'e'] = w else { return None };
    if !previous_capitalized || stem.is_empty() || is_vowel_letter(*before) || is_one_of(w, NOT_FINAL) {
        return None;
    }
    let mut vowels = 0;
    for (k, pair) in w.windows(2).enumerate() {
        let (c, next) = (pair[0], pair[1]);
        if matches!(c, 'q' | 'w' | 'x' | 'y') {
            return None;
        }
        if is_vowel_letter(c) {
            vowels += 1;
            // au stays (Gaube); the other pairs are not Slovenian (Moore, Pierce)
            if is_vowel_letter(next) && !(c == 'a' && next == 'u') {
                return None;
            }
        }
        // Hesse, Allende; th, ch, sh, ph, gh and ck
        let foreign_pair =
            k > 0 && ((c == 'h' && matches!(w[k - 1], 'c' | 't' | 's' | 'p' | 'g')) || (c == 'k' && w[k - 1] == 'c'));
        if next == c || foreign_pair {
            return None;
        }
    }
    if vowels == 0 || !is_given_name(previous) {
        return None;
    }
    let mut pron = w[..w.len() - 1].to_vec();
    pron.push('è');
    Some(pron)
}

/// The reading of a spelling at the head of a clause.
#[derive(Clone, Debug, PartialEq, Eq)]
pub struct HeadReading {
    pub pron: Vec<char>,
    /// A verb of saying (govori, odgovori, prosi): before a comma and a
    /// clause of da it is the present (Govori, da bo prišel).
    pub speech: bool,
}

/// Spellings whose reading depends on where they stand: at the head of a
/// clause (or after a conjunction or an adverb that opens a request) they
/// are the greeting, the imperative of a button label or the name of a
/// menu, elsewhere the present tense of the verb or another word spelled
/// the same (SSKJ, SP 2001: naredíti -ím, imperative narêdi; govoríti -ím,
/// govôri; dobíti -ím, dôbi; zapustíti -ím, zapústi; storíti -ím, stôri;
/// delíti -ím, dêli; uredíti -ím, urêdi; ponovíti -ím, ponôvi; obnovíti
/// -ím, obnôvi; naložíti -ím, nalôži; osvežíti -ím, osvéžil; žívijo the
/// greeting, živíjo "they live"). The head reading is given here; the
/// other comes from the lexicon. Meni has a rule of its own
/// (`clause_reading()`). The third member marks a verb of saying: the
/// present before a clause of da.
const HEAD_READINGS: &[(&str, &str, bool)] = &[
    ("živijo", "žívijo", false),
    ("živio", "žívio", false),
    ("naredi", "narêdi", false),
    ("govori", "govôri", true),
    ("odgovori", "odgovôri", true),
    ("dobi", "dôbi", false),
    ("zapusti", "zapústi", false),
    ("spusti", "spústi", false),
    ("stori", "stôri", false),
    ("prosi", "prôsi", true),
    ("deli", "dêli", false), // the Share button; déli "parts" elsewhere
    ("uredi", "urêdi", false),
    ("ponovi", "ponôvi", false),
    ("obnovi", "obnôvi", false),
    ("naloži", "nalôži", false),
    ("osveži", "osvéži", false),
];

/// The reading of a spelling at the head of a clause, where it is the
/// greeting or the imperative (žívijo, narêdi, govôri, and the commands of
/// the generated table of head imperatives: posodôbi, nadgrádi, namésti,
/// izvŕši); `None` for a word that reads the same everywhere. Elsewhere the
/// lexicon's reading holds (živíjo, naredí "they live", "he makes").
pub fn head_reading(w: &[char]) -> Option<HeadReading> {
    if let Some(&(_, head, speech)) = HEAD_READINGS.iter().find(|&&(spelling, ..)| eq(w, spelling)) {
        return Some(HeadReading { pron: chars(head), speech });
    }
    // The commands of software (slovenian_commands.tsv): the imperatives of
    // Sloleks where they retract the stress of the present (SP 2001 gradíti
    // grádi, namestíti namésti, vršíti vŕši; a listener's report of
    // 2026-11-03: Posodobi, Nadgradi, Namesti, Izvrši)
    find_in_table(HEAD_IMPERATIVES, w).map(|pron| HeadReading { pron, speech: false })
}

/// The imperative of a verb at the head of a clause that asks for something,
/// with prosim before the verb, after it or next to the clause (Izgovori
/// prosim pravilno! Prosim izgovori to. Poveži, prosim, napravo.): SP 2001
/// govoríti -ím, govôri -íte, and every verb in -íti -ím so. Without such a
/// word only the verbs of [`head_reading`] are commands: at the head of a
/// sentence of SUK 1.1 the class as a whole is the present 68 times and the
/// imperative 10. `None` for a spelling the lexicon reads as the imperative
/// already.
pub fn imperative_reading(w: &[char]) -> Option<Vec<char>> {
    find_in_table(IMPERATIVES, w)
}

/// A verb's present, imperative or neuter l-participle that no noun in the
/// feminine accusative or the neuter is spelled like (pišeš, poje, imajo,
/// uspelo; not zimo, pesem, prodajo).
pub fn is_finite_verb(w: &[char]) -> bool {
    table::contains(FINITE_VERBS, &key(w))
}

/// A participle in a form an adjective in -o agrees with too (urejeno,
/// povezano, dišeče; not majico, vreme, novo).
pub fn is_agreeing_participle(w: &[char]) -> bool {
    table::contains(PARTICIPLES, &key(w))
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
pub fn verb_noun_readings(w: &[char]) -> Option<VerbNounReadings> {
    let mut fields = table::lookup(VERB_NOUN, &key(w))?.split('\t');
    let (verb, nominal, flags) = (fields.next()?, fields.next()?, fields.next()?);
    let percent = flags.strip_prefix('l').unwrap_or(flags);
    Some(VerbNounReadings {
        verb: chars(verb),
        nominal: chars(nominal),
        participle: flags.starts_with('l'),
        noun_percent: percent.parse().ok(),
    })
}

// The readings were chosen on the tagged corpus SUK 1.1 (CLARIN.SI,
// 11356/1959; 6,863 tokens of these spellings): the more frequent reading
// alone is right for 93.3 %, these cues for 94.8 %. Cues that look
// plausible did worse than nothing over all the spellings and were left
// out: a clause-final word is as often a noun (pred leti, čez dve leti) as
// a verb, se earlier in the clause belongs to another verb as often as to
// this one, and a determiner or a copula before it says little.

/// The prepositions that decide a reading after them.
pub fn is_preposition(w: &[char]) -> bool {
    const PREPOSITIONS: &[&str] = &[
        "v", "na", "z", "s", "k", "h", "o", "pri", "po", "od", "do", "iz", "za", "pred", "nad", "pod", "med", "brez",
        "ob", "proti", "skozi", "čez", "zaradi", "okoli", "okrog", "blizu", "glede", "poleg", "razen", "sredi", "mimo",
        "preko", "prek", "kljub", "zoper", "izza", "izpod", "namesto", "vzdolž", "znotraj", "zunaj", "nasproti",
        "izmed",
    ];
    is_one_of(w, PREPOSITIONS)
}

/// A number as the number converter writes it (ena, dva, dve, dvanajst, sto,
/// tisoč), a part of a compound one split by the front end (ena in dvajset),
/// an oblique form (dveh, petih, petimi, enaindvajsetimi), an ordinal (prvi,
/// tretjem, dvajsetega, stoti), a "cela" or a multiplicative (dvakrat).
pub(super) fn is_number_word(w: &[char]) -> bool {
    #[rustfmt::skip] // rows of words
    const NUMBERS: &[&str] = &[
        "nič", "ena", "en", "eno", "dva", "dve", "tri", "trije", "štiri", "štirje", "pet", "šest", "sedem", "osem",
        "devet", "deset", "enajst", "dvanajst", "trinajst", "štirinajst", "petnajst", "šestnajst", "sedemnajst",
        "osemnajst", "devetnajst", "dvajset", "trideset", "štirideset", "petdeset", "šestdeset", "sedemdeset",
        "osemdeset", "devetdeset", "sto", "dvesto", "tristo", "štiristo", "petsto", "šeststo", "sedemsto", "osemsto",
        "devetsto", "tisoč", "milijon", "milijona", "milijoni", "milijonov", "milijarda", "milijardi", "milijarde",
        "milijard", "in", "cela", "celi", "cele", "celih", "minus", "enega", "ene", "enem", "eni", "enim", "enemu",
        "enih", "dveh", "dvema", "treh", "trem", "tremi", "štirih", "štirim", "štirimi",
    ];
    // The stems of the ordinals and of the oblique forms of 5 and above,
    // before an adjectival ending
    #[rustfmt::skip] // rows of words
    const STEMS: &[&str] = &[
        "prv", "drug", "tretj", "četrt", "sedm", "osm", "stot", "dvestot", "tristot", "štiristot", "petstot",
        "šeststot", "sedemstot", "osemstot", "devetstot", "milijont", "milijardt",
    ];
    const ENDINGS: &[&str] = &["imi", "ima", "ega", "emu", "em", "im", "ih", "i", "a", "o", "e"];
    let number = |w: &[char]| is_one_of(w, NUMBERS) || split_compound_numeral(w).is_some();
    if number(w) {
        return true;
    }
    if ends_with(w, "krat") {
        return number(&w[..w.len() - 4]);
    }
    ENDINGS.iter().any(|ending| {
        // (ini, ina: not numbers)
        let Some(stem) = w.len().checked_sub(ending.len()).filter(|&stem| stem > 0 && ends_with(w, ending)) else {
            return false;
        };
        let stem = &w[..stem];
        is_one_of(stem, STEMS) || (!is_one_of(stem, &["in", "en"]) && number(stem))
    })
}

/// The endings of the masculine definite adjectives that hardly anything
/// else has.
pub(super) fn is_adjective_ending(w: &[char]) -> bool {
    (w.len() >= 5 && ends_with(w, "ni")) || ends_with_any(w, &["ski", "ški", "čki", "nji", "oči", "eči"])
}

/// The other masculine definite adjectives of Sloleks (the generated table).
pub(super) fn is_definite_adjective(w: &[char]) -> bool {
    table::contains(DEFINITE_ADJECTIVES, &key(w))
}

pub(super) const FOOD_NOMINATIVE: u8 = 1;
pub(super) const FOOD_ACCUSATIVE: u8 = 2;
pub(super) const FOOD_GENITIVE: u8 = 4;

/// The cases a form of a food noun of the generated table is, or 0.
pub(super) fn food_cases(w: &[char]) -> u8 {
    table::lookup(FOOD, &key(w)).and_then(|cases| cases.parse().ok()).unwrap_or(0)
}

/// What a verb/noun spelling is in its clause.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum VerbOrNoun {
    /// No cue: the more frequent reading, the lexicon's, stays.
    Lexicon,
    Verb,
    Nominal,
}

/// A spelling in -l that is a noun's genitive plural (obvestíl [l], vozíl,
/// sporočíl) and a verb's l-participle (obvéstil [u̯], vôzil, sporóčil), and
/// `noun_percent` is how often it is the noun in Gigafida (Sloleks's form
/// frequencies): 91 % of vozil, 30 % of obvestil, 3 % of pojasnil. A participle
/// needs an auxiliary, a noun does not: that is the main cue. Of the 2,309 tokens
/// of such spellings in SUK 1.1 the cues below read 2,245 right, against 2,208
/// without them. (A listener's Ni obvestil, On me ni obvestil: November 2026.)
fn participle_or_genitive(clause: &Clause, i: usize, noun_percent: u8) -> VerbOrNoun {
    const AUXILIARIES: &[&str] = &[
        "sem", "si", "je", "bi", "ni", "nisem", "nisi", "bom", "boš", "bo", "smo", "ste", "so", "sva", "sta", "nismo",
        "niste", "niso", "nisva", "nista", "bomo", "boste", "bodo", "bova", "bosta",
    ];
    const CLITICS: &[&str] = &[
        "me", "te", "ga", "jo", "nas", "vas", "jih", "ju", "naju", "vaju", "mi", "ti", "mu", "ji", "nam", "vam", "jim",
        "se", "si",
    ];
    // What counts things, in the genitive plural after it (pet obvestil)
    #[rustfmt::skip] // rows of words
    const QUANTITIES: &[&str] = &[
        "dve", "dva", "tri", "štiri", "pet", "šest", "sedem", "osem", "devet", "deset", "enajst", "dvanajst",
        "trinajst", "štirinajst", "petnajst", "šestnajst", "sedemnajst", "osemnajst", "devetnajst", "dvajset",
        "trideset", "štirideset", "petdeset", "šestdeset", "sedemdeset", "osemdeset", "devetdeset", "sto", "dvesto",
        "tristo", "štiristo", "petsto", "tisoč", "milijon", "milijonov", "veliko", "več", "mnogo", "nekaj", "dosti",
        "manj", "koliko", "toliko", "preveč", "premalo", "največ", "najmanj", "oba", "obe", "vseh", "teh", "nobenih",
        "tistih", "nekaterih",
    ];
    const LACKING: &[&str] = &["nimam", "nimaš", "nima", "nimava", "nimata", "nimamo", "nimate", "nimajo"];
    // A noun with its genitive after it (seznam obvestil, pošiljanje sporočil)
    #[rustfmt::skip] // rows of words
    const HEAD_NOUNS: &[&str] = &[
        "seznam", "seznama", "število", "števila", "vrsta", "vrste", "vrst", "pregled", "pregleda", "prikaz", "prikaza",
        "pošiljanje", "pošiljanja", "zgodovina", "zgodovine", "center", "centra", "nastavitev", "nastavitve",
        "urejanje", "upravljanje", "uporaba", "uporabe", "prodaja", "prodaje", "nakup", "nakupa", "proizvodnja",
        "proizvodnje", "izdelava", "izdelave", "zaloga", "zaloge", "kup", "kupa", "niz", "niza", "večina", "večine",
        "polovica", "polovice", "tretjina", "odstotek", "odstotkov", "par", "para", "skupina", "skupine", "sistem",
        "sistema", "branje", "prejemanje", "brisanje", "izbris", "izvoz", "uvoz", "rezervacija", "pravilnik",
    ];
    // With these before ni it says something is not there, not who did not
    // do something (Trenutno ni obvestil, Janez ni obvestil)
    const EXISTENTIAL: &[&str] = &[
        "trenutno", "zdaj", "sedaj", "danes", "tu", "tukaj", "tam", "še", "že", "žal", "sploh", "nikjer", "več",
        "nikoli", "včeraj", "jutri",
    ];
    const COORDINATORS: &[&str] = &["in", "ter", "ali", "pa", "oziroma", "nato", "potem"];
    let looks_like_participle = |s: &[char]| s.len() > 3 && matches!(s, [.., 'a' | 'e' | 'i' | 'u', 'l']);
    // The noun when the cue holds, else the verb
    let verb_if = |nominal: bool| if nominal { VerbOrNoun::Nominal } else { VerbOrNoun::Verb };
    let words = clause.words;
    let n = words.len();
    // Alone, the commoner reading: a genitive plural is no heading (učil, poročil)
    if n == 1 {
        return verb_if(noun_percent >= 50);
    }
    let next = clause.word(i + 1);
    if let Some(prev) = clause.before(i) {
        if is_preposition(prev) {
            return VerbOrNoun::Nominal;
        }
        if prev.len() > 3 && ends_with(prev, "ih") && !eq(prev, "njih") {
            return verb_if(noun_percent >= 2);
        }
        let counted = noun_percent >= 3 && (is_one_of(prev, QUANTITIES) || is_one_of(prev, HEAD_NOUNS));
        if is_one_of(prev, LACKING) || counted {
            return VerbOrNoun::Nominal;
        }
    }
    // What a verb of saying takes: Ni obvestil, da pride. Ni sporočil, kdaj.
    if next.is_none() && clause.before_complement && noun_percent < 90 {
        return VerbOrNoun::Verb;
    }
    // At the head of the clause: Obvestil me je. Obvestil je policijo. But
    // Vozil je bilo malo.
    if i == 0 {
        let k = (1..n).find(|&k| !clause.one_of(k, CLITICS)).unwrap_or(n);
        if clause.one_of(k, AUXILIARIES) {
            return verb_if(noun_percent >= 5 && k == 1 && clause.is(k, "je") && clause.is(k + 1, "bilo"));
        }
    }
    let Some(aux) = (0..i.min(n)).rev().find(|&k| clause.one_of(k, AUXILIARIES)) else {
        return verb_if(noun_percent >= 5);
    };
    if (aux + 1..i).all(|k| clause.one_of(k, CLITICS)) {
        // Ni obvestil. Trenutno ni obvestil. Ni vozil na cesti. But Ni me
        // obvestil, On ni obvestil, Ni obvestil policije.
        return verb_if(
            clause.is(aux, "ni")
                && aux + 1 == i
                && noun_percent >= 20
                && (aux == 0 || (aux == 1 && clause.one_of(0, EXISTENTIAL)))
                && next.is_none_or(is_preposition),
        );
    }
    // se je poročil: the reflexive makes it the verb
    if (0..i).any(|k| clause.is(k, "se")) {
        return VerbOrNoun::Verb;
    }
    // Another participle has the auxiliary (Nisem dobil obvestil), unless
    // this one is coordinated with it (je prišel in obvestil)
    let other_participle = |k: usize| {
        clause.word(k).is_some_and(looks_like_participle) && !(k + 1..i).any(|c| clause.one_of(c, COORDINATORS))
    };
    if noun_percent >= 5 && (aux + 1..i).any(other_participle) {
        return VerbOrNoun::Nominal;
    }
    verb_if(noun_percent >= 40)
}

/// Which reading of the verb/noun spelling at `i` ([`verb_noun_readings`])
/// the clause asks for. A preposition or dve/obe before it make it the noun,
/// ne the verb; veseli and leti have rules of their own. A spelling in -l
/// that is a participle and a noun's genitive plural (`noun_percent`:
/// obvéstil, obvestíl) is told by its auxiliary. Of the clause only the
/// words, `before_complement` and `exclamation` are looked at.
pub fn context_reading(clause: &Clause, i: usize, participle: bool, noun_percent: Option<u8>) -> VerbOrNoun {
    const BITI: &[&str] = &[
        "sem", "si", "je", "sva", "sta", "smo", "ste", "so", "nisem", "nisi", "ni", "nisva", "nista", "nismo", "niste",
        "niso", "bi", "bom", "boš", "bo", "bova", "bosta", "bomo", "boste", "bodo", "bil", "bila", "bilo", "bili",
        "bile", "biti",
    ];
    const OBJECT_CLITICS: &[&str] = &[
        "me", "te", "ga", "jo", "nas", "vas", "jih", "ju", "naju", "vaju", "mi", "ti", "mu", "ji", "nam", "vam", "jim",
    ];
    const COUNTS: &[&str] = &[
        "dve", "dva", "obe", "oba", "tri", "štiri", "pet", "šest", "sedem", "osem", "devet", "deset", "nekaj", "več",
        "mnogo", "veliko",
    ];
    let words = clause.words;
    let Some(w) = clause.word(i) else { return VerbOrNoun::Lexicon };
    let prev = clause.before(i);
    let next = clause.word(i + 1);
    let last = next.is_none();

    // A verb's present opens the clause before its object: Veseli me, da sva se
    // spoznala. Žalosti me, da nisi prišel. Skrbi me. (SSKJ² veseli nas, da delo
    // napreduje; SP ~i me, da vas vidim.) All 20 occurrences of a spelling of this
    // table at the head of a clause before an object clitic in SUK 1.1 are verbs;
    // a noun there has a form of biti after the clitic (Pomoči mi ni).
    // Also after a conjunction (rojen sem bil leta 2026 in veseli me, da ...:
    // the sample text; a listener's report of 2026-11-03)
    const CONJUNCTIONS: &[&str] = &["in", "pa", "ter", "ki", "ampak", "a", "toda"];
    if prev.is_none_or(|prev| is_one_of(prev, CONJUNCTIONS))
        && !participle
        && clause.one_of(i + 1, OBJECT_CLITICS)
        && !clause.one_of(i + 2, BITI)
    {
        return VerbOrNoun::Verb;
    }

    // veseli: veselí (se) "looks forward, pleases", veséli "cheerful". The
    // adjective follows a form of biti (smo veseli, so bili zelo veseli);
    // the verb has its se, an object (nas veseli), ne, or ends the clause
    // (On se zelo veseli. To me veseli.) The imperative vesêli se! is read
    // by clause_reading(), which sees the exclamation mark.
    if eq(w, "veseli") {
        if (i.saturating_sub(3)..i).any(|k| clause.one_of(k, BITI)) {
            return VerbOrNoun::Nominal;
        }
        // An object clitic anywhere before it, not only right before it: Me
        // zelo veseli vaš obisk, To me srčno veseli (a listener's report of
        // 2026-11-03: "me veseli", "me zelo veseli", "me srčno veseli", "in me
        // veseli" are all the verb); with a form of biti before it the
        // adjective took the clause above (Otroci so ga veseli)
        let object = |x: &[char]| !is_one_of(x, &["ti", "te"]) && is_one_of(x, OBJECT_CLITICS);
        if words[..i].iter().any(|x| is_one_of(x, &["se", "si"]) || object(x)) {
            return VerbOrNoun::Verb;
        }
        if prev.is_some_and(|prev| is_one_of(prev, OBJECT_CLITICS) || eq(prev, "ne")) || last {
            return VerbOrNoun::Verb;
        }
        // An object clitic right after it anywhere in the clause (in veseli
        // me, da ...), not ti, which is also the demonstrative (veseli ti
        // ljudje)
        if next.is_some_and(|next| !eq(next, "ti") && is_one_of(next, OBJECT_CLITICS)) {
            return VerbOrNoun::Verb;
        }
        return VerbOrNoun::Lexicon;
    }
    // gori: gorí "burns" (Peč gori vsak dan), but gôri the locative and the
    // dative of gôra after a preposition (na gori) or an adjective (v Kranjski
    // Gori, Črni gori; the 24 occurrences of the noun in SUK 1.1 that have a
    // word before them), and the adverb gôri "up there" (SSKJ² gôri prisl.:
    // tam gori na hribu, daleč, visoko gori, gori v bregu, hiša stoji na
    // hribu in gori je veliko sonca; SP 2001 gôri mestov. prostor. prisl.):
    // after tam and the like, before a form of biti anywhere (a present
    // tense is never followed by the auxiliary), and at the head of a clause
    // before a preposition of place when the clause goes on past the phrase
    // of place, so the place is where something is or happens: Gori na gori
    // gori. Gori na seniku je lepo. Gori na podstrešju je krama. (a listener's
    // examples of 2026-11-03, the first syllable); not in an exclamation,
    // the alarm (Gori na podstrešju!), and not where nothing follows the
    // place (Gori v hiši.), the fire. SUK 1.1 has no gori at the head of a
    // clause (3 verbs inside one, 1 adverb after tam), so the listener and
    // the dictionaries decide. The lexicon's commoner gôri would read the
    // listener's Na gori gori ogenj twice as the noun.
    if eq(w, "gori") {
        const UP: &[&str] = &["tam", "tu", "tukaj", "tja", "dve", "obe", "tej", "neki", "visoko", "daleč"];
        const NOT_ADJECTIVE: &[&str] = &[
            "gori", "tudi", "vsi", "oni", "ali", "kateri", "nekateri", "zaradi", "hkrati", "nikoli", "lani", "ponoči",
            "sredi", "okoli", "zdi", "vidi", "pravi", "meni", "tebi", "sebi",
        ];
        // Where something is, not what it burns with or because of (gori s
        // plamenom, gori brez dima, gori zaradi, gori po vsej hiši)
        const PLACE: &[&str] = &[
            "na", "v", "pri", "pod", "nad", "ob", "za", "pred", "med", "sredi", "vrh", "blizu", "poleg", "zraven",
            "okoli", "okrog", "nasproti", "onkraj", "do", "proti", "k", "h",
        ];
        if clause.one_of(i + 1, BITI) {
            return VerbOrNoun::Nominal; // Gori je lepo. in gori je veliko sonca
        }
        let Some(prev) = prev else {
            // Gori na gori gori. Gori na seniku je lepo.
            if clause.one_of(i + 1, PLACE) && i + 3 < words.len() && !clause.exclamation {
                return VerbOrNoun::Nominal;
            }
            return VerbOrNoun::Verb; // Gori! Gori luč. Gori v hiši.
        };
        let adjective = prev.len() >= 4 && prev.last() == Some(&'i') && !is_one_of(prev, NOT_ADJECTIVE);
        return if is_preposition(prev) || is_one_of(prev, UP) || adjective {
            VerbOrNoun::Nominal
        } else {
            VerbOrNoun::Verb
        };
    }
    // leti: léti is the dual and the instrumental of léto (dve leti, pred
    // leti, z leti) or the plural of lèt "flight" (leti so odpovedani,
    // poskusni leti), always with a number or a preposition close before it,
    // its adjective right before it or a form of biti after it; letí "flies"
    // elsewhere (ptič leti, letalo leti nad mestom). The noun is 98 % of the
    // corpus, and every one of its occurrences there has such a word next
    // to it.
    if eq(w, "leti") {
        // Three words back, not counting the words of a number, which the
        // front end splits (pred ena in dvajsetimi leti; a listener's "pred
        // 21 leti" read the verb while the window ended at "ena"). Not the
        // conjunction in, a number word only inside a numeral, whose other
        // words count here already: until 2026-11-18 any in before it made
        // leti the noun (Ptič poje in leti)
        let counted = |x: &[char]| {
            is_preposition(x)
                || is_one_of(x, COUNTS)
                || (is_number_word(x) && !eq(x, "in"))
                || ends_with_any(x, &["imi", "ema"])
        };
        // The flights after their adjective (dogovori s srbskim Jatom in
        // poskusni leti v Beograd, SUK 1.1; redni leti, čarterski leti): the
        // shapes only adjectives have, so not ponoči leti, oni leti, and not
        // the two other words of that shape a bird flies after (Lastovka
        // jeseni leti na jug)
        let flights = prev.is_some_and(|prev| {
            (prev.len() >= 5 && ends_with(prev, "ni") && !is_one_of(prev, &["jeseni", "strani"]))
                || ends_with_any(prev, &["ski", "ški"])
        });
        return if flights || words[..i].iter().rev().take(3).any(|x| counted(x)) || clause.one_of(i + 1, BITI) {
            VerbOrNoun::Nominal
        } else {
            VerbOrNoun::Verb
        };
    }

    if participle && let Some(noun_percent) = noun_percent {
        return participle_or_genitive(clause, i, noun_percent);
    }
    // A noun in the nominative or the accusative that a participle is spelled
    // like (predál and predáw, hotél and hôtel, lokál, pokál): in a clause of
    // several words, with no auxiliary in it, it is no participle (Odpri predal)
    if participle && words.len() > 1 && !words.iter().any(|x| is_one_of(x, BITI)) {
        return VerbOrNoun::Nominal;
    }

    let Some(prev) = prev else { return VerbOrNoun::Lexicon };
    // brez novih vozil, teh sporočil: a genitive plural after an adjective
    if is_preposition(prev)
        || is_one_of(prev, &["dve", "dva", "obe", "oba"])
        || (participle && prev.len() > 3 && ends_with(prev, "ih"))
    {
        return VerbOrNoun::Nominal;
    }
    if eq(prev, "ne") {
        return VerbOrNoun::Verb;
    }
    // To me žalosti. Ga skrbi. An object clitic before a present (not ti and
    // te, also the demonstratives: ti pogoji, te dobe); after an auxiliary
    // only a participle could follow, so this is another word (so nas levi in
    // desni, so ga poleti). si is the reflexive here: si ga deli, si ga lasti.
    let after_auxiliary = i >= 2 && !clause.is(i - 2, "si") && clause.one_of(i - 2, BITI);
    if !participle && !is_one_of(prev, &["ti", "te"]) && is_one_of(prev, OBJECT_CLITICS) && !after_auxiliary {
        return VerbOrNoun::Verb;
    }
    // On to pogosto izgovori. Kdo odgovori? Naj odgovori. A verb's third
    // person in -i that a noun's plural is spelled like (izgovorí and
    // izgôvori, odgovorí and odgôvori, pogovorí and pogôvori; a listener's
    // report of 2026-11-17): the noun is the commoner word, and the verb has
    // a subject in the singular before it in the clause, an adverb of how
    // or how often right before it, or naj; never with a plural verb of
    // being in the clause, which the nouns have (To so samo izgovori). SUK
    // 1.1: 2,963 to 2,986 of 3,201 tokens of the verb/noun spellings. Not after an adjective that agrees with the noun (v prvi vrsti, pod
    // enakimi pogoji), and not poceni, the adverb before another verb (Kdor
    // poceni kupi)
    let after_adjective = prev.len() > 3 && ends_with_any(prev, &["i", "imi", "ih"]) && !eq(prev, "naj");
    if !participle && ends_with(w, "i") && !after_adjective && !eq(w, "poceni") {
        const SUBJECTS: &[&str] = &["on", "ona", "ono", "kdo", "kdor", "nekdo", "vsakdo", "nihče", "marsikdo"];
        const PLURAL: &[&str] = &["so", "niso", "bodo", "sta", "nista", "bosta", "bili", "bile"];
        #[rustfmt::skip] // rows of words
        const HOW: &[&str] = &[
            "pogosto", "redko", "vedno", "nikoli", "včasih", "običajno", "navadno", "hitro", "počasi", "takoj", "spet",
            "znova", "ponovno", "pravilno", "napačno", "jasno", "glasno", "tiho", "razločno", "rad", "rada", "raje",
            "denimo", "kmalu", "nato", "končno", "nazadnje", "lahko", "komaj", "najprej", "naj",
        ];
        let plural = words.iter().any(|x| is_one_of(x, PLURAL));
        if !plural && (is_one_of(prev, HOW) || words[..i].iter().any(|x| is_one_of(x, SUBJECTS))) {
            return VerbOrNoun::Verb;
        }
        // Einhard trdi, da ...: a verb of saying or thinking before its
        // clause; a noun stands there as well (nimam več moči, da bi ...)
        const SAYING: &[&str] = &["trdi", "zatrdi", "potrdi", "sodi", "dvomi", "pravi", "sporoči", "izjavi"];
        if clause.before_complement && last && is_one_of(w, SAYING) {
            return VerbOrNoun::Verb;
        }
    }
    // je vozil, sem pojasnila, so sporočili: the auxiliary right before
    // a participle that agrees with it (not -lo: to je besedilo, pravilo)
    const SINGULAR: &[&str] = &["sem", "si", "je", "bi", "ni", "nisem", "nisi"];
    const PLURAL: &[&str] = &["smo", "ste", "so", "sva", "sta", "nismo", "niste", "niso", "nisva", "nista"];
    if participle
        && ((is_one_of(prev, SINGULAR) && ends_with_any(w, &["l", "la"]))
            || (is_one_of(prev, PLURAL) && ends_with_any(w, &["li", "le"])))
    {
        return VerbOrNoun::Verb;
    }
    VerbOrNoun::Lexicon
}

/// A compound numeral (enaindvajset, štiriinštirideset) split into its ones
/// and tens; `None` for any other word. The front end reads it as three
/// words, ena in dvajset, with an accent on the ones and on the tens (SP
/// 2001: ênaindvájset, pétindvájset) and no pause between them.
pub fn split_compound_numeral(w: &[char]) -> Option<(Vec<char>, Vec<char>)> {
    const ONES: &[&str] = &["ena", "dva", "tri", "štiri", "pet", "šest", "sedem", "osem", "devet"];
    const TENS: &[&str] =
        &["dvajset", "trideset", "štirideset", "petdeset", "šestdeset", "sedemdeset", "osemdeset", "devetdeset"];
    // Does a stem of the ones (ena ... devet) stand before "in" + a tens word?
    ONES.iter().find_map(|ones| {
        let rest = w.get(ones.chars().count()..).filter(|_| starts_with(w, ones))?;
        let rest = rest.get(2..).filter(|_| starts_with(rest, "in"))?;
        TENS.iter().any(|tens| starts_with(rest, tens)).then(|| (chars(ones), rest.to_vec()))
    })
}
