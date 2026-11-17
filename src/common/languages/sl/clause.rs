//! Slovenian words read by the other words of their clause: samo, vprašaj,
//! je, glasu and the other genitives in -u, veseli, lepo and the other
//! adverbs in -o, lahko, meni, roka, afna.

use super::phonemize::{Capital, phonemize};
use super::readings::{
    Clause, FOOD_ACCUSATIVE, FOOD_GENITIVE, FOOD_NOMINATIVE, food_cases, is_adjective_ending, is_definite_adjective,
    is_given_name, is_number_word, is_preposition,
};
use super::words::{is_enclitic, is_function_word, is_proclitic, is_question_word};
use crate::formant::phones::Ph;
use crate::text::frontend::letters::{ends_with, ends_with_any, eq, find, is_one_of, is_vowel_letter, starts_with};

// meni is three words: mení the menu (SSKJ² "mení tudi menu [mení]", SP
// 2001 mení -ja), mêni the dative and locative of jàz (SSKJ², SP: mêne,
// mêni), méni the present of méniti "think" (SP: meníti in méniti -im
// tudi méniti -im). Alone in its clause it is the name of a menu, a
// button or a screen ("Meni", "Meni, gumb"); in a sentence it is one of
// the other two. In SUK 1.1 the verb is 200 of 242 occurrences and the
// pronoun 37: the pronoun is every one after a preposition (k meni, v
// meni, o meni: 17 of 17) and before se or je (meni se zdi, meni je
// všeč); the lexicon's méni stays elsewhere (minister meni, kaj meni).
//
// lepo is lepó at the end of its clause and lépo before another word, as
// a listener asked on 2026-10-30 (Hvala lepo. To je lepo. Lepo vreme.
// Lepo pozdravljeni.), and so are the other spellings Sloleks reads as an
// adverb on -ó and an adjective on its stem (težkó/têžko, slabó/slábo:
// the listener said yes to them the same day). The dictionaries have -ó
// for the adverb and the predicative (SP 2001: lepó nač. prisl., lepó
// povdk.) and the stem for the adjective (SP: lép -a -o tudi -ó), so
// before a word the adverb (lepo pozdravljeni, močno dežuje, težko je)
// follows the listener, not them: in SUK 1.1 that is 361 of the 579
// occurrences of these spellings, and the adjective before a word, which
// the lexicon read as the adverb (težko delo, slabo vreme), 158. Not
// lahko: the modal lahkó "can" stands before its verb (Lahko pridem).
// Not nelepo: Sloleks has no reading of its adverb. The readings are
// Sloleks's (the adverb's -ó, the adjective's first stem variant).

/// The spelling, the adverb at the end of its clause, the adjective before
/// another word.
const END_READINGS: &[(&str, &str, &str)] = &[
    ("lepo", "lepó", "lépo"),
    ("težko", "težkó", "têžko"),
    ("močno", "močnó", "môčno"),
    ("slabo", "slabó", "slábo"),
    ("hudo", "hudó", "húdo"),
    ("tesno", "tesnó", "têsno"),
    ("ostro", "ostró", "óstro"),
    ("trdo", "trdó", "tŕdo"),
    ("toplo", "topló", "tôplo"),
    ("grdo", "grdó", "gŕdo"),
    ("grenko", "grenkó", "grênko"),
    ("tenko", "tenkó", "tènko"),
    ("premočno", "premočnó", "premôčno"),
    ("preslabo", "preslabó", "preslábo"),
    ("predrago", "predragó", "predrágo"),
    ("prelepo", "prelepó", "prelépo"),
    ("pretežko", "pretežkó", "pretêžko"),
    ("prehudo", "prehudó", "prehúdo"),
    ("prelahko", "prelahkó", "preláhko"),
    ("pretesno", "pretesnó", "pretêsno"),
    ("pretoplo", "pretopló", "pretôplo"),
    ("premrzlo", "premrzló", "premŕzlo"),
    ("pretrdo", "pretrdó", "pretŕdo"),
    ("preostro", "preostró", "preóstro"),
    ("pretenko", "pretenkó", "pretènko"),
    ("pregrdo", "pregrdó", "pregŕdo"),
];

const SHORT_PRONOUNS: &[&str] = &[
    "mi", "ti", "mu", "ji", "nam", "vam", "jim", "me", "te", "ga", "jo", "nas", "vas", "jih", "se", "si", "ju", "naju",
    "vaju",
];

/// A verb in the first or second person, by its ending (poznam, vidiš, kupite).
fn addressed_verb(w: &[char]) -> bool {
    const NOT_VERBS: &[&str] = &[
        "potem", "zatem", "nekam", "drugam", "nikamor", "vsem", "njim", "njem", "mojim", "tvojim", "svojim", "našim",
        "vašim", "mojem", "tvojem", "svojem", "našem", "vašem",
    ];
    const ENDINGS: &[&str] = &["im", "am", "em", "iš", "aš", "eš", "imo", "amo", "emo", "ite", "ate", "ete"];
    w.len() >= 4 && !is_one_of(w, NOT_VERBS) && ends_with_any(w, ENDINGS)
}

