//! Phone inventory and acoustic targets (Croatian, Serbian, Bosnian,
//! Slovenian).
//!
//! Vowel formants follow the measurements of adult male speakers of standard
//! Croatian (Bakran & Stamenković 1990; Bakran 1996, "Zvučna slika hrvatskoga
//! govora"), placed between the running-speech averages and the more
//! peripheral values that listeners judge as standard in synthesis. Inherent
//! durations follow Bakran (1984). Consonant loci, noise spectra and source
//! amplitudes follow the cascade/parallel synthesis strategy of Klatt (1980),
//! adapted to the BCS inventory: unaspirated voiceless stops, fully voiced
//! stops, dental /t d/, the č/ć and dž/đ contrasts, tapped /r/ and the
//! labiodental /v/ (weakly fricated).
//!
//! Change a number here only with a measurement or a listening test behind
//! it; docs/formant.md records the sources.

use super::klatt::{NOISE_PEAKS, PARALLEL_FORMANTS};

/// Phones of standard Croatian/Serbian/Bosnian, including the allophones the
/// synthesizer produces on its own ([ŋ], [ɕ], [ʑ], [ɣ], [dz], schwa), and the
/// sounds Slovenian adds to them: the open e and o, the schwa as a vowel of
/// its own (pes, veter) and the [w] that l and v become before a consonant
/// and at the end of a word (bil, vse).
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq, Hash, PartialOrd, Ord)]
#[repr(u8)]
pub enum Ph {
    #[default]
    Sil,
    A,
    E,
    I,
    O,
    U,
    /// Vocalic element of syllabic r, letter names.
    Schwa,
    /// Slovenian open e [ɛ] (unstressed e, short stressed è, long ê).
    Eh,
    /// Slovenian open o [ɔ] (unstressed o, short stressed ò, long ô).
    Oh,
    /// Slovenian close e [eː] (long stressed é only).
    Ee,
    /// Slovenian close o [oː] (long stressed ó only).
    Oo,
    /// Slovenian schwa [ə] as a syllable of its own (pes, sem, veter).
    Ax,
    /// Slovenian [w] ([u̯]): l and v before a consonant or at a word end.
    W,
    J,
    /// Labiodental approximant [ʋ].
    V,
    L,
    /// lj [ʎ].
    Lj,
    M,
    N,
    /// nj [ɲ].
    Nj,
    /// n before k, g [ŋ].
    Ng,
    /// Alveolar tap (single contact of the trill).
    R,
    P,
    B,
    T,
    D,
    K,
    G,
    /// c [ts].
    C,
    /// c before a voiced obstruent [dz].
    Dz,
    /// č.
    Ch,
    /// dž.
    Dzh,
    /// ć.
    Tj,
    /// đ.
    Dj,
    F,
    S,
    Z,
    /// š.
    Sh,
    /// ž.
    Zh,
    /// š before ć [ɕ].
    Sj,
    /// ž before đ [ʑ].
    Zj,
    /// h [x].
    H,
    /// h before a voiced obstruent [ɣ].
    Gh,
}

impl Ph {
    pub const COUNT: usize = Ph::Gh as usize + 1;

    /// Acoustic definition of the phone.
    pub fn def(self) -> &'static PhDef {
        &TABLE[self as usize]
    }

    pub fn class(self) -> PhClass {
        self.def().cls
    }

    pub fn is_vowel(self) -> bool {
        self.class() == PhClass::Vowel
    }

    /// Obstruents take part in voicing assimilation; sonorants and /v/ do not.
    pub fn is_obstruent(self) -> bool {
        matches!(self.class(), PhClass::Stop | PhClass::Affricate | PhClass::Fricative)
    }

    pub fn is_consonant(self) -> bool {
        !matches!(self.class(), PhClass::Vowel | PhClass::Silence)
    }
}

#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub enum PhClass {
    Silence,
    Vowel,
    Glide,
    Liquid,
    Nasal,
    Tap,
    Stop,
    Affricate,
    Fricative,
}

/// Word accent type. `Neutral` is used when only the stress position is known.
#[derive(Clone, Copy, Debug, Default, PartialEq, Eq)]
pub enum Accent {
    #[default]
    None,
    Neutral,
    Falling,
    Rising,
}

