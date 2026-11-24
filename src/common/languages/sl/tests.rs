//! Direct front-end tests for Slovenian: letter-to-sound rules, the lexicon,
//! names, clitics and the spellings told apart by their clause.

use std::sync::Arc;

use super::marks::{
    BEFORE_COMPLEMENT_MARK, IN_EXCLAMATION_MARK, IN_QUESTION_MARK, REQUEST_MARK, VOCATIVE_MARK, is_address,
    opens_complement_clause,
};
use crate::formant::phones::{Accent, Ph};
use crate::language::Language;
use crate::languages::Frontend;
use crate::prosody::{ClauseKind, Utterance, split_at_breaks};
use crate::text::frontend::UserLexicon;
use crate::text::{Frontend as _, Punctuation};

fn chars(text: &str) -> Vec<char> {
    text.chars().collect()
}

fn slovenian() -> Frontend {
    Frontend::new(Language::Slovenian)
}

/// A clause read with letter names, the default.
fn read(frontend: &Frontend, text: &str) -> Utterance {
    frontend.process(&chars(text), Punctuation::Period, false)
}

/// The user's accent lexicon from its JSON.
fn user(json: &str) -> Option<Arc<UserLexicon>> {
    Some(UserLexicon::parse(json).0)
}

/// The phones of a clause as text: words apart, ' before a stressed nucleus,
/// : after a long one.
fn phones_of(utterance: &Utterance) -> String {
    const NAMES: [&str; Ph::COUNT] = [
        "_", "a", "e", "i", "o", "u", "@", "E", "O", "é", "ó", "ə", "w", "j", "v", "l", "lj", "m", "n", "nj", "ŋ", "r",
        "p", "b", "t", "d", "k", "g", "c", "dz", "č", "dž", "ć", "đ", "f", "s", "z", "š", "ž", "ś", "ź", "h", "ɣ",
    ];
    let mut out = String::new();
    let mut word = None;
    for phone in &utterance.phones {
        if word != Some(phone.word) {
            if word.is_some() {
                out.push(' ');
            }
            word = Some(phone.word);
        }
        if phone.nucleus && phone.stressed {
            out.push('\'');
        }
        out.push_str(NAMES[phone.ph as usize]);
        if phone.nucleus && phone.is_long {
            out.push(':');
        }
    }
    out
}

/// The phones of a clause before `punct`, letters by their names.
fn says(frontend: &Frontend, text: &str, punct: Punctuation) -> String {
    phones_of(&frontend.process(&chars(text), punct, false))
}

/// The phones of a statement, letters by their names.
fn phones(frontend: &Frontend, text: &str) -> String {
    says(frontend, text, Punctuation::Period)
}

/// The phones of a statement, letters by their sounds.
fn sounds(frontend: &Frontend, text: &str) -> String {
    phones_of(&frontend.process(&chars(text), Punctuation::Period, true))
}

/// Every text of a table reads as the phones next to it.
fn check(frontend: &Frontend, cases: &[(&str, &str)]) {
    for &(text, expected) in cases {
        assert_eq!(phones(frontend, text), expected, "{text}");
    }
}

/// Slovenian letters become the sounds of the Pravopis
#[test]
fn slovenian_letters_become_the_sounds_of_the_pravopis() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // Open and close e, o, the schwa and the final devoicing, from the lexicon
            ("mesto", "m'é:stO"),
            ("človek", "čl'O:vEk"),
            ("človeka", "člOv'é:ka"),
            ("otrok", "Otr'Ok"),
            ("otroka", "Otr'O:ka"),
            ("veter", "v'é:tər"),
            ("vetra", "v'é:tra"),
            ("pes", "p'əs"),
            ("grad", "gr'a:t"),
            // l: [w] in participles, after a schwa and in the listed words, [l] otherwise
            ("bil", "b'i:w"),
            ("delal", "d'é:law"),
            ("delala", "d'é:lala"),
            ("rekel", "r'é:kəw"),
            ("stol", "st'Ow"),
            ("volk", "v'O:wk"),
            ("kultura", "kult'u:ra"),
            ("film", "f'i:lm"),
            ("polje", "p'ó:ljE"),
            ("konj", "k'On"),
            // v: [v] before a vowel, [w] before a consonant and at the end, [u] at the head
            ("voda", "v'O:da"),
            ("nov", "n'Ow"), // nòv nôva: short
            ("avto", "'a:wtO"),
            ("vlak", "wl'a:k"),
            ("vsak", "us'a:k"),
            ("vrt", "v'ərt"),
            // Syllabic r, i before a vowel
            ("prst", "p'ərst"),
            ("Trst", "t'ərst"),
            ("pacient", "pacij'Ent"),
            // Compound numerals (an accent on the ones and on the tens, the
            // "in" a clitic, no boundary: SP 2001 ênaindvájset) and the words
            // of the number converter
            ("enaindvajset", "'E:naindv'a:jsEt"),
            ("petinpetdeset", "p'é:tinp'é:ddEsEt"),
            ("tisoč", "t'i:sOč"),
            ("milijonov", "milij'ó:nOw"),
        ],
    );
}

/// Slovenian words reported by the first users (October 2026)
#[test]
fn slovenian_words_reported_by_the_first_users_october_2026() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // SSKJ, SP 2001 on Fran.si: the accents the reports were checked against
            ("napajalnik", "napaj'a:lnik"),
            ("zaganjalnik", "zaganj'a:lnik"), // was zagánjalnik from the verb's stem
            ("otoplitev", "OtOpl'i:təw"),
            ("črkovanje", "čərkOv'a:njE"), // was črkóvanje: the -ovanje rule
            ("kupovati", "kupOv'a:ti"),
            ("zahtevati", "zaht'é:vati"),
            ("nadlegovanje", "nadlEgOv'a:njE"),
            ("nadležen", "nadl'é:žən"),
            ("seznam", "sEzn'am"), // seznàm, the listener's open e; was séznam
            ("seznama", "sEzn'a:ma"),
            ("način", "nač'i:n"), // načín, was náčin
            ("počiščeno", "pOč'i:ščEnO"),
            ("ena", "'E:na"), // êna, open; was éna
            ("en", "'En"),
            ("tapnite", "t'a:pnitE"),
            ("velika", "vEl'i:ka"), // vêlik, velíka
            ("velik", "v'E:lik"),
            ("del", "d'é:l"),                     // dél [tudi deu̯]: the [l] first
            ("dvanajst", "dv'a:najst"),           // SSKJ²: dvánajst in dvanájst, the first
            ("ob dvanajstih", "Ob dv'a:najstih"), // and the ordinal with it (Sloleks dvánajsti)
            ("sedemnajst", "s'é:dəmnajst"),
            ("bralec", "br'a:ləc"), // the stress moves before -lec, -lnik, -lišče
            ("gledališče", "glEdal'i:ščE"),
            ("domov", "dOm'O:w"),
        ],
    );
    // A spelling with two readings: the greeting or the imperative at the
    // head of the clause (and after a conjunction or an adverb that opens
    // a request), the verb's present elsewhere
    assert_eq!(phones(&frontend, "živijo"), "ž'i:vijO");
    assert_eq!(phones(&frontend, "Živijo, kako si"), "ž'i:vijO kak'ó: si");
    assert_eq!(phones(&frontend, "Oni živijo tukaj"), "'O:ni živ'i:jO t'u:kaj");
    assert_eq!(phones(&frontend, "Naredi to"), "nar'E:di t'ó:");
    assert_eq!(phones(&frontend, "Zdaj naredi to"), "zd'aj nar'E:di t'ó:");
    assert_eq!(phones(&frontend, "Program naredi kopijo"), "prOgr'a:m narEd'i: k'ó:pijO");
    assert_eq!(phones(&frontend, "Uredi"), "ur'E:di");
    // meni alone is the menu (SSKJ² mení); in a sentence the pronoun mêni
    // (after a preposition, before se, je, ni) or the verb méni
    assert_eq!(phones(&frontend, "Meni"), "mEn'i:");
    assert_eq!(phones(&frontend, "Meni je všeč"), "m'E:ni jE uš'é:č");
    assert_eq!(phones(&frontend, "Meni se zdi"), "m'E:ni sE zd'i:");
    assert_eq!(phones(&frontend, "Pridi k meni"), "pr'i:di k m'E:ni");
    assert_eq!(phones(&frontend, "Kaj meni o tem"), "k'a:j m'é:ni O t'é:m");
    assert_eq!(phones(&frontend, "Meni gumb"), "mEn'i: g'u:mp"); // a name and its role, since 2026-11-03
    // The participle of biti on its last syllable (SSKJ² bilà biló bilí bilè)
    assert_eq!(phones(&frontend, "bila je"), "bil'a jE");
    assert_eq!(phones(&frontend, "bilo je lepo"), "bil'ó: jE lEp'ó:");
    assert_eq!(phones(&frontend, "bili so"), "bil'i: sO");
    assert_eq!(phones(&frontend, "bile so"), "bil'E sO");
    // ... with the accent of a content word, not a function word's
    // (2026-10-30: bila was heard on its first syllable)
    for text in ["Bila je doma", "Ona je bila tukaj"] {
        for phone in &read(&frontend, text).phones {
            if phone.nucleus && phone.stressed && phone.ph == Ph::A && phone.word <= 2 {
                assert_eq!(phone.prominence, 2, "{text}");
            }
        }
    }
    // lepo: lepó at the end of the clause, lépo before another word
    // (a listener, 2026-10-30); before a pronoun or a verb lepó too
    // (2026-11-18, the test of that report), and before an adjective or a
    // participle (2026-11-24)
    assert_eq!(phones(&frontend, "Hvala lepo"), "hv'a:la lEp'ó:");
    assert_eq!(phones(&frontend, "To je lepo"), "t'ó: jE lEp'ó:");
    assert_eq!(phones(&frontend, "Lepo"), "lEp'ó:");
    assert_eq!(phones(&frontend, "Lepo vreme"), "l'é:pO wr'E:mE");
    assert_eq!(phones(&frontend, "Lepo pozdravljeni"), "lEp'ó: pOzdr'a:wljEni"); // since 2026-11-24
    // ... and so are the other adverbs on -ó with an adjective on the stem
    assert_eq!(phones(&frontend, "To je težko"), "t'ó: jE tEšk'ó:");
    assert_eq!(phones(&frontend, "Težko delo"), "t'E:škO d'é:lO");
    assert_eq!(phones(&frontend, "Počutim se slabo"), "pOč'u:tim sE slab'ó:");
    assert_eq!(phones(&frontend, "Slabo vreme"), "sl'a:bO wr'E:mE");
    assert_eq!(phones(&frontend, "Toplo"), "tOpl'ó:");
    assert_eq!(phones(&frontend, "Tenko nit"), "t'EŋkO n'it");
    // but not the modal lahkó, which stands before its verb
    assert_eq!(phones(&frontend, "Lahko pridem"), "lahk'ó: pr'i:dEm");
    // The number converter's words: the ones keep their accent before the tens
    assert_eq!(phones(&frontend, "sto enaindvajset"), "st'ó: 'E:naindv'a:jsEt");
}

/// Slovenian lexicon of every Sloleks lemma and verb/noun spellings (October 2026)
#[test]
fn slovenian_lexicon_of_every_sloleks_lemma_and_verb_noun_spellings_october_2026() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // Reported: SSKJ² karamból, zobobòl -ôla (lemmas below the table's old
            // frequency threshold, read by the second-last-syllable rule)
            ("karambol", "karamb'ó:l"),
            ("zobobol", "zObOb'Ol"),
            // A stem of one word read another: postavít*, of postavítev, gave
            // postavíti; the table is now checked form by form
            ("postaviti", "pOst'a:viti"),
            ("preveriti", "prEv'é:riti"),
            ("nastavitve", "nastav'i:tvE"),
            // A homograph by the frequency of the form, not of the lemma:
            // veselím "I look forward" is eight times the adjective's vesélim
            ("veselim", "vEsEl'i:m"),
            // Forms with two spellings (mesca, meseca) were dropped
            ("meseca", "m'é:sEca"),
            ("mesecih", "m'é:sEcih"),
            // The fleeting e is the schwa (rékel rêkla, začétek začétka), and
            // only then (vídel vídela; črkujem is not črkujmo's)
            ("začetek", "zač'é:tək"),
            ("videl", "v'i:dEw"),
            ("črkujem", "čərk'u:jEm"),
            // Hand entries written from memory that the dictionaries contradict
            ("beseda", "bEs'é:da"), // beséda, not béseda
            ("višina", "viš'i:na"),
            ("sestra", "s'E:stra"),       // sêstra
            ("septembra", "sEpt'E:mbra"), // septêmber -bra
        ],
    );
    // veseli: the verb with its se, an object or at the end of the clause,
    // the adjective after a form of biti and before a noun
    assert_eq!(phones(&frontend, "On se zelo veseli"), "'On sE zEl'ó: vEsEl'i:");
    assert_eq!(phones(&frontend, "To me veseli"), "t'ó: mE vEsEl'i:");
    assert_eq!(phones(&frontend, "Otroci so veseli"), "Otr'O:ci sO vEs'é:li");
    assert_eq!(phones(&frontend, "Veseli ljudje"), "vEs'é:li ljudj'é:");
    // leti: letí "flies", léti after a number or a preposition
    assert_eq!(phones(&frontend, "Letalo leti nad mestom"), "lEt'a:lO lEt'i: nad m'é:stOm");
    assert_eq!(phones(&frontend, "Ptič leti"), "pt'ič lEt'i:");
    assert_eq!(phones(&frontend, "Dve leti"), "dv'é: l'é:ti");
    assert_eq!(phones(&frontend, "Pred dvema leti"), "prE dv'é:ma l'é:ti");
    // A participle after its auxiliary, the noun after a preposition
    assert_eq!(phones(&frontend, "On je vozil avto"), "'On jE v'O:ziw 'a:wtO");
    assert_eq!(phones(&frontend, "Brez novih vozil"), "brEz n'O:vih vOz'i:l");
}