/// lahko is the modal and the adverb, lahkó (SP 2001), in 2,703 of its 2,718
/// occurrences in SUK 1.1, and láhko, the adjective lahek (SP "láhek -hka -o";
/// a Slovenian listener's Jesti moraš lahko hrano, Prosim daj mi lahko nalogo,
/// On nosi lahko torbo) in the others, so the adjective needs evidence. A noun
/// right after it does not decide by itself: the modal stands before its object
/// in 63 of those (Lahko kuhinjo prepleskamo, Preboliš lahko bolezen), and a
/// present of a perfective verb, which has no other way to be a present, comes
/// with the modal after it (Izberete lahko nalogo, Kupi lahko hrano). What does:
/// lahko noč (SSKJ² "kot pozdrav"); a preposition before it (z lahko nogo, na
/// lahko nafto; SP na láhko tudi na lahkó) unless an adverb is modifying an
/// adjective in -ljiv (na lahko obranljivem); a verb of obligation, which the
/// modal never follows, before a word that is no infinitive (moraš lahko hrano,
/// not mora lahko dostopati); and a noun things are called light by (SSKJ²
/// under láhek, the Collocations Dictionary of Modern Slovene at CJVT, the
/// corpus's lahko nafto), alone or after an adjective of its own (lahko poletno
/// obleko, lahko in hladno obleko), ending the clause or before a preposition or
/// a conjunction (a verb after it would make lahko the modal), with a verb before
/// lahko that carries such things in the present (nosim, pijem, kuham), or none
/// that would make it the modal. A question that opens with lahko and such a noun
/// asks whether something can be done, a statement says something of the light
/// thing, as a native listener answered in November 2026 (Lahko delo opravi vsak?
/// lahkó, Lahko delo opravi vsak. láhko); the only such sentence of SUK 1.1 is a
/// question. A verb in another person than the third has a subject of its own,
/// and is the modal in a statement too (Lahko hrano kupite v trgovini).
fn lahko_is_adjective(clause: &Clause, i: usize) -> bool {
    #[rustfmt::skip] // rows of words
    const NOUNS: &[&str] = &[
        "aktovko", "artilerijo", "atletiko", "baterijo", "besedilo", "blago", "bluzo", "bolezen", "branje", "cev",
        "copato", "delo", "desko", "dieto", "dihanje", "dostopnost", "figuro", "folijo", "gimnastiko", "glasbo",
        "gorivo", "gripo", "haljo", "harmoniko", "hojo", "hrano", "hruško", "igro", "industrijo", "izbiro", "jakno",
        "jed", "jedilo", "jopico", "jopo", "juho", "kamero", "kapico", "kapo", "kategorijo", "kitaro", "klobaso",
        "knjigo", "kolesce", "kolo", "komedijo", "konfekcijo", "konstrukcijo", "kosilo", "kost", "kovino", "kremo",
        "krilce", "krilo", "križarko", "krmo", "kroglo", "kuhinjo", "letalo", "letev", "letvo", "literaturo", "litino",
        "majico", "malico", "nafto", "nalogo", "nesrečo", "nogavico", "nogo", "oblačilo", "obleko", "obliko", "obutev",
        "obuvalo", "odejo", "odločitev", "ogrodje", "olje", "omako", "opeko", "operacijo", "opravilo", "opremo",
        "orodje", "orožje", "palico", "pasto", "pecivo", "peno", "perilo", "pero", "pijačo", "pito", "pivo", "pižamo",
        "platišče", "platno", "ploščo", "podvozje", "pohištvo", "pohodnico", "pokrivalo", "posodo", "pot", "poškodbo",
        "prehrano", "preizkušnjo", "preskušnjo", "prevleko", "prst", "puško", "raketo", "rekreacijo", "rezino",
        "rokavico", "roko", "salamo", "sandalo", "sladico", "slovo", "službo", "smrt", "snov", "solato", "spanje",
        "srajco", "srce", "steklo", "strojnico", "suknjo", "tarčo", "tekmo", "tekočino", "telesce", "telo", "telovadbo",
        "tipkovnico", "tiskalnico", "tkanino", "tkivo", "torbo", "torto", "tuniko", "turo", "usnje", "vajo", "vetrovko",
        "večerjo", "vino", "vozilo", "vožnjo", "vprašanje", "vrečko", "vrečo", "vzmet", "zaveso", "zaščito", "zemljo",
        "zlitino", "zmago", "čelado", "čokolado", "čtivo", "škatlo", "železo", "žganje", "življenje", "žogo",
    ];
    const OBLIGATION: &[&str] = &[
        "mora", "moram", "moraš", "morava", "morata", "moramo", "morate", "morajo", "moral", "morala", "moralo",
        "morali", "morale", "sme", "smem", "smeš", "smemo", "smete", "smejo", "hoče", "hočem", "hočeš", "hočemo",
        "hočete", "hočejo",
    ];
    const MODAL_BEFORE: &[&str] = &[
        "bi", "da", "če", "ki", "ali", "a", "kako", "kje", "kaj", "kdaj", "zakaj", "kdo", "bo", "bom", "boš", "bova",
        "bosta", "bomo", "boste", "bodo", "sem", "so", "sta", "smo", "ste", "vsak", "vsakdo", "vsi", "nihče",
    ];
    // Verbs that carry, eat, wear or look for such a thing in the present
    #[rustfmt::skip] // rows of words
    const CARRYING: &[&str] = &[
        "nosim", "nosiš", "nosi", "nosiva", "nosita", "nosimo", "nosijo", "jem", "ješ", "jemo", "jedo", "jejo", "pijem",
        "piješ", "pije", "pijemo", "pijejo", "oblačim", "oblačiš", "oblači", "oblačimo", "oblačijo", "kuham", "kuhaš",
        "kuha", "kuhamo", "kuhajo", "pripravljam", "pripravljaš", "pripravlja", "pripravljamo", "pripravljajo", "iščem",
        "iščeš", "išče", "iščemo", "iščejo", "maram", "maraš", "mara", "maramo", "marajo", "ljubim", "ljubiš", "ljubi",
        "ljubimo", "ljubijo", "želim", "želiš", "želi", "želimo", "želijo", "potrebujem", "potrebuješ", "potrebuje",
        "potrebujemo", "potrebujejo", "rabim", "rabiš", "rabi", "rabimo", "rabijo", "kupujem", "kupuješ", "kupuje",
        "kupujemo", "kupujejo", "prodajam", "prodajaš", "prodaja", "prodajamo", "prodajajo", "opravljam", "opravljaš",
        "opravlja", "opravljamo", "opravljajo", "prenašam", "prenašaš", "prenaša", "prenašamo", "prenašajo", "držim",
        "držiš", "drži", "držimo", "držijo", "vlečem", "vlečeš", "vleče", "vlečemo", "vlečejo", "uživam", "uživaš",
        "uživa", "uživamo", "uživajo", "berem", "bereš", "bere", "beremo", "berejo", "poslušam", "poslušaš", "posluša",
        "poslušamo", "poslušajo", "igram", "igraš", "igra", "igramo", "igrajo", "izdelujem", "izdeluješ", "izdeluje",
        "izdelujemo", "izdelujejo", "proizvajam", "proizvajaš", "proizvaja", "proizvajamo", "proizvajajo", "dobivam",
        "dobivaš", "dobiva", "dobivamo", "dobivajo", "imam", "imaš", "ima", "imava", "imata", "imamo", "imate", "imajo",
    ];
    // Perfective presents: with lahko after them they are its verb
    const PERFECTIVE: &[&str] = &[
        "izbere", "dobi", "kupi", "najde", "vzame", "uporabi", "naroči", "opravi", "prebere", "vidi", "odda",
        "pridobi", "spremeni", "prenese", "pošlje", "ponudi", "pripravi", "skuha", "sestavi", "zamenja", "doda",
        "prevzame", "preboli", "obleče", "sname", "poje", "popije", "ustvari",
    ];
    // Adverbs an adjective's -o is spelled like, which modify the modal's verb
    #[rustfmt::skip] // rows of words
    const NOT_ADJECTIVES: &[&str] = &[
        "tako", "nekako", "zato", "samo", "kako", "zelo", "malo", "veliko", "hitro", "lepo", "dobro", "slabo", "vedno",
        "skoraj", "celo", "nato", "tudi", "prej", "ravno", "kmalu", "mirno", "pravo", "neko", "eno", "vsako", "katero",
        "kakšno", "takšno", "tisto", "tole", "ono", "svojo", "mojo", "tvojo", "našo", "vašo", "njeno", "drugo", "isto",
        "marsikatero",
    ];
    const JOINING: &[&str] = &["in", "ter", "ali", "pa", "a", "ampak", "oziroma"];
    // A degree goes with the adjective, not the modal (zelo lahko torbo imam)
    #[rustfmt::skip] // rows of words
    const DEGREES: &[&str] = &[
        "zelo", "precej", "dokaj", "razmeroma", "sorazmerno", "dovolj", "izredno", "nadvse", "preveč", "res", "čisto",
        "povsem",
    ];

    let words = clause.words;
    let n = words.len();
    let prev = clause.before(i);
    // Z, S with a capital inside a clause are letters (intervalna opredelitev
    // Z lahko povzroča, model S lahko ponudi)
    let prev_is_letter = i > 1 && clause.capital(i - 1);
    // na lahko, the adverbial phrase, ends a clause too (jemati službo na lahko)
    let Some(next) = clause.word(i + 1) else {
        return prev.is_some_and(|prev| eq(prev, "na")) && !prev_is_letter;
    };
    // lahko prebavljiva hrana, the adverb; but z lahko prebavljivostjo
    if find(next, "ljiv").is_some() && find(next, "ljivost").is_none() {
        return false;
    }
    if eq(next, "noč") || (prev.is_some_and(is_preposition) && !prev_is_letter) {
        return true;
    }
    let k = (0..i).rev().find(|&k| !clause.one_of(k, SHORT_PRONOUNS)).map_or(0, |k| k + 1);
    let before = clause.before(k);
    if before.is_some_and(|before| is_one_of(before, OBLIGATION)) && !ends_with_any(next, &["ti", "či"]) {
        return true;
    }

    // The noun: right after it, after an adjective, or after a coordinated one
    let noun_at = |k: usize| clause.one_of(k, NOUNS);
    let adjective_o = |a: usize| {
        clause.word(a).is_some_and(|w| w.len() >= 3 && w.last() == Some(&'o') && !is_one_of(w, NOT_ADJECTIVES))
    };
    let noun = if noun_at(i + 1) {
        i + 1
    } else if adjective_o(i + 1) && noun_at(i + 2) {
        i + 2
    } else if eq(next, "in") && adjective_o(i + 2) && noun_at(i + 3) {
        i + 3
    } else {
        return false;
    };
    if before.is_some_and(|before| is_one_of(before, DEGREES)) {
        return true;
    }
    let at_head = before.is_none() && k == i; // Lahko delo, not Se lahko delo
    // What follows it: a verb there makes lahko the modal, but not a copula,
    // whose subject the noun is (Lahko vino je najboljše)
    let mut copula = false;
    if let Some(after) = clause.word(noun + 1) {
        copula = k == i && is_one_of(after, &["je", "ni", "bo"]);
        if !is_preposition(after) && !is_one_of(after, JOINING) && !copula {
            // A sentence that opens with it, not a question: Lahko delo opravi
            // vsak. Lahko torbo nosi. But Lahko hrano kupite, Lahko hrano kupijo.
            return at_head
                && clause.capital(i)
                && !clause.question
                && !ends_with_any(after, &["ajo", "ejo", "ijo"])
                && !words[noun + 1..n].iter().any(|w| addressed_verb(w));
        }
    }
    let Some(before) = before else {
        return at_head && (!clause.question || copula); // Lahko delo. Lahko delo?
    };
    if is_one_of(before, MODAL_BEFORE) {
        return false;
    }
    // An imperative in -jte (Dajte mi lahko nalogo)
    if is_one_of(before, CARRYING) || ends_with(before, "jte") {
        return true;
    }
    // A verb in the present before it (Kupim lahko hrano, Vidite lahko delo)
    !is_one_of(before, PERFECTIVE) && !ends_with_any(before, &["m", "š", "mo", "te", "ta", "va", "jo"])
}

/// roka is the hand's nominative, rôka, or the genitive of rók "deadline",
/// róka (SP 2001 rók -a), or the genitive and the accusative of the name Rok,
/// Róka (SP Rók -a). In SUK 1.1 the 28 genitives have a preposition (do roka,
/// brez roka), a genitive in -ega (vojaškega roka) or a noun of time limits
/// before them; the 16 hands have none of these, and a hand after a preposition
/// would be roke or roko.
fn roka_is_deadline(clause: &Clause, i: usize) -> bool {
    #[rustfmt::skip] // rows of words
    const LIMITS: &[&str] = &[
        "podaljšanje", "podaljšanja", "podaljšanju", "podaljšanjem", "iztek", "izteka", "izteku", "iztekom", "potek",
        "poteka", "poteku", "potekom", "služenje", "služenja", "služenju", "služenjem", "določitev", "določitve",
        "predpisuje", "upoštevanje", "upoštevanja", "spoštovanje", "spoštovanja", "zamuda", "zamude", "zamudo", "zamik",
        "zamika", "zamiku", "prestavitev", "prestavitve", "podaljševanje", "podaljševanja", "prekoračitev",
        "prekoračitve", "odsluženje", "odsluženja", "pretek", "preteka", "preteku", "štetje", "štetja", "zaključek",
        "zaključka", "dolžina", "dolžine", "sprememba", "spremembe", "konec", "konca", "začetek", "začetka",
    ];
    clause.before(i).is_some_and(|prev| is_preposition(prev) || ends_with(prev, "ega") || is_one_of(prev, LIMITS))
}