/// Spectrum of frication noise: shares that follow the formants F2-F4, up to
/// three peaks at fixed frequencies, and a flat share.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct NoiseSpec {
    pub fa: [f32; PARALLEL_FORMANTS],
    pub f: [f32; NOISE_PEAKS],
    pub b: [f32; NOISE_PEAKS],
    pub a: [f32; NOISE_PEAKS],
    pub bypass: f32,
}

/// Acoustic definition of one phone.
#[derive(Clone, Copy, Debug, PartialEq)]
pub struct PhDef {
    pub cls: PhClass,
    pub voiced: bool,
    /// Steady-state formants (vowels, sonorants, nasal murmur).
    pub f: [f32; 3],
    /// Formant locus that neighbouring sounds are pulled towards.
    pub loc: [f32; 3],
    /// How far the boundary value follows the neighbour (0 = fixed locus,
    /// 1 = the neighbour's own value).
    pub coart: [f32; 3],
    pub bw: [f32; 3],
    pub f4: f32,
    /// Voicing level.
    pub av: f32,
    /// Aspiration level.
    pub ah: f32,
    /// Frication level.
    pub af: f32,
    /// Extra source tilt (dB), e.g. the muffled /v/.
    pub tilt: f32,
    /// Inherent duration (ms).
    pub dur: f32,
    /// Floor at very fast rates (ms).
    pub min_dur: f32,
    /// Transition time imposed on neighbours (ms).
    pub tr_out: f32,
    /// Dominance in transitions.
    pub rank: i32,
    /// Colouring of the phone's own F2/F3 target by the context vowel.
    pub tgt_coart: f32,
    pub noise: NoiseSpec,
}

const BASE: PhDef = PhDef {
    cls: PhClass::Silence,
    voiced: false,
    f: [490.0, 1340.0, 2350.0],
    loc: [490.0, 1340.0, 2350.0],
    coart: [1.0; 3],
    bw: [70.0, 100.0, 160.0],
    f4: 3400.0,
    av: 0.0,
    ah: 0.0,
    af: 0.0,
    tilt: 0.0,
    dur: 60.0,
    min_dur: 25.0,
    tr_out: 40.0,
    rank: 0,
    tgt_coart: 0.0,
    noise: NoiseSpec { fa: [0.0; 3], f: [2500.0, 4000.0, 6000.0], b: [500.0, 800.0, 1200.0], a: [0.0; 3], bypass: 0.0 },
};

/// F1 F2 F3 F4, B1 B2 B3, level, inherent duration.
const fn vowel(f: [f32; 4], bw: [f32; 3], av: f32, dur: f32) -> PhDef {
    let f3 = [f[0], f[1], f[2]];
    PhDef {
        cls: PhClass::Vowel,
        voiced: true,
        f: f3,
        loc: f3,
        coart: [0.5; 3],
        bw,
        f4: f[3],
        av,
        dur,
        min_dur: 30.0,
        tr_out: 40.0,
        rank: 1,
        ..BASE
    }
}

#[allow(clippy::too_many_arguments)]
const fn sonorant(
    cls: PhClass,
    f: [f32; 3],
    bw: [f32; 3],
    av: f32,
    dur: f32,
    coart: f32,
    tr_out: f32,
    rank: i32,
    tgt_coart: f32,
) -> PhDef {
    PhDef {
        cls,
        voiced: true,
        f,
        loc: f,
        coart: [coart; 3],
        bw,
        av,
        dur,
        min_dur: 24.0,
        tr_out,
        rank,
        tgt_coart,
        ..BASE
    }
}

/// Murmur F2 F3, oral locus F2 (coart), F3 (coart), duration, transition.
#[allow(clippy::too_many_arguments)]
const fn nasal(m2: f32, m3: f32, l2: f32, c2: f32, l3: f32, c3: f32, dur: f32, tr_out: f32) -> PhDef {
    PhDef {
        cls: PhClass::Nasal,
        voiced: true,
        // Murmur: low nasal resonance, weak upper formants with wide bandwidths.
        f: [480.0, m2, m3],
        bw: [50.0, 280.0, 300.0],
        // Oral place of articulation as seen by neighbouring vowels.
        loc: [280.0, l2, l3],
        // F1 of a neighbouring vowel hardly gives way before the closure: it
        // jumps to the murmur's (in synthesizer A and synthesizer B within 10 ms).
        coart: [0.70, c2, c3],
        av: 1.0,
        dur,
        min_dur: 28.0,
        tr_out,
        rank: 6,
        ..BASE
    }
}