/// Slovenian names of persons and places (October 2026)
#[test]
fn slovenian_names_of_persons_and_places_october_2026() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // Surnames in -ović, -ević (reported: Srebrović, Petrović, Turković
            // on the first syllable): the syllable before the suffix, in every
            // case, spelled with ć or č, in capitals or small letters with ć;
            // four syllables on the second (Sloleks: Stojánović, Milóšević)
            ("Petrović", "p'é:trOvić"),
            ("Turković", "t'u:rkOvić"),
            ("Srebrović", "sr'é:brOvić"),
            ("Pri Petroviću", "pri p'é:trOviću"),
            ("petrović", "p'é:trOvić"),
            ("PETROVIČ", "p'é:trOvič"),
            ("Stojanović", "stOj'a:nOvić"),
            ("Milošević", "mil'ó:šEvić"),
            ("Antonijević", "ant'ó:nijEvić"),
            ("Mihailović", "mih'a:ilOvić"),
            // Names the lexicon does not know: the patterns of the names of
            // Sloleks (-Cčič on the second-last syllable, other -ič on the
            // third-last, -nc at the end, -mir on the first, -ija before);
            // a feminine in -ija only inside a clause
            ("Gospod Ferdolenc", "gOsp'ó:t fErdOl'é:nc"),
            ("Gospod Brdolinič", "gOsp'ó:d bərd'ó:linič"),
            ("Gospod Pirnatčič", "gOsp'ó:t pirn'a:tčič"),
            ("Gospod Zalomir", "gOsp'ó:d z'a:lOmir"),
            ("Obiskal je Kvarnelijo", "Obisk'aw jE kvarn'é:lijO"),
            ("Kvarnelija je lepa", "kvarnEl'i:ja jE l'é:pa"),
            // A stem of the lexicon with an ending of the declension
            ("Videl je Cankarjev dom", "v'i:dEw jE c'a:ŋkarjEw d'ó:m"),
            ("Pri Korošcu", "pri kOr'ó:šcu"),
            // Names spelled like common words: with a capital inside a clause
            // (Múrska Sóbota), at its head only the commoner name (Jêlena; but
            // "Sobota je lep dan" is the day), never the words of God
            ("Grem v Mursko Soboto", "gr'é:m u m'u:rskO s'ó:bOtO"),
            ("Sobota je lep dan", "sOb'ó:ta jE l'é:p d'a:n"),
            ("V soboto", "u sOb'ó:tO"),
            ("Jelena je doma", "j'E:lEna jE dOm'a:"),
            ("Lov na jelena", "l'Ow na jEl'é:na"),
            ("Verujem v Boga", "vEr'u:jEm u bOg'a:"),
            ("Stare navade", "st'a:rE nav'a:dE"),
            // Places of Slovenski pravopis 2001 in their cases (Bôhinj -hínja,
            // Litíja, Trbôvlje -velj, Domžále)
            ("Bohinj", "b'O:hin"),
            ("V Bohinju", "u bOh'i:nju"),
            ("Litija", "lit'i:ja"),
            ("Iz Trbovelj", "is tərb'O:vəl"),
            ("Živim v Domžalah", "živ'i:m u dOmž'a:lah"),
        ],
    );
}

/// Slovenian clitics, sandhi and clause types
#[test]
fn slovenian_clitics_sandhi_and_clause_types() {
    let frontend = slovenian();
    // Proclitics and enclitics lose their stress and length; v before a
    // consonant is the vowel u; the final d of a preposition stays voiced
    assert_eq!(phones(&frontend, "Bil sem v šoli"), "b'i:w səm u š'ó:li");
    assert_eq!(phones(&frontend, "v Ameriki"), "w am'E:riki");
    assert_eq!(phones(&frontend, "od orača"), "Od Or'a:ča");
    // Regressive voicing assimilation across a clitic boundary and inside a word
    assert_eq!(phones(&frontend, "k bratu"), "g br'a:tu");
    assert_eq!(phones(&frontend, "s časom"), "š č'a:sOm");
    assert_eq!(phones(&frontend, "kdo"), "gd'ó:");
    // An acronym is stressed on its last letter
    assert_eq!(phones(&frontend, "SP"), "Esp'é:");
    // Clause types: a question word, a yes/no question, an exclamation
    let kind = |text: &str, punct| frontend.process(&chars(text), punct, false).kind;
    assert_eq!(kind("Kdo je to", Punctuation::Question), ClauseKind::WhQuestion);
    assert_eq!(kind("Ali greš domov", Punctuation::Question), ClauseKind::YesNoQuestion);
    assert_eq!(kind("Pojdi", Punctuation::Exclamation), ClauseKind::Exclamation);
    // Every Slovenian word has the plain accent, never a tone
    for phone in &read(&frontend, "Dober dan, kako ste").phones {
        if phone.stressed {
            assert_eq!(phone.accent, Accent::Neutral);
        }
    }
}