/// The first words of a clause that are no hand: a noun after a negated verb of
/// being is in the genitive, and that genitive can only be the name's or the
/// deadline's (Roka že ni, Roka ni več, Roka ni bilo na sestanku, Roka ne bo; a
/// listener's examples, November 2026), but a hand is in the nominative with an
/// adjective or a participle (Roka ni zlomljena, Roka ni več bolna, Roka ne bo
/// bolela). A verb in the first or second person or in the plural has another
/// subject, so roka is its object (Roka nisem videl, Roka poznam, Roka so
/// našli), and so it is before a surname (Roka Žarnića).
fn roka_is_absent_or_object(clause: &Clause, i: usize) -> bool {
    const OPENERS: &[&str] =
        &["in", "pa", "ampak", "a", "toda", "ker", "da", "če", "ko", "zakaj", "kje", "ali", "še", "tudi", "vendar"];
    const ADVERBS: &[&str] = &[
        "že", "še", "tudi", "pa", "sploh", "danes", "včeraj", "jutri", "zdaj", "sedaj", "trenutno", "nikjer", "nikoli",
        "nikdar", "več", "tu", "tukaj", "tam", "doma", "notri", "zunaj", "tja", "dolgo", "spet", "vedno", "res", "žal",
    ];
    #[rustfmt::skip] // rows of words
    const PLACES: &[&str] = &[
        "službi", "šoli", "pisarni", "sestanku", "delu", "poti", "treningu", "tekmi", "vaji", "predavanju", "zabavi",
        "nas", "vas", "njih", "meni", "tebi",
    ];
    const OTHER_SUBJECT: &[&str] = &[
        "sem", "sva", "sta", "smo", "ste", "so", "nisem", "nisi", "nisva", "nista", "nismo", "niste", "niso", "bom",
        "boš", "bova", "bosta", "bomo", "boste", "bodo",
    ];
    #[rustfmt::skip] // rows of words
    const IMPERATIVES: &[&str] = &[
        "pokliči", "pokličite", "vprašaj", "vprašajte", "počakaj", "počakajte", "pozdravi", "pozdravite", "poišči",
        "poiščite", "obvesti", "obvestite", "kliči", "kličite",
    ];
    if clause.before(i).is_some_and(|prev| !is_one_of(prev, OPENERS)) {
        return false;
    }
    let n = clause.words.len();
    // The first word from `k` on that is no adverb
    let past_adverbs = |k: usize| (k..n).find(|&k| !clause.one_of(k, ADVERBS)).unwrap_or(n);
    let k = past_adverbs(i + 1);
    let Some(word) = clause.word(k) else { return false };
    if eq(word, "ni") {
        if clause.is(k + 1, "bilo") {
            return true;
        }
        let k = past_adverbs(k + 1);
        return k == n || (k + 2 == n && clause.one_of(k, &["v", "na", "pri"]) && clause.one_of(k + 1, PLACES));
    }
    if eq(word, "ne") {
        return if clause.is(k + 1, "bo") {
            past_adverbs(k + 2) == n
        } else {
            clause.word(k + 1).is_some_and(addressed_verb)
        };
    }
    is_one_of(word, OTHER_SUBJECT)
        || is_one_of(word, IMPERATIVES)
        || addressed_verb(word)
        || (k == i + 1 && clause.capital(i) && clause.capital(k))
}

/// roki and roke are the hand's (v roki, obe roki, dvigni roke) unless a word
/// around them says they are deadlines, in the plural (SP rók -a; SUK 1.1 has
/// 12 of them against 75 hands): an adjective that goes with deadlines (the
/// Collocations Dictionary of Modern Slovene at CJVT: plačilni roki, izpitne
/// roke), the instrumental (z roki, prijavnimi roki: a hand's is roko, rokama),
/// the plural verb (da so roki, roki so kratki) or what a deadline is for (roki
/// za prijavo, roki plačila).
fn roki_roke_reading(clause: &Clause, i: usize) -> Option<&'static str> {
    #[rustfmt::skip] // rows of words
    const ADJECTIVES_I: &[&str] = &[
        "plačilni", "časovni", "dobavni", "izvedbeni", "odpovedni", "izpitni", "prijavni", "razpisni", "pritožbeni",
        "zastaralni", "zakonski", "zakoniti", "pogodbeni", "predvideni", "dogovorjeni", "določeni", "predpisani",
        "skrajni", "prestopni", "garancijski", "setveni", "vpisni", "kratki", "daljši", "krajši", "zimski", "jesenski",
        "spomladanski", "novi", "vsi", "ti", "taki", "taksni", "predvidljivi",
    ];
    #[rustfmt::skip] // rows of words
    const ADJECTIVES_E: &[&str] = &[
        "plačilne", "časovne", "dobavne", "izvedbene", "odpovedne", "izpitne", "prijavne", "razpisne", "pritožbene",
        "zastaralne", "zakonske", "zakonite", "pogodbene", "predvidene", "dogovorjene", "predpisane", "skrajne",
        "prestopne", "garancijske", "setvene", "vpisne", "zimske", "jesenske", "spomladanske", "predvidljive",
    ];
    const FOR_WHAT: &[&str] =
        &["plačila", "dobave", "trajanja", "vračanja", "izvedbe", "oddaje", "prijave", "odplačila"];
    let prev = clause.before(i);
    if clause.is(i, "roke") {
        return prev.is_some_and(|prev| is_one_of(prev, ADJECTIVES_E)).then_some("róke");
    }
    if let Some(prev) = prev {
        if is_one_of(prev, &["z", "s"]) {
            return Some("róki");
        }
        if is_preposition(prev) || is_one_of(prev, &["dve", "obe"]) {
            return None;
        }
        if is_one_of(prev, ADJECTIVES_I)
            || ends_with_any(prev, &["nimi", "skimi"])
            || is_one_of(prev, &["so", "bodo", "niso"])
        {
            return Some("róki");
        }
    }
    let deadlines = clause.one_of(i + 1, &["so", "niso", "bodo"])
        || clause.one_of(i + 1, FOR_WHAT)
        || (clause.is(i + 1, "za") && prev.is_none());
    deadlines.then_some("róki")
}

/// The names a screen reader speaks for the quotation marks and brackets
/// around a word when its punctuation level is high (NVDA's Slovenian symbol
/// names: narekovaj, začetni and končni narekovaj, dvojni spodnji narekovaj,
/// dvojni začetni and zaključni narekovaj for » and «, okrogli oklepaj, oglati
/// zaklepaj, zaviti oklepaj, leva and desna kljukica, opuščaj, manjše, večje,
/// zvezdica, navpičnica, podčrtaj, pomišljaj; the engine's own spelled names
/// are the same words). The index after the run of such names that starts at
/// `k`, or `k` when there is none.
fn after_symbol_names(words: &[Vec<char>], k: usize) -> usize {
    #[rustfmt::skip] // rows of words
    const NAMES: &[&str] = &[
        "narekovaj", "narekovaji", "oklepaj", "zaklepaj", "kljukica", "opuščaj", "apostrof", "manjše", "večje",
        "zvezdica", "navpičnica", "podčrtaj", "krativec", "ostrivec", "poševnica", "pomišljaj", "črtica", "vezaj",
    ];
    #[rustfmt::skip] // rows of words
    const QUALIFIERS: &[&str] = &[
        "začetni", "končni", "zaključni", "dvojni", "enojni", "spodnji", "zgornji", "levi", "desni", "leva", "desna",
        "okrogli", "oglati", "zaviti", "kratka", "dolga", "double",
    ];
    let is = |k: usize, list: &[&str]| words.get(k).is_some_and(|w| is_one_of(w, list));
    let mut j = k;
    loop {
        let name = (j..).find(|&q| !is(q, QUALIFIERS)).unwrap_or(j);
        if !is(name, NAMES) {
            return j;
        }
        j = name + 1;
    }
}

/// The index of the first name of the run of such names that ends before
/// `k`, or `k`.
fn before_symbol_names(words: &[Vec<char>], k: usize) -> usize {
    let mut start = 0;
    while start < k {
        // The longest run of names starting at `start` that ends exactly at k
        let end = after_symbol_names(words, start);
        if end == k {
            return start;
        }
        start = if end > start && end < k { end } else { start + 1 };
    }
    k
}

