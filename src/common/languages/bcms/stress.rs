//! Stress by rule: the verbs of the verb tables, the suffixes whose accent
//! position is fixed, and the word classes told by their shape.

use super::verbs::{VerbRoot, is_verbal_prefix, verb_roots};
use crate::formant::phones::Accent;
use crate::language::Language;
use crate::text::frontend::Word;
use crate::text::frontend::letters::{
    ends_with, ends_with_any, eq, find, has_at, has_inner_syllabic_r, is_one_of, is_vowel_letter, starts_with, tail,
};

/// Where a rule puts the stress. The syllables are counted as the C++ rules
/// counted them, with -1 for none, since the rules do arithmetic on them.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct StressResult {
    pub nucleus: i32,
    pub is_long: bool,
    /// An unstressed long syllable (kapacìtēt).
    pub long_after: i32,
    pub accent: Accent,
}

impl Default for StressResult {
    fn default() -> Self {
        Self { nucleus: -1, is_long: false, long_after: -1, accent: Accent::None }
    }
}

fn length(s: &str) -> usize {
    s.chars().count()
}

const fn index(k: usize) -> i32 {
    // No word has more syllables than letters, and no word is that long
    k as i32
}

/// The rules for one word.
pub struct StressRules<'a> {
    w: &'a [char],
    language: Language,
    opens_clause: bool,
    user_verbs: Option<&'a [VerbRoot]>,
    capitalized: bool,
    /// The letter of each syllable's nucleus.
    letters: Vec<usize>,
}

// Case endings of a noun in -a, and of one in a consonant.
const A_CASES: &[&str] = &["a", "e", "i", "u", "om", "ama"];
const NOUN_ENDINGS: &[&str] = &["", "a", "u", "om", "e", "i", "ima"];

impl<'a> StressRules<'a> {
    pub fn new(word: &'a Word, language: Language, opens_clause: bool, user_verbs: Option<&'a [VerbRoot]>) -> Self {
        Self {
            w: &word.w,
            language,
            opens_clause,
            user_verbs,
            capitalized: word.capitalized && !word.all_caps,
            letters: word.nucleus_letters(),
        }
    }

    fn croatian(&self) -> bool {
        self.language == Language::Croatian
    }

    /// Verbs keep the accent of the infinitive on their root in the other
    /// forms too: uréditi, urédi, urédio; otvòriti, otvòri; podijéliti,
    /// podijéli; pročìtati, pročìtaj; pokrénuti, pokréni. The verbs come from
    /// two tables: roots with a long "ije", which take any prefix, and whole
    /// stems.
    ///
    /// In the present and the passive participle the dictionaries often move
    /// the accent back (ùrēdīm, ùrēđen, òtvorīm, pòdijēlīm). Croatian is
    /// commonly spoken without that shift, and the Croatian voice keeps the
    /// accent on the root in every form; the other two follow the dictionaries.
    pub fn verb_form(&self) -> StressResult {
        let mut r = StressResult::default();
        if self.user_verbs.is_some_and(|verbs| self.verb_form_in(verbs, &mut r))
            || self.verb_form_in(verb_roots(), &mut r)
        {
            r
        } else {
            StressResult::default()
        }
    }

    pub fn verb_form_in(&self, roots: &[VerbRoot], r: &mut StressResult) -> bool {
        // "ne" + passive participle is an adjective (nepročitan, nedovršen,
        // nepromijenjen); see form_of() for its accent.
        let negated = self.count() >= 3 && starts_with(self.w, "ne");
        for root in roots {
            if root.whole_stem {
                if self.form_of(root, 0, r, false) || (negated && self.form_of(root, 2, r, true)) {
                    return true;
                }
                continue;
            }
            // The root's "ije" has to be one syllable, and not the first.
            for &e in self.letters.iter().skip(1) {
                let Some(i) = e.checked_sub(2).filter(|_| e > root.vowel) else { continue };
                if !has_at(self.w, i, "ije") || self.nucleus_at(i) >= 0 {
                    continue;
                }
                let start = e - root.vowel;
                let mut prefix = &self.w[..start];
                let mut negated_here = false;
                if negated && start > 2 && !is_verbal_prefix(prefix) {
                    prefix = &prefix[2..];
                    negated_here = true;
                }
                let allowed = if root.prefixes.is_empty() {
                    is_verbal_prefix(prefix)
                } else {
                    root.prefixes.iter().any(|known| known == prefix)
                };
                if allowed && self.form_of(root, start, r, negated_here) {
                    return true;
                }
            }
        }
        false
    }