/// Slovenian accents written in the text and in the user's lexicon win
#[test]
fn slovenian_accents_written_in_the_text_and_in_the_users_lexicon_win() {
    let mut frontend = slovenian();
    // Dictionary marks typed in the text: acute = long close, circumflex =
    // long open, grave = short
    assert_eq!(phones(&frontend, "mésto"), "m'é:stO");
    assert_eq!(phones(&frontend, "mêsto"), "m'E:stO");
    assert_eq!(phones(&frontend, "mèsto"), "m'EstO");
    assert_eq!(phones(&frontend, "mestó"), "mEst'ó:");
    // A word the lexicon does not know: the second-last syllable, long and close
    assert_eq!(phones(&frontend, "bruhoberko"), "bruhOb'é:rkO");
    // The user's entries, in the Slovenian notation, before the built-in ones
    frontend.set_user_lexicon(user(
        r#"{"entries":[{"word":"bruhobêrk*","language":"sl"},{"word":"mestó","language":"sl"},{"word":"p'əs","language":"sl"}]}"#,
    ));
    assert_eq!(phones(&frontend, "bruhoberko"), "bruhOb'E:rkO");
    assert_eq!(phones(&frontend, "bruhoberkom"), "bruhOb'E:rkOm");
    assert_eq!(phones(&frontend, "mesto"), "mEst'ó:");
    // A malformed entry is counted and skipped
    let report =
        UserLexicon::parse(r#"{"entries":[{"word":"mesto","language":"sl"},{"verb":"ur'e:d=i","language":"sl"}]}"#).1;
    assert_eq!(report.rejected, 2);
    assert_eq!(report.words, 0);
}

/// A lone consonant letter is its sound in letter-sound mode (the Slovenian
/// part; Croatian and Serbian are with their own tests)
#[test]
fn a_lone_consonant_letter_is_its_sound_in_letter_sound_mode() {
    let sl = slovenian();
    assert_eq!(sounds(&sl, "izbrisano v"), "izbr'i:sanO v'ə");
    assert_eq!(sounds(&sl, "B C"), "b'ə c'ə");
    assert_eq!(sounds(&sl, "v šoli"), phones(&sl, "v šoli"));
    assert_eq!(sounds(&sl, "SP"), phones(&sl, "SP"));
}

/// Slovenian spellings told apart by the clause (November 2026)
#[test]
fn slovenian_spellings_told_apart_by_the_clause_november_2026() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // A noun's genitive plural: obvestíl, final [l]; the participle:
            // obvéstil, final [w]. The participle has an auxiliary.
            ("Ni obvestil", "ni ObvEst'i:l"),
            ("Trenutno ni obvestil", "trEn'u:tnO ni ObvEst'i:l"),
            ("Nimam obvestil", "n'i:mam ObvEst'i:l"),
            ("Brez obvestil", "brEz ObvEst'i:l"),
            ("Pet obvestil", "p'é:t ObvEst'i:l"),
            ("Veliko obvestil", "vEl'i:kO ObvEst'i:l"),
            ("Nimate novih obvestil", "n'i:matE n'O:vih ObvEst'i:l"),
            ("Seznam obvestil", "sEzn'am ObvEst'i:l"),
            ("Seznam obvestil je prazen", "sEzn'am ObvEst'i:l jE pr'a:zən"),
            ("Nisem dobil obvestil", "n'i:sEm dOb'i:w ObvEst'i:l"),
            ("Ni vozil na cesti", "ni vOz'i:l na c'é:sti"),
            ("Vozil je bilo malo", "vOz'i:l jE bil'ó: m'a:lO"),
            ("On me ni obvestil", "'On mE ni Obv'é:stiw"),
            ("Ni me obvestil", "ni mE Obv'é:stiw"),
            ("Janez ni obvestil", "j'a:nEs ni Obv'é:stiw"),
            ("Ni obvestil policije", "ni Obv'é:stiw pOlic'i:jE"),
            ("Janez je obvestil Marijo", "j'a:nEs jE Obv'é:stiw mar'i:jO"),
            ("Obvestil me je", "Obv'é:stiw mE jE"),
            ("Obvestil je policijo", "Obv'é:stiw jE pOlic'i:jO"),
            ("Prišel je in obvestil policijo", "priš'əw jE in Obv'é:stiw pOlic'i:jO"),
            ("Ni opravil", "ni Opr'a:viw"),
            ("Vozil je hitro", "v'O:ziw jE h'i:trO"),
            // ... also where the accent is the same and only the l differs
            ("Odpri predal", "Otpr'i prEd'a:l"),
            ("v predal", "u prEd'a:l"),
            // ... and a noun's [l] after a schwa or in one syllable
            ("Brez stekel", "brE st'E:kəl"),
            ("Stekel je domov", "st'é:kəw jE dOm'O:w"),
            // del "part" and moral the verb have nothing to tell apart
            ("To je del", "t'ó: jE d'é:l"),
            ("Moral je iti", "m'ó:raw jE 'i:ti"),
            // lahko: the adjective before a noun things are called light by, the
            // modal otherwise and before a verb
            ("On nosi lahko torbo", "'On n'ó:si l'a:hkO t'ó:rbO"),
            ("On nosi lahko torbo v roki", "'On n'ó:si l'a:hkO t'ó:rbO u r'O:ki"),
            ("On nosi lahko rdečo torbo", "'On n'ó:si l'a:hkO ərd'é:čO t'ó:rbO"),
            ("Nosim lahko torbo", "n'ó:sim l'a:hkO t'ó:rbO"),
            ("Pijem lahko vino", "p'i:jEm l'a:hkO v'i:nO"),
            ("Oblekla je lahko in hladno obleko", "Obl'é:kla jE l'a:hkO iŋ hl'a:dnO Obl'é:kO"),
            ("Zelo lahko torbo imam", "zEl'ó: l'a:hkO t'ó:rbO im'a:m"),
            ("Lahko vino je najboljše", "l'a:hkO v'i:nO jE najb'ó:lšE"),
            ("Lahko me pokličeš", "lahk'ó: mE pOkl'i:čEš"),
            ("Lahko torbo nosi", "l'a:hkO t'ó:rbO n'ó:si"),
            ("Lahko delo opravi vsak", "l'a:hkO d'é:lO Opr'a:vi us'a:k"),
            ("Lahko delo vsak opravi", "l'a:hkO d'é:lO us'a:k Opr'a:vi"),
            ("Lahko hrano kupite v trgovini", "lahk'ó: hr'a:nO k'u:pitE u tərgOv'i:ni"),
            ("Lahko hrano kupijo", "lahk'ó: hr'a:nO k'u:pijO"),
            ("Lahko delo danes opravimo", "lahk'ó: d'é:lO d'a:nəs Opr'a:vimO"),
            ("Lahko kuhinjo prepleskamo drugače", "lahk'ó: k'u:hinjO prEpl'é:skamO drug'a:čE"),
            ("Lahko kavo", "lahk'ó: k'a:vO"),
            ("Kupim lahko hrano", "k'u:pim lahk'ó: hr'a:nO"),
            ("Kupi lahko hrano", "k'u:pi lahk'ó: hr'a:nO"),
            ("Vidimo lahko delo umetnikov", "v'i:dimO lahk'ó: d'é:lO um'é:tnikOw"),
            ("To je lahko res", "t'ó: jE lahk'ó: r'é:s"),
            ("Ni lahko", "ni lahk'ó:"),
            (
                "Najbolj nazoren primer je lahko neko besedilo",
                "n'a:jbOl naz'ó:rən prim'é:r jE lahk'ó: n'é:kO bEsEd'i:lO",
            ),
            // je: the auxiliary "is", unstressed [jɛ] and stressed jè (SP 2001
            // poudarjeno jè), or jé "eats" (SSKJ² jésti jém, jé: long close e), a
            // listener's report of 2026-11-03: after ne, before a food object,
            // after an adverb of eating at the end of the clause, before a
            // utensil; "is" at the head of the clause, after to, a question word,
            // a clitic or a food nominative, and in the perfect tense
            ("On je krompir", "'On j'é: krOmp'i:r"),
            ("Ona je palačinke", "'O:na j'é: palač'i:ŋkE"),
            ("Marko zelo veliko je", "m'a:rkO zEl'ó: vEl'i:kO j'é:"),
            ("Marko zelo rad je palačinke", "m'a:rkO zEl'ó: r'at j'é: palač'i:ŋkE"),
            ("Mama je juho", "m'a:ma j'é: j'u:hO"),
            ("Pes je meso", "p'əs j'é: mEs'ó:"),
            ("Marko je sendvič", "m'a:rkO j'é: s'é:ndvič"),
            ("On je veliko sadja", "'On j'é: vEl'i:kO s'a:dja"),
            ("Otrok je nezdravo hrano", "Otr'Ok j'é: nEzdr'a:vO hr'a:nO"),
            ("Za kosilo običajno je korenčkovo juho", "za kOs'i:lO Obič'a:jnO j'é: kOr'é:nčkOvO j'u:hO"),
            ("Otrok ne je", "Otr'Ok nE j'é:"),
            ("naj ne je", "naj nE j'é:"),
            ("On rad je", "'On r'at j'é:"),
            ("On je z žlico", "'On j'é: žl'i:cO"),
            ("ki je z vilicami", "ki j'é: z v'i:licami"),
            ("On je priden", "'On jE pr'i:dən"),
            ("Ona je zelo pametna", "'O:na jE zEl'ó: p'a:mEtna"),
            ("Marko je zelo navdušen nad Laprdusom", "m'a:rkO jE zEl'ó: nawd'u:šEn nad l'a:pərdusOm"),
            ("Je to res", "j'E t'ó: r'é:s"),
            ("Je veliko solate", "j'E vEl'i:kO sOl'a:tE"),
            ("Kaj je", "k'a:j jE"),
            ("To je krompir", "t'ó: jE krOmp'i:r"),
            ("Kosilo je krompir", "kOs'i:lO jE krOmp'i:r"),
            ("Hrvoje je krompir", "hərv'OjE j'é: krOmp'i:r"),
            ("Priloga je krompir", "pril'ó:ga j'é: krOmp'i:r"), // any subject (the maintainer's word)
            ("Zdaj je kosilo", "zd'aj jE kOs'i:lO"),
            ("Jutri je kosilo", "j'u:tri jE kOs'i:lO"),
            ("V Ljubljani je kosilo ob dvanajstih", "u ljublj'a:ni jE kOs'i:lO Ob dv'a:najstih"),
            ("Videla je ribe", "v'i:dEla jE r'i:bE"),
            ("Suči je hrano darovala pred vsakim obedom", "s'u:či jE hr'a:nO darOv'a:la prEd us'a:kim Ob'é:dOm"),
            ("Zdelo se je", "zd'é:lO sE jE"),
            ("Strah ga je", "str'a:ɣ ga jE"),
            ("Lepo je", "lEp'ó: jE"),
            ("Tega je veliko", "t'é:ga jE vEl'i:kO"),
            ("Tako je", "tak'ó: jE"),
            ("On je", "'On jE"),
            // meni: the pronoun, the menu, the verb
            ("Ti si v meni", "t'i: si u m'E:ni"),
            ("Tudi meni", "t'u:di m'E:ni"),
            ("Daj meni", "d'aj m'E:ni"),
            ("To je meni znano", "t'ó: jE m'E:ni zn'a:nO"),
            ("Meni za to ni mar", "m'E:ni za t'ó: ni m'ar"),
            ("Meni pa je všeč", "m'E:ni pa jE uš'é:č"),
            ("Ponudili so meni", "pOn'u:dili sO m'E:ni"),
            ("Meni se odpre", "mEn'i: sE Otpr'E"),
            ("Meni je prazen", "mEn'i: jE pr'a:zən"),
            ("Meni za kosilo", "mEn'i: za kOs'i:lO"),
            ("Meni aplikacij", "mEn'i: aplik'a:cij"),
            ("Meni ima pet možnosti", "mEn'i: im'a: p'é:t m'ó:žnOsti"),
            ("Gumb meni", "g'u:mp mEn'i:"),
            ("Padajoči meni", "padaj'ó:či mEn'i:"),
            ("Restavracija ima dober meni", "rEstawr'a:cija im'a: d'ó:bər mEn'i:"),
            ("Kaj meni o tem", "k'a:j m'é:ni O t'é:m"),
            // roka: the hand; the name's or the deadline's when it cannot be one
            ("Čakam na Roka", "č'a:kam na r'ó:ka"),
            ("Roka že ni", "r'ó:ka žE n'i:"),
            ("Roka ni več", "r'ó:ka ni v'Eč"),
            ("Roka ni doma", "r'ó:ka ni dOm'a:"),
            ("Roka ni bilo na sestanku", "r'ó:ka ni bil'ó: na sEst'a:ŋku"),
            ("Roka ne bo več", "r'ó:ka nE bO v'Eč"),
            ("Roka nisem videl", "r'ó:ka n'i:sEm v'i:dEw"),
            ("Roka poznam", "r'ó:ka pOzn'a:m"),
            ("Roka so našli", "r'ó:ka sO n'a:šli"),
            ("Roka ni zlomljena", "r'O:ka ni zl'ó:mljEna"),
            ("Roka ni več bolna", "r'O:ka ni v'Eč b'O:lna"),
            ("Roka ne bo bolela", "r'O:ka nE bO bOl'é:la"),
            ("Roka me boli", "r'O:ka mE bOl'i:"),
            ("Desna roka", "d'é:sna r'O:ka"),
            ("Rok je potekel", "r'ó:k jE pOt'é:kəw"),
            // roki and roke: the hands, or deadlines
            ("Dobavni roki so kratki", "dOb'a:wni r'ó:ki sO kr'a:tki"),
            ("Plačilni roki", "plač'i:lni r'ó:ki"),
            ("Roki za prijavo", "r'ó:ki za prij'a:vO"),
            ("z roki", "z r'ó:ki"),
            ("Spremenili so odpovedne roke", "sprEmEn'i:li sO Otp'O:vEdnE r'ó:kE"),
            ("V roki drži knjigo", "u r'O:ki dərž'i: knj'i:gO"),
            ("Obe roki", "Ob'é: r'O:ki"),
            ("Roki sta hladni", "r'O:ki sta hl'a:dni"),
            ("Dvignil je roke", "dv'i:gniw jE r'O:kE"),
            // odstranjevalnik, as napajalnik and zaganjalnik, in every case
            ("odstranjevalnik", "OtstranjEv'a:lnik"),
            ("odstranjevalnika", "OtstranjEv'a:lnika"),
            ("odstranjevalniku", "OtstranjEv'a:lniku"),
            ("odstranjevalnikom", "OtstranjEv'a:lnikOm"),
            ("odstranjevalniki", "OtstranjEv'a:lniki"),
            ("odstranjevalnikov", "OtstranjEv'a:lnikOw"),
        ],
    );

    // ł in a notation keeps the l where the rules would say [w], w says [w]
    // where they would say [l]: the user's own entries can say so too
    assert_eq!(phones(&frontend, "bruhoberal"), "bruhOb'é:raw");
    let mut custom = slovenian();
    custom.set_user_lexicon(user(r#"{"entries":[{"word":"bruhobérał","language":"sl"}]}"#));
    assert_eq!(phones(&custom, "bruhoberal"), "bruhOb'é:ral");
}

// A native listener's report: a verb's present before or after its object clitic,
// meni after any adjective (SSKJ2, SP: veselí, žalostí; Sloleks for the adjectives)
/// Slovenian verbs by their object clitic and meni after an adjective
#[test]
fn slovenian_verbs_by_their_object_clitic_and_meni_after_an_adjective() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            ("Veseli me", "vEsEl'i: mE"),
            ("Žalosti me", "žalOst'i: mE"),
            ("Veseli nas", "vEsEl'i: nas"),
            ("To me žalosti", "t'ó: mE žalOst'i:"),
            ("Zelo me žalosti", "zEl'ó: mE žalOst'i:"),
            ("To me veseli", "t'ó: mE vEsEl'i:"),
            // The nouns and adjectives of the same spellings stay
            ("Polna je žalosti", "p'O:wna jE ž'a:lOsti"),
            ("Žalosti ne morem skriti", "ž'a:lOsti nE m'ó:rEm skr'i:ti"),
            ("Otroci so veseli", "Otr'O:ci sO vEs'é:li"),
            // After an auxiliary and a clitic only a participle could be the verb
            ("So nas levi", "s'O nas l'é:vi"),
            // ... but si is the reflexive
            ("Si ga deli", "s'i ga dEl'i:"),
            // The menu
            ("zaprti meni nedavno", "zap'ərti mEn'i: nEd'a:wnO"),
            ("Zaprti meni nedavno", "zap'ərti mEn'i: nEd'a:wnO"),
            ("Zaprti meni", "zap'ərti mEn'i:"),
            ("odprti meni", "Otp'ərti mEn'i:"),
            ("Skriti meni", "skr'i:ti mEn'i:"),
            ("Najboljši meni", "najb'ó:lši mEn'i:"),
            ("Dobri meni", "d'O:bri mEn'i:"),
            ("Zaprt meni", "zap'ərt mEn'i:"),
            ("Meni zaprt", "mEn'i: zap'ərt"),
            // The pronoun and the verb
            ("Tudi meni", "t'u:di m'E:ni"),
            ("Daj meni", "d'aj m'E:ni"),
            ("K meni", "k m'E:ni"),
            ("Meni se zdi", "m'E:ni sE zd'i:"),
            ("Minister meni", "min'i:stər m'é:ni"),
            ("Pravi meni", "pr'a:vi m'é:ni"),
            ("V knjigi meni", "u knj'i:gi m'é:ni"),
            ("Ki meni", "ki m'é:ni"),
        ],
    );
}

// The same report: the prefix of a web address letter by letter, www as ve ve ve,
// and w with its v by the spelling mode, in every language
/// Web address prefixes and the letter w
#[test]
fn web_address_prefixes_and_the_letter_w() {
    let sl = slovenian();
    for text in ["www", "WWW", "Www"] {
        assert_eq!(phones(&sl, text), "v'é: v'é: v'é:", "{text}");
    }
    for text in ["https", "HTTPS"] {
        assert_eq!(phones(&sl, text), "h'a: t'é: t'é: p'é: 'Es", "{text}");
    }
    for text in ["http", "HTTP"] {
        assert_eq!(phones(&sl, text), "h'a: t'é: t'é: p'é:", "{text}");
    }
    for text in ["ftp", "FTP"] {
        assert_eq!(phones(&sl, text), "'Ef t'é: p'é:", "{text}");
    }
    assert_eq!(
        phones(&sl, "https://www.index.hr"),
        "h'a: t'é: t'é: p'é: 'Es v'é: v'é: v'é: p'i:ka 'i:ndEks p'i:ka ha'Er"
    );
    assert_eq!(phones(&sl, "w"), "dv'ó:jni v'é:");
    assert_eq!(sounds(&sl, "w"), "dv'ó:jni v'ə");
    assert_eq!(sounds(&sl, "Pritisni W"), "prit'i:sni dv'ó:jni v'ə");
    // An abbreviation has one accent, on its last letter, and two are two
    assert_eq!(phones(&sl, "BMW"), "béEmdvójniv'é:");
    assert_eq!(phones(&sl, "USB"), "uEzb'é:");
    assert_eq!(phones(&sl, "USB PDF"), "uEzb'é: pédé'Ef");
    assert_eq!(phones(&sl, "s.a.r.s."), "EsaEr'Es");

    for language in [Language::Croatian, Language::Serbian, Language::Bosnian] {
        let other = Frontend::new(language);
        assert_eq!(phones(&other, "www"), "v'e v'e v'e");
        assert_eq!(phones(&other, "HTTPS"), "h'a t'e t'e p'e 'es");
        assert_eq!(phones(&other, "ftp"), "'ef t'e p'e");
        assert_eq!(phones(&other, "w"), "d'uplo v'e");
        assert_eq!(sounds(&other, "w"), "d'uplo v'ə");
        assert_eq!(phones(&other, "BMW"), "b'e 'em d'uplo v'e");
    }
}