/// meni after a preposition: the pronoun's form after those with the dative
/// and the locative (k meni, proti meni, o meni, pri meni, ob meni, po meni,
/// v meni, na meni), but after those with the accusative or the instrumental
/// the pronoun is mene or mano (pod mano, za mano, skozi mene), so meni is
/// the menu (pod meni, a Slovenian listener's example; SP 2001 mení -ja).
/// After v and na it is the menu's accusative when something moves there
/// (pojdi v meni, kliknite na meni, nazaj v meni); in SUK 1.1 the other 17
/// are the pronoun. Before those the menu follows an adjective or a possessive
/// (glavni meni, vaš meni, padajoči meni: the Collocations Dictionary of Modern
/// Slovene at CJVT) or a verb that takes a menu (odpri meni), and comes before
/// its name, itself with a capital (Meni Start) or after v or na (v meni
/// Dokumenti), and inside a clause with a capital it is a name (Gumb Meni). A
/// small meni before a capital is the verb of the news (meni Kučan, tako meni
/// Križanič: 30 occurrences in SUK 1.1), and so is meni after a surname that
/// looks like an adjective (Franco Frattini meni). The pronoun comes after a
/// word that stresses it (tudi meni, samo meni, ti meni), after an auxiliary or a
/// participle, since the verb's present cannot follow those (je meni rekel,
/// ponudili so meni), and before what is said of whom (meni ne, meni je všeč,
/// meni se zdi); but what opens, closes and is empty is the menu (Meni se
/// odpre, Meni je prazen). An adjective before it, whatever its meaning (zaprti
/// meni nedavno, a listener's example; odprti meni, najboljši meni), says the
/// menu by itself: the verb's subject is not an adjective and a pronoun has no
/// use for one. Those the ending does not show are in the table of Sloleks
/// (`is_definite_adjective()`). Of the 237 tokens of the verb and the pronoun in
/// SUK 1.1 none stands after one of these.
///
/// A listener's report of 2026-11-03: Meni daj žogo, Meni prinesi kavo, Meni
/// podari ljubezen, Meni bo hladno are the pronoun mêni (SSKJ², SP: jàz, mêne,
/// mêni, the open e), and NVDA meni, JAWS meni, kontekst meni, context meni,
/// meni start, start meni are the menu. So an acronym or the name of a menu's
/// kind before it, or the name of a menu after it, says the menu; a verb that
/// gives, brings, shows or tells to someone (daj, prinesi, podari, pokaži,
/// povej ...) or an auxiliary (bo, sem, so, bi) after it says the pronoun; and
/// so does a capitalized Meni at the head of its clause before any word: in
/// SUK 1.1 the verb at a clause head before a word is written small after a
/// comma before its subject (, meni župan; , meni Kučan: 36 tokens, left as
/// they are) or capitalized before a particle and its clause of da (Meni tudi,
/// da; Meni pa, da: 5, read by `clause_reading()` from the mark of the comma),
/// never before a verb or an auxiliary (0 of 242). SDS meni, da je prav keeps
/// the verb: an acronym before it, last in its clause before a clause of da.
fn meni_reading(clause: &Clause, i: usize) -> Option<&'static str> {
    const MENU: Option<&str> = Some("mení");
    const PRONOUN: Option<&str> = Some("mêni");
    const ACCUSATIVE_ONLY: &[&str] = &["pod", "nad", "pred", "med", "za", "čez", "skozi"];
    #[rustfmt::skip] // rows of words
    const TOWARDS: &[&str] = &[
        "pojdi", "pojdite", "pojdimo", "pojdiva", "pojdem", "pojdeš", "gre", "greš", "grem", "gremo", "greste", "gredo",
        "gresta", "odidi", "odidite", "vstopi", "vstopite", "vstopimo", "vstop", "stopi", "stopite", "preidi",
        "preidite", "preide", "prehod", "vrni", "vrnite", "vrnimo", "vrnitev", "nazaj", "skoči", "skočite", "premakni",
        "premaknite", "povleci", "povlecite", "dodaj", "dodajte", "dodajanje", "pripni", "pripnite", "vstavi",
        "vstavite", "klikni", "kliknite", "klik", "tapni", "tapnite", "pritisni", "pritisnite", "dotakni", "dotaknite",
    ];
    // What a noun's nominative or accusative is qualified by, without the
    // adjective's endings below
    const QUALIFYING: &[&str] = &[
        "vaš", "tvoj", "moj", "naš", "svoj", "njihov", "njegov", "njen", "novi", "levi", "hitri", "stari", "ta",
        "tale", "tisti", "isti", "ves", "cel", "celoten", "poseben", "dober", "okusen", "bogat", "pester", "kratek",
        "velik", "majhen", "lep", "nov", "star", "lahek", "prvi", "drugi", "tretji", "četrti", "vsak", "neki", "nek",
        "kakšen", "cenejši", "dražji", "večji", "manjši",
        "skriti", // the adjective's form is also the infinitive, which is ten times as frequent
    ];
    // The state of a menu, right before or after it: Zaprt meni, Meni zaprt
    #[rustfmt::skip] // rows of words
    const MENU_STATES: &[&str] = &[
        "odprt", "zaprt", "skrit", "prikazan", "izbran", "prazen", "razširjen", "strnjen", "onemogočen", "omogočen",
        "aktiven", "neaktiven", "naložen",
    ];
    // Nouns it is the name or the kind of (Gumb meni, Tipka meni)
    const NAMING: &[&str] = &[
        "gumb", "gumba", "ikona", "ikone", "ikono", "tipka", "tipke", "tipko", "zavihek", "zavihka", "postavka",
        "možnost", "ukaz", "element", "vrstica", "izbira",
    ];
    #[rustfmt::skip] // rows of words
    const TAKES_MENU: &[&str] = &[
        "odpri", "odprite", "odpre", "odprl", "odprla", "zapri", "zaprite", "zapre", "zaprl", "zaprla", "prikaži",
        "prikažite", "pokaži", "pokažite", "izberi", "izberite", "izbral", "izbrala", "skrij", "skrijte", "poišči",
        "poiščite", "razširi", "razširite", "strni", "strnite", "aktiviraj", "aktivirajte", "uporabi", "uporabite",
        "klikni", "kliknite", "tapni", "tapnite", "sestavi", "sestavite", "sestavil", "sestavila", "pripravi",
        "pripravite", "pripravil", "pripravila", "ima", "imajo", "ponuja", "ponujajo", "uredi", "uredite", "nastavi",
        "nastavite", "spremeni", "spremenite", "prilagodi", "prilagodite", "preglej", "preglejte", "najdi", "najdite",
        // The verbs the Collocations Dictionary lists with a menu as the object
        "odpira", "odpirajo", "odpiraj", "odpirajte", "izbira", "izbirajo", "izbiraj", "izbirajte", "sestavlja",
        "sestavljajo", "sestavljaj", "sestavljajte", "uporablja", "uporabljajo", "uporabljaj", "uporabljajte",
        "upravlja", "upravljajo", "upravljaj", "upravljajte", "menjaj", "menjajte", "oblikuj", "oblikujte", "ustvari",
        "ustvarite", "ustvaril", "ustvarila", "izdelaj", "izdelajte", "izdelal", "izdelala", "vsebuje", "vsebujejo",
        "vključuje", "vključujejo",
    ];
    // A word before it that stresses the pronoun, or a verb with a dative
    #[rustfmt::skip] // rows of words
    const BEFORE_PRONOUN: &[&str] = &[
        "tudi", "samo", "le", "prav", "celo", "vsaj", "predvsem", "zlasti", "kar", "ti", "tebi", "njemu", "njej", "nam",
        "vam", "niti", "sem", "si", "je", "bi", "smo", "ste", "so", "sva", "sta", "bom", "boš", "bo", "bomo", "boste",
        "bodo", "bova", "bosta", "nisem", "nisi", "ni", "nismo", "niste", "niso", "daj", "dajte", "reci", "recite",
        "povej", "povejte", "pomagaj", "pomagajte", "piši", "pišite", "zaupaj", "verjemi", "oprosti", "vrni", "vrnite",
        "prinesi", "prinesite", "pošlji", "pošljite", "hvala", "gre", "gredo", "edino",
    ];
    const NOT_PARTICIPLES: &[&str] =
        &["general", "admiral", "kardinal", "portal", "tribunal", "kanal", "kapital", "šola", "sila", "skupščina"];
    // After it: said of whom. The verb's present does not take these.
    #[rustfmt::skip] // rows of words
    const AFTER_PRONOUN: &[&str] = &[
        "ne", "že", "nič", "osebno", "doma", "všeč", "znano", "znana", "znan", "zdi", "zdijo", "ugaja", "diši", "paše",
        "manjka", "preostane", "pripada", "dovolj", "mar", "žal", "dolgčas", "vseeno", "hladno", "vroče", "toplo",
        "slabo", "dobro", "lepo", "jasno", "nerazumljivo", "se", "je", "ni",
        // the auxiliaries, which the verb's present cannot take: Meni bo
        // hladno, Meni so povedali, Meni bi bilo všeč
        "bo", "bom", "boš", "bomo", "boste", "bodo", "bova", "bosta", "bi", "sem", "si", "smo", "ste", "so", "sva",
        "sta",
    ];
    // A predicate anywhere after a pronoun at the head (Meni za to ni mar)
    const DATIVE_PREDICATE: &[&str] =
        &["mar", "všeč", "žal", "zdi", "zdijo", "dolgčas", "vseeno", "ugaja", "manjka", "preostane"];
    // What a menu does, after meni se
    #[rustfmt::skip] // rows of words
    const MENU_VERBS_SE: &[&str] = &[
        "odpre", "odpira", "zapre", "zapira", "prikaže", "prikazuje", "pokaže", "pokazuje", "razširi", "skrije",
        "pojavi", "naloži", "nalaga", "strne", "spremeni", "premakne", "premika", "osveži",
    ];
    // What a menu is, after meni je, ni, bo
    #[rustfmt::skip] // rows of words
    const MENU_PREDICATES: &[&str] = &[
        "odprt", "zaprt", "prazen", "skrit", "viden", "prikazan", "izbran", "onemogočen", "omogočen", "aktiven",
        "neaktiven", "razširjen", "strnjen", "naložen", "poln", "dolg", "pregleden", "nepregleden", "zapleten",
        "sestavljen", "dostopen", "nedostopen",
    ];
    // What a menu does, after it
    #[rustfmt::skip] // rows of words
    const MENU_VERBS: &[&str] = &[
        "ima", "vsebuje", "omogoča", "prikazuje", "ponuja", "vključuje", "stane", "sestavljajo", "odpre", "zapre",
        "izgine",
    ];
    // A genitive or a name of what the menu is of
    #[rustfmt::skip] // rows of words
    const MENU_OF: &[&str] = &[
        "aplikacij", "aplikacije", "možnosti", "nastavitev", "orodij", "orodja", "jedi", "dneva", "restavracije",
        "palete", "telefona", "gumba", "datoteke", "pomoči", "povezav", "gostilne", "vrstica", "vrstice",
    ];
    #[rustfmt::skip] // rows of words
    const MENU_FOR: &[&str] = &[
        "kosilo", "večerjo", "zajtrk", "otroke", "goste", "vegetarijance", "krmarjenje", "navigacijo", "dostopnost",
        "pomoč", "izbiro", "pokušino", "cel", "ves",
    ];
    // The kind or the program of a menu before it (kontekst meni, start meni,
    // NVDA meni) and the name of a menu after it, in small letters (meni start,
    // Meni orodja); not a word that can be the subject of what happens to me
    // (aplikacija meni ne dela, Meni pomoč ni potrebna)
    // (the spellings are those after rewrite_foreign(): x = ks, w = v)
    const MENU_KINDS: &[&str] =
        &["kontekst", "contekst", "start", "nvda", "jaws", "vindovs", "podmeni", "popup", "sistemski"];
    const MENU_NAMES_AFTER: &[&str] = &["start", "nvda", "jaws", "kontekst", "contekst", "vindovs", "orodja"];
    // A verb that gives, brings, shows or tells to someone, after the
    // pronoun: Meni daj žogo, meni prinesi kavo, Meni podari ljubezen
    #[rustfmt::skip] // rows of words
    const DATIVE_VERBS: &[&str] = &[
        "daj", "dajte", "dajmo", "prinesi", "prinesite", "podari", "podarite", "pokaži", "pokažite", "povej", "povejte",
        "reci", "recite", "pošlji", "pošljite", "vrni", "vrnite", "pusti", "pustite", "posodi", "posodite", "kupi",
        "kupite", "skuhaj", "skuhajte", "nalij", "nalijte", "pripravi", "pripravite", "piši", "pišite", "napiši",
        "napišite", "pojasni", "pojasnite", "razloži", "razložite", "zaupaj", "zaupajte", "verjemi", "verjemite",
        "oprosti", "oprostite", "pomagaj", "pomagajte", "svetuj", "svetujte", "izroči", "izročite", "predaj",
        "predajte", "plačaj", "plačajte", "obljubi", "obljubite", "dovoli", "dovolite", "omogoči", "omogočite",
        "ponudi", "ponudite", "sporoči", "sporočite", "odgovori", "odgovorite", "prepusti", "prepustite", "prihrani",
        "prihranite", "naroči", "naročite", "pokliči", "pokličite", "zapoj", "zapojte", "zaigraj", "zaigrajte",
        "prizanesi", "prizanesite", "odpusti", "odpustite", "nakaži", "nakažite", "dela", "daje", "prinaša", "pošilja",
        "kaže", "pomaga", "pravi", "govori", "piše", "manjka", "ustreza", "odgovarja", "pripada", "ostane", "preostane",
        "gre", "ni",
    ];
    let words = clause.words;
    let n = words.len();
    if n == 1 {
        return MENU;
    }
    let prev = clause.before(i);
    let next = clause.word(i + 1);
    let capitalized = clause.capital(i);
    let before_capital = next.is_some() && clause.capital(i + 1);
    if clause.one_of(i + 1, MENU_NAMES_AFTER) {
        return MENU; // meni start, Meni orodja
    }
    // The name of a menu in quotation marks or brackets, named by a screen
    // reader whose punctuation is on: meni narekovaj Start narekovaj, meni
    // dvojni začetni narekovaj start dvojni zaključni narekovaj (»start«),
    // meni oklepaj start zaklepaj, and narekovaj start narekovaj meni (a
    // listener's report of 2026-11-03: meni "start" read the pronoun). The
    // marks themselves are silent in the engine and leave meni start; the
    // word inside them says the menu, as the name alone does.
    let after_marks = after_symbol_names(words, i + 1);
    if after_marks > i + 1 && after_marks < n {
        return MENU;
    }
    let before_marks = before_symbol_names(words, i);
    if before_marks < i
        && before_marks > 0
        && (clause.one_of(before_marks - 1, MENU_KINDS) || clause.is_acronym(before_marks - 1))
    {
        return MENU;
    }
    // Meni 1, Meni 2: a menu named by a number, nothing after it (the
    // listener's answer of 2026-11-03; Meni ena stvar ni jasna is the pronoun)
    if prev.is_none() && capitalized && next.is_some() && words.iter().skip(1).all(|w| is_number_word(w)) {
        return MENU;
    }
    // A verb of menus or of going somewhere, two words or more before it
    let earlier =
        |lists: &[&[&str]]| words[..i.saturating_sub(1)].iter().any(|w| lists.iter().any(|list| is_one_of(w, list)));
    if let Some(prev) = prev {
        if is_one_of(prev, ACCUSATIVE_ONLY) {
            return MENU;
        }
        if is_one_of(prev, &["v", "na"]) {
            // v meni Dokumenti
            return if before_capital || earlier(&[TOWARDS]) { MENU } else { PRONOUN };
        }
        if is_preposition(prev) {
            return PRONOUN;
        }
        let acronym = clause.is_acronym(i - 1);
        // NVDA meni, JAWS meni, kontekst meni; but SDS meni, da je prav
        // is the news verb with its subject: last in its clause before
        // a clause of da, with no verb of menus earlier in the clause
        if (acronym || is_one_of(prev, MENU_KINDS))
            && !(acronym && clause.before_complement && i + 1 == n && !earlier(&[TAKES_MENU, TOWARDS]))
        {
            return MENU;
        }
        if before_capital && capitalized {
            return MENU; // To je Meni Start
        }
        if [QUALIFYING, NAMING, TAKES_MENU, MENU_STATES].iter().any(|list| is_one_of(prev, list)) {
            return MENU;
        }
        let a_name = clause.capital(i - 1) && i > 1;
        if !a_name && (is_adjective_ending(prev) || is_definite_adjective(prev)) {
            return MENU; // glavni meni, stranski meni, zadnji meni, padajoči meni, zaprti meni
        }
        if capitalized {
            return MENU; // Gumb Meni
        }
        // ponudili meni, je rekla meni: a participle, not a name or a noun
        let participle = !clause.capital(i - 1)
            && prev.len() >= 4
            && !is_one_of(prev, NOT_PARTICIPLES)
            && ends_with_any(prev, &["li", "le", "la", "al", "il", "el"]);
        if is_one_of(prev, BEFORE_PRONOUN) || participle {
            return PRONOUN;
        }
    } else if before_capital && capitalized {
        return MENU; // Meni Start, Meni Datoteka
    }
    let next = next?;
    if is_one_of(next, MENU_STATES) {
        return MENU; // Meni zaprt
    }
    if eq(next, "se") && clause.one_of(i + 2, MENU_VERBS_SE) {
        return MENU;
    }
    if is_one_of(next, &["je", "ni", "bo"]) && clause.one_of(i + 2, MENU_PREDICATES) {
        return MENU;
    }
    if prev.is_none() {
        if is_one_of(next, MENU_VERBS) || is_one_of(next, MENU_OF) || (eq(next, "za") && clause.one_of(i + 2, MENU_FOR))
        {
            return MENU;
        }
        if eq(next, "pa") || words[i + 1..].iter().any(|w| is_one_of(w, DATIVE_PREDICATE)) {
            return PRONOUN;
        }
    }
    if is_one_of(next, AFTER_PRONOUN) || is_one_of(next, DATIVE_VERBS) {
        return PRONOUN;
    }
    if prev.is_none() && is_one_of(next, NAMING) {
        return MENU; // Meni gumb, Meni ikona: a screen reader's name and role
    }
    // Meni daj žogo, Meni podari ljubezen: capitalized at the head of its
    // clause before any word, the pronoun (see above; the verb at a
    // sentence head stands before a comma, Meni, da; Meni tudi, da)
    (prev.is_none() && capitalized).then_some("mêni")
}