    /// Suffixes whose accent position is fixed.
    pub fn strong(&self) -> StressResult {
        let mut r = self.verb_form();
        if r.nucleus >= 0 {
            return r;
        }
        let n = self.count();
        let r = &mut r;

        // Superlatives of the two-syllable comparatives (bȍljī, vȅćī,
        // mȁnjē, ljȅpšī, jȁčī, bȑžī) have the long falling accent on naj-
        // and none on the rest: nȃjboljī, nȃjmanjē, nȃjljepšī, nȃjjačī
        // (Hrvatski mrežni rječnik and Školski rječnik hrvatskoga jezika;
        // Jonke; Alić for Bosnian). The superlatives of the comparatives
        // in -iji keep the comparative's accent with naj- unaccented
        // (najnòvijī, najvàžnijī), which the rule for -ij gives below.
        if self.superlative_of_short_comparative() {
            return StressResult { nucleus: 0, is_long: true, accent: Accent::Falling, ..*r };
        }

        // Word classes whose accent sits before the endings the rules below
        // would take for another suffix (-enje, -iti, -an).
        if self.time_compound(r)
            || self.slavic_compound_name(r)
            || self.laziti_verb(r)
            || self.teljstvo_noun(r)
            || self.ljiv_adjective(r)
            || self.ant_adjective(r)
        {
            return *r;
        }

        // Present, imperative and present participle of the verbs in -ivati
        // and -ovati: the accent is on the syllable before -uj (ukljùčujem,
        // prikàzuje, urèđuju, kùpujem, pùtujući). The few verbs in -ovati
        // that keep it on the first syllable (nàpredujem, sùdjelujem) are
        // in the lexicon.
        const UJ_ENDINGS: &[&str] = &["em", "eš", "e", "emo", "ete", "u", "ući", "", "mo", "te"];
        if n >= 3 && self.before_suffix("uj", UJ_ENDINGS, r) {
            r.accent = Accent::Rising;
            return *r;
        }

        // The same forms of the verbs that keep -av- or -iv- (označavam,
        // obavještavaju, uživaš, pokrivaj). The dictionaries have the accent
        // one syllable before the suffix (oznàčāvām, ùžīvām); the Croatian
        // voice keeps it where the infinitive has it (označávam, užívam),
        // like the other verbs (see verb_form). The third person plural
        // keeps the infinitive's accent in the dictionaries too (označávajū,
        // održávajū, doživljávajū, užívajū in Školski rječnik). The third
        // person in -ava is spelled like many nouns and adjectives (država,
        // zabava, krvava), so it counts only from four syllables on
        // (označava, održava), and the one in -iva not at all (osjetljiva,
        // perspektiva); short verbs are in the VERBS table (rješava, uživa).
        const AM_ENDINGS: &[&str] = &["am", "aš", "amo", "ate", "aju", "aj", "ajmo", "ajte"];
        let mut suffix = StressResult::default();
        if (n >= 3
            && (self.at_suffix("av", AM_ENDINGS, 0, &mut suffix) || self.at_suffix("iv", AM_ENDINGS, 0, &mut suffix)))
            || (n >= 4 && !ends_with(self.w, "slava") && self.at_suffix("av", &["a"], 0, &mut suffix))
        {
            if self.croatian() || ends_with(self.w, "aju") {
                r.nucleus = suffix.nucleus;
                r.is_long = true;
            } else {
                r.nucleus = suffix.nucleus - 1;
                r.long_after = suffix.nucleus;
                r.accent = Accent::Rising;
            }
            return *r;
        }
        *r = StressResult::default();

        // Surnames in -ović/-ević of four or more syllables carry a long
        // rising accent on the syllable before the suffix (Jovánović,
        // Kováčević, Stefánović, Milénković, Halílović); the three-syllable
        // ones keep the first (Pètrović, Màrković, Jánković), which is the
        // default. Exceptions (Ìvanović, Jòsipović) are in the lexicon.
        const IC_CASES: &[&str] = &[
            "ić", "ića", "iću", "ićem", "ići", "ićima", "ićev", "ićeva", "ićevu", "ićevo", "ićevi", "ićeve", "ićevim",
            "ićevih",
        ];
        if n >= 4 && (self.before_suffix("ov", IC_CASES, r) || self.before_suffix("ev", IC_CASES, r)) {
            r.is_long = true;
            return *r;
        }

        // Feminine agent nouns in -ačica are stressed on the syllable before
        // the suffix (pjevàčica, prodavàčica, čistàčica). The masculine
        // nouns in -ač (prodàvāč, prodaváča) are handled with the other
        // nouns whose last stem syllable is long, see long_stem().
        const AC_CASES: &[&str] = &["ica", "ice", "ici", "icu", "icom", "ico", "icama"];
        if n >= 3 && self.before_suffix("ač", AC_CASES, r) {
            return *r;
        }

        // Nouns in -ina: abstract nouns, loans and female names are stressed
        // on the i (veličìna, Katarína, balerína, vitamína, četvrtìna); the
        // possessive -ovina/-evina on the syllable before it (dòmovina,
        // králjevina, polòvina). Nouns in -bina go both ways (sudbìna,
        // rȍdbina) and are left to the lexicon, as are the exceptions
        // (gȍdina, ȉstina, cȁrina, svȉnjetina, Vȍjvodina).
        //
        // Not every -ina is that suffix. The possessives of nouns in -ica,
        // -ka and -ja (kraljičin, učiteljičina, bakina, mačkine, Majina)
        // keep the accent of their noun, the inhabitants in -čanin, -đanin
        // (gràđanina) are anin_noun()'s, and the augmentatives in -etina,
        // -urina, -ština, -avina and -čina have it on the syllable before
        // the suffix (kućètina, ptičùrina, čakàvština, mješàvina,
        // junàčina). Three-syllable words are left alone: the common
        // abstract nouns (brzìna, planìna, širìna, ravnìna, trećìna) are
        // lexicon entries, and the rest of that shape are mostly
        // possessives (mȁmina, sȅstrina, Ȁnina, Ȉvina) and nouns with the
        // first syllable stressed (nȍvina, mȁlina, kȕpina, Tùrčina).
        if n >= 4
            && !ends_with_any(self.w, &["bina", "bine", "bini", "binu", "binom", "binama"])
            && let Some(in_pos) = self.suffix_at("in", A_CASES).filter(|&at| at >= 2 && self.nucleus_at(at) >= 2)
        {
            let base = &self.w[..in_pos];
            if !ends_with_any(base, &["ič", "ic", "k", "j", "čan", "ćan", "đan", "šan", "žan", "jan"])
                && (self.before_suffix("ovin", A_CASES, r)
                    || self.before_suffix("evin", A_CASES, r)
                    || (ends_with_any(base, &["et", "ur", "št", "av", "č"]) && self.before_suffix("in", A_CASES, r))
                    || self.at_suffix("in", A_CASES, 0, r))
            {
                return *r;
            }
        }

        if self.irati_verb(r) {
            return *r;
        }

        // -ivati / -avati verbs: pokazívati, održávanje
        const VA_ENDINGS: &[&str] = &[
            "ati", "at", "ao", "ala", "alo", "ali", "ale", "ah", "aše", "asmo", "aste", "ahu", "anje", "anja", "anju",
            "anjem", "ajući",
        ];
        if n >= 4 && (self.at_suffix("iv", VA_ENDINGS, 0, r) || self.at_suffix("av", VA_ENDINGS, 0, r)) {
            r.is_long = true;
            return *r;
        }

        // -ija and its case forms, comparatives in -iji: stress falls on the
        // syllable before (policija, organizácija, Itàlija, pamètnijī). The
        // adjectives in -ijski keep their noun's accent (polìcījskī,
        // simulácījskī, informácījskī, enèrgījskī in HJP and Školski
        // rječnik).
        const IJ_ENDINGS: &[&str] = &[
            "a", "e", "i", "u", "o", "om", "ama", "em", "eg", "ega", "emu", "ih", "im", "ima", "oj", "in", "ina",
            "ine", "ini", "inu", "inom", "ski", "ska", "sko", "ske", "sku", "skog", "skom", "skoj", "skih", "skim",
            "skoga", "skome", "skomu", "skima",
        ];
        if n >= 3 && self.before_suffix("ij", IJ_ENDINGS, r) {
            let letter = usize::try_from(r.nucleus).ok().and_then(|k| self.letters.get(k)).copied().unwrap_or(0);
            r.is_long = has_at(self.w, letter, "acij");
            return *r;
        }

        if self.izam_noun(r) {
            return *r;
        }

        // -itet: the case forms have a long rising e (kapacitéta,
        // identitétu, kvalitéta). In the nominative the dictionaries move
        // the accent one syllable back (kapacìtēt); Croatian is commonly
        // spoken with the e still long and prominent there (kapacitét), and
        // that is what the Croatian voice does.
        if n >= 3 && self.at_suffix("itet", NOUN_ENDINGS, 2, r) {
            r.is_long = true;
            if !self.croatian() && ends_with(self.w, "itet") {
                r.long_after = r.nucleus;
                r.nucleus -= 1;
                r.is_long = false;
            }
            return *r;
        }
        // -ator: organizátor
        if n >= 3 && self.at_suffix("ator", NOUN_ENDINGS, 0, r) {
            r.is_long = true;
            return *r;
        }

        // Loans stressed on the last syllable at source move one syllable
        // back: dokùment, specijàlist, telèfon, kilògram, dirèktor
        const PENULT: &[&str] = &["ent", "ant", "ist", "log", "graf", "fon", "gram", "skop", "tor"];
        if n >= 3 && PENULT.iter().any(|suffix| self.before_suffix(suffix, NOUN_ENDINGS, r)) {
            return *r;
        }

        // -ura: kultúra, temperatúra
        if n >= 3 && self.at_suffix("ur", A_CASES, 0, r) {
            r.is_long = true;
            return *r;
        }

        // -tika: matemàtika, polìtika
        if n >= 4 && (self.before_suffix("tik", A_CASES, r) || self.before_suffix("tic", A_CASES, r)) {
            return *r;
        }

        // -ičan, -ički: elèktričan, polìtički
        const IC_ENDINGS: &[&str] = &[
            "an", "na", "no", "ni", "ne", "nu", "nog", "nom", "nih", "nim", "noj", "ki", "ka", "ko", "ke", "ku", "kog",
            "kom", "kih", "kim", "koj", "koga", "kome", "nima", "kima",
        ];
        if n >= 4 && self.before_suffix("ič", IC_ENDINGS, r) {
            return *r;
        }

        // Verbal nouns: putòvānje, zanímanje; upozorénje, odobrénje
        const NJ_ENDINGS: &[&str] = &["e", "a", "u", "em", "ima"];
        if n >= 4 && self.before_suffix("anj", NJ_ENDINGS, r) {
            return *r;
        }
        if n >= 4 && self.at_suffix("enj", NJ_ENDINGS, 0, r) {
            r.is_long = true;
            return *r;
        }

        // Word classes rather than single suffixes, from the most to the
        // least specific. The verbs first: a prefixed verb's forms would
        // otherwise be read as nouns (izabrana, pokušamo).
        if self.prefixed_verb(r)
            || self.ov_verb(r)
            || self.ica_noun(r)
            || self.anin_noun(r)
            || self.izan_adjective(r)
            || self.long_stem(r)
        {
            return *r;
        }
        StressResult::default()
    }