// A question opening with lahko and a noun asks whether something can be done,
// a statement says something of the light thing (a native listener, November 2026)
/// Slovenian lahko at the head of a question and of a statement
#[test]
fn slovenian_lahko_at_the_head_of_a_question_and_of_a_statement() {
    let frontend = slovenian();
    let question = |text: &str| says(&frontend, text, Punctuation::Question);
    assert_eq!(question("Lahko delo opravi vsak"), "lahk'ó: d'é:lO Opr'a:vi us'a:k");
    assert_eq!(phones(&frontend, "Lahko delo opravi vsak"), "l'a:hkO d'é:lO Opr'a:vi us'a:k");
    assert_eq!(question("Lahko torbo nosi"), "lahk'ó: t'ó:rbO n'ó:si");
    assert_eq!(question("Lahko delo"), "lahk'ó: d'é:lO");
    assert_eq!(phones(&frontend, "Lahko delo"), "l'a:hkO d'é:lO");
    assert_eq!(question("Lahko noč"), "l'a:hkO n'ó:č");
    // The noun as the subject of a copula has no modal to ask about
    assert_eq!(question("Lahko vino je najboljše"), "l'a:hkO v'i:nO jE najb'ó:lšE");
    // Inside a clause nothing changes
    assert_eq!(question("On nosi lahko torbo"), "'On n'ó:si l'a:hkO t'ó:rbO");
    // The mark says so in a clause before a comma
    let marked = format!("Lahko delo opravi vsak{IN_QUESTION_MARK}");
    assert_eq!(says(&frontend, &marked, Punctuation::Comma), "lahk'ó: d'é:lO Opr'a:vi us'a:k");
    // ... also next to the other mark, in either order
    let marked = format!("{marked}{BEFORE_COMPLEMENT_MARK}");
    assert_eq!(says(&frontend, &marked, Punctuation::Comma), "lahk'ó: d'é:lO Opr'a:vi us'a:k");
}

// The clause after a comma, which one clause does not show
/// A clause mark after a Slovenian comma says a complement follows
#[test]
fn a_clause_mark_after_a_slovenian_comma_says_a_complement_follows() {
    let frontend = slovenian();
    let marked = |text: &str| says(&frontend, &format!("{text}{BEFORE_COMPLEMENT_MARK}"), Punctuation::Comma);
    assert_eq!(marked("Meni"), "m'é:ni");
    assert_eq!(marked("Ta meni"), "t'a: m'é:ni");
    assert_eq!(marked("Tudi meni"), "t'u:di m'é:ni");
    assert_eq!(marked("Minister meni"), "min'i:stər m'é:ni");
    assert_eq!(marked("k meni"), "k m'E:ni");
    assert_eq!(marked("Odprite meni"), "Otpr'i:tE mEn'i:");
    assert_eq!(marked("Ni obvestil"), "ni Obv'é:stiw");
    assert_eq!(marked("Ni sporočil"), "ni spOr'ó:čiw");
    assert_eq!(marked("Ni zdravil"), "ni zdrav'i:l"); // the noun is 95 % of this spelling
    assert_eq!(phones(&frontend, "Meni"), "mEn'i:");
    assert!(opens_complement_clause(&chars("da je prav")));
    assert!(opens_complement_clause(&chars(" Kako je")));
    assert!(!opens_complement_clause(&chars("gumb")));
    assert!(!opens_complement_clause(&chars("dan")));
}

/// Slovenian listener's report of November 2026
#[test]
fn slovenian_listeners_report_of_november_2026() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // afna is the noun, also the name of @; the verb only next to se
            ("afna", "'a:fna"),
            ("janez afna gmail", "j'a:nEs 'a:fna gma'il"),
            ("To se afna", "t'ó: sE afn'a:"),
            // lahko: the adjective with evidence for it, otherwise the modal
            ("Jesti moraš lahko hrano", "j'é:sti m'ó:raš l'a:hkO hr'a:nO"),
            ("Prosim daj mi lahko nalogo", "pr'ó:sim d'aj mi l'a:hkO nal'ó:gO"),
            ("Lahko noč", "l'a:hkO n'ó:č"),
            ("Z lahko nogo", "z l'a:hkO n'O:gO"),
            ("To je lahko delo", "t'ó: jE l'a:hkO d'é:lO"),
            ("Ali lahko prideš", "ali lahk'ó: pr'i:dEš"),
            ("Lahko ti skuham kavo", "lahk'ó: ti sk'u:ham k'a:vO"),
            ("Izberete lahko nalogo", "izb'E:rEtE lahk'ó: nal'ó:gO"),
            ("Lahko hrano kupite v trgovini", "lahk'ó: hr'a:nO k'u:pitE u tərgOv'i:ni"),
            ("Uporabnik mora lahko dostopati", "upOr'a:bnik m'ó:ra lahk'ó: dOst'ó:pati"),
            ("To je lahko", "t'ó: jE lahk'ó:"),
            // gori: the verb, but the noun or the adverb after these
            ("Na gori gori ogenj", "na g'O:ri gOr'i: 'O:gən"),
            ("Peč gori vsak dan", "p'é:č gOr'i: us'a:k d'a:n"),
            ("Gori", "gOr'i:"),
            ("V Kranjski Gori", "u kr'a:nski g'O:ri"),
            ("Tam gori", "t'am g'O:ri"),
            // The adverb gôri "up there" at the head of a clause before a place
            // the clause goes on from, and before a form of biti (a listener's
            // examples of 2026-11-03; SSKJ² gôri prisl.); the fire in an
            // exclamation and where nothing follows the place
            ("Gori na gori gori", "g'O:ri na g'O:ri gOr'i:"),
            ("Gori na seniku je lepo", "g'O:ri na sEn'i:ku jE lEp'ó:"),
            ("Gori na podstrešju je krama", "g'O:ri na pOtstr'é:šju jE kr'a:ma"),
            ("Gori je lepo", "g'O:ri jE lEp'ó:"),
            ("Hiša stoji na hribu in gori je veliko sonca", "h'i:ša stOj'i: na hr'i:bu iŋ g'O:ri jE vEl'i:kO s'ó:nca"),
            ("Gori v hiši", "gOr'i: u h'i:ši"),
            ("Gori s plamenom brez dima", "gOr'i: s plam'é:nOm brEz d'i:ma"),
            // A surname in -e after a given name, on its open last e
            ("Jani Perše", "j'a:ni pErš'E"),
            ("Drago Lekše", "dr'a:gO lEkš'E"),
            ("Vesna Smode", "v'é:sna smOd'E"),
            ("Marija Smode", "mar'i:ja smOd'E"),
            ("Janez Stare", "j'a:nEs star'E"),
            ("Smode je rekla", "sm'ó:dE jE r'E:kla"), // alone: too many foreign names
            ("Ivan Lapajne", "'i:van lap'a:jnE"),     // Sloleks: Lapájne
            ("Miroslava Krleže", "m'i:rOslava kərl'é:žE"), // a genitive
            ("Janez Novak", "j'a:nEs nOv'a:k"),
            // posodobiti on the i in every form, as the listener answered; the
            // passive participle, which has no i to carry it, as the dictionaries
            ("Aplikacija se je posodobila", "aplik'a:cija sE jE pOsOdOb'i:la"),
            ("Posodobil sem", "pOsOdOb'i:w səm"),
            ("posodobili", "pOsOdOb'i:li"),
            ("Aplikacija naj se posodobi", "aplik'a:cija naj sE pOsOdOb'i:"), // alone: the command posodôbi
            ("posodobiti", "pOsOdOb'i:ti"),
            ("posodobim", "pOsOdOb'i:m"),
            ("Zdaj jo posodobite", "zd'aj jO pOsOdOb'i:tE"), // alone: the command posodôbite
            ("posodobijo", "pOsOdOb'i:jO"),
            ("posodobljen", "pOsOd'O:bljEn"),
            ("posodobitev", "pOsOdOb'i:təw"),
            // The final l of a participle is [w], also stressed on the last vowel
            // (Sloleks, SP); a loanword and a noun's genitive plural keep [l]
            ("želel", "žEl'é:w"),
            ("sledil", "slEd'i:w"),
            ("rodil", "rOd'i:w"),
            ("učil", "uč'i:w"),
            ("gradil", "grad'i:w"),
            ("posodobil", "pOsOdOb'i:w"),
            ("trdil", "tərd'i:w"), // a stem with the l, the syllabic r
            ("oddal", "Odd'aw"),   // dd one sound in the transcription
            ("zatrl", "zat'ərw"),
            ("je vozil", "j'E v'O:ziw"), // the auxiliary stressed at the head: jè, short open (SP 2001)
            ("brez vozil", "brEz vOz'i:l"),
            ("kanal", "kan'a:l"),
            ("model", "mOd'é:l"),
            ("april", "apr'i:l"),
            ("avtomobil", "awtOmOb'i:l"),
            ("del", "d'é:l"),
            // Loanwords and other words with no [-u̯] in SSKJ² and SP keep [l],
            // with the stress anywhere (the listener: ángel, módul, móbitel);
            // the words the dictionaries mark, and the participles, are [w]
            ("angel", "'a:ŋgEl"),
            ("modul", "m'ó:dul"),
            ("mobitel", "m'ó:bitEl"),
            ("konzul", "k'ó:nzul"),
            ("stil", "st'i:l"),
            ("strel", "str'é:l"),
            ("ansambel", "ans'a:mbəl"),
            ("kabel", "k'a:bəl"),
            ("cikel", "c'i:kəl"),
            ("dizel", "d'i:zəl"),
            ("sil", "s'i:l"), // genitive plurals: the l is the stem's
            ("škatel", "šk'a:təl"),
            ("Izrael", "'i:zraEl"),
            ("Daniel", "d'a:nijEl"),
            ("Michael", "m'i:chaEl"), // no lexicon: a capital is a foreign name
            ("Paul", "p'a:ul"),
            ("email", "Em'a:il"),
            ("misel", "m'i:səw"),
            ("posel", "p'O:səw"),
            ("topel", "t'O:pəw"),
            ("mrzel", "m'ərzəw"),
            ("vesel", "vEs'é:w"),
            ("žival", "živ'a:w"),
            ("rekel", "r'é:kəw"),
            ("videl", "v'i:dEw"),
            ("delal", "d'é:law"),
            ("bil", "b'i:w"),
            ("zadeklamiral", "zadEklam'i:raw"), // no lexicon, small letters: a participle
            // seznam with the open e in every form
            ("seznam", "sEzn'am"),
            ("na seznamu", "na sEzn'a:mu"),
            ("seznamih", "sEzn'a:mih"),
            // roka: the hand, open; the deadline's genitive and the name, close
            ("Roka je hladna", "r'O:ka jE hl'a:dna"),
            ("Njegova roka", "njEg'O:va r'O:ka"),
            ("Daj mi roko", "d'aj mi r'O:kO"),
            ("do roka", "dO r'ó:ka"),
            ("vojaškega roka", "vOj'a:škEga r'ó:ka"),
            ("To je Roka hiša", "t'ó: jE r'ó:ka h'i:ša"),
            // meni: the menu, the pronoun, the verb of the news
            ("pod meni", "pOd mEn'i:"),
            ("Pojdi v meni", "p'ó:jdi u mEn'i:"),
            ("Kliknite na meni", "kl'i:knitE na mEn'i:"),
            ("Glavni meni", "gl'a:wni mEn'i:"),
            ("Odpri meni", "Otpr'i mEn'i:"),
            ("Meni Start", "mEn'i: st'a:rt"),
            ("v meni Dokumenti", "u mEn'i: dOkum'E:nti"),
            ("podmeni", "pOdmEn'i:"),
            ("Meni je všeč", "m'E:ni jE uš'é:č"),
            ("K meni", "k m'E:ni"),
            ("V meni je jeza", "u m'E:ni jE j'é:za"),
            ("meni Kučan", "m'é:ni k'u:čan"),
            ("Franco Frattini meni", "fr'a:ncO fr'a:ttini m'é:ni"),
        ],
    );
    // Gori na podstrešju! is the fire: the exclamation mark keeps the verb
    assert_eq!(says(&frontend, "Gori na podstrešju", Punctuation::Exclamation), "gOr'i: na pOtstr'é:šju");
    assert_eq!(
        says(&frontend, "Gori na podstrešju je krama", Punctuation::Exclamation),
        "gOr'i: na pOtstr'é:šju jE kr'a:ma"
    );
}

