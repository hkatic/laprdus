//! Intonation model shared by the formant and the recorded voices: word
//! accents, question and boundary movements and declination of one clause, as
//! a pitch contour in semitones.
//!
//! The rules were tuned on the formant voices against recordings of read
//! Croatian (docs/formant.md, "Intonation") and are used unchanged by the
//! recorded voices, whose pitch is set by the PSOLA renderer.

use super::{ClauseKind, MINOR_BREAK_MOVEMENT, Utterance};
use crate::formant::phones::Accent;

/// Timing and accent of one syllable of a clause.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct Syllable {
    /// Nucleus start (ms), negative when the syllable has no timing.
    pub t0: f32,
    /// Nucleus end (ms).
    pub t1: f32,
    pub stressed: bool,
    pub accent: Accent,
    pub prominence: u8,
    pub word: i32,
}

impl Default for Syllable {
    fn default() -> Self {
        Self { t0: -1.0, t1: -1.0, stressed: false, accent: Accent::None, prominence: 0, word: 0 }
    }
}

impl Syllable {
    fn timed(&self) -> bool {
        self.t0 >= 0.0
    }

    fn middle(&self) -> f32 {
        self.t0 + 0.5 * (self.t1 - self.t0)
    }
}

/// Piecewise-linear pitch movement, zero before its own time span and held at
/// its last value after it.
#[derive(Clone, Debug, Default)]
struct Bump(Vec<(f32, f32)>);

impl Bump {
    fn new<const N: usize>(points: [(f32, f32); N]) -> Self {
        Self(points.to_vec())
    }

    fn at(&self, t: f32) -> f32 {
        let (Some(first), Some(last)) = (self.0.first(), self.0.last()) else { return 0.0 };
        if t >= last.0 {
            return last.1;
        }
        if t <= first.0 {
            return 0.0;
        }
        self.0.windows(2).find(|pair| t < pair[1].0).map_or(last.1, |pair| {
            let ((ta, va), (tb, vb)) = (pair[0], pair[1]);
            let span = tb - ta;
            let x = if span > 0.0 { (t - ta) / span } else { 1.0 };
            va + (vb - va) * x
        })
    }
}

/// The pitch contour of one clause, built from its syllables.
///
/// Every stressed syllable of a content word gets an accent movement (its
/// shape chosen by the word's accent type), a question its rise on the
/// focused word, the clause its boundary movement (fall, suspended rise,
/// final rise) and a declining baseline. [`at`](Self::at) sums them all at a
/// time (ms from the start of the clause's sound) in semitones around the
/// voice's base pitch, before the inflection level is applied.
#[derive(Clone, Debug)]
pub struct Intonation {
    bumps: Vec<Bump>,
    total: f32,
    drop: f32,
}