    /// Prefixed verbs keep the accent of the infinitive on the root in the
    /// whole paradigm: poglèdati, poglèdala, poglèdavši, poglèdasmo,
    /// poglèdam, poglèdate; napràviti, napràvio; iskùsiti, iskùsio;
    /// dočèkati, dočèkala; pokùšati, pokùšamo. The long infinitives
    /// already had their antepenult from weak(); this gives the other forms
    /// the same syllable, which is what the verbs in VERBS do by name. It
    /// takes a word that starts with a verbal prefix, has a thematic vowel
    /// (a, i, u, or je) before one of the verbal endings, and at least one
    /// root syllable between the two, and stresses the syllable before the
    /// thematic vowel. The present in -im/-i/-e, the third person -a, the
    /// imperative in -aj and the passive participle in -an are left out:
    /// they are spelled like too many nouns and adjectives (dosljednim,
    /// pokušaj, događaju, pȍseban, dȍsadan, pȍznat).
    fn prefixed_verb(&self, r: &mut StressResult) -> bool {
        // The ending, and whether it is one only after the thematic a
        const ENDINGS: [(&str, bool); 17] = [
            ("jući", true),
            ("vši", false),
            ("smo", false),
            ("ste", false),
            ("še", false),
            ("hu", false),
            ("ti", false),
            ("t", false),
            ("la", false),
            ("lo", false),
            ("li", false),
            ("le", false),
            ("o", false),
            ("m", true),
            ("š", true),
            ("mo", true),
            ("te", true),
        ];
        let w = self.w;
        for (ending, a_only) in ENDINGS {
            if w.len() < length(ending) + 3 || !ends_with(w, ending) {
                continue;
            }
            let t = w.len() - length(ending) - 1;
            let v = w[t];
            let thematic = matches!(v, 'a' | 'i' | 'u') || (v == 'e' && w[t - 1] == 'j');
            if !thematic || (a_only && v != 'a') || is_vowel_letter(w[t - 1]) {
                continue;
            }
            if !self.prefixed_before(t, false) {
                return false;
            }
            let k = self.nucleus_at(t);
            if k < 2 {
                return false; // the prefix's vowel and a root vowel
            }
            r.nucleus = k - 1;
            r.accent = Accent::Rising;
            return true;
        }
        false
    }

    /// Verbs in -ovati carry the accent on the o in the infinitive, the
    /// participle, the aorist and the fused future (kupòvati, putòvao,
    /// stanòvala, darovàsmo, poštòvaću);
    /// the present in -ujem is handled above (poštòvati, pòštujēm). The few
    /// that keep the first syllable (vjȅrovati, rȁdovati se, mìlovati) are
    /// lexicon entries.
    fn ov_verb(&self, r: &mut StressResult) -> bool {
        const OV_ENDINGS: &[&str] = &[
            "ati", "at", "ao", "ala", "alo", "ali", "ale", "avši", "asmo", "aste", "aše", "ahu", "aću", "aćeš", "aće",
            "aćemo", "aćete",
        ];
        if !self.at_suffix("ov", OV_ENDINGS, 0, r) {
            return false;
        }
        r.accent = Accent::Rising;
        true
    }

    /// Nouns in -ica of four or more syllables built on -ar, -on, -un,
    /// -ovn, -ern, -arn and -telj have the accent on that syllable
    /// (čuvàrica, kuhàrica, radiònica, račùnica, putòvnica, cvjećàrnica,
    /// ravnatèljica, voditèljica), like the -ačica above. The others are
    /// left alone: many keep the first syllable (pȍzornica, ȉskaznica,
    /// slȕžbenica, djèvōjčica) and the rest are lexicon entries.
    fn ica_noun(&self, r: &mut StressResult) -> bool {
        const ICA_CASES: &[&str] = &["ica", "ice", "ici", "icu", "icom", "ico", "icama"];
        if self.count() < 4 {
            return false;
        }
        let Some(pos) = self.suffix_at("", ICA_CASES) else { return false };
        if !ends_with_any(&self.w[..pos], &["ar", "on", "un", "ovn", "ern", "arn", "telj"]) {
            return false;
        }
        let k = self.nucleus_before(pos);
        if k <= 0 {
            return false;
        }
        r.nucleus = k;
        true
    }

    /// Inhabitant nouns in -čanin, -ćanin, -đanin, -šanin, -žanin, -janin,
    /// -ljanin, -njanin: the accent is on the syllable before the suffix
    /// (gràđanin, Rìmljanin, kr̀šćanin, držàvljanin, Lìčanin), in the
    /// singular cases that keep the -in-.
    fn anin_noun(&self, r: &mut StressResult) -> bool {
        const ANIN: &[&str] = &["in", "ina", "inu", "inom", "ine", "ini"];
        let Some(pos) = self.suffix_at("an", ANIN).filter(|&pos| pos > 0) else { return false };
        if !matches!(self.w[pos - 1], 'č' | 'ć' | 'đ' | 'š' | 'ž' | 'j') {
            return false;
        }
        let k = self.nucleus_before(pos);
        if k < 0 {
            return false;
        }
        r.nucleus = k;
        true
    }

    /// Adjectives in -izan and -ozan: precìzan, koncìzna, nervózan,
    /// religiózni, grandiózno.
    fn izan_adjective(&self, r: &mut StressResult) -> bool {
        const ADJ_ENDINGS: &[&str] = &[
            "an", "na", "no", "ni", "ne", "nu", "nog", "nom", "nih", "nim", "noj", "noga", "nome", "nomu", "nima",
            "nija", "nije", "niji",
        ];
        if self.at_suffix("iz", ADJ_ENDINGS, 0, r) {
            return true;
        }
        if self.at_suffix("oz", ADJ_ENDINGS, 0, r) {
            r.is_long = true;
            return true;
        }
        false
    }

    /// Slavic names of two parts joined by a vowel, in -mir, -dar and -zar,
    /// have the accent on the first syllable in every case: Rȁdomīr,
    /// Vlàdimir, Krȅšimir, Brȁnimir, Slȁvomir, Tȉhomir, Bȍžidār, Svȅtozār,
    /// Rȁdomira (HJP, Wiktionary). The rule for loans with a long last stem
    /// syllable would give radòmir, radomíra. The tone differs from name to
    /// name; the names in the table of personal names have theirs. Only for
    /// a word written with a capital letter: gospòdār, deprimíra and
    /// uznemíri are common words of the same shape; and not for a form in -a
    /// at the head of a clause, where Deprimira, Animira are verbs.
    fn slavic_compound_name(&self, r: &mut StressResult) -> bool {
        const ENDINGS: &[&str] = &[
            "", "a", "u", "om", "e", "ov", "ova", "ovo", "ovi", "ove", "ovu", "ovog", "ovom", "ovoj", "ovih", "ovim",
            "ka", "ke", "ki", "ku", "kom",
        ];
        if !self.capitalized || self.count() < 3 || starts_with(self.w, "gospod") {
            return false;
        }
        for (root, root_a) in [("mir", "mira"), ("dar", "dara"), ("zar", "zara")] {
            let Some(pos) = self.suffix_at(root, ENDINGS).filter(|&pos| pos >= 3) else { continue };
            if self.opens_clause && ends_with(self.w, root_a) {
                continue;
            }
            let linking = self.w[pos - 1];
            if !matches!(linking, 'o' | 'i' | 'e') || (root != "mir" && linking == 'e') {
                continue;
            }
            // the part before the linking vowel is a syllable of its own
            // (Rad-o-mir, Vlad-i-mir), not a prefix (nemir, odar, pozar)
            if self.nucleus_before(pos - 1) < 0 {
                continue;
            }
            r.nucleus = 0;
            return true;
        }
        false
    }