/// Slovenian listener's report of 2026-11-03: commands, veseli se, Pero
#[test]
fn slovenian_listeners_report_of_2026_11_03_commands_veseli_se_pero() {
    let frontend = slovenian();
    // The verbs of commands at the head of a clause are the imperative, which
    // retracts the stress (SP 2001: namestíti namésti -íte, gradíti grádi,
    // vršíti vŕši, posodôbiti); elsewhere their spelling is the present
    check(
        &frontend,
        &[
            ("Posodobi", "pOsOd'O:bi"),
            ("Posodobite", "pOsOdOb'i:tE"), // the listener: posodobíte as a command too
            ("Nadgradi", "nadgr'a:di"),
            ("Namesti", "nam'é:sti"),
            ("Namestite", "namEst'i:tE"), // SP namésti -íte
            ("Izvrši", "izv'ərši"),
            ("Shrani", "shr'a:ni"),     // the same in both; by the lexicon
            ("Dovolite", "dOvOl'i:tE"), // SP dovôli -íte; the present dovólite
            ("Prijavi se", "prij'a:vi sE"),
            ("Nemudoma posodobi to aplikacijo", "nEm'u:dOma pOsOd'O:bi t'ó: aplik'a:cijO"),
            ("Takoj nadgradi Dodavo", "tak'ó:j nadgr'a:di dOd'a:vO"),
            ("Aplikacija naj se posodobi", "aplik'a:cija naj sE pOsOdOb'i:"),
            ("To moraš nadgraditi", "t'ó: m'ó:raš nadgrad'i:ti"),
            ("To naj se takoj izvrši", "t'ó: naj sE tak'ó:j izvərš'i:"),
            ("Program nadgradi sistem", "prOgr'a:m nadgrad'i: sist'é:m"),
            // veseli: an object clitic anywhere before it is the verb of pleasing
            ("Me zelo veseli vaš obisk", "m'é: zEl'ó: vEsEl'i: v'aš Ob'i:sk"),
            ("To me srčno veseli", "t'ó: mE s'ərčnO vEsEl'i:"),
            ("In me veseli", "in mE vEsEl'i:"),
            // an object clitic right after it, also after a conjunction (the sample text)
            ("rojen sem bil lani in veseli me", "rOj'En səm b'i:w l'a:ni in vEsEl'i: mE"),
            ("To veseli nas", "t'ó: vEsEl'i: nas"),
            ("Toda žalosti nas", "t'ó:da žalOst'i: nas"),
            ("Veseli me", "vEsEl'i: mE"),
            ("Otroci so ga veseli", "Otr'O:ci sO ga vEs'é:li"),
            ("Veseli se novega dneva", "vEs'é:li sE n'O:vEga dn'é:va"), // a statement: the present
            // The pen; the name only inside a clause and as an address (below)
            ("Lahek si kot pero", "l'a:hək si k'Ot pEr'ó:"),
            ("ki ga vsi kličejo Pero bo tudi prišel", "ki ga us'i kl'i:čEjO p'E:rO bO t'u:di priš'əw"),
            ("Dragi Pero", "dr'a:gi p'E:rO"),
            ("Pero Novak je prišel", "p'E:rO nOv'a:k jE priš'əw"), // before a surname, at the head too
            ("Pero je na mizi", "pEr'ó: jE na m'i:zi"),
        ],
    );
    // Veseli se! The imperative vesêli (SP vesêli -íte) by the exclamation mark,
    // of its own clause or, as the engine's mark, of the sentence
    assert_eq!(says(&frontend, "Veseli se", Punctuation::Exclamation), "vEs'E:li sE");
    assert_eq!(says(&frontend, "Veseli se novega dneva", Punctuation::Exclamation), "vEs'E:li sE n'O:vEga dn'é:va");
    let marked = |text: &str, mark: char| says(&frontend, &format!("{text}{mark}"), Punctuation::Comma);
    assert_eq!(marked("Veseli se", IN_EXCLAMATION_MARK), "vEs'E:li sE");
    assert_eq!(says(&frontend, "Veseli me", Punctuation::Exclamation), "vEsEl'i: mE");
    // Govori, da bo prišel: the verb of saying before its clause of da is the present
    assert_eq!(marked("Govori", BEFORE_COMPLEMENT_MARK), "gOvOr'i:");
    assert_eq!(says(&frontend, "Govori", Punctuation::Comma), "gOv'O:ri");
    assert_eq!(marked("Poskrbi", BEFORE_COMPLEMENT_MARK), "pOsk'ərbi"); // an instruction: the imperative
    // Pero, kje si? Pero, pridi sem. The address, marked by the engine
    assert_eq!(marked("Pero", VOCATIVE_MARK), "p'E:rO");
    assert_eq!(says(&frontend, "Pero", Punctuation::Comma), "pEr'ó:");
    // The engine's test of the mark: one capitalized word before a comma
    assert!(is_address(&chars("Pero")));
    assert!(is_address(&chars("Dragi")));
    assert!(!is_address(&chars("Dragi Pero")));
    assert!(!is_address(&chars("pero")));
    assert!(!is_address(&chars("USB")));
}

/// Slovenian listener's report of 2026-11-03: meni to me, menu phrases, podari, vprašaj
#[test]
fn slovenian_listeners_report_of_2026_11_03_meni_to_me_menu_phrases_podari_vprasaj() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // The pronoun mêni (SSKJ², SP: jàz, mêne, mêni; the open e): capitalized at
            // the head of its clause before a word, before a verb that gives or tells to
            // someone, before an auxiliary
            ("Meni daj žogo", "m'E:ni d'aj ž'ó:gO"),
            ("Meni prinesi kavo", "m'E:ni prin'E:si k'a:vO"),
            ("Meni podari ljubezen", "m'E:ni pOd'a:ri ljub'é:zən"), // and the command podári
            ("Meni bo hladno", "m'E:ni bO hl'a:dnO"),
            ("Meni so povedali", "m'E:ni sO pOv'é:dali"),
            ("meni daj žogo", "m'E:ni d'aj ž'ó:gO"),
            ("potem meni prinesi kavo", "pOt'é:m m'E:ni prin'E:si k'a:vO"),
            ("Meni pomoč ni potrebna", "m'E:ni pOm'ó:č ni pOtr'é:bna"),
            ("Meni pa je všeč", "m'E:ni pa jE uš'é:č"),
            // The menu mení after an acronym or the kind of a menu, before the name of one
            ("NVDA meni", "Envédé'a: mEn'i:"),
            ("Odpri NVDA meni", "Otpr'i Envédé'a: mEn'i:"),
            ("JAWS meni", "j'a:ws mEn'i:"),
            ("kontekst meni", "kOnt'é:kst mEn'i:"),
            ("context meni", "cOnt'é:kst mEn'i:"),
            ("meni start", "mEn'i: st'a:rt"),
            // The name in quotation marks or brackets, silent in the engine or
            // named by a screen reader with its punctuation on (a listener's
            // report of 2026-11-03)
            ("meni \"start\"", "mEn'i: st'a:rt"),
            ("meni »start«", "mEn'i: st'a:rt"),
            ("meni narekovaj start narekovaj", "mEn'i: narEkOv'a:j st'a:rt narEkOv'a:j"),
            ("Meni narekovaj Start narekovaj", "mEn'i: narEkOv'a:j st'a:rt narEkOv'a:j"),
            (
                "meni dvojni začetni narekovaj start dvojni zaključni narekovaj",
                "mEn'i: dv'ó:jni zač'é:tni narEkOv'a:j st'a:rt dv'ó:jni zaklj'u:čni narEkOv'a:j",
            ),
            ("meni okrogli oklepaj start okrogli zaklepaj", "mEn'i: Okr'ó:gli OklEp'a:j st'a:rt Okr'ó:gli zaklEp'a:j"),
            ("narekovaj start narekovaj meni", "narEkOv'a:j st'a:rt narEkOv'a:j mEn'i:"),
            ("Meni narekovaj", "m'E:ni narEkOv'a:j"),
            // The numerals of the number converter since 2026-11-03: oblique
            // forms, ordinals and multiplicatives are number words (Meni 21 in
            // the genitive), the lexicon accents them, and a number before leti
            // makes it the noun across a split compound
            ("Meni enaindvajsetega", "mEn'i: 'E:naindv'a:jsEtEga"),
            ("pred enaindvajsetimi leti", "prEd 'E:naindv'a:jsEtimi l'é:ti"),
            ("pred petimi leti", "prEt p'E:timi l'é:ti"),
            ("v petih urah", "u p'E:tih 'u:rah"),
            ("prvo mesto", "p'ərvO m'é:stO"),
            ("na tretjem mestu", "na tr'é:tjEm m'é:stu"),
            ("dvakrat", "dv'a:krat"),
            ("enkrat", "'E:ŋkrat"),
            ("dvoletni", "dvó:l'é:tni"),
            ("desetletnica", "dEsé:tl'é:tnica"),
            ("minus pet stopinj Celzija", "m'i:nus p'é:t stOp'i:n c'é:lzija"),
            ("start meni", "st'a:rt mEn'i:"),
            ("Start meni", "st'a:rt mEn'i:"),
            ("začetni meni", "zač'é:tni mEn'i:"),
            ("Pojdi v start meni", "p'ó:jdi u st'a:rt mEn'i:"),
            ("Meni ikona", "mEn'i: ik'ó:na"),
            // Meni 1, Meni 2: a menu named by a number with nothing after it (the
            // listener's answer); with more words after it the pronoun
            ("Meni ena", "mEn'i: 'E:na"),
            ("Meni dva", "mEn'i: dv'a:"),
            ("Meni enaindvajset", "mEn'i: 'E:naindv'a:jsEt"),
            ("Meni ena stvar ni jasna", "m'E:ni 'E:na stv'a:r ni j'a:sna"),
            // The verb méni stays small after a comma before its subject, and elsewhere
            ("meni Kučan", "m'é:ni k'u:čan"),
            ("Tako meni župan", "tak'ó: m'é:ni žup'an"),
            ("Kaj meni o tem", "k'a:j m'é:ni O t'é:m"),
            // podari: the command at the head, also after Meni; the present after naj
            // and after a subject (SP 2001 podaríti -ím, podári -íte)
            ("On mi naj podari to stvar", "'On mi naj pOdar'i: t'ó: stv'a:r"),
            ("Program podari sistemu čas", "prOgr'a:m pOdar'i: sist'é:mu č'as"),
            ("Nam pokaži pot", "n'am pOk'a:ži p'ó:t"),
            // vprašaj the command (SP vprášaj -te) unless the question mark is meant
            ("Vprašaj me", "upr'a:šaj mE"),
            ("Vedno me vprašaj", "v'é:dnO mE upr'a:šaj"),
            ("Postavi pod vprašaj", "pOst'a:vi pOd upraš'a:j"),
            ("Napiši vprašaj", "nap'i:ši upraš'a:j"),
            ("velik vprašaj", "v'E:lik upraš'a:j"),
            ("imam eno vprašanje", "im'a:m 'E:nO upraš'a:njE"),
            // the participles on the first syllable in every form (SP vprášal -ála
            // tudi vprášala; the listener's reading); the prefixed verbs already were
            ("Vprašala si me", "upr'a:šala si mE"),
            ("Vprašali so me", "upr'a:šali sO mE"),
            ("Vprašal sem ga", "upr'a:šaw səm ga"),
            ("povprašala je", "pOwpr'a:šala jE"),
        ],
    );
    // Podari mi to! The command at the head of a clause
    assert_eq!(says(&frontend, "Podari mi to", Punctuation::Exclamation), "pOd'a:ri mi t'ó:");
    // Vprašaj alone: the name of the character ? (a screen reader's), the command
    // with an exclamation mark; last in a question it is the character too (NVDA
    // reads Kako si? as Kako si vprašaj?), but not after an object clitic
    assert_eq!(says(&frontend, "Vprašaj", Punctuation::None), "upraš'a:j");
    assert_eq!(says(&frontend, "Vprašaj", Punctuation::Exclamation), "upr'a:šaj");
    assert_eq!(says(&frontend, "Kako si vprašaj", Punctuation::Question), "kak'ó: si upraš'a:j");
    assert_eq!(says(&frontend, "Vedno me vprašaj", Punctuation::Question), "v'é:dnO mE upr'a:šaj");
    // Across a comma: SDS meni, da je prav is the news verb with an acronym for its
    // subject; Meni tudi, da / Meni pa, da bo too, with a particle before the clause
    let marked = |text: &str| says(&frontend, &format!("{text}{BEFORE_COMPLEMENT_MARK}"), Punctuation::Comma);
    assert_eq!(marked("SDS meni"), "Ezdé'Es m'é:ni");
    assert_eq!(marked("Meni tudi"), "m'é:ni t'u:di");
    assert_eq!(marked("Meni pa"), "m'é:ni p'a:");
    assert_eq!(marked("Vedno me vprašaj"), "v'é:dnO mE upr'a:šaj");
}

