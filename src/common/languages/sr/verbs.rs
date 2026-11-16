//! The verbs whose forms keep the accent of the infinitive on their root:
//! the entries of the two verb tables and of the user's lexicon.

use super::lexicon::{IJE_VERBS, VERBS};
use crate::formant::phones::Accent;
use crate::text::frontend::letters::{chars, ends_with, find, is_base_letter, starts_with, to_lower};
use crate::text::frontend::notation::mark_accent;
use std::sync::LazyLock;

/// A verb root from `IJE_VERBS` or a whole stem from `VERBS`, or one of the
/// user's.
#[derive(Clone, Debug, Default, PartialEq, Eq)]
pub struct VerbRoot {
    pub root: Vec<char>,
    /// Before the participle's -en: dijel -> dijelj.
    pub soft: Vec<Vec<char>>,
    /// Letter of the accented vowel within the root.
    pub vowel: usize,
    pub is_long: bool,
    /// Optional explicit tone of a whole stem.
    pub accent: Accent,
    /// Starts the word, takes no further prefix.
    pub whole_stem: bool,
    /// urediti, uredim, uredi, uredio, uređen.
    pub iti: bool,
    /// pročitati, pročitao, pročitan.
    pub ati: bool,
    /// ... and pročitam, pročitaj.
    pub ati_present: bool,
    /// pokrenuti, pokrenem, pokreni, pokrenuo.
    pub nuti: bool,
    /// pokažem, pokaži.
    pub e_present: bool,
    /// "potvrdi", "uredi" are also forms of a noun.
    pub noun_twin: bool,
    /// Dictionaries: ùrēdīm for uréditi.
    pub present_shifts: bool,
    /// Dictionaries: 1 = one syllable back, 2 = first.
    pub passive: u8,
    /// Empty: any verbal prefixes.
    pub prefixes: Vec<Vec<char>>,
}

/// The consonant changes before -en: podijeljen, zamijenjen, zalijepljen,
/// proslijeđen, primijećen, obaviješten, očišćen, zamišljen, odbačen.
fn soft_roots(root: &[char]) -> Vec<Vec<char>> {
    const CHANGES: [(&str, &str, Option<&str>); 16] = [
        ("st", "št", Some("šć")),
        ("sn", "šnj", None),
        ("zn", "žnj", None),
        ("sl", "šlj", None),
        ("tl", "tlj", None),
        ("d", "đ", None),
        ("t", "ć", None),
        ("n", "nj", None),
        ("l", "lj", None),
        ("p", "plj", None),
        ("b", "blj", None),
        ("v", "vlj", None),
        ("m", "mlj", None),
        ("s", "š", None),
        ("z", "ž", None),
        ("c", "č", None),
    ];
    let Some(&(from, to, also)) = CHANGES.iter().find(|(from, ..)| ends_with(root, from)) else {
        return vec![root.to_vec()];
    };
    let base = &root[..root.len() - from.len()];
    let soft = |to: &str| base.iter().copied().chain(to.chars()).collect();
    std::iter::once(to).chain(also).map(soft).collect()
}