    /// Compounds of a number or quantity with the words for a year, month,
    /// week or day, which give ages and durations (petogodišnji dječak,
    /// šestomjesečna beba). With -godišnji, -godišnjak, -godišnjica and
    /// -godište the accent of gòdišnjī stays on "go": petogòdišnjī,
    /// stogòdišnjī, dvadesetogòdišnjī, novogòdišnjī, desetogòdišnjāk,
    /// pedesetogòdišnjica, polugòdīšte (HJP, Školski rječnik, Rečnik Matice
    /// srpske), but petogodišnjàkinja (Školski rječnik). With -mjesečni,
    /// -tjedni, -dnevni and -ljetan it is on the linking vowel before
    /// them: šestòmjesečnī, višèmjesečnī, tròtjednī,
    /// petòdnēvnī, malòljetan, punòljetnōst (Školski rječnik). Rečnik
    /// Matice srpske has šestomèsečnī, devetomèsečnī (but dvòmesečno,
    /// tròmesečno) and dvonèdēljnī, and petòdnēvnī, malòletan, punòletan like
    /// Croatian; the Serbian and Bosnian voices follow it.
    fn time_compound(&self, r: &mut StressResult) -> bool {
        const ADJ: &[&str] = &[
            "i", "a", "e", "o", "u", "eg", "ega", "em", "emu", "og", "oga", "om", "ome", "omu", "oj", "ih", "im", "ima",
        ];
        // -godišnjak, -godišnjica; not -godišnjakinja
        const YEAR_NOUN: &[&str] = &[
            "ak", "aka", "aku", "akom", "ače", "aci", "acima", "ake", "ica", "ice", "ici", "icu", "ico", "icom",
            "icama",
        ];
        // mjesečan, mjesečni
        const N_ADJ: &[&str] =
            &["an", "na", "no", "ni", "ne", "nu", "nog", "noga", "nom", "nome", "nomu", "noj", "nih", "nim", "nima"];
        // maloljetan, maloljetnik, maloljetnost
        const LJET: &[&str] = &[
            "an", "na", "no", "ni", "ne", "nu", "nog", "noga", "nom", "nome", "nomu", "noj", "nih", "nim", "nima",
            "nost", "nosti", "nošću", "nostima", "nik", "nika", "niku", "nikom", "niče", "nici", "nicima", "nike",
            "nica", "nice", "nicu", "nico", "nicom", "nicama",
        ];
        let w = self.w;
        let rest_is = |from: usize, list: &[&str]| is_one_of(tail(w, from), list);
        let croatian = self.croatian();

        let year = find(w, "godišnj").filter(|&p| p > 0);
        if let Some(p) = year.filter(|&p| rest_is(p + 7, ADJ) || rest_is(p + 7, YEAR_NOUN)) {
            r.nucleus = self.nucleus_at(p + 1);
            r.accent = Accent::Rising;
            return r.nucleus >= 0;
        }
        if let Some(p) =
            year.filter(|&p| has_at(w, p + 7, "akinj") && rest_is(p + 12, &["a", "e", "i", "u", "o", "om", "ama"]))
        {
            r.nucleus = self.nucleus_at(p + 7); // petogodišnjàkinja
            r.accent = Accent::Rising;
            return r.nucleus >= 0;
        }
        if let Some(p) = find(w, "godišt").filter(|&p| p > 0 && rest_is(p + 6, &["e", "a", "u", "em", "ima"])) {
            r.nucleus = self.nucleus_at(p + 1);
            r.accent = Accent::Rising;
            if !croatian {
                r.long_after = self.nucleus_at(p + 3); // polugòdīšte
            }
            return r.nucleus >= 0;
        }

        /// A root of the compounds.
        struct Root {
            text: &'static str,
            /// Its vowel, for the Serbian -mèsečnī, -nèdēljnī.
            root_vowel: usize,
            /// A vowel long after the accent (dnēvnī, nèdēljnī), or 0.
            long_vowel: usize,
            /// The Serbian dictionary stresses the root itself after a prefix
            /// of this many syllables or more (0: never).
            on_root: i32,
            /// -ljetan and its nouns.
            ljet: bool,
        }
        const fn root(text: &'static str, root_vowel: usize, long_vowel: usize, on_root: i32, ljet: bool) -> Root {
            Root { text, root_vowel, long_vowel, on_root, ljet }
        }
        const ROOTS: [Root; 8] = [
            root("mjeseč", 2, 0, 2, false),
            root("meseč", 1, 0, 2, false),
            root("nedjeljn", 1, 4, 1, false),
            root("nedeljn", 1, 3, 1, false),
            root("dnevn", 2, 2, 0, false),
            root("tjedn", 2, 0, 0, false),
            root("ljet", 2, 0, 0, true),
            root("let", 1, 0, 0, true),
        ];
        for root in &ROOTS {
            let Some(p) = find(w, root.text).filter(|&p| p > 0 && Self::is_quantity_prefix(&w[..p])) else { continue };
            let after = p + length(root.text);
            let endings = if root.ljet {
                LJET
            } else if root.text.ends_with('n') {
                ADJ
            } else {
                N_ADJ
            };
            if !rest_is(after, endings) {
                continue;
            }
            let linking = self.nucleus_before(p);
            r.nucleus = if root.on_root != 0 && !croatian && linking + 1 >= root.on_root {
                self.nucleus_at(p + root.root_vowel)
            } else {
                linking
            };
            if r.nucleus < 0 {
                return false;
            }
            r.accent = Accent::Rising;
            if !croatian && root.long_vowel != 0 {
                let k = self.nucleus_at(p + root.long_vowel);
                if k > r.nucleus {
                    r.long_after = k;
                }
            }
            return true;
        }
        false
    }

    /// jedno-, dvo-, tro-, četvero-, peto- ... dvadeseto-, dvadesetpeto-,
    /// sto-, više-, malo-, puno-, polu-: the first part of the compounds
    /// above (-godišnji takes any first part: novogodišnji, dugogodišnji).
    fn is_quantity_prefix(p: &[char]) -> bool {
        const TENS: [&str; 8] =
            ["dvadeset", "trideset", "četrdeset", "pedeset", "šezdeset", "sedamdeset", "osamdeset", "devedeset"];
        if is_one_of(
            p,
            &[
                "jedno",
                "dvo",
                "tro",
                "četvero",
                "četvoro",
                "više",
                "malo",
                "puno",
                "polu",
                "sto",
                "višesto",
                "dvjesto",
                "dvesto",
                "tristo",
                "četiristo",
                "petsto",
                "šeststo",
                "sedamsto",
                "osamsto",
                "devetsto",
            ],
        ) {
            return true;
        }
        // dvadesetpeto-
        if TENS.iter().any(|tens| {
            let n = length(tens);
            starts_with(p, tens) && p.len() > n + 1 && Self::is_quantity_prefix(&p[n..])
        }) {
            return true;
        }
        let [stem @ .., 'o'] = p else { return false };
        stem.len() >= 2
            && is_one_of(
                stem,
                &[
                    "pet",
                    "šest",
                    "sedm",
                    "osm",
                    "devet",
                    "deset",
                    "jedanaest",
                    "dvanaest",
                    "trinaest",
                    "četrnaest",
                    "petnaest",
                    "šesnaest",
                    "sedamnaest",
                    "osamnaest",
                    "devetnaest",
                    "dvadeset",
                    "trideset",
                    "četrdeset",
                    "pedeset",
                    "šezdeset",
                    "sedamdeset",
                    "osamdeset",
                    "devedeset",
                ],
            )
    }