/// vprašaj is the imperative vprášaj of vprášati (SP 2001: vprášaj -te tudi
/// -ájte; SSKJ² vprášati tudi vprašáti) or the question mark vprašáj (SSKJ²,
/// SP: vprašáj -a). In SUK 1.1 the spelling is the imperative once (Daj no,
/// vprašaj že) and the noun never; Sloleks's count for the noun (7,105 against
/// 11) is its tagger's, and had made the table read the noun, so Vprašaj me
/// came out as the question mark (a listener's report of 2026-11-03). The
/// noun: alone in its clause, which is how a screen reader names the
/// character ? (but Vprašaj! alone is the command), after a preposition (pod
/// vprašaj), a quantifier or an adjective (en vprašaj, velik vprašaj, dvojni
/// vprašaj), a verb that writes or sets one (napiši vprašaj, postavlja pod
/// vprašaj), and last in a clause of a sentence that ends in a question mark
/// when no object clitic comes before it: NVDA's punctuation reading puts the
/// name before the mark (Kako si vprašaj?), and a command does not end a
/// question.
fn vprasaj_reading(clause: &Clause, i: usize) -> &'static str {
    #[rustfmt::skip] // rows of words
    const QUANTIFIERS: &[&str] = &[
        "en", "enega", "kakšen", "kak", "vsak", "velik", "dvojni", "obrnjeni", "končni", "zadnji", "ta", "tisti", "še",
        "dodaten", "nov", "velikanski", "ogromen", "rdeč", "črn",
    ];
    #[rustfmt::skip] // rows of words
    const WRITES: &[&str] = &[
        "postavi", "postavlja", "postavljajo", "postavil", "postavila", "postaviti", "postavimo", "postavite", "napiši",
        "napišite", "piši", "pišite", "vstavi", "vstavite", "dodaj", "dodajte", "vtipkaj", "vtipkajte", "vnesi",
        "vnesite", "izbriši", "izbrišite", "odstrani", "odstranite", "briše", "visi", "pomeni", "predstavlja",
        "napisal", "napisala", "stoji", "manjka",
    ];
    const OBJECT_CLITICS: &[&str] = &[
        "me", "ga", "jo", "jih", "nas", "vas", "ju", "mu", "ji", "jim", "to", "kaj", "koga", "ne", "že", "pa", "raje",
        "kar", "no", "vedno", "prosim", "potem", "zdaj",
    ];
    const NOUN: &str = "vprašáj";
    const COMMAND: &str = "vprášaj";
    let n = clause.words.len();
    if n == 1 {
        return if clause.exclamation { COMMAND } else { NOUN };
    }
    let prev = clause.before(i);
    let marks = |prev: &[char]| {
        is_preposition(prev)
            || is_one_of(prev, QUANTIFIERS)
            || is_one_of(prev, WRITES)
            || is_adjective_ending(prev)
            || is_definite_adjective(prev)
    };
    let ends_question = i + 1 == n && clause.question && !prev.is_some_and(|prev| is_one_of(prev, OBJECT_CLITICS));
    if prev.is_some_and(marks) || ends_question { NOUN } else { COMMAND }
}