/// Loci F1 F2 F3, coarticulation, duration, transition, rank.
const fn obstruent(
    cls: PhClass,
    voiced: bool,
    loc: [f32; 3],
    coart: [f32; 3],
    dur: f32,
    tr_out: f32,
    rank: i32,
) -> PhDef {
    PhDef {
        cls,
        voiced,
        loc,
        f: loc,
        coart,
        bw: [if voiced { 70.0 } else { 120.0 }, 130.0, 200.0],
        dur,
        min_dur: 30.0,
        tr_out,
        rank,
        ..BASE
    }
}

/// Frication level, noise shares at the moving formants F2-F4, the fixed
/// peaks (frequency, bandwidth, amplitude) and the flat share.
const fn with_noise(mut d: PhDef, af: f32, fa: [f32; 3], peaks: [[f32; 3]; 3], bypass: f32) -> PhDef {
    d.af = af;
    d.noise = NoiseSpec {
        fa,
        f: [peaks[0][0], peaks[1][0], peaks[2][0]],
        b: [peaks[0][1], peaks[1][1], peaks[2][1]],
        a: [peaks[0][2], peaks[1][2], peaks[2][2]],
        bypass,
    };
    d
}

static TABLE: [PhDef; Ph::COUNT] = table();

const fn table() -> [PhDef; Ph::COUNT] {
    use PhClass::{Affricate, Fricative, Glide, Liquid, Stop, Tap};
    let mut t = [BASE; Ph::COUNT];

    t[Ph::Sil as usize] = PhDef { dur: 0.0, min_dur: 0.0, ..BASE };

    // ---- Vowels ----
    // Bandwidths are wider than Klatt's defaults: measured on synthesizer A's
    // vowels they are 100-140 Hz, and the narrow ones rang (see "Voice
    // colour" in docs/formant.md).
    t[Ph::A as usize] = vowel([710.0, 1230.0, 2500.0, 3600.0], [105.0, 110.0, 140.0], 1.00, 80.0);
    t[Ph::E as usize] = vowel([460.0, 1820.0, 2480.0, 3550.0], [90.0, 110.0, 150.0], 0.92, 74.0);
    t[Ph::I as usize] = vowel([290.0, 2150.0, 2700.0, 3300.0], [75.0, 115.0, 180.0], 0.80, 66.0);
    t[Ph::O as usize] = vowel([480.0, 900.0, 2550.0, 3500.0], [95.0, 105.0, 150.0], 0.92, 74.0);
    t[Ph::U as usize] = vowel([320.0, 760.0, 2450.0, 3200.0], [80.0, 100.0, 150.0], 0.80, 66.0);
    // Schwa: the vocalic part of syllabic r (measured as a full, rather open
    // vowel in "prst") and of spelled-out consonant clusters. It takes little
    // colour from the vowels around it: F2 is 1300-1400 Hz in the recorded
    // "prvi", "brzina", not the 1600 Hz a stronger pull towards the i gave
    // (which made the word sound like "pervi", "tervi").
    t[Ph::Schwa as usize] = PhDef {
        tgt_coart: 0.10,
        min_dur: 12.0,
        ..vowel([500.0, 1380.0, 2600.0, 3600.0], [95.0, 115.0, 150.0], 0.80, 40.0)
    };

    // ---- Slovenian ----
    // The eight vowels of standard Slovenian: the close e and o are always
    // long and stressed, the open ones stand for every unstressed and short
    // stressed e and o (Slovenski pravopis 2001, § 618). Targets are the means
    // of Tivadar's trained male speakers (Slavistična revija 52/1, 2004,
    // table 4: i 351/2218, e 397/2169, ɛ 526/1821, a 603/1324, ɔ 551/1000,
    // o 397/872, u 385/754 Hz), the open vowels moved a little towards
    // Toporišič's (1978) more peripheral ɛ 585/1849 and ɔ 565/993, F3 from
    // Toporišič; see "Slovenian" in docs/formant.md.
    t[Ph::Eh as usize] = vowel([560.0, 1830.0, 2600.0, 3600.0], [100.0, 115.0, 150.0], 0.96, 74.0);
    t[Ph::Oh as usize] = vowel([560.0, 1000.0, 2580.0, 3400.0], [100.0, 110.0, 150.0], 0.96, 74.0);
    t[Ph::Ee as usize] = vowel([400.0, 2120.0, 2760.0, 3550.0], [85.0, 110.0, 160.0], 0.88, 76.0);
    t[Ph::Oo as usize] = vowel([400.0, 860.0, 2550.0, 3300.0], [90.0, 105.0, 150.0], 0.88, 76.0);
    // The schwa as a syllable of its own (pes, sem, veter, the ər of prst): a
    // short mid-central vowel (Toporišič 539/1376/2487, Petek, Šuštaršič and
    // Komar 1996: 456/1370/2121), always short, stressed or not (Slovenian has
    // no long schwa), coloured a little by its neighbours.
    t[Ph::Ax as usize] = PhDef {
        tgt_coart: 0.20,
        min_dur: 20.0,
        ..vowel([500.0, 1380.0, 2500.0, 3450.0], [100.0, 120.0, 160.0], 0.86, 52.0)
    };

    // ---- Glides and liquids ----
    // /j/ is a true constriction, not a weak /i/: F1 low (250-300 Hz in
    // synthesizer A and in the recordings at a word start), F2 about 2000 Hz
    // between two a's, and above all F3 well above the vowels' (2900-3400 Hz
    // in the recorded "ja", "ji", "moj"; 3000 Hz in synthesizer A's "ieri").
    // The high F3 and the low F1 are what tell /j/ from /i/ in "ji", "ija",
    // "moj"; with F3 at 2750 the two were the same sound. Transitions are
    // quicker than a diphthong's.
    t[Ph::J as usize] = PhDef {
        f4: 3600.0,
        ..sonorant(Glide, [270.0, 2150.0, 3050.0], [60.0, 110.0, 180.0], 0.88, 58.0, 0.20, 48.0, 3, 0.18)
    };
    // /v/ is labiodental: a weak, dull murmur with some friction, as in the
    // classic synthesizers (synthesizer A's v carries flat noise up to 5 kHz,
    // synthesizer C's is a voiced fricative). Without the friction, and with
    // F1-F3 where an l has them (300 / 1130 / 2400 Hz), a phoneme recognizer
    // heard it as l in nearly every word ("voli" as "loli") and listeners
    // could not tell V from L; the lips also lower F3 (synthesizer C 2080 Hz)
    // where an l keeps it high. Its voicing is weak: a louder murmur with this
    // low F1 is heard as m. See "v, l, nj and d" in docs/formant.md.
    t[Ph::V as usize] = with_noise(
        PhDef {
            tilt: 12.0,
            ..sonorant(Glide, [250.0, 1050.0, 2100.0], [70.0, 110.0, 170.0], 0.30, 52.0, 0.30, 40.0, 3, 0.20)
        },
        0.12,
        [0.3, 0.4, 0.5],
        [[2500.0, 2500.0, 0.7], [4500.0, 3000.0, 1.0], [7000.0, 3000.0, 0.35]],
        0.10,
    );
    // Slovenian [w] (the u̯ of bil, vse, nov, polno): a labiovelar glide, a
    // brief close back vocoid with F2 well below the u's (Šuštaršič, Komar and
    // Petek, JIPA 1995, describe it as a bilabial approximant), fully voiced
    // and without the friction of v. Shorter than a vowel, and it imposes its
    // low F2 on the vowel next to it the way u does.
    t[Ph::W as usize] = PhDef {
        f4: 3300.0,
        ..sonorant(Glide, [300.0, 720.0, 2250.0], [70.0, 110.0, 170.0], 0.82, 56.0, 0.25, 50.0, 3, 0.10)
    };
    // /l/: clearly louder than /v/, visible F2 and F3. It is a dark l: F2
    // stays low whatever the vowels around it (recorded 1150-1300 Hz next to e
    // and i, 1000-1100 Hz next to o and u). With more colour from the vowels
    // it reached 1460 Hz next to i, on the way to the clear l of Slovenian
    // (1650-1800 Hz). F1 is the low one of the classic synthesizers
    // (synthesizer A 310, synthesizer C 310, synthesizer B 390-430 Hz), not
    // the recorded speaker's 450 Hz: the deep, quick dip of F1 is what makes
    // an l stand out between vowels, most of all at high rates. F3 and F4 lie
    // close together in laterals and reinforce each other; with them a little
    // higher and F3 sharper (2600 / 3000 Hz, between the recorded speaker's
    // 2500 / 2900 and synthesizer B's and synthesizer C's 2700-2900 Hz) an l
    // next to e and i is no longer heard as n or m.
    t[Ph::L as usize] = PhDef {
        f4: 3000.0,
        ..sonorant(Liquid, [330.0, 1100.0, 2600.0], [60.0, 100.0, 60.0], 0.85, 62.0, 0.25, 40.0, 4, 0.15)
    };
    t[Ph::Lj as usize] = PhDef {
        tilt: 6.0,
        ..sonorant(Liquid, [335.0, 1800.0, 2820.0], [55.0, 120.0, 200.0], 0.55, 70.0, 0.15, 55.0, 4, 0.05)
    };

    // ---- Nasals ----
    // The murmurs of m and n are told apart by their second resonance: near
    // 1050 Hz for m and 1650 Hz for n, as in synthesizer A (1000 / 1700) and
    // synthesizer B (1080 / 1620). The recorded speaker's are closer (1150 /
    // 1400), and with those n was taken for m. The murmur of n is also the
    // duller of the two: a clear one is taken for l. The murmur of nj lies a
    // little above that of n (it was below it, and nj was heard as n), but far
    // below synthesizer A's and synthesizer B's 2100 Hz, which was heard as j;
    // its oral locus is a firm palatal one, so the vowel after it starts high
    // (see also the slow release in the clause builder).
    t[Ph::M as usize] = nasal(1050.0, 2250.0, 900.0, 0.70, 2150.0, 0.65, 60.0, 36.0);
    t[Ph::N as usize] = PhDef { tilt: 3.0, ..nasal(1650.0, 2600.0, 1750.0, 0.40, 2750.0, 0.40, 54.0, 45.0) };
    t[Ph::Nj as usize] = nasal(1750.0, 2600.0, 2400.0, 0.10, 2850.0, 0.30, 68.0, 55.0);
    t[Ph::Ng as usize] = nasal(1150.0, 2300.0, 1700.0, 0.45, 2100.0, 0.45, 52.0, 55.0);

    // ---- Tap /r/ ----
    // The values are those of the vocalic stretch around the contacts (F1
    // about 430, F2 about 1350 Hz; F3 drops some 300 Hz below the vowel's, to
    // 2250 Hz, in the recordings and in synthesizer A alike). av is the level
    // left during a contact; the noise spectrum is that of the brief transient
    // when the tongue tip leaves the ridge.
    t[Ph::R as usize] = with_noise(
        PhDef {
            av: 0.10,
            min_dur: 16.0,
            bw: [90.0, 120.0, 170.0],
            ..obstruent(Tap, true, [430.0, 1350.0, 2250.0], [0.45, 0.25, 0.30], 50.0, 45.0, 5)
        },
        0.20,
        [0.5, 1.0, 0.6],
        [[3400.0, 1200.0, 0.3], [5000.0, 2000.0, 0.0], [7000.0, 2000.0, 0.0]],
        0.02,
    );

    // ---- Stops ----
    // Bursts excite the formants of the moment, so they move with the
    // transition into the vowel: /p/ weak and falling, /t/ high, /k/ compact
    // with a dominant F2 (plus F3 when the two are pinched together before
    // front vowels).
    //
    // The labial burst is a click with a spectrum of its own, the same before
    // every vowel (so it is in synthesizer A and synthesizer B): strongest
    // below 500 Hz, with a flat floor up to 5 kHz about 10 dB under it.
    // Following the formants it could peak where a /t/ or /k/ burst does.
    const LABIAL: [[f32; 3]; 3] = [[350.0, 500.0, 0.8], [4000.0, 3000.0, 0.0], [6000.0, 3000.0, 0.0]];
    let labial = [0.25, 0.70, 0.65];
    t[Ph::P as usize] =
        with_noise(obstruent(Stop, false, [220.0, 850.0, 2100.0], labial, 85.0, 36.0, 8), 0.45, [0.0; 3], LABIAL, 0.20);
    t[Ph::B as usize] =
        with_noise(obstruent(Stop, true, [200.0, 850.0, 2100.0], labial, 65.0, 36.0, 8), 0.45, [0.0; 3], LABIAL, 0.20);

    // The /t/ burst is strongest at 3.5-4.5 kHz (the recorded speaker's final
    // /t/ 3.5-4 kHz, synthesizer A's 4-5 kHz). With F3 as its strongest part
    // it peaked at 2.7 kHz, where the /k/ burst sits next to e and i, and the
    // two were hard to tell apart there. The fixed peak was 4300/900 at full
    // strength until November 2026; the broader, lower one takes 1 dB off the
    // burst and nothing off its place (measured at -20 dB re the vowel, the
    // recording's -19; see "Consonants on a phone" in docs/formant.md).
    // t's af was 0.50 until the match to the reference balance: the burst's
    // loudest 6 ms 17 dB under the vowel against synthesizer A's 22; 0.27
    // gives 23 (the 16 ms of the release -24, synthesizer A -25). p 0.63 ->
    // 0.45 (-24 against -23), k 0.65 -> 0.85 with less breath and the shares
    // below.
    const DENTAL: [[f32; 3]; 3] = [[4000.0, 1600.0, 0.85], [6200.0, 2000.0, 0.15], [8000.0, 2500.0, 0.0]];
    let dental = [0.25, 0.40, 0.40];
    t[Ph::T as usize] = with_noise(
        obstruent(Stop, false, [220.0, 1750.0, 2800.0], dental, 76.0, 45.0, 8),
        0.27,
        [0.2, 0.3, 1.0],
        DENTAL,
        0.05,
    );
    t[Ph::D as usize] = with_noise(
        obstruent(Stop, true, [200.0, 1750.0, 2800.0], dental, 56.0, 45.0, 8),
        0.27,
        [0.2, 0.3, 1.0],
        DENTAL,
        0.05,
    );

    const VELAR: [[f32; 3]; 3] = [[1800.0, 350.0, 0.0], [3600.0, 1000.0, 0.0], [5500.0, 2000.0, 0.08]];
    let velar = [0.25, 0.45, 0.45];
    t[Ph::K as usize] = with_noise(
        obstruent(Stop, false, [240.0, 1700.0, 2100.0], velar, 81.0, 60.0, 8),
        0.85,
        [1.0, 0.25, 0.15],
        VELAR,
        0.01,
    );
    t[Ph::G as usize] = with_noise(
        obstruent(Stop, true, [220.0, 1700.0, 2100.0], velar, 62.0, 60.0, 8),
        0.85,
        [1.0, 0.25, 0.15],
        VELAR,
        0.01,
    );

    // ---- Fricatives ----
    // Sibilants are softer than in natural speech on purpose: their levels are
    // those of the classic 10-11 kHz formant synthesizers (s -14.5 dB re the
    // loudest vowel frame, š -17, c -14.7, č -16.7, ć -15.1;
    // tools/formant/sibilants.py on asa, aša, aca, ača, aća), 10 dB and more
    // under recorded Croatian speech. Their place is the recorded one since
    // 10 November 2026: matched to synthesizer A's centroids under the 5.3 kHz
    // ceiling, s, z and c had four fifths of their energy between 3.8 and
    // 5.0 kHz, where the ear is most sensitive, and were heard as a slight š,
    // ž and č. The friction of s now lies between 4.6 and 6.9 kHz with its
    // middle at 5.8 (a recorded Croatian speaker 4.4-8.2 and 5.2, synthesizer
    // B 3.9-7.8 and 5.3), that of š between 2.6 and 4.8 kHz with its middle at
    // 3.3 (recorded 2.7-5.3 and 3.0), and the palatals have the recorded tail
    // above 4.5 kHz instead of a wall. Weighted by the ear's sensitivity the s
    // is 3 dB softer than before at the same level. č and ć, the two loudest
    // sounds of the voice in the ear's band, are 1.5 and 2 dB down. See
    // "Sibilants" and "The hiss where the ear is sharpest" in docs/formant.md.
    //
    // f was 0.13 until the match to the reference balance: -21.5 dB re the
    // vowel against synthesizer A's -29.5; 0.08 gives -26.4, kept a little
    // stronger for Croatian, where f carries hvala, kafa, telefon.
    t[Ph::F as usize] = with_noise(
        obstruent(Fricative, false, [290.0, 1000.0, 2250.0], [0.40, 0.75, 0.70], 88.0, 34.0, 7),
        0.08,
        [0.3, 0.4, 0.5],
        [[2500.0, 2500.0, 0.7], [4500.0, 3000.0, 1.0], [7000.0, 3000.0, 0.35]],
        0.10,
    );

    const S_NOISE: [[f32; 3]; 3] = [[4500.0, 1100.0, 0.55], [5500.0, 1500.0, 1.0], [7000.0, 2000.0, 0.55]];
    let s = [0.40, 0.45, 0.50];
    t[Ph::S as usize] = with_noise(
        obstruent(Fricative, false, [290.0, 1650.0, 2650.0], s, 95.0, 44.0, 7),
        0.38,
        [0.0; 3],
        S_NOISE,
        0.0,
    );
    t[Ph::Z as usize] =
        with_noise(obstruent(Fricative, true, [270.0, 1650.0, 2650.0], s, 70.0, 44.0, 7), 0.38, [0.0; 3], S_NOISE, 0.0);

    // BCS š/ž are "hard" (flat postalveolar), lower-pitched than English sh.
    const SH_NOISE: [[f32; 3]; 3] = [[2550.0, 600.0, 0.75], [3300.0, 900.0, 1.0], [4700.0, 1600.0, 0.40]];
    let sh = [0.40, 0.35, 0.40];
    t[Ph::Sh as usize] = with_noise(
        obstruent(Fricative, false, [290.0, 1800.0, 2400.0], sh, 100.0, 50.0, 7),
        0.31,
        [0.0; 3],
        SH_NOISE,
        0.0,
    );
    t[Ph::Zh as usize] = with_noise(
        obstruent(Fricative, true, [270.0, 1800.0, 2400.0], sh, 72.0, 50.0, 7),
        0.31,
        [0.0; 3],
        SH_NOISE,
        0.0,
    );

    // Alveolo-palatal [ɕ ʑ]: the friction of ć/đ, and š/ž before them.
    const SJ_NOISE: [[f32; 3]; 3] = [[3100.0, 650.0, 0.50], [3900.0, 950.0, 1.0], [5400.0, 1900.0, 0.50]];
    let sj = [0.40, 0.25, 0.30];
    t[Ph::Sj as usize] = with_noise(
        obstruent(Fricative, false, [270.0, 2150.0, 2850.0], sj, 95.0, 55.0, 7),
        0.40,
        [0.0; 3],
        SJ_NOISE,
        0.0,
    );
    t[Ph::Zj as usize] = with_noise(
        obstruent(Fricative, true, [260.0, 2150.0, 2850.0], sj, 70.0, 55.0, 7),
        0.40,
        [0.0; 3],
        SJ_NOISE,
        0.0,
    );

    // Velar fricative: aspiration and friction shaped by the neighbouring
    // vowel's formants. h was af 0.06, ah 0.30 until the match to the
    // reference balance: -14 dB re the vowel against synthesizer A's -20 to
    // -22; now -20.
    const H_NOISE: [[f32; 3]; 3] = [[1500.0, 450.0, 0.0], [3500.0, 1200.0, 0.0], [4800.0, 1500.0, 0.3]];
    let h = [0.90, 0.92, 0.92];
    let h_bw = [160.0, 160.0, 240.0];
    t[Ph::H as usize] = with_noise(
        PhDef { ah: 0.16, bw: h_bw, ..obstruent(Fricative, false, [400.0, 1400.0, 2400.0], h, 72.0, 30.0, 2) },
        0.032,
        [1.0, 0.6, 0.5],
        H_NOISE,
        0.01,
    );
    t[Ph::Gh as usize] = with_noise(
        PhDef { ah: 0.16, bw: h_bw, ..obstruent(Fricative, true, [380.0, 1400.0, 2400.0], h, 60.0, 30.0, 2) },
        0.032,
        [1.0, 0.6, 0.5],
        H_NOISE,
        0.01,
    );

    // ---- Affricates (closure + friction) ----
    // c has the place of s, a touch lower (a listening test found an earlier,
    // sharper c too sharp).
    const C_NOISE: [[f32; 3]; 3] = [[4400.0, 1100.0, 0.55], [5400.0, 1500.0, 1.0], [6900.0, 2000.0, 0.50]];
    let c = [0.30, 0.45, 0.50];
    t[Ph::C as usize] = with_noise(
        obstruent(Affricate, false, [250.0, 1650.0, 2650.0], c, 110.0, 44.0, 8),
        0.34,
        [0.0; 3],
        C_NOISE,
        0.0,
    );
    t[Ph::Dz as usize] =
        with_noise(obstruent(Affricate, true, [230.0, 1650.0, 2650.0], c, 80.0, 44.0, 8), 0.34, [0.0; 3], C_NOISE, 0.0);

    // The friction of č/dž and ć/đ sits about 200 Hz below that of š and [ɕ]:
    // measured on recordings the affricates' centroids are lower than the
    // fricatives', and the listening test asked for it.
    const CH_NOISE: [[f32; 3]; 3] = [[2500.0, 600.0, 0.70], [3250.0, 900.0, 1.0], [4650.0, 1600.0, 0.40]];
    let ch = [0.30, 0.35, 0.40];
    t[Ph::Ch as usize] = with_noise(
        obstruent(Affricate, false, [250.0, 1800.0, 2400.0], ch, 108.0, 50.0, 8),
        0.40,
        [0.0; 3],
        CH_NOISE,
        0.0,
    );
    t[Ph::Dzh as usize] = with_noise(
        obstruent(Affricate, true, [230.0, 1800.0, 2400.0], ch, 84.0, 50.0, 8),
        0.40,
        [0.0; 3],
        CH_NOISE,
        0.0,
    );

    const TJ_NOISE: [[f32; 3]; 3] = [[2900.0, 650.0, 0.50], [3700.0, 950.0, 1.0], [5200.0, 1900.0, 0.50]];
    let tj = [0.30, 0.25, 0.30];
    t[Ph::Tj as usize] = with_noise(
        obstruent(Affricate, false, [240.0, 2150.0, 2850.0], tj, 108.0, 55.0, 8),
        0.40,
        [0.0; 3],
        TJ_NOISE,
        0.0,
    );
    t[Ph::Dj as usize] = with_noise(
        obstruent(Affricate, true, [220.0, 2150.0, 2850.0], tj, 88.0, 55.0, 8),
        0.40,
        [0.0; 3],
        TJ_NOISE,
        0.0,
    );

    t
}