    /// Verbs in -laziti keep the accent on the syllable before -la- (the
    /// prefix) in the infinitive, the participles, the aorist and the
    /// imperfect, and so does their verbal noun: dòlaziti, dòlazio,
    /// dòlazēći, dòlažāh, pronàlaziti, snàlaziti, obìlaziti; dòlažēnje,
    /// snàlažēnje, pronàlažēnje (Školski rječnik, Hrvatski mrežni rječnik,
    /// HJP). The rule for long infinitives and the one for -enje would put it
    /// on -la- or -že-. The forms -lazi, -laze are also the plurals of the
    /// nouns prȍlaz, ȉzlaz, ȕlaz, nȁlaz, prȉlaz, zȁlaz, rȁzlaz, prȅlaz, so
    /// after those prefixes they count only where the accent is not on the
    /// first syllable anyway (pronàlazi; but snàlazi, dòlazi).
    fn laziti_verb(&self, r: &mut StressResult) -> bool {
        const LAZ: &[&str] = &[
            "iti", "it", "io", "ila", "ilo", "ili", "ile", "im", "iš", "imo", "ite", "eći", "ih", "ismo", "iste",
            "iše", "ivši", "iću", "ićeš", "iće", "ićemo", "ićete", "i", "e",
        ];
        const LAZ_SOFT: &[&str] = &["enje", "enja", "enju", "enjem", "enjima", "ah", "aše", "asmo", "aste", "ahu"];
        let (pos, third_person) = if let Some(pos) = self.suffix_at("laz", LAZ) {
            (pos, is_one_of(tail(self.w, pos + 3), &["i", "e"]))
        } else if let Some(pos) = self.suffix_at("laž", LAZ_SOFT) {
            (pos, false)
        } else {
            return false;
        };
        let mut before = &self.w[..pos];
        let noun_twin = third_person && is_one_of(before, &["pro", "iz", "u", "na", "pri", "za", "raz", "pre"]);
        // The prefixes, with the linking i of obi-, nai-, razi-, si- and
        // the ne- of a negated verbal noun (nesnàlažēnje)
        if !is_verbal_prefix(before) && starts_with(before, "ne") {
            before = &before[2..];
        }
        if !is_verbal_prefix(before)
            && let [rest @ .., 'i'] = before
        {
            before = rest;
        }
        if before.is_empty() || !is_verbal_prefix(before) {
            return false;
        }
        let k = self.nucleus_before(pos);
        if k < 0 || (noun_twin && k == 0) {
            return false;
        }
        r.nucleus = k;
        r.accent = Accent::Rising;
        true
    }

    /// Nouns in -teljstvo have the accent of -telj- in every case:
    /// prijatèljstvo, prijatèljstāvā, roditèljstvo, gledatèljstvo,
    /// ugostitèljstvo, pokrovitèljstvo (Školski rječnik; HJP has
    /// ròditēljstvo and gràditēljstvo for two of them).
    fn teljstvo_noun(&self, r: &mut StressResult) -> bool {
        const STVO: &[&str] = &["vo", "va", "vu", "vom", "vima", "ava"];
        if !self.at_suffix("teljst", STVO, 1, r) {
            return false;
        }
        r.accent = Accent::Rising;
        true
    }

    /// Adjectives in -ljiv, their adverbs and the nouns in -ljivost have the
    /// accent on the syllable before the suffix: vìdljiv, nevìdljiv,
    /// nevìdljivōst, čìtljiv, razùmljiv, prihvàtljiv, snalàžljiv,
    /// nepopràvljiv, neodòljiv (HJP, Školski rječnik, Mrežnik). The
    /// comparative has it on -ljì- (vidljìvijī), which the rule for -ij gives.
    fn ljiv_adjective(&self, r: &mut StressResult) -> bool {
        const LJIV: &[&str] = &[
            "", "a", "o", "i", "e", "u", "og", "om", "oj", "ih", "im", "ima", "oga", "ome", "omu", "ost", "osti",
            "ošću", "ostima",
        ];
        if !self.before_suffix("ljiv", LJIV, r) {
            return false;
        }
        r.accent = Accent::Rising;
        true
    }

    /// Loan adjectives in -antan and -entan keep the accent of -ant-, -ent-
    /// in every form: interesàntan, interesàntno, elegàntna, toleràntnī,
    /// kompetèntan, inteligèntnōst (HJP, Školski rječnik, Rečnik Matice
    /// srpske). The comparative (interesàntnijī) comes from the rule for -ij.
    fn ant_adjective(&self, r: &mut StressResult) -> bool {
        const ADJ: &[&str] = &[
            "an", "na", "no", "ni", "ne", "nu", "nog", "nom", "noj", "nih", "nim", "noga", "nome", "nomu", "nima",
            "nost", "nosti", "nošću", "nostima",
        ];
        if !self.at_suffix("ant", ADJ, 0, r) && !self.at_suffix("ent", ADJ, 0, r) {
            return false;
        }
        r.accent = Accent::Rising;
        true
    }