/// Slovenian samo distinguishes solitude from only
#[test]
fn slovenian_samo_distinguishes_solitude_from_only() {
    let frontend = slovenian();
    let cases = [
        ("Pustil sem čisto samo", "Pustil sem čisto sámo"),
        ("čeprav samo za deset minut", "čeprav samó za deset minut"),
        ("Pustil sem jo samo doma", "Pustil sem jo samó doma"), // ambiguous: retain particle
        ("sem te pustil samo", "sem te pustil sámo"),
        ("eno samo besedo", "eno sámo besedo"),
        ("samo po sebi", "sámo po sebi"),
        ("samo od sebe", "sámo od sebe"),
        ("presenetila samo sebe", "presenetila sámo sebe"),
        ("ne samo sebe", "ne samó sebe"),
        ("samo eno besedo", "samó eno besedo"),
        ("čisto samo zate", "čisto samó zate"),
    ];
    for (plain, accented) in cases {
        assert_eq!(phones(&frontend, plain), phones(&frontend, accented), "{plain}");
    }

    // The name Sámo (SP 2001: Sámo -a, oseb. i.; a listener's report of
    // 2026-11-10): an address, marked by the engine (Samo, pridi sem. Hvala,
    // Samo.), a capitalized Samo inside a clause, and the subject of a man's
    // perfect or future at the head of a sentence (Samo je prišel: the
    // listener's rule); otherwise the head of a clause stays samó, "only",
    // also before a capitalized word
    let marked = format!("Samo{VOCATIVE_MARK}");
    assert_eq!(says(&frontend, &marked, Punctuation::Comma), "s'a:mO");
    assert_eq!(says(&frontend, &marked, Punctuation::Period), "s'a:mO");
    assert_eq!(says(&frontend, "Samo", Punctuation::Comma), "sam'ó:");
    assert!(is_address(&chars("Samo")));
    check(
        &frontend,
        &[
            ("Dragi Samo", "dr'a:gi s'a:mO"),
            ("To je rekel Samo", "t'ó: jE r'é:kəw s'a:mO"),
            ("moj prijatelj Samo je prišel", "m'ó:j prij'a:tEl s'a:mO jE priš'əw"),
            ("Samo pridi sem", "sam'ó: pr'i:di səm"),
            ("Samo Janez je prišel", "sam'ó: j'a:nEs jE priš'əw"),
            ("To je rekel samo enkrat", "t'ó: jE r'é:kəw sam'ó: 'E:ŋkrat"),
            ("Samo je prišel", "s'a:mO jE priš'əw"),
            ("On je samo prišel", "'On jE sam'ó: priš'əw"),
            ("samo je prišel", "sam'ó: jE priš'əw"),
            ("SAMO JE PRIŠEL", "sam'ó: jE priš'əw"),
            ("Samo se je smejal", "s'a:mO sE jE smEj'a:w"),
            ("Samo ga ni videl", "s'a:mO ga ni v'i:dEw"),
            ("Samo bo jutri prišel", "s'a:mO bO j'u:tri priš'əw"),
            ("Samo je bil tam", "s'a:mO jE b'i:w t'am"),
            ("Samo je cel dan spal", "s'a:mO jE c'é:w d'a:n sp'a:w"),
            // the mi after the name is the clitic, after "only" the pronoun mí
            ("Samo mi je povedal", "s'a:mO mi jE pOv'é:daw"),
            ("samo mi je povedal", "sam'ó: m'i: jE pOv'é:daw"),
            ("Samo je bilo prepozno", "sam'ó: jE bil'ó: prEp'O:znO"),
            ("Samo je prišla", "sam'ó: jE prišl'a"),
            ("Samo je del problema", "sam'ó: jE d'é:l prObl'é:ma"),
            ("Samo je en model", "sam'ó: jE 'En mOd'é:l"),
            ("Samo je žal prepozno", "sam'ó: jE ž'aw prEp'O:znO"),
            ("Samo bi rad vedel", "sam'ó: bi r'at v'é:dEw"),
            ("Samo je Janez prišel", "sam'ó: jE j'a:nEs priš'əw"),
            ("Samo je še čas", "sam'ó: jE šE č'as"),
        ],
    );
}

/// Slovenian glasu: the genitive on the ending, the dative and locative on the stem
#[test]
fn slovenian_glasu_the_genitive_on_the_ending_the_dative_and_locative_on_the_stem() {
    let mut frontend = slovenian();
    check(
        &frontend,
        &[
            // A listener's report of 2026-11-10 (SSKJ² glás -ú tudi -a; SP 2001
            // glás -ú tudi glás -a, "dalje -u -- -u -om"; Sloleks glasú, glásu)
            ("Nisem imel glasu", "n'i:sEm im'é:w glas'u:"),
            ("V glasu je hripavost", "u gl'a:su jE hr'i:pavOst"),
            ("Glasu ne morem preveriti", "glas'u: nE m'ó:rEm prEv'é:riti"),
            ("V tvojem glasu je slišati toplino", "u tv'O:jEm gl'a:su jE sl'i:šati tOpl'i:nO"),
            // The locative has a preposition, with or without adjectives
            ("Spoznal sem ga po glasu", "spOzn'aw səm ga pO gl'a:su"),
            ("Bil je na dobrem glasu", "b'i:w jE na d'O:brEm gl'a:su"),
            ("O glasu", "O gl'a:su"),
            ("Pri glasu", "pri gl'a:su"),
            ("Ob glasu zvona", "Ob gl'a:su zv'ó:na"),
            ("Z očitkom v glasu", "z Oč'i:tkOm u gl'a:su"),
            ("Po njegovem hripavem glasu", "pO njEg'O:vEm hr'i:pavEm gl'a:su"),
            ("V zelo tihem glasu", "u zEl'ó: t'i:hEm gl'a:su"),
            ("V čigar glasu", "u č'i:gar gl'a:su"),
            ("V tihem in mirnem glasu", "u t'i:hEm in m'i:rnEm gl'a:su"),
            ("V njegovem ali njenem glasu", "u njEg'O:vEm ali nj'é:nEm gl'a:su"),
            // The dative: a preposition, an adjective in -emu, what takes it
            ("Proti glasu", "prOti gl'a:su"),
            ("Kljub glasu", "klj'u:b gl'a:su"),
            ("Zaupam svojemu notranjemu glasu", "za'u:pam sv'O:jEmu n'ó:tranjEmu gl'a:su"),
            ("Prisluhni glasu srca", "prisl'u:hni gl'a:su sərc'a:"),
            ("Sledi glasu vesti", "slEd'i: gl'a:su vEst'i:"),
            ("Sledil je glasu", "slEd'i:w jE gl'a:su"),
            ("Verjel je glasu", "vErj'é:w jE gl'a:su"),
            ("Glasu ne zaupam", "gl'a:su nE za'u:pam"),
            ("Glasu je prisluhnil", "gl'a:su jE prisl'u:hniw"),
            ("Podoben glasu", "pOd'ó:bən gl'a:su"),
            // The genitive: of another noun, of a negation, of a quantity, of its
            // verb, after its prepositions and adjectives
            ("Barva glasu", "b'a:rva glas'u:"),
            ("Višina glasu", "viš'i:na glas'u:"),
            ("Hitrost glasu", "hitr'ó:st glas'u:"),
            ("Nastavitve glasu", "nastav'i:tvE glas'u:"),
            ("V nastavitvah glasu", "u nastav'i:tvah glas'u:"),
            ("Ostal je brez glasu", "Ost'aw jE brEz glas'u:"),
            ("Iz glasu sem razbral jezo", "iz glas'u: səm razbr'a:w j'é:zO"),
            ("Zaradi glasu", "zar'a:di glas'u:"),
            ("Do njegovega glasu", "dO njEg'O:vEga glas'u:"),
            ("Ni dala glasu od sebe", "ni d'a:la glas'u: Ot s'E:bE"),
            ("Glasu ni bilo slišati", "glas'u: ni bil'ó: sl'i:šati"),
            ("Zmanjkalo ji je glasu", "zm'a:ŋkalO ji jE glas'u:"),
            ("Spomnim se glasu napovedovalca", "sp'ó:mnim sE glas'u: napOvEdOv'a:wca"),
            ("Podobnost glasu", "pOd'ó:bnOst glas'u:"),
            ("Brez sledi glasu", "brE slEd'i: glas'u:"),
            ("Pri tem še glasu ni dal od sebe", "pri t'é:m šE glas'u: ni d'a:w Ot s'E:bE"),
            ("Potem glasu ni bilo več", "pOt'é:m glas'u: ni bil'ó: v'Eč"),
            // A noun joined to another has its case
            ("V strahu in glasu", "u str'a:hu iŋ gl'a:su"),
            ("Šola diha in glasu", "š'ó:la d'i:ha iŋ glas'u:"),
            ("Zaradi strahu in glasu", "zar'a:di strah'u: iŋ glas'u:"),
            // The rest of the paradigm is as it was
            ("glas glasa glasom", "gl'a:s gl'a:sa gl'a:sOm"),
            ("glasova glasov glasovi glasove glasovih", "glas'O:va glas'O:w glas'O:vi glas'O:vE glas'O:vih"),
            // The other nouns Slovenski pravopis 2001 declines so (strah -ú,
            // mir -ú, grad -ú, led -ú, med -ú, nos -ú, pas -ú, vrat -ú ...,
            // "dalje -u -- -u -om"; most and rod with the open o in the dative
            // and locative: SSKJ² môstu in móstu, rôdu tudi ródu)
            ("Pusti me pri miru", "p'u:sti mE pri m'i:ru"),
            ("Želimo si miru", "žEl'i:mO si mir'u:"),
            ("Javnega reda in miru", "j'a:wnEga r'é:da in mir'u:"),
            ("Živim v strahu", "živ'i:m u str'a:hu"),
            ("Brez strahu", "brE strah'u:"),
            ("Iz strahu pred kaznijo", "i strah'u: prEt k'a:znijO"),
            ("Bil sem na gradu", "b'i:w səm na gr'a:du"),
            ("Na Ljubljanskem gradu", "na ljublj'a:nskEm gr'a:du"),
            ("Šel sem do gradu", "š'əw səm dO grad'u:"),
            ("Obnova gradu", "Obn'O:va grad'u:"),
            ("Na mostu", "na m'O:stu"),
            ("Z mostu", "z mOst'u:"),
            ("Do mostu", "dO mOst'u:"),
            ("Po rodu je Hrvat", "pO r'O:du jE hərv'at"),
            ("Slovenskega rodu", "slOv'é:nskEga rOd'u:"),
            ("Na ledu", "na l'é:du"),
            ("Kocka ledu", "k'ó:cka lEd'u:"),
            ("V medu", "u m'é:du"),
            ("Žlica medu", "žl'i:ca mEd'u:"),
            ("Na nosu", "na n'ó:su"),
            ("Iz nosu", "iz nOs'u:"),
            ("V pasu", "u p'a:su"),
            ("Do pasu", "dO pas'u:"),
            ("Na vratu", "na wr'a:tu"),
            ("Okoli vratu", "Ok'ó:li wrat'u:"),
            ("Na tekočem traku", "na tEk'ó:čEm tr'a:ku"),
            ("S tekočega traku", "s tEk'ó:čEga trak'u:"),
            ("Mleko v prahu", "ml'é:kO u pr'a:hu"),
            ("Plast prahu", "pl'a:st prah'u:"),
            ("Sledi prahu", "slEd'i: prah'u:"),
            ("Ni sledi strahu", "ni slEd'i: strah'u:"),
            ("Bližje gradu", "bl'i:žjE gr'a:du"),
            ("Ušesih in nosu", "uš'é:sih in n'ó:su"),
            ("Levem voznem pasu", "l'é:vEm v'O:znEm p'a:su"),
            // Names with a capital keep the reading of the tables of names
            ("Povej to Miru", "pOv'é:j t'ó: m'i:ru"),
            ("V Novem Sadu", "u n'O:vEm s'a:du"),
            // Nouns whose genitive in -ú the dictionaries do not have keep the stem
            ("Do robu", "dO r'ó:bu"),
            ("Povej sinu", "pOv'é:j s'i:nu"),
        ],
    );
    // An accent written in the text and the user's entry win
    assert_eq!(phones(&frontend, "V glasú"), "u glas'u:");
    assert_eq!(phones(&frontend, "Barva glásu"), "b'a:rva gl'a:su");
    frontend.set_user_lexicon(user(r#"{"entries":[{"word":"glásu","language":"sl"}]}"#));
    assert_eq!(phones(&frontend, "Barva glasu"), "b'a:rva gl'a:su");
}

/// A listener's report of 2026-11-18: lepo before a pronoun or a verb is the
/// adverb lepó, before a noun the adjective lépo (SP 2001: lepó nač. prisl.,
/// lepó povdk.; lép -a -o), and so are težko, slabo, močno ...
#[test]
fn slovenian_listeners_report_of_2026_11_18_lepo_before_a_verb() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // The listener's sentences: the adverb ...
            ("Lepo mi je s tabo", "lEp'ó: mi jE s t'a:bO"),
            ("Lepo se imejte", "lEp'ó: sE im'E:jtE"),
            ("Lepo bodi", "lEp'ó: b'ó:di"),
            ("Lepo bodite", "lEp'ó: bOd'i:tE"),
            ("Lepo vas pozdravljam", "lEp'ó: vas pOzdr'a:wljam"),
            ("Lepo pišeš", "lEp'ó: p'i:šEš"),
            // ... and the adjective
            ("Lepo majico nosiš", "l'é:pO m'a:jicO n'ó:siš"),
            ("Lepo mačko imaš", "l'é:pO m'a:čkO im'a:š"),
            ("lepo obalo imajo na Hrvaškem", "l'é:pO Ob'a:lO im'a:jO na hərv'a:škEm"),
            // Not only at the head of a sentence, and pa between
            ("On lepo piše", "'On lEp'ó: p'i:šE"),
            ("Lepo pa je tukaj", "lEp'ó: pa jE t'u:kaj"),
            // Nouns whose endings a verb has too stay nouns: the table of
            // verb forms leaves them out
            ("Lepo pesem poje", "l'é:pO p'é:səm p'O:jE"),
            ("Lepo zimo smo imeli", "l'é:pO z'i:mO smO im'é:li"),
            ("Imam lepo mamo", "im'a:m l'é:pO m'a:mO"),
            ("Lepo šolo imajo", "l'é:pO š'ó:lO im'a:jO"),
            // After a preposition the adjective, whatever follows (the
            // instrumental of misel, spelled like the verb's mislijo)
            ("Z lepo mislijo", "z l'é:pO m'i:slijO"),
            // Before an adjective or a participle the adverb too since the
            // listener's report of 2026-11-24 (on 2026-10-30 the adjective)
            ("Lepo pozdravljeni", "lEp'ó: pOzdr'a:wljEni"),
            // The other spellings of the class
            ("Težko mi je", "tEšk'ó: mi jE"),
            ("Slabo se počutim", "slab'ó: sE pOč'u:tim"),
            ("Močno dežuje", "mOčn'ó: dEž'u:jE"),
            ("Težko delo", "t'E:škO d'é:lO"),
            // lahko keeps its own rule
            ("Lahko pridem", "lahk'ó: pr'i:dEm"),
        ],
    );
}