/// je: the auxiliary of bíti "is", unstressed [jɛ] and stressed jè (SP 2001
/// poudarjeno sèm sì jè; Sloleks jè), or the verb of eating jé (SSKJ², SP
/// jésti jém, 3. os. jé, long close e), as a listener asked on 2026-11-03:
/// On je krompir, Ona je palačinke, Marko zelo veliko je against On je
/// priden, Ona je zelo pametna. In SUK 1.1 je is the auxiliary 24,471 times
/// and "eats" 107 times, so "is" is the reading unless the clause says
/// otherwise, by cues measured there: ne right before it (the negated
/// auxiliary is ni; 10 of 10 in SUK; SSKJ² od skrbi ne je, SP Kdor ne
/// dela, naj ne jé); a food noun as its object (the table of food nouns, from
/// the collocations of jesti), in the accusative, or in the genitive after an
/// amount or ne (veliko sadja, premalo mesa), after adverbs, an adjective
/// or za kosilo (je nezdravo hrano, je korenčkovo juho, je za kosilo
/// ribe), with no l-participle in the clause (Videla je ribe, je hrano
/// darovala: the perfect tense, 5 of 5 auxiliaries SUK had before a food
/// accusative), no clitic, question word or to/tisto right before je (se
/// je, jo je, Kaj je, To je krompir), and, where the form is the
/// nominative too (krompir, meso, palačinke, kosilo), a subject: a pronoun
/// or any word that is not a function word or an adverb of time (Hrvoje je
/// krompir, Priloga je krompir at the maintainer's word: a food noun after
/// je is the eating; not Zdaj je kosilo, Ob dvanajstih je kosilo, where no
/// subject is left); an adverb of amount or manner of eating before je ending its
/// clause (Marko zelo veliko je, On rad je; SSKJ² dosti, rad, zmerno je)
/// after a subject, since Lepo je, Normalno je, da and Le malo je tistih,
/// the auxiliaries SUK had after such an adverb, have none; and a utensil
/// after it (Kralj, ki je z vilicami). Je at the head of a clause stays the
/// auxiliary: Je krompir. "It is potato." and Je veliko solate. "There is a
/// lot of salad." are as good as the eating.
fn je_reading(clause: &Clause, i: usize) -> Option<&'static str> {
    const EATS: Option<&str> = Some("jé");
    // The subject's pronouns, and those that mean one who eats
    const PRONOUNS: &[&str] = &[
        "on", "ona", "ono", "oni", "one", "onadva", "onidve", "ki", "kdor", "kateri", "katera", "nekdo", "vsak",
        "vsakdo", "marsikdo", "vsi", "vsakdor", "jaz", "ti", "mi", "vi",
    ];
    // Between the subject and je, or je and its object: amounts (a genitive
    // may follow them), manner, time, particles
    #[rustfmt::skip] // rows of words
    const AMOUNTS: &[&str] = &[
        "veliko", "malo", "preveč", "premalo", "dosti", "precej", "ogromno", "manj", "več", "največ", "nekaj", "mnogo",
        "dovolj", "toliko", "koliko", "dva", "dve", "tri", "štiri", "pet", "šest", "sedem", "osem", "devet", "deset",
        "par", "kup", "polno", "kos", "košček", "kozarec", "skodelico", "krožnik", "žlico", "porcijo", "pol", "četrt",
        "kilo", "kilogram", "liter",
    ];
    #[rustfmt::skip] // rows of words
    const MANNER: &[&str] = &[
        "rad", "rada", "rado", "radi", "rade", "nerad", "nerada", "neradi", "najraje", "raje", "hitro", "počasi",
        "prehitro", "hitreje", "počasneje", "zdravo", "nezdravo", "dobro", "pogosto", "redko", "redno", "zmerno",
        "hlastno", "zvečer", "zjutraj", "opoldne", "doma", "zunaj", "sam", "sama", "sami", "same",
    ];
    #[rustfmt::skip] // rows of words
    const PARTICLES: &[&str] = &[
        "zelo", "precej", "tako", "res", "zares", "kar", "bolj", "najbolj", "čisto", "vedno", "včasih", "še", "že",
        "tudi", "pa", "sicer", "navadno", "običajno", "zdaj", "potem", "danes", "samo", "le", "vsak", "dan", "spet",
        "zopet", "ponavadi", "večinoma", "seveda", "morda", "mogoče", "verjetno", "najprej", "skupaj", "jutri",
        "včeraj", "popoldne", "ponoči", "kmalu", "takrat", "tedaj", "nato", "letos", "lani", "zjutraj", "zvečer",
        "opoldne", "dopoldne", "trenutno", "sedaj", "tam", "tu", "tukaj",
    ];
    const MEALS: &[&str] = &["zajtrk", "kosilo", "večerjo", "malico", "predjed", "sladico"];
    // Not roko, rokami, prsti: Gospa je z roko segla v žep (SUK 1.1)
    const UTENSILS: &[&str] = &["žlico", "žličko", "vilicami", "vilico", "nožem", "paličicami", "palčkami"];
    const NEVER_SUBJECT: &[&str] =
        &["to", "tole", "tisto", "ono", "ni", "kaj", "kar", "nič", "ničesar", "vse", "tega", "vsega", "česa", "nekaj"];
    // An adverb of eating for the clause-final je: manner, or an amount
    // that is an adverb (veliko, malo), not a numeral (vprašanja (7) je, da)
    const AMOUNT_ADVERBS: &[&str] =
        &["veliko", "malo", "preveč", "premalo", "dosti", "precej", "ogromno", "manj", "več", "največ", "mnogo"];

    let words = clause.words;
    let n = words.len();
    let prev = clause.before(i)?;
    if eq(prev, "ne") {
        return EATS;
    }
    let is_modifier = |w: &[char]| is_one_of(w, AMOUNTS) || is_one_of(w, MANNER) || is_one_of(w, PARTICLES);
    let function = |w: &[char]| is_function_word(w) || is_proclitic(w) || is_enclitic(w);
    // An l-participle: the perfect tense, whose je is the auxiliary (Videla
    // je ribe, Podala jima je kavo, je hrano darovala). The words between je
    // and its object are not asked (je zrele banane).
    let participle_like = |k: usize| {
        let Some(w) = clause.word(k) else { return false };
        if w.len() < 4 || function(w) || food_cases(w) != 0 || (clause.capital(k) && (k > 0 || is_given_name(w))) {
            return false;
        }
        matches!(w, [.., 'l'] | [.., 'l', 'a' | 'o' | 'i' | 'e'])
    };
    // The word right before je: no clitic (se je, jo je, ga je: the perfect
    // tense or Strah ga je), no question word, nothing that cannot eat
    let cannot_eat = |w: &[char]| is_enclitic(w) || is_question_word(w) || is_one_of(w, NEVER_SUBJECT);
    if cannot_eat(prev) {
        return None;
    }
    // The subject: the word before the adverbs and prepositional phrases
    // between it and je (On za kosilo rad je juho); a clause may have none
    // (Za kosilo običajno je korenčkovo juho; Lepo je)
    let mut s = i;
    let mut manner_adverb = false;
    loop {
        while let Some(before) = clause.before(s).filter(|before| is_modifier(before)) {
            manner_adverb |= is_one_of(before, MANNER) || is_one_of(before, AMOUNT_ADVERBS);
            s -= 1;
        }
        if s >= 2 && is_preposition(&words[s - 2]) && !is_modifier(&words[s - 1]) {
            s -= 2;
        } else {
            break;
        }
    }
    let subject = clause.before(s);
    // Kaj je, To je krompir, Kosilo je krompir
    if subject.is_some_and(|subject| cannot_eat(subject) || food_cases(subject) & FOOD_NOMINATIVE != 0) {
        return None;
    }
    // A subject: a pronoun, or any word that is not a function word (Hrvoje,
    // Priloga, otrok, pes; not tega, zdaj)
    let lexical_subject = subject
        .is_some_and(|subject| is_one_of(subject, PRONOUNS) || (!is_function_word(subject) && !is_proclitic(subject)));
    if (0..i).any(participle_like) {
        return None;
    }
    // 1. The object: a food noun after je, after an amount, an adverb, up
    // to two adjectives or "za kosilo". A participle in -l, -la, -lo, -li
    // between them is the perfect tense (je izgubilo Žito, je zmanjkalo
    // hrane); -le is left to the adjectives (je zrele banane).
    let mut genitive_ok = false;
    let mut k = i + 1;
    let mut adjectives = 0;
    let mut skipped = 0;
    while let Some(w) = clause.word(k).filter(|_| skipped < 4) {
        skipped += 1;
        if is_one_of(w, AMOUNTS) {
            genitive_ok = true;
            k += 1;
            continue;
        }
        if is_modifier(w) {
            k += 1;
            continue;
        }
        if eq(w, "za") && clause.one_of(k + 1, MEALS) {
            k += 2;
            skipped += 1;
            continue;
        }
        let cases = food_cases(w);
        if cases & FOOD_ACCUSATIVE != 0 || (genitive_ok && cases & FOOD_GENITIVE != 0) {
            let subject_missing = cases & FOOD_NOMINATIVE != 0 && !lexical_subject;
            return if subject_missing || (k + 1..n).any(participle_like) { None } else { EATS };
        }
        if participle_like(k) && w.last() != Some(&'e') {
            return None;
        }
        // An adjective of the object
        if cases == 0 && adjectives < 2 && !function(w) && w.last().is_some_and(|&c| is_vowel_letter(c)) {
            k += 1;
            adjectives += 1;
            continue;
        }
        break;
    }
    if !lexical_subject {
        return None;
    }
    // 2. A utensil after je, and no participle after it (je z vilicami jedel)
    if clause.one_of(i + 1, &["z", "s"]) && clause.one_of(i + 2, UTENSILS) {
        return if (i + 3..n).any(participle_like) { None } else { EATS };
    }
    // 3. An adverb of eating before je ending its clause, after a subject
    (i + 1 == n && manner_adverb).then_some("jé")
}