    /// Nouns whose last stem syllable is long: loans (telefon, restoran,
    /// kapetan, rezultat, programer, piramida, analiza) and the derived
    /// nouns in -ač, -ar, -aš, -njak, -ljak, -enik (prodavač, čuvar,
    /// košarkaš, čarobnjak, zemljak, zaposlenik). Every case form has a long
    /// rising accent on that syllable (telefóna, restorána, rezultáta,
    /// programéra, prodaváča, čuvára, zemljáka, zaposleníka), and so do the
    /// possessives (čuvárov, zemljákova) and the genitive plural. In the
    /// nominative, where the syllable is the last one and cannot carry a
    /// rising accent, the dictionaries move the accent one syllable back
    /// and leave the length behind it (telèfōn, restòrān, prodàvāč,
    /// čaròbnjāk); Stojan and Mirsad say exactly that, the
    /// Croatian voice leaves the unstressed length out (restòran,
    /// prodàvač), as Croatian is commonly spoken.
    ///
    /// Which stem endings are long is decided by their shape, and only
    /// where that shape is reliable; the rest stays with the default and
    /// the lexicon. Where an ending is also an inflection of native words
    /// (-ala of participles, -ana of passive participles, -ina of
    /// possessives) it is left out or limited, see long_rhyme().
    fn long_stem(&self, r: &mut StressResult) -> bool {
        if self.negated_prefixed() {
            return false; // nȅpoznata, nȅpotpisana
        }
        const ENDINGS: [&str; 32] = [
            "ovom", "ovog", "ovoj", "ovih", "ovim", "evom", "evog", "evoj", "evih", "evim", "ima", "ama", "ova", "ovo",
            "ovi", "ove", "ovu", "eva", "evo", "evi", "eve", "evu", "om", "em", "ov", "ev", "a", "e", "i", "u", "o",
            "",
        ];
        let w = self.w;
        // -anac, -inac, -unac, -onac, -enac, -irac: the vowel before the
        // fleeting a is the long one, in the nominative as well
        // (Amerikánac, Dalmatínac, bjegúnac, Alžírac).
        if ends_with(w, "ac")
            && w.len() >= 5
            && is_one_of(&w[w.len() - 4..w.len() - 2], &["an", "in", "un", "on", "en", "ir"])
        {
            let k = self.nucleus_at(w.len() - 4);
            if k >= 1 {
                r.nucleus = k;
                r.is_long = true;
                return true;
            }
        }
        for ending in ENDINGS {
            if !ends_with(w, ending) {
                continue;
            }
            let stem = &w[..w.len() - ending.len()];
            if stem.len() < 3 || stem.last().is_some_and(|&c| is_vowel_letter(c)) {
                continue;
            }
            // The last vowel of the stem
            let Some(p) = stem.iter().rposition(|&c| is_vowel_letter(c)) else { continue };
            if stem.len() - p > 4 {
                continue; // rhyme: vowel + up to three consonants
            }
            let k = self.nucleus_at(p);
            if k < 1 {
                continue; // not the first syllable
            }
            if !self.long_rhyme(stem, p, k, ending) {
                continue;
            }
            let nik = has_at(stem, p, "ik") || has_at(stem, p, "ic");
            if nik && k < 3 {
                // The nouns in -nik of three syllables keep the first
                // syllable in every form, with the length of -nīk behind
                // it (ȕčenīk, ȕčenīka, ȕčenīci; pòznanīk, pòdanīk,
                // ùstanīk, ìzgnanīk, prògnanīk in Školski rječnik and HJP)
                r.nucleus = 0;
                if !self.croatian() {
                    r.long_after = k;
                }
            } else if ending.is_empty() {
                // The longer nouns in -enik, -anik keep the accent of the
                // participle they come from, the syllable before the
                // suffix (zaròbljenīk, zapòslenīk, osigùranīk); the rest
                // move it one syllable back.
                r.nucleus = if nik { (k - 2).max(0) } else { k - 1 };
                if !self.croatian() {
                    r.long_after = k;
                }
            } else {
                r.nucleus = k;
                r.is_long = true;
            }
            return true;
        }
        false
    }

    /// Is the stem's last syllable, whose vowel is at `p` (nucleus `k`),
    /// one of the long shapes? `ending` is the inflection after the stem.
    fn long_rhyme(&self, stem: &[char], p: usize, k: i32, ending: &str) -> bool {
        let rhyme = &stem[p..];
        // The e of "ije" (svijeta, cvijeta), but not that of the loans in
        // -ier, whose "ije" is two syllables (premìjēr, premijéra,
        // karijéra, hotelijéra; see is_ije_diphthong)
        if p >= 2 && has_at(stem, p - 2, "ije") && !(eq(rhyme, "er") && self.nucleus_at(p - 2) >= 0) {
            return false;
        }
        let preceded_by = |list: &[&str]| ends_with_any(&stem[..p], list);
        let rhyme_is = |list: &[&str]| is_one_of(rhyme, list);
        let ending_is = |list: &[&str]| list.contains(&ending);
        let nominative = ending.is_empty();
        let possessive = ending.starts_with("ov") || ending.starts_with("ev");
        // Unconditional: prodavač/prodaváča, garaža, čuvar/čuvára, papir,
        // suvenir, račun/račúna, majmun, analiza, košarkaš/košarkáša.
        if rhyme_is(&["ač", "až", "ar", "ir", "un"]) {
            return true;
        }
        // -aš: the same, but -aše after one syllable is the imperfect
        // (gledaše, imaše), not nogometáše.
        if rhyme_is(&["aš"]) {
            return ending != "e" || k >= 2;
        }
        // Not after a verbal prefix, where native words have the same
        // shape with the first syllable stressed (zákon, nàslon, pòznat,
        // zàhvat, nágrada, západ, ùgriz): telefon, balkon, sezona,
        // rezultat, aparat, format, čokolada, limunada, komad, analiza.
        if rhyme_is(&["on", "ad", "iz"]) {
            return !self.prefixed_word();
        }
        // -at the same, and not before -e and -i, which are the present of
        // every verb in -ati (imate, gledate, čitati): rezultata, aparatu,
        // formatom, kandidatima.
        if rhyme_is(&["at"]) {
            return !self.prefixed_word() && !ending_is(&["e", "i"]);
        }
        // -an also needs two syllables before it and no -av-/-iv-/-ir-,
        // because of the passive participles (poslana, prodavana,
        // planirana) and the names (Ìvana, Stjȅpana): kapetan, Talijan,
        // šarlatan. Not -alan: the adjectives in -alan keep the first
        // syllable whatever their length (lègālan, nȍrmālan, ȉlegālan,
        // ȍriginālan, prȍfesionālan).
        if rhyme_is(&["an"]) {
            return k >= 2 && !self.prefixed_word() && !preceded_by(&["av", "iv", "ir", "al"]);
        }
        // -er, -et, -id need two syllables before them: programer,
        // inženjer, dizajner, kabinet, piramida, invalid, but jȅzera,
        // vȅčera, sȅrvera, prȅdmeta, dȅteta, ȕvida. -ete is the present
        // of the verbs in -eti (odaberete, pošaljete).
        if rhyme_is(&["er", "id"]) {
            return k >= 2;
        }
        if rhyme_is(&["et"]) {
            return k >= 2 && ending != "e";
        }
        // -in: the case forms are the -ina rule's, and -ino is a
        // possessive (mȁmino, Ȁnino); here the nominative of longer words
        // (vitàmin, magàzin, aspìrin; not the possessives Ìvanin, Jȅlenin,
        // kràljičin, bȁkin, Mȁjin, told by the letter before the suffix),
        // the dative plural and the possessives of the nouns (vitamínima,
        // Antonínov).
        if rhyme_is(&["in"]) {
            if ending_is(&["a", "e", "i", "u", "o", "om", "ama"]) {
                return false;
            }
            if nominative {
                return k >= 2 && !p.checked_sub(1).is_some_and(|at| matches!(stem[at], 'n' | 'c' | 'č' | 'k' | 'j'));
            }
            return true;
        }
        // -njak, -ljak, with the plural in -njaci: čarobnjak, zemljak,
        // seljak, bezveznjak, duhovnjak, čarobnjácima; the other nouns in
        // -ak go both ways (jùnāk, čàrdak, pȅtak).
        if rhyme_is(&["ak"]) || (rhyme_is(&["ac"]) && ending_is(&["i", "ima"])) {
            return preceded_by(&["nj", "lj"]);
        }
        // -nik after a vowel in a prefixed word, with its plural in -nici:
        // zarobljenik, zaposlenik, zaposleníci, osiguranik, povjerenik;
        // not rȁdnīk, pȕtnīk, kȍrisnīk (consonant before the n). Those of
        // three syllables (ȕčenīk, pòznanīk) keep the first syllable, see
        // long_stem().
        if rhyme_is(&["ik"]) || (rhyme_is(&["ic"]) && ending_is(&["i", "ima"])) {
            return p >= 2 && stem[p - 1] == 'n' && is_vowel_letter(stem[p - 2]) && self.prefixed_word();
        }
        // -tiv, -hiv, -siv: motiv, arhiv, kolektiv, perspektiva, masiv;
        // not the adjectives in -ljiv, -čiv and the nouns in -ziv (náziv).
        if rhyme_is(&["iv"]) {
            return preceded_by(&["t", "h", "s"]);
        }
        // -uz: Francuza, kukuruza; not the dative of zadruga, usluga.
        if rhyme_is(&["uz"]) {
            return ending_is(&["", "a", "u", "om", "em", "ima"]) || possessive;
        }
        // -al only where no participle can be spelled the same way
        // (kanálu, kanálom, generálima, materijálom; not -ala, -ali).
        if rhyme_is(&["al"]) {
            return ending_is(&["", "u", "om", "em", "ima"]) || possessive;
        }
        // -itak, -utak: dobitka, užitka, imutka (the nominatives dobitak,
        // užitak have the fleeting a, which puts the long syllable before
        // the last one: dobítak). Not -inka: lȍzīnka next to Dalmatínka.
        // And the case forms of the -anac, -inac nouns above.
        if rhyme_is(&["itk", "utk", "anc", "inc", "unc", "onc", "enc", "irc", "alc", "olc"]) {
            return !nominative;
        }
        false
    }