impl Intonation {
    /// `total_ms` is the length of the clause's sound, `pitch_range` the size
    /// of the movements (1.0 = as measured).
    pub fn new(utt: &Utterance, syls: &[Syllable], total_ms: f32, pitch_range: f32) -> Self {
        let total = total_ms.max(1.0);
        let mut bumps = Vec::new();
        let accents: Vec<usize> = (0..syls.len()).filter(|&s| syls[s].stressed && syls[s].timed()).collect();

        let kind = utt.kind;
        let question = matches!(kind, ClauseKind::YesNoQuestion | ClauseKind::WhQuestion);
        let mut range = pitch_range * if kind == ClauseKind::Exclamation { 1.35 } else { 1.0 };
        // A lone syllable (a letter name while spelling, "da", "ne") has no
        // room for a full sentence melody: squeezed into it, the movement is a
        // steep glide that also hides the pitch steps screen readers use to
        // mark capital letters. A question keeps its rise and an exclamation
        // ("Ne!", "Stoj!") most of its fall, or the punctuation could not be
        // heard at all.
        if syls.len() == 1 && !question {
            range *= if kind == ClauseKind::Exclamation { 0.70 } else { 0.45 };
        }

        // In a short clause the end is all there is to tell a question from a
        // statement; up to five words get the full final rise, longer clauses
        // gradually less.
        let words = utt.phones.last().map_or(0, |phone| i32::from(phone.word) + 1);
        let brevity = ((9.0 - words as f32) / 4.0).clamp(0.4, 1.0);
        let final_index = syls.len() as i32 - 1;
        // Whether the stretch before the first accent is raised (see below).
        let sentence_onset = utt.sentence_initial && accents.len() >= 2;

        // The syllable that carries a question's rise.
        let question_syl: Option<usize> = match (kind, accents.last()) {
            (ClauseKind::YesNoQuestion, Some(&last)) => Some(
                accents
                    .iter()
                    .copied()
                    .rfind(|&a| utt.focus_word >= 0 && syls[a].word == utt.focus_word)
                    .unwrap_or(last),
            ),
            // "Što?", "A gdje?", "Tko je to?": the last accent is the last
            // syllable, so the final rise is that accent's own.
            (ClauseKind::WhQuestion, Some(&last)) if last as i32 == final_index => Some(last),
            _ => None,
        };

        for (ai, &s) in accents.iter().enumerate() {
            let syl = &syls[s];
            let (t0, t1) = (syl.t0, syl.t1);
            let d = t1 - t0;

            // The next syllable, if unstressed, carries the tail of the accent.
            let post = syls.get(s + 1).filter(|next| !next.stressed && next.timed());
            let more_after = s + 2 < syls.len();

            if question_syl == Some(s) {
                // Low on the stressed syllable, high right after it.
                let mut q = 7.0 * range.min(1.2);
                if kind == ClauseKind::WhQuestion {
                    // After the peak on the question word the rise is the
                    // smaller of the two movements.
                    q *= brevity * if ai > 0 { 0.8 } else { 1.0 };
                }
                let low = 0.3 * q;
                bumps.push(match post {
                    Some(post) => Bump::new([
                        (t0 - 30.0, 0.0),
                        (t0 + 0.4 * d, -low),
                        (t1, 0.45 * q),
                        (post.middle(), q),
                        (total, if more_after { 0.30 * q } else { 0.85 * q }),
                    ]),
                    // The whole movement has to fit into one vowel, and its
                    // end fades: low from the start of the vowel, at the top
                    // by three quarters of it and held there.
                    None => Bump::new([
                        (t0 - 60.0, 0.0),
                        (t0 - 10.0, -low),
                        (t0 + 0.15 * d, -low),
                        (t0 + 0.75 * d, 0.85 * q),
                        (total, 0.85 * q),
                    ]),
                });
                continue;
            }

            // Every content word gets an audible movement (the "pointed hats"
            // of the classic rule-based synthesizers), the first one the
            // largest: read Croatian (and synthesizer A) opens a sentence
            // high, then keeps the voice on a level with movements of two to
            // three semitones. A clause that goes on a sentence after a comma
            // starts again, but lower than the sentence did. After a short
            // pause inside the clause (12:30, 10.11.2026., a cell's address)
            // nothing starts again: the first accent moves like one in the
            // middle of the clause.
            let mut p = 1.4;
            if syl.prominence >= 2 {
                p = if ai == 0 {
                    if utt.continued {
                        2.6
                    } else if utt.sentence_initial {
                        4.5
                    } else {
                        3.4
                    }
                } else {
                    2.6 * f32::max(0.8, 1.0 - 0.04 * (ai - 1) as f32)
                };
            } else if utt.slovenian && s > 0 && syls[s - 1].word == syl.word && syls[s - 1].timed() {
                // A weaker Slovenian word stressed after its first syllable
                // (potém, sevéda, zeló, lahkó): with the small movement of a
                // function word the syllable before the stress, which the high
                // start of a sentence raises even more, was as high as the
                // stressed one or higher and was heard as stressed
                // (2026-10-30: po +2, tem +1 semitones in "Potem smo šli
                // domov."). Such a word moves like a content word in the
                // middle of a sentence, and as the first accent it rises above
                // the high start as well.
                p = 2.4 + if ai == 0 && sentence_onset { 1.3 } else { 0.0 };
            }
            if ai == 0 && kind == ClauseKind::WhQuestion && !utt.continued {
                p = 5.5;
            }
            if kind == ClauseKind::Exclamation {
                // Emphatic: a high start and a strong last accent to fall from.
                if ai == 0 {
                    p += 1.0;
                }
                if ai + 1 == accents.len() && syl.prominence >= 2 {
                    p = f32::max(p + 1.5, 4.0);
                }
            }
            if question_syl.is_some_and(|q| s > q) {
                p *= 0.4;
            }
            p *= range;

            bumps.push(match (syl.accent, post) {
                // High early in the stressed vowel, falling within it.
                (Accent::Falling, _) => Bump::new([
                    (t0 - 40.0, 0.0),
                    (t0 + 0.25 * d, p),
                    (t1, 0.40 * p),
                    (post.map_or(t1 + 70.0, Syllable::middle), -0.3),
                    (post.map_or(t1 + 110.0, |post| post.t1 + 20.0), 0.0),
                ]),
                // High and nearly level through the stressed vowel, up to a
                // peak at its end; the following syllable starts as high and
                // comes down (the "55.53" of standard Croatian speakers,
                // Pletikos Olof & Bradfield 2019). With the peak inside the
                // following syllable, as until October 2026, that syllable was
                // heard as the stressed one: inteligenCIja, istoVREmeno,
                // uključENo.
                (Accent::Rising, Some(post)) => Bump::new([
                    (t0 - 25.0, 0.0),
                    (t0 + 0.15 * d, 0.45 * p),
                    (t0 + 0.85 * d, p),
                    (post.middle(), 0.7 * p),
                    (post.t1 + 40.0, 0.0),
                ]),
                // Neutral, and a rising accent with no syllable to rise into.
                _ => Bump::new([
                    (t0 - 30.0, 0.0),
                    (t0 + 0.55 * d, p),
                    (t1, 0.85 * p),
                    (post.map_or(t1 + 70.0, Syllable::middle), 0.20 * p),
                    (post.map_or(t1 + 120.0, |post| post.t1 + 30.0), 0.0),
                ]),
            });
        }

        // Boundary movement at the end of the clause.
        if let (Some(final_syl), Some(&last_accent)) = (syls.last(), accents.last()) {
            let nuclear = &syls[last_accent];
            let nuclear_is_last = last_accent + 1 == syls.len();
            // A fall to the bottom of the range right after the last accent,
            // where the voice then stays instead of sliding down all the way
            // to the end.
            let fall_after_nucleus = |depth: f32| {
                let start = if nuclear_is_last { nuclear.t0 + 0.35 * (nuclear.t1 - nuclear.t0) } else { nuclear.t1 };
                let length = (total - start).clamp(60.0, 180.0);
                Bump::new([(start, 0.0), (start + length, -depth * range)])
            };

            if kind == ClauseKind::Continuation {
                // A comma: a smaller fall after the last accent and a slight
                // rise on the last syllable, synthesizer C's "weaker fall
                // followed by a slight continuation rise". Croatian Radio
                // readers raise the last word before a pause inside a sentence
                // by 2.4 to 3.2 semitones (Langston 2018). Until October 2026
                // the rise was 4 semitones from the level, which sounded like
                // a question at every comma.
                // Before a short pause inside the clause the melody is only
                // suspended: under half of a comma's movement, or the hours of
                // "u dvanaest trideset" would end a phrase of their own.
                let strength = if utt.minor_break { MINOR_BREAK_MOVEMENT } else { 1.0 };
                if !nuclear_is_last {
                    bumps.push(fall_after_nucleus(1.5 * strength));
                }
                let start = if nuclear_is_last {
                    nuclear.middle()
                } else if final_syl.timed() {
                    final_syl.t0 - 20.0
                } else {
                    total - 120.0
                };
                bumps.push(Bump::new([(start, 0.0), (total, 2.5 * range * strength)]));
            } else if question {
                // Every question ends going up. Where the main movement lies
                // earlier (on the word before "li", on a question word), the
                // last syllable rises on its own; in a wh-question it comes
                // back up from the fall that follows the last accent.
                let rise_at_end = question_syl.is_some_and(|q| q as i32 >= final_index - 1);
                if !rise_at_end {
                    let mut lift = 3.0;
                    if kind == ClauseKind::WhQuestion {
                        bumps.push(fall_after_nucleus(3.5));
                        lift += 3.5;
                    }
                    lift *= range * brevity;
                    let (rise_from, top_at) = if final_syl.timed() {
                        (final_syl.t0 - 30.0, final_syl.t0 + 0.75 * (final_syl.t1 - final_syl.t0))
                    } else {
                        (total - 150.0, total)
                    };
                    bumps.push(Bump::new([(rise_from, 0.0), (top_at, lift), (total, lift)]));
                }
            } else {
                bumps.push(fall_after_nucleus(if kind == ClauseKind::Exclamation { 5.5 } else { 4.5 }));
            }
        }

        // A sentence starts high: the stretch before its first accent is
        // raised and comes down into the accent's peak (voice D and the
        // natural voices start a sentence three to four semitones above the
        // level they then keep).
        if sentence_onset {
            let first = &syls[accents[0]];
            bumps.push(Bump::new([
                (-1.0, 1.5 * range),
                (first.t0 - 40.0, 1.3 * range),
                (first.t0 + 0.25 * (first.t1 - first.t0), 0.0),
            ]));
        }

        // Declination: the level drifts down only a little along the clause,
        // 1.3 to 2 semitones in all, however long it is; the fall that ends a
        // sentence belongs to its last word. (A baseline that fell two
        // semitones per second, up to six, as before October 2026, took the
        // second half of a long sentence down with it, where read speech
        // keeps it level.)
        Self { bumps, total, drop: (total * 0.0006).clamp(1.3, 2.0) }
    }

    /// Pitch at `t_ms` from the start of the clause's sound, in semitones.
    pub fn at(&self, t_ms: f32) -> f32 {
        let x = (t_ms / self.total).clamp(0.0, 1.0);
        self.bumps.iter().fold(self.drop * (0.37 - x), |value, bump| value + bump.at(t_ms))
    }
}