impl VerbRoot {
    /// "root:classes[:prefixes]" (roots with a long ije) or "st'e:m=classes"
    /// (whole stems, accent marked as in the lexicon); see the verb tables.
    /// `None` for an entry that cannot be used.
    pub fn parse(text: &[char], whole: bool) -> Option<VerbRoot> {
        let split = text.iter().position(|&c| c == if whole { '=' } else { ':' })?;
        let mut classes = &text[split + 1..];
        let mut root = VerbRoot { whole_stem: whole, ..VerbRoot::default() };
        let letters = text[..split].iter().map(|&raw| to_lower(raw));
        if whole {
            let mut marked = false;
            for c in letters {
                if let Some(accent) = mark_accent(c) {
                    if marked {
                        return None;
                    }
                    root.vowel = root.root.len();
                    root.accent = accent;
                    marked = true;
                } else if c == ':' {
                    if !marked || root.vowel + 1 != root.root.len() {
                        return None;
                    }
                    root.is_long = true;
                } else if is_base_letter(c) {
                    root.root.push(c);
                } else {
                    return None;
                }
            }
            if !marked || root.vowel >= root.root.len() {
                return None;
            }
        } else {
            for c in letters {
                if !is_base_letter(c) {
                    return None;
                }
                root.root.push(c);
            }
            root.vowel = find(&root.root, "ije")? + 2;
            root.is_long = true;
            // These all shift in the dictionaries: pòdijēlīm, pòdijēljen.
            root.present_shifts = true;
            root.passive = 1;
            if let Some(second) = classes.iter().position(|&c| c == ':') {
                root.prefixes = classes[second + 1..]
                    .split(|&c| c == ' ')
                    .filter(|p| !p.is_empty())
                    .map(<[char]>::to_vec)
                    .collect();
                classes = &classes[..second];
            }
        }
        root.soft = soft_roots(&root.root);
        let mut any_class = false;
        for &c in classes {
            match c {
                'i' => root.iti = true,
                'a' => (root.ati, root.ati_present) = (true, true),
                't' => root.ati = true,
                'u' => root.nuti = true,
                'e' => root.e_present = true,
                'n' => root.noun_twin = true,
                '<' => root.present_shifts = true,
                'p' => root.passive = 1,
                'P' => root.passive = 2,
                // Marks a verb whose accent the Croatian voice shifts too; in
                // Serbian every verb of the tables shifts
                '!' => {}
                _ => return None,
            }
            any_class |= matches!(c, 'i' | 'a' | 't' | 'u' | 'e');
        }
        any_class.then_some(root)
    }

    /// A verb entry of the user's lexicon: a whole stem when it has "=", a
    /// root with a long ije otherwise.
    pub fn parse_entry(text: &[char]) -> Option<VerbRoot> {
        VerbRoot::parse(text, text.contains(&'='))
    }

    /// What is wrong with a verb entry of the user's lexicon, if anything.
    pub fn check_entry(text: &[char]) -> Result<(), &'static str> {
        match VerbRoot::parse_entry(text) {
            Some(_) => Ok(()),
            None if text.contains(&'=') => Err("expected st'e:m=classes (i a t u e n < p P)"),
            None => Err("expected root:classes[:prefixes] with a long ije (i a t e)"),
        }
    }
}

/// The verbs of the two built-in tables: the roots with a long ije, then
/// the whole stems.
pub fn verb_roots() -> &'static [VerbRoot] {
    static ROOTS: LazyLock<Vec<VerbRoot>> = LazyLock::new(|| {
        let table = |entries: &[&str], whole| -> Vec<VerbRoot> {
            entries.iter().filter_map(|entry| VerbRoot::parse(&chars(entry), whole)).collect()
        };
        [table(IJE_VERBS, false), table(VERBS, true)].concat()
    });
    &ROOTS
}

/// One or more verbal prefixes: po, ras-po, u-na, is.
pub fn is_verbal_prefix(s: &[char]) -> bool {
    const PREFIXES: [&str; 25] = [
        "do", "iz", "is", "na", "nad", "o", "ob", "od", "po", "pod", "pot", "pre", "pred", "pri", "pro", "raz", "ras",
        "s", "sa", "su", "u", "uz", "us", "za", "obez",
    ];
    // From the end: whether the letters from each place on are prefixes
    // (a row of s, u, o can be cut in very many ways)
    let mut prefixes_from = vec![false; s.len() + 1];
    prefixes_from[s.len()] = true;
    for at in (0..s.len()).rev() {
        prefixes_from[at] = PREFIXES
            .iter()
            .any(|prefix| starts_with(&s[at..], prefix) && prefixes_from.get(at + prefix.len()) == Some(&true));
    }
    prefixes_from[0]
}