    /// Does the word begin with one or more verbal prefixes (po, iz, raz-po,
    /// u) followed by a root that has a vowel before letter `end`? The lone
    /// s- is not counted when the whole word is asked about (sezona, senat
    /// are not prefixed).
    fn prefixed_before(&self, end: usize, whole_word: bool) -> bool {
        let w = self.w;
        (1..end.min(7)).any(|len| {
            !(whole_word && len == 1 && w[0] == 's')
                && is_verbal_prefix(&w[..len])
                && w[len..end].iter().any(|&c| is_vowel_letter(c))
        })
    }

    fn prefixed_word(&self) -> bool {
        self.prefixed_before(self.w.len(), true)
    }

    /// "ne" followed by a prefixed word: the negated adjectives (nepoznat,
    /// nepotreban, neutralan, nepotpisan), which keep the accent of the
    /// first syllable or of the participle, never a loan's long syllable.
    fn negated_prefixed(&self) -> bool {
        let w = self.w;
        w.len() >= 5
            && starts_with(w, "ne")
            && (1..=6usize)
                .take_while(|len| len + 2 < w.len())
                .any(|len| is_verbal_prefix(&w[2..2 + len]) && w[len + 2..].iter().any(|&c| is_vowel_letter(c)))
    }

    /// A word spelled the English way that is a native word made from an
    /// English stem keeps the native suffix's accent (chatírati,
    /// photoshopírati, thatcherìzam), and a verb of the user's accent
    /// dictionary its accent; see "English words".
    pub fn derivation(&self) -> StressResult {
        let mut r = StressResult::default();
        if self.user_verbs.is_some_and(|verbs| self.verb_form_in(verbs, &mut r)) {
            return r;
        }
        r = StressResult::default();
        if self.irati_verb(&mut r) || self.izam_noun(&mut r) { r } else { StressResult::default() }
    }

    /// Long infinitives are most often stressed on the antepenult
    /// (govòriti, zabòraviti, razùmjeti).
    pub fn weak(&self) -> StressResult {
        let mut r = StressResult::default();
        let n = self.count();
        if n >= 4 && ends_with(self.w, "ti") && self.w.len() >= 3 && is_vowel_letter(self.w[self.w.len() - 3]) {
            r.nucleus = n - 3;
        }
        r
    }

    fn count(&self) -> i32 {
        index(self.letters.len())
    }

    /// -irati verbs: telefonírati, kombinírām, kopíran
    fn irati_verb(&self, r: &mut StressResult) -> bool {
        const IR_ENDINGS: &[&str] = &[
            "ati", "am", "aš", "a", "amo", "ate", "aju", "ao", "ala", "alo", "ali", "ale", "an", "ana", "ano", "ani",
            "ane", "anu", "anje", "anja", "anju", "anjem", "ajući", "aj", "ajte",
        ];
        if self.count() < 3 || !self.at_suffix("ir", IR_ENDINGS, 0, r) {
            return false;
        }
        r.is_long = true;
        true
    }

    /// -izam: realìzam, turìzma
    fn izam_noun(&self, r: &mut StressResult) -> bool {
        const IZAM: &[&str] = &["am", "ma", "mu", "mom", "me", "mi", "mima"];
        self.count() >= 3 && self.at_suffix("iz", IZAM, 0, r)
    }