/// A listener's report of 2026-11-24: Lepo pozdravljeni on the last syllable
/// of lepo. Before an adjective or a participle the adverb lepó (SSKJ² lepó
/// prisl.: lepo oblikovane roke, lepo zelena trava; SP 2001 lepó nač.
/// prisl.: ~ pameten bodi), the adjective lépo only before a word it can
/// agree with
#[test]
fn slovenian_listeners_report_of_2026_11_24_lepo_pozdravljeni() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // The listener's phrase and the dictionaries' examples
            ("Lepo pozdravljeni", "lEp'ó: pOzdr'a:wljEni"),
            ("Lepo oblikovane roke", "lEp'ó: OblikOv'a:nE r'O:kE"),
            ("Lepo zelena trava", "lEp'ó: zEl'E:na tr'a:va"),
            ("Lepo pameten bodi", "lEp'ó: p'a:mEtən b'ó:di"),
            // Every form an adjective in -o cannot agree with, a participle in
            // -o of the table, after a preposition too, and the other spellings
            ("Lepo urejen vrt", "lEp'ó: urEj'En v'ərt"),
            ("Lepo urejeno stanovanje", "lEp'ó: urEj'E:nO stanOv'a:njE"),
            ("Z lepo urejenimi vrtovi", "z lEp'ó: urEj'E:nimi vərt'O:vi"),
            ("Močno zadolžena država", "mOčn'ó: zadOlž'E:na dərž'a:va"),
            ("Tesno povezane", "tEsn'ó: pOv'é:zanE"),
            // The adjective before a word it agrees with: a noun in -o, -e or
            // a consonant, the feminine and neuter nouns spelled like an
            // adjective's form, a relational adjective in -o and its noun
            ("Lepo vreme", "l'é:pO wr'E:mE"),
            ("Lepo življenje", "l'é:pO žiwlj'E:njE"),
            ("Lepo novo hišo", "l'é:pO n'O:vO h'i:šO"),
            ("Lepo jesen", "l'é:pO jEs'é:n"),
            ("Lepo popoldne", "l'é:pO pOp'O:ldnE"),
            ("Lepo naše mesto", "l'é:pO n'a:šE m'é:stO"),
            ("Težko finančno situacijo", "t'E:škO fin'a:nčnO situ'a:cijO"),
            ("lepo oziroma grdo vreme", "l'é:pO Oz'i:rOma g'ərdO wr'E:mE"),
            // Found while checking: dekle had been read as an l-participle
            ("Lepo dekle", "l'é:pO dEkl'E"),
        ],
    );
}

/// A listener's report of 2026-11-17: meja, vasi, izgovori, sprosti, petega
#[test]
fn slovenian_listeners_report_of_2026_11_17_meja_vasi_izgovori_sprosti() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // mêja the nominative, mejá the genitive plural (SSKJ² mêja -e, rod.
            // mn. mêj in mejá): the listener's two sentences, then the cues
            ("Levo od tebe je meja", "l'é:vO O t'E:bE jE m'E:ja"),
            ("Igre brez meja", "'i:grE brEz mEj'a:"),
            ("Državna meja je zaprta", "dərž'a:wna m'E:ja jE zap'ərta"),
            ("Meja poteka po reki", "m'E:ja pOt'é:ka pO r'é:ki"),
            ("Zunaj naših meja", "z'u:naj n'a:ših mEj'a:"),
            ("Do skrajnih meja", "dO skr'a:jnih mEj'a:"),
            ("Varovanje meja", "varOv'a:njE mEj'a:"),
            ("Veliko meja", "vEl'i:kO mEj'a:"),
            ("Domišljija ne pozna meja", "dOmišlj'i:ja nE pOzn'a: mEj'a:"),
            ("Tu ni meja", "t'u ni mEj'a:"),
            ("To ni meja", "t'ó: ni m'E:ja"),
            // The other nouns SSKJ² and SP 2001 give the plural -é -á of
            ("Seznam ima pet želja", "sEzn'am im'a: p'é:t žElj'a:"),
            ("Moja želja je mir", "m'O:ja ž'E:lja jE m'i:r"),
            ("Iz gora", "iz gOr'a:"),
            ("Gora je visoka", "g'O:ra jE vis'O:ka"),
            ("Brez solza", "brE sOwz'a:"),
            ("Moja sestra pride", "m'O:ja s'E:stra pr'i:dE"),
            // vasí the genitive and the plural, vási the dative and the
            // locative (SSKJ² vás -í; SP 2001 vás -í ž -i -- -i -jó)
            ("On je iz vasi", "'On jE iz vas'i:"),
            ("V tej vasi je veliko hiš", "u t'E:j v'a:si jE vEl'i:kO h'i:š"),
            ("Živim na vasi", "živ'i:m na v'a:si"),
            ("V sosednji vasi", "u sOs'é:dnji v'a:si"),
            ("Na robu vasi", "na r'ó:bu vas'i:"),
            ("Sredi vasi", "sr'é:di vas'i:"),
            ("V bližini vasi", "u bliž'i:ni vas'i:"),
            ("Prebivalci vasi", "prEbiv'a:wci vas'i:"),
            ("Vasi so majhne", "vas'i: sO m'a:jhnE"),
            ("Iz te vasi", "is tE vas'i:"),
            ("V tri vasi", "u tr'i: vas'i:"),
            // The same for the other feminine nouns declined so
            ("Na drugi strani", "na dr'u:gi str'a:ni"),
            ("Z druge strani", "z dr'u:gE stran'i:"),
            ("Po očetovi strani", "pO Oč'é:tOvi str'a:ni"),
            ("Strani knjige", "stran'i: knj'i:gE"),
            ("Na poti domov", "na p'ó:ti dOm'O:w"),
            ("Dolge poti", "d'O:wgE pOt'i:"),
            ("Del poti", "d'é:l pOt'i:"),
            ("Na državni in lokalni ravni", "na dərž'a:wni in lOk'a:lni r'a:wni"),
            ("Do ravni", "dO rawn'i:"),
            ("V noči", "u n'O:či"),
            ("Sredi noči", "sr'é:di nOč'i:"),
            ("Na klopi", "na kl'ó:pi"),
            ("Brez pomoči", "brEs pOmOč'i:"),
            ("Pri pomoči", "pri pOm'O:či"),
            ("V luči", "u l'u:či"),
            ("Brez luči", "brEz luč'i:"),
            ("V stvari", "u stv'a:ri"),
            ("Vpliva na stvari", "upl'i:va na stvar'i:"),
            ("Te stvari", "t'é: stvar'i:"),
            ("V smeri", "u sm'é:ri"),
            ("Brez smeri", "brE smEr'i:"),
            // izgovorí the present, izgôvori the noun's plural (SP 2001
            // izgovoríti -ím; izgôvor)
            ("On to pogosto izgovori", "'On t'ó: pOg'ó:stO izgOvOr'i:"),
            ("Kdo to izgovori", "gd'ó: t'ó: izgOvOr'i:"),
            ("Naj odgovori", "naj OdgOvOr'i:"),
            ("To so samo izgovori", "t'ó: sO sam'ó: izg'O:vOri"),
            ("Njegovi izgovori so prazni", "njEg'O:vi izg'O:vOri sO pr'a:zni"),
            ("Odgovori so bili taki", "Odg'O:vOri sO bil'i: t'a:ki"),
            ("Izgovori odgovornih", "izg'O:vOri OdgOv'ó:rnih"),
            ("Govori se o tem", "gOvOr'i: sE O t'é:m"),
            ("Ona govori razločno", "'O:na gOvOr'i: razl'ó:čnO"),
            ("Glasba me sprosti", "gl'a:zba mE sprOst'i:"),
            ("Sodišče ga oprosti", "sOd'i:ščE ga OprOst'i:"),
            // A request with prosim: any verb at the head is the imperative
            ("Prosim izgovori to besedo", "pr'ó:sim izgOv'O:ri t'ó: bEs'é:dO"),
            ("Poveži prosim napravo", "pOv'é:ži pr'ó:sim napr'a:vO"),
            // pogovoriti se, dogovoriti se at the head, unless a plural verb
            // follows the se
            ("Pogovori se z njim", "pOgOv'O:ri sE z nj'i:m"),
            ("Dogovori se z njo", "dOgOv'O:ri sE z nj'ó:"),
            ("Pogovori se bodo nadaljevali", "pOg'O:vOri sE bOdO nadaljEv'a:li"),
            ("Pogovori se nadaljujejo", "pOg'O:vOri sE nadalj'u:jEjO"),
            // vás the village (SSKJ² vás -í), vas the pronoun, a clitic, and
            // vàs the full pronoun after a preposition, which leans on it (v
            // vas was read "vé vas")
            ("Ta vas je lepa", "t'a: v'a:s jE l'é:pa"),
            ("Naša vas leži ob reki", "n'a:ša v'a:s lEž'i: Ob r'é:ki"),
            ("Grem v vas", "gr'é:m u v'a:s"),
            ("Vas je majhna", "v'a:s jE m'a:jhna"),
            ("Zaupam v vas", "za'u:pam u v'as"),
            ("Hvala za vas", "hv'a:la za v'as"),
            ("Pridi k nam", "pr'i:di k n'am"),
            ("V nas je upanje", "u n'as jE 'u:panjE"),
            ("Lepo vas prosim", "lEp'ó: vas pr'ó:sim"),
            ("Ana vas čaka", "'a:na vaš č'a:ka"),
            ("Vas je strah", "v'as jE str'a:h"),
            ("To vas ne briga", "t'ó: vas nE br'i:ga"),
            // The rarer nouns declined like vas
            ("V zveri", "u zv'é:ri"),
            ("Brez zveri", "brE zvEr'i:"),
            ("Po brvi", "pO b'ərvi"),
            ("Brez masti", "brEz mast'i:"),
            // pêtega, the ordinal (SSKJ², SP 2001 pêti -a -o), not the
            // participle pét -a -o of péti "to sing"
            ("Do petega leta", "dO p'E:tEga l'é:ta"),
            ("K petemu", "k p'E:tEmu"),
        ],
    );
    // The imperatives (SP 2001 govôri -íte, sprôsti -íte, oprôsti -íte)
    let exclaimed = |text: &str| says(&frontend, text, Punctuation::Exclamation);
    assert_eq!(exclaimed("Izgovori prosim pravilno"), "izgOv'O:ri pr'ó:sim prav'i:lnO");
    assert_eq!(exclaimed("Govori razločno"), "gOv'O:ri razl'ó:čnO");
    assert_eq!(exclaimed("Sprosti prostor na telefonu"), "spr'O:sti pr'O:stOr na tElEf'ó:nu");
    assert_eq!(exclaimed("Oprosti"), "Opr'O:sti");
    // Outside the list of commands a verb at the head of a clause is the
    // present; with prosim after it or before it the imperative (SP 2001
    // razložíti -ím, razlôži -íte), also when commas set prosim off, which the
    // engine's mark says
    check(
        &frontend,
        &[
            ("Razloži to", "razlOž'i: t'ó:"),
            ("Razloži prosim to", "razl'O:ži pr'ó:sim t'ó:"),
            ("Prosim razloži to", "pr'ó:sim razl'O:ži t'ó:"),
        ],
    );
    assert_eq!(says(&frontend, "Položi", Punctuation::Comma), "pOlOž'i:");
    assert_eq!(says(&frontend, &format!("Položi{REQUEST_MARK}"), Punctuation::Comma), "pOl'O:ži");
}