/// One phone of an utterance, as produced by the text front end.
#[derive(Clone, Copy, Debug, PartialEq, Eq)]
pub struct Phone {
    pub ph: Ph,
    /// Carries a syllable (vowel, first part of syllabic r).
    pub nucleus: bool,
    /// Rest of a syllabic r (tap and second vocoid).
    pub nucleus_tail: bool,
    pub stressed: bool,
    pub is_long: bool,
    /// First half of the "ije" diphthong.
    pub short_glide: bool,
    pub word_start: bool,
    /// The whole word is this one unstressed vowel (i, a, u, o).
    pub vowel_word: bool,
    /// A short pause stands before this word (`split_at_breaks()`).
    pub break_before: bool,
    /// ... and it is the longer one between two items (`ITEM_BREAK_MARK`).
    pub item_break: bool,
    pub accent: Accent,
    /// 0 clitic, 1 function word, 2 content word.
    pub prominence: u8,
    /// Word index within the clause.
    pub word: u16,
    /// Syllable index within the clause, -1 for none.
    pub syllable: i16,
    /// Syllable count of the containing word.
    pub word_syllables: u8,
}

impl Default for Phone {
    fn default() -> Self {
        Self {
            ph: Ph::Sil,
            nucleus: false,
            nucleus_tail: false,
            stressed: false,
            is_long: false,
            short_glide: false,
            word_start: false,
            vowel_word: false,
            break_before: false,
            item_break: false,
            accent: Accent::None,
            prominence: 0,
            word: 0,
            syllable: -1,
            word_syllables: 1,
        }
    }
}

/// A word boundary whose open sounds need a brief separation cue.
pub fn needs_vowel_separation(left: &Phone, right: &Phone) -> bool {
    if !right.word_start || left.word == right.word {
        return false;
    }
    let full_vowel = |ph: Ph| ph.is_vowel() && ph != Ph::Schwa;
    if full_vowel(left.ph) && full_vowel(right.ph) {
        return true;
    }
    // Keep the cue next to /j/ for one-vowel words: "ja i ona", "njoj i".
    (left.vowel_word || right.vowel_word)
        && (full_vowel(left.ph) || left.ph == Ph::J)
        && (full_vowel(right.ph) || right.ph == Ph::J)
}
