// -*- coding: utf-8 -*-
// formant_phonemes.cpp - Acoustic targets for Croatian/Serbian/Bosnian phones
//
// Vowel formants follow the measurements of adult male speakers of standard
// Croatian (Bakran & Stamenković 1990; Bakran 1996, "Zvučna slika hrvatskoga
// govora"), placed between the running-speech averages and the more
// peripheral values that listeners judge as standard in synthesis. Inherent
// durations follow Bakran (1984). Consonant loci, noise spectra and source
// amplitudes follow the cascade/parallel synthesis strategy of Klatt (1980),
// adapted to the BCS inventory: unaspirated voiceless stops, fully voiced
// stops, dental /t d/, the č/ć and dž/đ contrasts, tapped /r/ and the
// approximant /v/.

#include "formant_phonemes.hpp"
#include <initializer_list>

namespace laprdus {
namespace formant {

namespace {

struct Table {
    PhDef defs[static_cast<size_t>(Ph::COUNT)];
    Table();
    PhDef& operator[](Ph ph) { return defs[static_cast<size_t>(ph)]; }
};

void set3(float* dst, float a, float b, float c) {
    dst[0] = a;
    dst[1] = b;
    dst[2] = c;
}

// Fixed-frequency noise peaks (frequency, bandwidth, amplitude) and flat share.
void set_noise(PhDef& d,
               float f1, float b1, float a1,
               float f2, float b2, float a2,
               float f3, float b3, float a3,
               float bypass) {
    d.noise.f[0] = f1; d.noise.b[0] = b1; d.noise.a[0] = a1;
    d.noise.f[1] = f2; d.noise.b[1] = b2; d.noise.a[1] = a2;
    d.noise.f[2] = f3; d.noise.b[2] = b3; d.noise.a[2] = a3;
    d.noise.bypass = bypass;
}

// Noise shares at the moving formants F2, F3, F4.
void set_formant_noise(PhDef& d, float a2, float a3, float a4) {
    d.noise.fa[0] = a2;
    d.noise.fa[1] = a3;
    d.noise.fa[2] = a4;
}

void vowel(PhDef& d, float f1, float f2, float f3, float f4,
           float b1, float b2, float b3, float av, float dur) {
    d.cls = PhClass::Vowel;
    d.voiced = true;
    set3(d.f, f1, f2, f3);
    set3(d.loc, f1, f2, f3);
    set3(d.coart, 0.5f, 0.5f, 0.5f);
    set3(d.bw, b1, b2, b3);
    d.f4 = f4;
    d.av = av;
    d.dur = dur;
    d.min_dur = 30.0f;
    d.tr_out = 40.0f;
    d.rank = 1;
}

void sonorant(PhDef& d, PhClass cls, float f1, float f2, float f3,
              float b1, float b2, float b3, float av, float dur,
              float coart, float tr_out, int rank, float tgt_coart) {
    d.cls = cls;
    d.voiced = true;
    set3(d.f, f1, f2, f3);
    set3(d.loc, f1, f2, f3);
    set3(d.coart, coart, coart, coart);
    set3(d.bw, b1, b2, b3);
    d.av = av;
    d.dur = dur;
    d.min_dur = 24.0f;
    d.tr_out = tr_out;
    d.rank = rank;
    d.tgt_coart = tgt_coart;
}

void nasal(PhDef& d, float m2, float m3, float l2, float c2, float l3, float c3,
           float dur, float tr_out) {
    d.cls = PhClass::Nasal;
    d.voiced = true;
    // Murmur: low nasal resonance, weak upper formants with wide bandwidths.
    set3(d.f, 480.0f, m2, m3);
    set3(d.bw, 50.0f, 280.0f, 300.0f);
    // Oral place of articulation as seen by neighbouring vowels.
    set3(d.loc, 280.0f, l2, l3);
    set3(d.coart, 0.35f, c2, c3);
    d.av = 0.62f;
    d.dur = dur;
    d.min_dur = 28.0f;
    d.tr_out = tr_out;
    d.rank = 6;
}

void obstruent(PhDef& d, PhClass cls, bool voiced,
               float l1, float l2, float l3, float c1, float c2, float c3,
               float dur, float tr_out, int rank) {
    d.cls = cls;
    d.voiced = voiced;
    set3(d.loc, l1, l2, l3);
    set3(d.f, l1, l2, l3);
    set3(d.coart, c1, c2, c3);
    set3(d.bw, voiced ? 70.0f : 120.0f, 130.0f, 200.0f);
    d.dur = dur;
    d.min_dur = 30.0f;
    d.tr_out = tr_out;
    d.rank = rank;
}

Table::Table() {
    Table& t = *this;

    // ---- Silence ----
    t[Ph::SIL].cls = PhClass::Silence;
    t[Ph::SIL].dur = 0.0f;
    t[Ph::SIL].min_dur = 0.0f;

    // ---- Vowels: F1 F2 F3 F4, B1 B2 B3, level, inherent duration ----
    // Bandwidths are wider than Klatt's defaults: measured on Eloquence's
    // vowels they are 100-140 Hz, and the narrow ones rang (see "Voice
    // colour" in docs/formant.md).
    vowel(t[Ph::A], 710.0f, 1230.0f, 2500.0f, 3600.0f, 105.0f, 110.0f, 140.0f, 1.00f, 80.0f);
    vowel(t[Ph::E], 460.0f, 1820.0f, 2480.0f, 3550.0f, 90.0f, 110.0f, 150.0f, 0.92f, 74.0f);
    vowel(t[Ph::I], 290.0f, 2150.0f, 2700.0f, 3300.0f, 75.0f, 115.0f, 180.0f, 0.80f, 66.0f);
    vowel(t[Ph::O], 480.0f, 900.0f, 2550.0f, 3500.0f, 95.0f, 105.0f, 150.0f, 0.92f, 74.0f);
    vowel(t[Ph::U], 320.0f, 760.0f, 2450.0f, 3200.0f, 80.0f, 100.0f, 150.0f, 0.80f, 66.0f);
    // Schwa: the vocalic part of syllabic r (measured as a full, rather
    // open vowel in "prst") and of spelled-out consonant clusters.
    vowel(t[Ph::SCHWA], 500.0f, 1380.0f, 2600.0f, 3600.0f, 95.0f, 115.0f, 150.0f, 0.80f, 40.0f);
    t[Ph::SCHWA].tgt_coart = 0.35f;
    t[Ph::SCHWA].min_dur = 12.0f;

    // ---- Glides and liquids ----
    // /j/ is a true constriction, not a weak /i/: F1 low (250-300 Hz in
    // Eloquence and in the recordings at a word start), F2 about 2000 Hz
    // between two a's, and above all F3 well above the vowels' (2900-3400 Hz
    // in the recorded "ja", "ji", "moj"; 3000 Hz in Eloquence's "ieri"). The
    // high F3 and the low F1 are what tell /j/ from /i/ in "ji", "ija",
    // "moj"; with F3 at 2750 the two were the same sound. Transitions are
    // quicker than a diphthong's.
    sonorant(t[Ph::J], PhClass::Glide, 270.0f, 2150.0f, 3050.0f,
             60.0f, 110.0f, 180.0f, 0.88f, 58.0f, 0.20f, 48.0f, 3, 0.18f);
    t[Ph::J].f4 = 3600.0f;
    // /v/ is an approximant in BCS: no friction, a weak low murmur about
    // 16-20 dB below the vowels with hardly any energy above 500 Hz.
    sonorant(t[Ph::V], PhClass::Glide, 300.0f, 1130.0f, 2400.0f,
             70.0f, 110.0f, 170.0f, 0.50f, 52.0f, 0.30f, 40.0f, 3, 0.20f);
    t[Ph::V].tilt = 10.0f;
    // /l/: clearly louder than /v/, higher F1, visible F2 and F3. Its F2
    // leans on the neighbouring vowels and glides slowly into the next one.
    sonorant(t[Ph::L], PhClass::Liquid, 450.0f, 1160.0f, 2470.0f,
             60.0f, 100.0f, 90.0f, 0.85f, 62.0f, 0.25f, 50.0f, 4, 0.30f);
    // F3 and F4 lie close together in laterals and reinforce each other.
    t[Ph::L].f4 = 2880.0f;
    sonorant(t[Ph::LJ], PhClass::Liquid, 335.0f, 1800.0f, 2820.0f,
             55.0f, 120.0f, 200.0f, 0.55f, 70.0f, 0.15f, 55.0f, 4, 0.05f);
    t[Ph::LJ].tilt = 6.0f;

    // ---- Nasals: murmur F2 F3, oral locus F2 (coart), F3 (coart) ----
    nasal(t[Ph::M], 1150.0f, 2330.0f, 900.0f, 0.70f, 2150.0f, 0.65f, 60.0f, 36.0f);
    nasal(t[Ph::N], 1350.0f, 2400.0f, 1750.0f, 0.40f, 2750.0f, 0.40f, 54.0f, 45.0f);
    nasal(t[Ph::NJ], 1500.0f, 2450.0f, 2250.0f, 0.20f, 2850.0f, 0.30f, 68.0f, 55.0f);
    nasal(t[Ph::NG], 1150.0f, 2300.0f, 1700.0f, 0.45f, 2100.0f, 0.45f, 52.0f, 55.0f);

    // ---- Tap /r/ ----
    // The values are those of the vocalic stretch around the contacts (F1
    // about 430, F2 about 1350 Hz; F3 drops some 300 Hz below the vowel's,
    // to 2250 Hz, in the recordings and in Eloquence alike). av is the level
    // left during a contact; the noise spectrum is that of the brief
    // transient when the tongue tip leaves the ridge.
    obstruent(t[Ph::R], PhClass::Tap, true, 430.0f, 1350.0f, 2250.0f,
              0.45f, 0.25f, 0.30f, 50.0f, 45.0f, 5);
    t[Ph::R].av = 0.10f;
    t[Ph::R].af = 0.20f;
    t[Ph::R].min_dur = 16.0f;
    set3(t[Ph::R].bw, 90.0f, 120.0f, 170.0f);
    set_formant_noise(t[Ph::R], 0.5f, 1.0f, 0.6f);
    set_noise(t[Ph::R], 3400.0f, 1200.0f, 0.3f, 5000.0f, 2000.0f, 0.0f,
              7000.0f, 2000.0f, 0.0f, 0.02f);

    // ---- Stops: loci F1 F2 F3, coarticulation, duration, transition ----
    // Bursts excite the formants of the moment, so they move with the
    // transition into the vowel: /p/ weak and falling, dental /t/ diffuse
    // with F3-F5 strongest, /k/ compact with a dominant F2 (plus F3 when the
    // two are pinched together before front vowels).
    obstruent(t[Ph::P], PhClass::Stop, false, 220.0f, 850.0f, 2100.0f,
              0.25f, 0.70f, 0.65f, 85.0f, 36.0f, 8);
    obstruent(t[Ph::B], PhClass::Stop, true, 200.0f, 850.0f, 2100.0f,
              0.25f, 0.70f, 0.65f, 65.0f, 36.0f, 8);
    for (Ph ph : {Ph::P, Ph::B}) {
        t[ph].af = 0.30f;
        set_formant_noise(t[ph], 0.7f, 0.4f, 0.25f);
        set_noise(t[ph], 700.0f, 700.0f, 0.6f, 4000.0f, 3000.0f, 0.0f,
                  6000.0f, 3000.0f, 0.0f, 0.03f);
    }

    obstruent(t[Ph::T], PhClass::Stop, false, 220.0f, 1750.0f, 2800.0f,
              0.25f, 0.40f, 0.40f, 76.0f, 45.0f, 8);
    obstruent(t[Ph::D], PhClass::Stop, true, 200.0f, 1750.0f, 2800.0f,
              0.25f, 0.40f, 0.40f, 56.0f, 45.0f, 8);
    for (Ph ph : {Ph::T, Ph::D}) {
        t[ph].af = 0.50f;
        set_formant_noise(t[ph], 0.6f, 1.0f, 0.9f);
        set_noise(t[ph], 4600.0f, 1300.0f, 0.5f, 6200.0f, 2000.0f, 0.15f,
                  8000.0f, 2500.0f, 0.0f, 0.02f);
    }

    obstruent(t[Ph::K], PhClass::Stop, false, 240.0f, 1700.0f, 2100.0f,
              0.25f, 0.45f, 0.45f, 81.0f, 60.0f, 8);
    obstruent(t[Ph::G], PhClass::Stop, true, 220.0f, 1700.0f, 2100.0f,
              0.25f, 0.45f, 0.45f, 62.0f, 60.0f, 8);
    for (Ph ph : {Ph::K, Ph::G}) {
        t[ph].af = 1.0f;
        set_formant_noise(t[ph], 1.0f, 0.35f, 0.30f);
        set_noise(t[ph], 1800.0f, 350.0f, 0.0f, 3600.0f, 1000.0f, 0.0f,
                  5500.0f, 2000.0f, 0.08f, 0.01f);
    }

    // ---- Fricatives ----
    // Sibilants are softer and lower than in natural speech on purpose: they
    // follow the classic 10-11 kHz formant synthesizers (measured on ETI
    // Eloquence: /s/ centred near 4.3 kHz, /sh/ near 3.1 kHz, both 13-17 dB
    // below the vowels). See "Sibilants" in docs/formant.md.
    obstruent(t[Ph::F], PhClass::Fricative, false, 290.0f, 1000.0f, 2250.0f,
              0.40f, 0.75f, 0.70f, 88.0f, 34.0f, 7);
    t[Ph::F].af = 0.13f;
    set_formant_noise(t[Ph::F], 0.3f, 0.4f, 0.5f);
    set_noise(t[Ph::F], 2500.0f, 2500.0f, 0.7f, 4500.0f, 3000.0f, 1.0f,
              7000.0f, 3000.0f, 0.7f, 0.10f);

    obstruent(t[Ph::S], PhClass::Fricative, false, 290.0f, 1650.0f, 2650.0f,
              0.40f, 0.45f, 0.50f, 95.0f, 44.0f, 7);
    obstruent(t[Ph::Z], PhClass::Fricative, true, 270.0f, 1650.0f, 2650.0f,
              0.40f, 0.45f, 0.50f, 70.0f, 44.0f, 7);
    for (Ph ph : {Ph::S, Ph::Z}) {
        t[ph].af = 0.62f;
        set_noise(t[ph], 4850.0f, 750.0f, 1.0f, 5800.0f, 1000.0f, 0.60f,
                  7000.0f, 1500.0f, 0.15f, 0.0f);
    }

    // BCS š/ž are "hard" (flat postalveolar), lower-pitched than English sh.
    obstruent(t[Ph::SH], PhClass::Fricative, false, 290.0f, 1800.0f, 2400.0f,
              0.40f, 0.35f, 0.40f, 100.0f, 50.0f, 7);
    obstruent(t[Ph::ZH], PhClass::Fricative, true, 270.0f, 1800.0f, 2400.0f,
              0.40f, 0.35f, 0.40f, 72.0f, 50.0f, 7);
    for (Ph ph : {Ph::SH, Ph::ZH}) {
        t[ph].af = 0.60f;
        set_noise(t[ph], 2700.0f, 450.0f, 0.75f, 3650.0f, 750.0f, 1.0f,
                  4800.0f, 1000.0f, 0.35f, 0.0f);
    }

    // Alveolo-palatal [ɕ ʑ]: the friction of ć/đ, and š/ž before them.
    obstruent(t[Ph::SJ], PhClass::Fricative, false, 270.0f, 2150.0f, 2850.0f,
              0.40f, 0.25f, 0.30f, 95.0f, 55.0f, 7);
    obstruent(t[Ph::ZJ], PhClass::Fricative, true, 260.0f, 2150.0f, 2850.0f,
              0.40f, 0.25f, 0.30f, 70.0f, 55.0f, 7);
    for (Ph ph : {Ph::SJ, Ph::ZJ}) {
        t[ph].af = 0.60f;
        set_noise(t[ph], 3200.0f, 450.0f, 0.50f, 4050.0f, 750.0f, 1.0f,
                  5300.0f, 1200.0f, 0.40f, 0.0f);
    }

    // Velar fricative: aspiration and friction shaped by the neighbouring
    // vowel's formants.
    obstruent(t[Ph::H], PhClass::Fricative, false, 400.0f, 1400.0f, 2400.0f,
              0.90f, 0.92f, 0.92f, 72.0f, 30.0f, 2);
    obstruent(t[Ph::GH], PhClass::Fricative, true, 380.0f, 1400.0f, 2400.0f,
              0.90f, 0.92f, 0.92f, 60.0f, 30.0f, 2);
    for (Ph ph : {Ph::H, Ph::GH}) {
        t[ph].af = 0.06f;
        t[ph].ah = 0.30f;
        set3(t[ph].bw, 160.0f, 160.0f, 240.0f);
        set_formant_noise(t[ph], 1.0f, 0.6f, 0.5f);
        set_noise(t[ph], 1500.0f, 450.0f, 0.0f, 3500.0f, 1200.0f, 0.0f,
                  4800.0f, 1500.0f, 0.3f, 0.01f);
    }

    // ---- Affricates (closure + friction) ----
    obstruent(t[Ph::C], PhClass::Affricate, false, 250.0f, 1650.0f, 2650.0f,
              0.30f, 0.45f, 0.50f, 110.0f, 44.0f, 8);
    obstruent(t[Ph::DZ], PhClass::Affricate, true, 230.0f, 1650.0f, 2650.0f,
              0.30f, 0.45f, 0.50f, 80.0f, 44.0f, 8);
    // c is a little lower and softer than s (Eloquence's [ts] sits at
    // 4.4 kHz, 16 dB down; a listening test found ours too sharp).
    for (Ph ph : {Ph::C, Ph::DZ}) {
        t[ph].af = 0.60f;
        set_noise(t[ph], 4500.0f, 750.0f, 1.0f, 5400.0f, 1000.0f, 0.50f,
                  6500.0f, 1500.0f, 0.10f, 0.0f);
    }

    obstruent(t[Ph::CH], PhClass::Affricate, false, 250.0f, 1800.0f, 2400.0f,
              0.30f, 0.35f, 0.40f, 108.0f, 50.0f, 8);
    obstruent(t[Ph::DZH], PhClass::Affricate, true, 230.0f, 1800.0f, 2400.0f,
              0.30f, 0.35f, 0.40f, 84.0f, 50.0f, 8);
    // The friction of č/dž and ć/đ sits about 200 Hz below that of š and
    // [ɕ]: measured on recordings the affricates' centroids are lower than
    // the fricatives', and the listening test asked for it.
    for (Ph ph : {Ph::CH, Ph::DZH}) {
        t[ph].af = 0.70f;
        set_noise(t[ph], 2550.0f, 450.0f, 0.75f, 3450.0f, 750.0f, 1.0f,
                  4550.0f, 1000.0f, 0.35f, 0.0f);
    }

    obstruent(t[Ph::TJ], PhClass::Affricate, false, 240.0f, 2150.0f, 2850.0f,
              0.30f, 0.25f, 0.30f, 108.0f, 55.0f, 8);
    obstruent(t[Ph::DJ], PhClass::Affricate, true, 220.0f, 2150.0f, 2850.0f,
              0.30f, 0.25f, 0.30f, 88.0f, 55.0f, 8);
    for (Ph ph : {Ph::TJ, Ph::DJ}) {
        t[ph].af = 0.70f;
        set_noise(t[ph], 3000.0f, 450.0f, 0.50f, 3850.0f, 750.0f, 1.0f,
                  5050.0f, 1200.0f, 0.40f, 0.0f);
    }
}

} // namespace

const PhDef& ph_def(Ph ph) {
    static Table table;
    size_t index = static_cast<size_t>(ph);
    if (index >= static_cast<size_t>(Ph::COUNT)) {
        index = 0;
    }
    return table.defs[index];
}

} // namespace formant
} // namespace laprdus