    /// Is the word, from `start` on, a form of the verb with this root? With
    /// `negated` the word is "ne" + that form, and only the passive
    /// participle counts.
    fn form_of(&self, root: &VerbRoot, start: usize, r: &mut StressResult, negated: bool) -> bool {
        #[derive(Clone, Copy, PartialEq, Eq)]
        enum Form {
            Plain,
            Present,
            Passive,
        }
        use Form::{Passive, Plain, Present};
        let w = self.w;
        let after_root = start + root.root.len();
        let mut form: Option<Form> = None;

        if w.get(start..).is_some_and(|rest| rest.starts_with(&root.root)) {
            let ending = &w[after_root..];
            let is = |list: &[&str]| is_one_of(ending, list);
            if root.iti {
                if is(&[
                    "iti", "it", "imo", "ite", "io", "ila", "ilo", "ili", "ile", "ih", "ismo", "iste", "iše", "ivši",
                    "iću", "ićeš", "iće", "ićemo", "ićete",
                ]) {
                    // "uredimo" is both; the imperative keeps the accent in
                    // every voice, the present is the commoner reading.
                    form = Some(if is(&["imo"]) { Present } else { Plain });
                } else if is(&["i"]) {
                    // "Potvrdi", "Uredi", "Otvori" at the head of a clause
                    // are commands; elsewhere (u potvrdi, uredi su...) the
                    // noun has priority.
                    if !root.noun_twin || self.opens_clause {
                        form = Some(Plain);
                    }
                } else if is(&["im", "iš"]) || (is(&["e"]) && !root.noun_twin) {
                    form = Some(Present);
                }
            }
            if form.is_none() && root.ati {
                if is(&[
                    "ati", "at", "ao", "ala", "alo", "ali", "ale", "ah", "asmo", "aste", "aše", "avši", "ajući",
                    "anje", "anja", "anju", "anjem", "anjima", "aću", "aćeš", "aće", "aćemo", "aćete",
                ]) {
                    form = Some(Plain);
                } else if root.ati_present && is(&["aju"]) {
                    // The third person plural keeps the infinitive's accent
                    // where the other persons move it (otvárajū, òtvārām;
                    // ponávljajū, pònāvljām in Školski rječnik)
                    form = Some(Plain);
                } else if root.ati_present
                    && (is(&["am", "aš", "amo", "ate", "aj", "ajmo", "ajte"]) || (is(&["a"]) && !root.noun_twin))
                {
                    form = Some(Present);
                } else if is(&[
                    "an", "ana", "ano", "ani", "ane", "anu", "anog", "anoga", "anom", "anome", "anoj", "anih", "anim",
                    "anima",
                ]) {
                    form = Some(Passive);
                }
            }
            if form.is_none() && root.nuti {
                if is(&[
                    "nuti", "nuo", "nula", "nulo", "nuli", "nule", "nuh", "nusmo", "nuste", "nuše", "nuvši", "nimo",
                    "nite", "nuću", "nućeš", "nuće", "nućemo", "nućete",
                ]) {
                    form = Some(Plain);
                } else if is(&["ni"]) {
                    // "Napomeni mu" is the command, "u napomeni" the noun
                    if !root.noun_twin || self.opens_clause {
                        form = Some(Plain);
                    }
                } else if is(&["nem", "neš", "nemo", "nete"]) || (is(&["ne", "nu"]) && !root.noun_twin) {
                    form = Some(Present);
                } else if is(&[
                    "nut", "nuta", "nuto", "nute", "nutu", "nutog", "nutoga", "nutom", "nutoj", "nutih", "nutim",
                    "nutima",
                ]) {
                    form = Some(Passive);
                }
            }
            if form.is_none() && root.e_present {
                if is(&[
                    "i", "imo", "ite", "en", "ena", "eno", "eni", "ene", "enu", "enog", "enoga", "enom", "enoj",
                    "enih", "enim", "enima",
                ]) {
                    form = Some(Plain);
                } else if is(&["em", "eš", "e", "emo", "ete", "u", "ući"]) {
                    form = Some(Present);
                }
            }
        }
        if form.is_none()
            && root.iti
            && root.soft.iter().any(|soft| {
                w.get(start..).is_some_and(|rest| rest.starts_with(soft))
                    && is_one_of(
                        &w[start + soft.len()..],
                        &[
                            "en", "ena", "eno", "eni", "ene", "enu", "enog", "enoga", "enom", "enome", "enomu", "enoj",
                            "enih", "enim", "enima",
                        ],
                    )
            })
        {
            form = Some(Passive);
        }
        let Some(form) = form else { return false };
        if negated && form != Passive {
            return false;
        }

        // The verbal nouns of the prefixed perfectives in -ati with a short
        // accent and the passive one syllable back (pobòljšati, pȍboljšān)
        // have a long rising accent on -ánje, in every voice:
        // poboljšánje, povećánje, pojačánje, pogoršánje, obećánje,
        // olakšánje (HJP, Školski rječnik)
        if form == Plain
            && root.whole_stem
            && root.ati_present
            && !root.is_long
            && root.passive == 1
            && has_at(w, after_root, "anj")
        {
            let a = self.nucleus_at(after_root);
            if a >= 0 {
                *r = StressResult { nucleus: a, is_long: true, accent: Accent::Rising, ..*r };
                return true;
            }
        }

        let k = self.nucleus_at(start + root.vowel);
        if k < 0 {
            return false;
        }
        let mut shifted = k;
        if negated {
            // The negated participles are adjectives with the accent the
            // dictionaries give the participle, in every voice: nepròčitan,
            // nedòvršen, nepòvezan, neìspunjen, nepròmijenjen (one syllable
            // before the root vowel), neòbračunan for ȍbračunan, and
            // neprèslušan, nezàustavljen where the verb's own accent is
            // already on its prefix. An unprefixed verb hands it to "ne"
            // (nȅplaćen, nȅpisan, nèsrēđen).
            if root.passive == 2 {
                shifted = self.nucleus_after(start);
            } else if root.passive == 1 || !root.whole_stem || !self.prefixed_root(root, start) {
                shifted = (k - 1).max(0);
            }
        } else if !self.croatian() || root.shifts_everywhere {
            if (form == Present && root.present_shifts) || (form == Passive && root.passive == 1) {
                shifted = k - 1;
            }
            if form == Passive && root.passive == 2 {
                shifted = 0;
            }
        }
        if shifted >= 0 && shifted < k {
            r.nucleus = shifted;
            // The Croatian voice has no length after the accent (ùključen
            // for the dictionaries' ùkljūčen)
            let croatian_shift = !negated && self.croatian();
            if root.is_long && !croatian_shift {
                r.long_after = k;
            }
            r.accent = Accent::Rising;
        } else {
            r.nucleus = k;
            r.is_long = root.is_long;
            r.accent = root.accent;
        }
        true
    }

    /// naj- + a comparative of one syllable and its ending: najbolji,
    /// najveće, najmanjoj, najbrži, najprije. The stem of such a comparative
    /// ends in a palatal (bolj, već, mlađ, draž, ljepš, češć) or is gor.
    fn superlative_of_short_comparative(&self) -> bool {
        const ENDINGS: [&str; 14] = ["ima", "ega", "emu", "eg", "em", "oj", "om", "ih", "im", "i", "a", "e", "o", "u"];
        if self.count() < 2 || !starts_with(self.w, "naj") {
            return false;
        }
        let rest = &self.w[3..];
        // najam, najava
        if rest.first().is_none_or(|&c| is_vowel_letter(c)) {
            return false;
        }
        let Some(ending) = ENDINGS.iter().find(|ending| ends_with(rest, ending)) else { return false };
        let stem = &rest[..rest.len() - ending.len()];
        let vowels = stem.iter().filter(|&&c| is_vowel_letter(c)).count();
        if vowels > 1 || (vowels == 0 && !has_inner_syllabic_r(stem)) {
            return false;
        }
        eq(stem, "gor") || ends_with_any(stem, &["j", "ć", "đ", "č", "ž", "š"])
    }

    /// The syllable whose nucleus is at `letter`, -1 for none.
    fn nucleus_at(&self, letter: usize) -> i32 {
        self.letters.iter().position(|&at| at == letter).map_or(-1, index)
    }

    /// The last syllable before `letter`, -1 for none.
    fn nucleus_before(&self, letter: usize) -> i32 {
        self.letters.iter().rposition(|&at| at < letter).map_or(-1, index)
    }

    /// The first nucleus at or after `letter`.
    fn nucleus_after(&self, letter: usize) -> i32 {
        self.letters.iter().position(|&at| at >= letter).map_or(-1, index)
    }

    /// Does the verb whose root begins at `start` carry its accent in or
    /// after a verbal prefix (prèslušati, zaùstaviti), rather than on an
    /// unprefixed root (sréditi, plátiti)? The lone s- is not a prefix here.
    fn prefixed_root(&self, root: &VerbRoot, start: usize) -> bool {
        let w = self.w;
        let from = tail(w, start);
        let before = &from[..root.vowel.min(from.len())];
        if !before.is_empty() {
            // spremiti, srediti, snimiti, skratiti: the s is part of the root
            if before[0] == 's' && !starts_with(before, "sa") && !starts_with(before, "su") {
                return false;
            }
            if is_verbal_prefix(before) {
                return true;
            }
        }
        // ìspuniti, ùdarati, prèslušati: the accent is in the prefix itself
        (root.vowel + 1..=6).take_while(|len| len < &from.len()).any(|len| is_verbal_prefix(&from[..len]))
    }

    /// Where `suffix` begins when the word ends in it and one of `endings`,
    /// with something before it.
    fn suffix_at(&self, suffix: &str, endings: &[&str]) -> Option<usize> {
        let w = self.w;
        endings.iter().find_map(|ending| {
            let stem = w.len().checked_sub(length(ending))?;
            let at = stem.checked_sub(length(suffix)).filter(|&at| at > 0)?;
            (ends_with(w, ending) && ends_with(&w[..stem], suffix)).then_some(at)
        })
    }

    /// Stress on the vowel at `offset` inside the suffix.
    fn at_suffix(&self, suffix: &str, endings: &[&str], offset: usize, r: &mut StressResult) -> bool {
        let Some(pos) = self.suffix_at(suffix, endings) else { return false };
        let k = self.nucleus_at(pos + offset);
        if k < 0 || self.nucleus_before(pos) < 0 {
            return false;
        }
        r.nucleus = k;
        true
    }

    /// Stress on the syllable before the suffix.
    fn before_suffix(&self, suffix: &str, endings: &[&str], r: &mut StressResult) -> bool {
        let Some(pos) = self.suffix_at(suffix, endings) else { return false };
        let k = self.nucleus_before(pos);
        if k < 0 {
            return false;
        }
        r.nucleus = k;
        true
    }
}