// glasu is two forms of glás read differently: the genitive glasú and the
// dative and locative glásu (SSKJ² glás -ú tudi -a; Slovenski pravopis 2001
// glás -ú tudi glás -a, "prva oblika dalje -u -- -u -om"; Sloleks 3.1 glasú
// for the genitive, glásu for the other two). A listener's report of
// 2026-11-10: Nisem imel glasu and Glasu ne morem preveriti on the last
// syllable, V glasu je hripavost and V tvojem glasu je slišati toplino on the
// first. The lexicon had one reading for the spelling, the genitive's.
//
// The same holds for the other masculine nouns of one syllable whose
// genitive SP 2001 gives in -ú with "dalje -u -- -u -om" (strahú, v stráhu;
// mirú, pri míru; gradú, na grádu; ledú, na lédu): the lexicon read each by
// the commoner form, so pri miru, v strahu, na mostu, v medu had the
// genitive's accent and do gradu, kocka ledu, iz nosu the locative's. The
// table is Sloleks's nouns with both readings less those whose genitive in
// -ú the dictionaries do not have (rób, vál, cvét, dróg, drób) or have as a
// variant hardly ever written (žár, žléb: 2 and 7 genitives in -u in
// Gigafida; sín, whose sinú is stylistic): they keep the stem everywhere.
// The dative and locative of móst and ród have the open o (SSKJ² daj., mest.
// ed. môstu in móstu, rôdu tudi ródu; SP môstu, rôdu tudi ródu).

/// The spelling, the genitive, the stem (the dative and the locative).
const GENITIVE_U: &[(&str, &str, &str)] = &[
    ("glasu", "glasú", "glásu"),
    ("strahu", "strahú", "stráhu"),
    ("miru", "mirú", "míru"),
    ("gradu", "gradú", "grádu"),
    ("pasu", "pasú", "pásu"),
    ("mostu", "mostú", "môstu"),
    ("zidu", "zidú", "zídu"),
    ("ledu", "ledú", "lédu"),
    ("rodu", "rodú", "rôdu"),
    ("nosu", "nosú", "nósu"),
    ("prahu", "prahú", "práhu"),
    ("traku", "trakú", "tráku"),
    ("vratu", "vratú", "vrátu"),
    ("plodu", "plodú", "plódu"),
    ("plazu", "plazú", "plázu"),
    ("sadu", "sadú", "sádu"),
    ("tatu", "tatú", "tátu"),
    ("daru", "darú", "dáru"),
    ("medu", "medú", "médu"),
    ("sramu", "sramú", "srámu"),
    ("sledu", "sledú", "slédu"),
    ("slapu", "slapú", "slápu"),
    ("stanu", "stanú", "stánu"),
    ("potu", "potú", "pótu"),
    ("mahu", "mahú", "máhu"),
    ("jezu", "jezú", "jézu"),
    ("smradu", "smradú", "smrádu"),
    ("godu", "godú", "gódu"),
    ("plotu", "plotú", "plótu"),
    ("hladu", "hladú", "hládu"),
    ("lanu", "lanú", "lánu"),
    ("satu", "satú", "sátu"),
    ("tramu", "tramú", "trámu"),
    ("sladu", "sladú", "sládu"),
    ("latu", "latú", "látu"),
    ("gladu", "gladú", "gládu"),
];

#[derive(Clone, Copy, PartialEq, Eq)]
enum CaseCue {
    None,
    Genitive,
    Stem,
}

/// The case the words before `words[i]` ask of it: a preposition, right before
/// it or before its adjectives (v glasu, v tvojem glasu, ob prvem glasu; brez
/// glasu, do njegovega glasu, z mostu), or an adjective that agrees and says
/// it alone (njegovega glasu, svojemu glasu). The locative and dative
/// prepositions have the stem, the genitive ones the ending.
fn case_before(words: &[Vec<char>], i: usize) -> CaseCue {
    const LOCATIVE_DATIVE: &[&str] =
        &["v", "na", "o", "ob", "po", "pri", "k", "h", "proti", "kljub", "nasproti", "navkljub"];
    const GENITIVE: &[&str] = &[
        "brez", "do", "iz", "od", "zaradi", "okoli", "okrog", "krog", "razen", "poleg", "z", "s", "glede", "namesto",
        "mimo", "blizu", "sredi", "izmed", "iznad", "izpod", "izza", "vzdolž", "znotraj", "zunaj", "prek", "preko",
        "zraven", "vpričo", "spričo", "zavoljo", "vrh", "kraj", "konec", "onkraj", "izven",
    ];
    // Between a preposition and its noun: v zelo tihem glasu, v tihem in
    // mirnem glasu
    const INSIDE: &[&str] = &["zelo", "tako", "precej", "bolj", "še", "že", "tudi", "sicer"];
    const JOINING: &[&str] = &["in", "ali", "ter", "oziroma"];
    // In -em and no adjective
    const NOT_ADJECTIVES: &[&str] = &[
        "potem", "medtem", "zatem", "predtem", "povsem", "predvsem", "sem", "nisem", "bom", "vem", "grem", "smem",
        "jem", "morem", "hočem", "nočem",
    ];
    let i = i.min(words.len());
    let mut k = i;
    let mut adjectives_only = true;
    for _ in 0..6 {
        let Some(before) = k.checked_sub(1).map(|at| &words[at][..]) else { break };
        if is_one_of(before, LOCATIVE_DATIVE) {
            return CaseCue::Stem;
        }
        if is_one_of(before, GENITIVE) || ends_with(before, "ega") {
            return CaseCue::Genitive;
        }
        if ends_with(before, "emu") {
            return CaseCue::Stem;
        }
        // An adverb or a conjunction only before an adjective passed
        // already: Pri tem še glasu ni dal od sebe has no "še glasu" of the
        // preposition, and v strahu in glasu is two nouns
        if k < i && is_one_of(before, JOINING) {
            // (a conjunction between two adjectives)
        } else if eq(before, "čigar") || (k < i && is_one_of(before, INSIDE)) {
            adjectives_only = false;
        } else if !ends_with(before, "em") || is_one_of(before, NOT_ADJECTIVES) {
            return CaseCue::None;
        }
        k -= 1;
    }
    // (stala na nasprotnem,) levem voznem pasu: the adjectives of a locative
    // open the clause, their preposition is before the comma
    if k == 0 && i > 0 && adjectives_only { CaseCue::Stem } else { CaseCue::None }
}

/// What takes the dative: prisluhniti glasu, slediti glasu, zaupati, verjeti,
/// prepustiti se glasu, podoben, namenjen, posvečen, hvaležen glasu (the
/// Collocations Dictionary of Modern Slovene, glagol + GLASU and pridevnik +
/// GLASU), pripadati rodu, bližje gradu. Not their nouns, which take the
/// genitive (podobnost glasu, prisluškovanje glasu), and not sledi alone,
/// which is also the traces of something (sledi prahu, ni sledi strahu):
/// `genitive_u_reading()` has it for glasu.
fn governs_dative(w: &[char]) -> bool {
    #[rustfmt::skip] // rows of words
    const STEMS: &[&str] = &[
        "prisluhn", "prisluškova", "sledi", "zaupa", "verjam", "verjel", "verjem", "verjeti", "prepusti", "prepušča",
        "prepuščen", "pripada", "podobn", "podoben", "namenj", "posveč", "posveti", "hvalež",
    ];
    const NOUN_ENDINGS: &[&str] =
        &["nje", "nja", "nju", "njem", "ost", "osti", "ostjo", "tev", "tve", "tvi", "lnik", "lnika", "lec", "lca"];
    if is_one_of(w, &["bližje", "bliže"]) {
        return true;
    }
    !eq(w, "sledi") && !ends_with_any(w, NOUN_ENDINGS) && STEMS.iter().any(|stem| starts_with(w, stem))
}