// A listener's report of November 2026: "Marjan A17", a cell's contents and
// its address as a screen reader says them, was heard as "Marijana 17", and
// the hours of a clock time ran into its minutes. The number converter leaves
// two marks in the text (`text::marks`): U+FDD1 after the letters of a
// designation, U+FDD0 where a short pause belongs.
/// The letters of a designation are letters, whatever word they spell (the
/// Slovenian part)
#[test]
fn the_letters_of_a_designation_are_letters_whatever_word_they_spell() {
    let names = phones;
    // Slovenian: v, s, z, k and h are prepositions, and v before a word is [u]
    let sl = slovenian();
    assert_eq!(sounds(&sl, "V pet"), "u p'é:t");
    assert_eq!(sounds(&sl, "V\u{FDD1} pet"), "v'ə p'é:t");
    assert_eq!(names(&sl, "V\u{FDD1} pet"), "v'é: p'é:t");
    assert_eq!(sounds(&sl, "Marjan A\u{FDD1} dva"), "m'a:rjan 'a: dv'a:");
    assert_eq!(sounds(&sl, "Z\u{FDD1} tri"), "z'ə tr'i:");
}

/// A short pause inside a clause splits it into parts (the Slovenian part)
#[test]
fn a_short_pause_inside_a_clause_splits_it_into_parts() {
    let breaks = |utt: &Utterance| -> Vec<u16> {
        utt.phones.iter().filter(|phone| phone.break_before).map(|phone| phone.word).collect()
    };
    // The pause stands before the first word made of what follows it: the
    // parts of a Slovenian compound numeral
    let sl = slovenian();
    let time = sl.process(&chars("dvanajst \u{FDD0} enaindvajset"), Punctuation::Period, true);
    assert_eq!(breaks(&time), [1]);
    assert_eq!(split_at_breaks(&time).len(), 2);
    assert!(split_at_breaks(&time)[1].slovenian);
}

/// A listener's report of 2026-11-18: Minimiraj is stressed on its third
/// syllable. A verb in -írati has the stress on that í in every form (SSKJ²,
/// SP 2001: kopírati -am), also written with a capital, as a button or a menu
/// item is (the rules of names had read the -aj as in Andréj), and after a
/// stem of the lexicon with another accent (resèt + iraj).
#[test]
fn slovenian_listeners_report_of_2026_11_18_minimiraj() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            ("minimiraj", "minim'i:raj"),
            ("Minimiraj", "minim'i:raj"),
            ("Minimiraj okno", "minim'i:raj 'O:knO"),
            ("Gumb Minimiraj", "g'u:mp minim'i:raj"),
            ("Kliknite Minimiraj", "kl'i:knitE minim'i:raj"),
            ("Okno se Minimira", "'O:knO sE minim'i:ra"),
            ("Minimirajo okna", "minim'i:rajO 'O:kna"),
            ("minimiral", "minim'i:raw"),
            // The other commands, inside a clause with a capital
            ("Kliknite Kopiraj", "kl'i:knitE kOp'i:raj"),
            ("Kliknite Formatirajte", "kl'i:knitE fOrmat'i:rajtE"),
            // After a stem with an accent of its own
            ("resetiraj", "rEsEt'i:raj"),
            ("resetira", "rEsEt'i:ra"),
            ("Resetirajte", "rEsEt'i:rajtE"),
            ("to je skadriral", "t'ó: jE skadr'i:raw"),
            // The names keep their rule
            ("Gospod Andrej", "gOsp'ó:t andr'E:j"),
            ("Gospod Jernej", "gOsp'ó:t jErn'E:j"),
        ],
    );
}

/// A listener's report of 2026-11-18: the noun čast (častí the genitive and
/// the plural, části the dative and the locative), SSKJ² částiti "to treat"
/// on the stem, častíti "to honour" on the ending, and the imperative počásti.
#[test]
fn slovenian_listeners_report_of_2026_11_18_cast_castiti() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // The listener's six sentences
            ("To je hiša časti", "t'ó: jE h'i:ša čast'i:"),
            ("Mama časti vse nas", "m'a:ma č'a:sti us'E nas"),
            ("Danes lahko ti častiš", "d'a:nəs lahk'ó: ti č'a:stiš"),
            ("Kriminalci nimajo nobene časti", "krimin'a:wci n'i:majO nOb'E:nE čast'i:"),
            ("Časti ne moreš z lahkoto dobiti nazaj", "čast'i: nE m'ó:rEž z lahk'O:tO dOb'i:ti naz'a:j"),
            ("Počasti se z dobro večerjo", "pOč'a:sti sE z d'O:brO vEč'é:rjO"),
            // To treat: SSKJ² danes te častim jaz, častiti komu pijačo
            ("Kdo časti", "gd'ó: č'a:sti"),
            ("Jaz častim", "j'as č'a:stim"),
            ("Danes častim jaz", "d'a:nəs č'a:stim j'as"),
            ("Častim rundo", "č'a:stim r'u:ndO"),
            ("Šef nas je častil s kosilom", "š'é:f nas jE č'a:stiw s kOs'i:lOm"),
            ("Včeraj je častila vso družbo", "uč'é:raj jE č'a:stila us'O dr'u:žbO"),
            ("Danes časti Janez", "d'a:nəs č'a:sti j'a:nEs"),
            ("Janez časti", "j'a:nEs č'a:sti"),
            ("Časti me", "č'a:sti mE"),
            ("Časti pijačo", "č'a:sti pij'a:čO"),
            ("Kolegi častijo", "kOl'é:gi č'a:stijO"),
            // To honour: SSKJ² častijo ga kakor boga, častiti spomin
            ("Verniki častijo Marijo", "v'é:rniki čast'i:jO mar'i:jO"),
            ("Častijo ga kot boga", "čast'i:jO ga k'Ot bOg'a:"),
            ("Stari Slovani so častili Peruna", "st'a:ri slOv'a:ni sO čast'i:li pEr'u:na"),
            ("Katoliška cerkev ga časti kot svetnika", "kat'ó:liška c'é:rkəw ga čast'i: k'Ot sv'é:tnika"),
            ("Častiti moramo spomin prednikov", "čast'i:ti m'ó:ramO spOm'i:n pr'é:dnikOw"),
            // Its imperative is on the stem too (SP 2001 části -íte)
            ("Časti očeta in mater", "č'a:sti Oč'é:ta in m'a:tEr"),
            // The noun
            ("Brez časti", "brEš čast'i:"),
            ("To je stvar časti", "t'ó: jE stv'a:r čast'i:"),
            ("Časti ni več", "čast'i: ni v'Eč"),
            ("Izkazali so mu vse časti", "iskaz'a:li sO mu us'E čast'i:"),
            ("Tudi v časti je sreča", "t'u:di u č'a:sti jE sr'é:ča"),
            ("Časti", "čast'i:"),
            // The present of počastiti, and the derived words
            ("Predsednik počasti goste", "prEts'é:dnik pOčast'i: g'O:stE"),
            ("Bil je častilec sonca", "b'i:w jE čast'i:ləc s'ó:nca"),
        ],
    );
}

/// An audit of the tables of names and of the rules that ask for a numeral
/// (2026-11-18): the genitives of plural place names the generator had put
/// together wrongly
/// (Stóperce -erc gave "stópercerc", Répnje -enj "répnjenj"), the fleeting e
/// of such a genitive, a noun joined by in to one in the locative, leti after
/// in, and the adjective that begins a name in every case alike.
#[test]
fn slovenian_place_genitives_and_the_conjunction_in_2026_11_18() {
    let frontend = slovenian();
    check(
        &frontend,
        &[
            // SP 2001: Ráteče -eč, Stóperce -erc, Hrpélje -élj, Senožéče -éč,
            // Črnéče -éč, Šebrélje -élj; the ending replaces the stem's tail
            ("Do Rateč", "dO r'a:tEč"),
            ("Do Stoperc", "dO st'ó:pErc"),
            ("Do Hrpelj", "dO hərp'é:l"),
            ("Do Senožeč", "dO sEnOž'é:č"),
            ("Do Črneč", "dO čərn'é:č"),
            ("Do Šebrelj", "dO šEbr'é:l"),
            ("V Ratečah", "u r'a:tEčah"),
            // Podbrézje -ij, Gôrje -ij: the i is put into the stem
            ("Do Podbrezij", "dO pOdbr'é:zij"),
            ("Do Gorij", "dO g'O:rij"),
            // Répnje -enj, Žáblje -elj, Kôtlje -telj, Pékre -ker: the e put
            // in is fleeting, a schwa (Sloleks: Trbôvelj, Cérkelj, čéšenj);
            // the l of Déskel stays [l], as in every genitive plural
            ("Do Repenj", "dO r'é:pən"),
            ("Do Velikih Žabelj", "dO vEl'i:kih ž'a:bəl"),
            ("Do Kotelj", "dO k'O:təl"),
            ("Do Peker", "dO p'é:kər"),
            ("Do Deskel", "dO d'é:skəl"),
            ("Iz Trbovelj", "is tərb'O:vəl"),
            // The adjective of a plural name declines as one
            ("Pred Velikimi Laščami", "prEd vEl'i:kimi l'a:ščami"),
            // A noun joined by in has the case of the one before it: the
            // locative on the stem (SUK 1.1: o specifikacijah in moči, pakt o
            // stabilnosti in rasti). in is no numeral there
            ("O specifikacijah in moči", "O spEcifik'a:cijah in m'O:či"),
            ("O specifikacijah ali moči", "O spEcifik'a:cijah ali m'O:či"),
            ("Pakt o stabilnosti in rasti", "p'a:kt O stab'i:lnOsti in r'a:sti"),
            ("Po učinkovitosti in moči", "pO učiŋkOv'i:tOsti in m'O:či"),
            // ... and the accusative plural or the genitive on the ending
            ("Na ljudi in zveri", "na ljud'i: in zvEr'i:"),
            ("Brez strahu in moči", "brE strah'u: in mOč'i:"),
            ("V tri vasi", "u tr'i: vas'i:"),
            // leti after in is the verb; the flights have their adjective
            ("Ptič poje in leti", "pt'ič p'O:jE in lEt'i:"),
            ("Poskusni leti v Beograd", "pOsk'u:sni l'é:ti u b'é:Ograt"),
            ("Ponoči leti", "pOn'O:či lEt'i:"),
            ("Lastovka jeseni leti domov", "l'a:stOwka jEs'é:ni lEt'i: dOm'O:w"),
            ("pred enaindvajsetimi leti", "prEd 'E:naindv'a:jsEtimi l'é:ti"),
            // The adjective that begins a name is the lexicon's in every
            // case (the maintainer's decision; SP 2001 has Vélika Británija):
            // veliko alone had been the name's
            ("Velika Britanija", "vEl'i:ka brit'a:nija"),
            ("V Veliki Britaniji", "u vEl'i:ki brit'a:niji"),
            ("Nad Veliko Britanijo", "nad vEl'i:kO brit'a:nijO"),
            ("Grem na Veliko planino", "gr'é:m na vEl'i:kO plan'i:nO"),
            ("Pred Veliko nočjo", "prEd vEl'i:kO nOčj'ó:"),
            ("Veliko ljudi", "vEl'i:kO ljud'i:"),
            // A name of one word that an adjective is spelled like stays
            // the name (Kózje, Jezêrsko; kôzji, jézerski)
            ("To je Kozje", "t'ó: jE k'ó:zjE"),
            ("Na Jezerskem", "na jEz'E:rskEm"),
        ],
    );
    // Veliko, če ne kar vse, je odvisno ...: alone before a comma it had been
    // read as an address, by the table of names
    assert_eq!(phones(&frontend, &format!("Veliko{VOCATIVE_MARK}")), "vEl'i:kO");
}