/// glasú or glásu, and the like of the other nouns of `GENITIVE_U`. The
/// locative has a preposition and the dative one or an adjective in -emu or
/// a word that takes it; whatever has none of them is the genitive, of
/// another noun (barva glasu, ton glasu, kocka ledu), of a negation (Nisem
/// imel glasu, Glasu ne morem preveriti), of a quantity (veliko prahu) or of
/// its verb (spomnil sem se glasu, želimo si miru). A noun joined to another
/// by in, ali, ter has that one's case (v miru in strahu, zaradi napora in
/// strahu, reda in miru). In SUK 1.1 the lexicon's one reading was right for
/// 200 of the 305 forms of these nouns, this for 304; all 14 of glasu.
fn genitive_u_reading(clause: &Clause, i: usize) -> Option<&'static str> {
    const CONJUNCTIONS: &[&str] = &["in", "ali", "ter", "oziroma", "niti"];
    const CLITICS: &[&str] = &[
        "se", "si", "je", "ne", "ga", "jo", "mu", "ji", "jim", "jih", "bi", "sem", "so", "smo", "ste", "bo", "bom",
        "boš",
    ];
    // Sledi glasu srca: the verb (the Collocations Dictionary has slediti
    // + glasu 106 times and no sled + glasu), unless the words before it
    // make it the traces (ni sledi glasu, brez sledi glasu)
    const BEFORE_TRACES: &[&str] = &["ni", "brez", "nobene", "nobenih", "nikakršne", "o", "po", "na"];
    let words = clause.words;
    let w = clause.word(i)?;
    let &(_, genitive, stem) = GENITIVE_U.iter().find(|&&(spelling, ..)| eq(w, spelling))?;
    let mut cue = case_before(words, i);
    if cue == CaseCue::None && i >= 2 && is_one_of(&words[i - 1], CONJUNCTIONS) {
        cue = case_before(words, i - 2);
        // (po lasišču, obrazu,) ušesih in nosu: a locative plural before it
        if cue == CaseCue::None && ends_with_any(&words[i - 2], &["ih", "ah", "eh"]) {
            cue = CaseCue::Stem;
        }
    }
    if cue == CaseCue::None {
        let governs = |k: usize| {
            let Some(word) = clause.word(k) else { return false };
            if eq(word, "sledi") {
                eq(w, "glasu") && !clause.before(k).is_some_and(|before| is_one_of(before, BEFORE_TRACES))
            } else {
                governs_dative(word)
            }
        };
        if (i >= 1 && governs(i - 1)) || (i >= 2 && is_one_of(&words[i - 1], CLITICS) && governs(i - 2)) {
            cue = CaseCue::Stem; // Prisluhni glasu. Prepusti se glasu.
        } else if i == 0 {
            // Glasu zaupam. Glasu ne verjamem. Glasu se je prepustil.
            let k = (1..3).find(|&k| !clause.one_of(k, CLITICS)).unwrap_or(3);
            if governs(k) {
                cue = CaseCue::Stem;
            }
        }
    }
    // A capitalized word inside a clause with nothing to tell its case may be
    // a name (Miru, the dative of Miro): as the lexicon has it
    if cue == CaseCue::None && clause.capital(i) && i > 0 {
        return None;
    }
    Some(if cue == CaseCue::Stem { stem } else { genitive })
}

/// Samo je prišel. Samo se je smejal. Samo ga ni videl. Samo bo prišel. The
/// name Sámo (SP 2001: Sámo -a, oseb. i.) as the subject at the head of a
/// sentence, where the capital says nothing: a listener's rule of 2026-11-10,
/// that a sentence opening with Samo and such a verb names the man (On je
/// samo prišel is "only"). The cue is the perfect or the future of a man:
/// right after Samo the clitics with je, ni or bo among them, and within
/// three words a masculine participle in -l. Not bi, sem, si, which other
/// persons take (Samo bi rad vedel), no participle of another gender (Samo je
/// bilo prepozno) and no predicate without one (Samo je še čas, Samo je to
/// res?), which stay samó, as do Samo pridi sem and Samo Janez je prišel.
/// SUK 1.1 has no such sentence: its 53 that open with Samo are 49 particles,
/// none of which the rule touches, and 4 names before a surname (Samo Uhan),
/// which stay samó. The particle reading of such a sentence (Samo je prišel
/// prepozno, "but he came too late") is given up at the listener's word.
fn samo_is_subject(clause: &Clause, i: usize) -> bool {
    const CLITICS: &[&str] = &[
        "se", "si", "me", "te", "ga", "jo", "mi", "ti", "mu", "ji", "nas", "vas", "jih", "nam", "vam", "jim", "ju",
        "jima",
    ];
    const AUXILIARIES: &[&str] = &["je", "ni", "bo"];
    // Words whose -l is [w] that are no participle: žal, cel dan, the nouns.
    // A final [l] is none either (del, model, general), by the reading
    // the word gets.
    const NOT_PARTICIPLES: &[&str] =
        &["žal", "cel", "misel", "posel", "pepel", "kotel", "pekel", "kozel", "osel", "vozel"];
    if i != 0 || !clause.capital(0) {
        return false;
    }
    let words = clause.words;
    let k = (1..words.len()).find(|&k| !clause.one_of(k, AUXILIARIES) && !clause.one_of(k, CLITICS));
    let k = k.unwrap_or(words.len());
    if !(1..k).any(|a| clause.one_of(a, AUXILIARIES)) {
        return false;
    }
    for (k, x) in words.iter().enumerate().skip(k).take(3) {
        if clause.capital(k) {
            return false; // another subject: Samo je Janez prišel
        }
        if x.len() < 3
            || !matches!(x[..], [.., 'a' | 'e' | 'i' | 'u' | 'r', 'l'])
            || is_one_of(x, NOT_PARTICIPLES)
            || (!eq(x, "bil") && is_function_word(x))
        {
            continue;
        }
        let phones = phonemize(x, &vec![0; x.len()], None, false, None, Capital::None).phones;
        if phones.last().is_some_and(|last| last.ph == Ph::W) {
            return true;
        }
    }
    false
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
pub fn clause_reading(clause: &Clause, i: usize) -> Option<&'static str> {
    let words = clause.words;
    let w = clause.word(i)?;
    let prev = clause.before(i);
    let next = clause.word(i + 1);
    // SSKJ² / SP 2001 (Fran), sám -a -o versus samó (only).
    // Keep the common particle unless an agreeing eno, reflexive phrase,
    // or an explicit solitude construction identifies the pronoun.
    // SUK 1.1 lemma/tag audit is documented in docs/formant.md.
    if eq(w, "samo") {
        let prev_is = |list: &[&str]| prev.is_some_and(|prev| is_one_of(prev, list));
        let itself = &["sebe", "sebi"];
        let reflexive =
            clause.one_of(i + 1, itself) || (clause.one_of(i + 1, &["od", "po"]) && clause.one_of(i + 2, itself));
        let alone = prev_is(&["čisto", "povsem", "popolnoma", "pustil", "pustila", "pustili", "pustiti"])
            && next.is_none_or(|next| is_one_of(next, &["", "doma", "tam"]));
        let pronoun = prev_is(&["eno"]) || (reflexive && !prev_is(&["ne"])) || alone || samo_is_subject(clause, i);
        return Some(if pronoun { "sámo" } else { "samó" });
    }
    if eq(w, "vprašaj") {
        return Some(vprasaj_reading(clause, i));
    }
    if eq(w, "je") {
        return je_reading(clause, i);
    }
    // glasú the genitive, glásu the dative and locative
    if w.last() == Some(&'u')
        && let Some(reading) = genitive_u_reading(clause, i)
    {
        return Some(reading);
    }
    // Veseli se! Veseli se novega dneva! The imperative vesêli (SP 2001
    // veselíti -ím, vesêli -íte) at the head of a clause before se or si in
    // a sentence that ends in an exclamation mark, the cue a listener named
    // (2026-11-03); without it the shape is the present veselí (Veseli se
    // novega dneva. "he looks forward to the new day"), and before an object
    // clitic the verb of pleasing (Veseli me, da ...), context_reading()
    if eq(w, "veseli") && i == 0 && clause.exclamation && clause.one_of(1, &["se", "si"]) {
        return Some("vesêli");
    }
    if w.last() == Some(&'o') {
        if let Some(&(_, at_end, before_word)) = END_READINGS.iter().find(|&&(spelling, ..)| eq(w, spelling)) {
            return Some(if next.is_none() { at_end } else { before_word });
        }
        return (eq(w, "lahko") && lahko_is_adjective(clause, i)).then_some("láhko");
    }
    if eq(w, "meni") {
        // Meni tudi, da ... Meni pa, da bo ... Meni namreč, da ...: the verb of
        // the news at the head of its sentence, a particle between it and
        // its clause of da (SUK 1.1: 5 of 5)
        const PARTICLES: &[&str] = &["tudi", "pa", "namreč", "sicer", "celo", "še", "seveda", "torej", "tako", "zato"];
        const VERB: Option<&str> = Some("méni");
        if clause.before_complement && i == 0 && next.is_some() && words[1..].iter().all(|x| is_one_of(x, PARTICLES)) {
            return VERB;
        }
        let reading = meni_reading(clause, i);
        // Meni, da je prav. Ta meni, da je prav. Tudi meni, da je prav. The
        // clause after it is what a verb of thinking takes; but a purpose
        // clause follows a menu (Odprite meni, da izberete), and a preposition
        // has the pronoun (k meni, da).
        if clause.before_complement
            && next.is_none()
            && !prev.is_some_and(is_preposition)
            && (i == 0 || (i == 1 && clause.is(0, "ta")) || reading != Some("mení"))
        {
            return VERB;
        }
        return reading;
    }
    if eq(w, "roka") {
        return (roka_is_deadline(clause, i) || roka_is_absent_or_object(clause, i)).then_some("róka");
    }
    if eq(w, "roki") || eq(w, "roke") {
        return roki_roke_reading(clause, i);
    }
    if eq(w, "afna") {
        return (prev.is_some_and(|prev| eq(prev, "se")) || clause.is(i + 1, "se")).then_some("afná");
    }
    None
}

/// Whether the reading [`clause_reading`] gave is a content word although
/// its spelling is a function word's: the adjective láhko has a full accent.
pub fn clause_reading_is_content(w: &[char], reading: &str) -> bool {
    match reading {
        "sámo" => eq(w, "samo"),
        "jé" => eq(w, "je"), // "eats", a verb with an accent of its own
        "láhko" => eq(w, "lahko"),
        _ => false,
    }
}
