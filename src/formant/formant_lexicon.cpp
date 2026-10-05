// -*- coding: utf-8 -*-
// formant_lexicon.cpp - Built-in accent lexicon for the formant voices
//
// Stress in Croatian/Serbian/Bosnian is lexical. The front end defaults to
// the first syllable (the most common Neo-Štokavian position) and applies
// suffix rules; this lexicon covers frequent words those do not get right,
// plus vowel length and tone for very common words.
//
// Notation:
//   '  before the stressed vowel (or syllabic r); tone chosen automatically
//   ^  before the stressed vowel: falling accent
//   /  before the stressed vowel: rising accent
//   :  after a long vowel
//   *  at the end: stem, also matches forms with up to three more letters

#include "formant_frontend.hpp"

namespace laprdus {
namespace formant {

namespace {

const char* const COMMON[] = {
    // ---- Numbers ----
    u8"n/ula", u8"j/edan", u8"j/edna", u8"j/edno", u8"d^va:", u8"dv^ije", u8"dv^e:",
    u8"t^ri:", u8"č/etiri", u8"p^e:t", u8"š^e:st", u8"s^edam", u8"^osam",
    u8"d^eve:t", u8"d^ese:t",
    u8"jed'anaest", u8"dv'a:naest", u8"tr'i:naest", u8"čet'rnaest", u8"p'etnaest",
    u8"š'esnaest", u8"sed'amnaest", u8"os'amnaest", u8"dev'etnaest",
    u8"dv'a:deset", u8"tr'i:deset", u8"četrd'ese:t", u8"ped'ese:t", u8"šezd'ese:t",
    u8"sedamd'ese:t", u8"osamd'ese:t", u8"deved'ese:t",
    u8"st^o:", u8"dvj^esto", u8"dvj^esta", u8"dv^esta", u8"tr^isto", u8"tr^ista",
    u8"č/etiristo", u8"p^e:tsto", u8"š^e:ststo", u8"s^edamsto", u8"^osamsto",
    u8"d^evetsto", u8"t^isuć*", u8"h^iljad*", u8"mil'iju:n*", u8"mil'io:n*",
    u8"milij'a:rd*", u8"bil'iju:n*", u8"bil'io:n*", u8"p'rv*", u8"dr^ug*",
    u8"tr^eć*", u8"č/etvrt*", u8"z^arez", u8"c^ije:l*", u8"c^e:l*",

    // ---- Very frequent words: tone and length ----
    u8"d^a:n", u8"d^obar", u8"d^obro", u8"d^obra", u8"hv/a:la", u8"m^oli:m",
    u8"n^o:ć", u8"j^utro", u8"v^eče:r", u8"zdr^avo", u8"b^o:g", u8"d^a", u8"n^e",
    u8"v/oda", u8"v/ode", u8"ž/ena", u8"ž/ene", u8"j/ezik*", u8"k^uć*", u8"gl/a:v*",
    u8"r/u:k*", u8"n/og*", u8"s/estr*", u8"z/emlj*", u8"gr^a:d", u8"s^i:n",
    u8"m^a:jk*", u8"^otac", u8"br^at", u8"lj^u:di", u8"d/ije:te", u8"sv^ije:t",
    u8"r^ije:č", u8"vr/ije:me", u8"l/ije:p*", u8"ml/ije:k*", u8"r/ije:k*",
    u8"b/ije:l*", u8"mj^esto", u8"p^osao", u8"ž/ivot*", u8"r/a:di*", u8"r^a:d",
    u8"zn^a:m", u8"zn^a:š", u8"zn^a:", u8"zn^a:mo", u8"zn^a:te",
    u8"^ima:m", u8"^ima:š", u8"^ima:", u8"n^ema:", u8"n^ema:m",
    u8"s^ada", u8"s^ad", u8"t^ada", u8"k^ada", u8"/o:vdje", u8"/o:vde",
    u8"t/a:mo", u8"/ovamo", u8"d^anas", u8"s^utra", u8"j^uče:r", u8"j^uče:",

    // ---- Words with non-initial stress ----
    u8"doviđ'e:nja", u8"dobrod'oš*", u8"izv'ol*", u8"izv'i:ni*", u8"opr'osti*",
    u8"zahv'a:lj*", u8"gosp'odin*", u8"gosp'odo", u8"međ'utim", u8"več'eras",
    u8"ov'ako", u8"on'ako", u8"kol'iko", u8"tol'iko", u8"ukol'iko", u8"odj'ednom",
    u8"otpr'ilike", u8"zan'i:ma*", u8"zan'imljiv*", u8"izgl'e:da*", u8"post'oj*",
    u8"raz'umije*", u8"raz'ume*", u8"raz'umje*", u8"gov'orio", u8"gov'orila",
    u8"gov'orili", u8"gov'oril*", u8"poč'e:tak", u8"poč'e:tk*", u8"završ'e:tak",
    u8"završ'e:tk*", u8"dod'a:tak", u8"dod'a:tk*", u8"dod'a:tn*", u8"pod'a:tak",
    u8"pod'a:tk*", u8"zad'a:tak", u8"zad'a:tk*", u8"ost'a:tak", u8"ost'a:tk*",
    u8"izuz'e:tak", u8"izuz'e:tk*", u8"tren'u:tak", u8"tren'u:tk*", u8"tren'u:tn*",
    u8"slob'od*", u8"zadov'oljstv*", u8"jedn'ostav*", u8"nem'oguć*",
    u8"omog'ući*", u8"omog'ućen*", u8"onemog'ući*", u8"onemog'ućen*",
    u8"dov'rši*", u8"dov'ršen*", u8"zav'rši*", u8"zav'ršen*", u8"pok'reni*",
    u8"pokr'e:nu*", u8"prek'i:ni*", u8"prek'i:nu*", u8"intelig'ent*",

    // ---- Abstract nouns in -ina ----
    u8"brz'in*", u8"vis'in*", u8"duž'in*", u8"šir'in*", u8"dub'in*", u8"dalj'in*",
    u8"topl'in*", u8"tiš'in*", u8"vruć'in*", u8"velič'in*", u8"količ'in*",
    u8"tež'in*", u8"sred'in*", u8"plan'in*", u8"dol'in*", u8"već'in*",
    u8"manj'in*", u8"cjel'in*", u8"cel'in*", u8"bliz'in*", u8"jač'in*",
    u8"glasn'oć*",

    // ---- Technology and screen reader vocabulary ----
    u8"sint'e:z*", u8"rač'unal*", u8"rač'una:r*", u8"kompj'u:ter*", u8"mob'itel*",
    u8"datot'e:k*", u8"pretraž'ivač*", u8"kontr'o:l*", u8"adr'es*", u8"tastat'u:r*",
    u8"'internet*", u8"'auto", u8"'autor*", u8"aut'obus*", u8"autom'obi:l*",
    u8"aut'omat*", u8"telev'i:zor*", u8"kil'omet*", u8"cent'imet*", u8"mil'imet*",
    u8"sek'u:nd*", u8"min'u:t*", u8"r/a:dio", u8"L'aprdus*", u8"l'aprd*",

    // ---- Places ----
    u8"Z'a:greb*", u8"Be'ograd*", u8"S/arajev*", u8"Ljublj'a:n*", u8"Eur'o:p*",
    u8"Evr'o:p*", u8"Am'erik*", u8"H'rva:tsk*", u8"H'rva:t*",
    u8"S'rbij*", u8"s'rpsk*", u8"B^osn*", u8"b^osa:nsk*", u8"H/ercegovin*",
    u8"'Austrij*", u8"Austr'a:lij*", u8"C'rn*",

    // ---- Days ----
    u8"pon'edjelj*", u8"pon'edelj*", u8"^utor*", u8"sr/ije:d*", u8"sr/e:d*",
    u8"četv'rtak", u8"četv'rtk*", u8"p/e:tak", u8"p/e:tk*", u8"s^ubot*", u8"n^edjelj*", u8"n^edelj*",

    // ---- Food, places in town, everyday loans ----
    u8"rest'ora:n*", u8"aer'odrom*", u8"apot'e:k*", u8"ljek'a:rn*", u8"trg'ovin*",
    u8"kupa'onic*", u8"bibliot'e:k*", u8"ban'a:n*", u8"čokol'a:d*", u8"sal'a:t*",
    u8"par'adajz*", u8"fak'ulte:t*", u8"sveuč'ilišt*",
};

const char* const CROATIAN[] = {
    u8"pr/ofesor*", u8"pr/ocent*",
    u8"s^iječ*", u8"v/eljač*", u8"/ožuj*", u8"tr/a:v*", u8"sv/i:b*", u8"l/i:p*",
    u8"s'rp*", u8"k^olovoz*", u8"r/u:j*", u8"l^istopad*", u8"st^uden*",
    u8"pr^osin*",
};

const char* const SERBIAN[] = {
    u8"prof'esor*", u8"proc'enat", u8"proc'ent*",
    u8"j^anua:r*", u8"f^ebrua:r*", u8"m^art*", u8"/apri:l*", u8"m^a:j", u8"j^u:n",
    u8"j^u:l", u8"^avgust*", u8"sept'embar", u8"sept'embr*", u8"okt'o:bar",
    u8"okt'o:br*", u8"nov'embar", u8"nov'embr*", u8"dec'embar", u8"dec'embr*",
    u8"d/e:te", u8"vr/e:me", u8"l/e:p*", u8"ml/e:k*", u8"r/e:k*", u8"b/e:l*",
    u8"sv^e:t", u8"r^e:č",
};

const char* const BOSNIAN[] = {
    u8"prof'esor*", u8"proc'enat", u8"proc'ent*",
    u8"j^anua:r*", u8"f^ebrua:r*", u8"m^art*", u8"/apri:l*", u8"m^a:j", u8"j^u:n",
    u8"j^u:l", u8"^august*", u8"sept'embar", u8"sept'embr*", u8"okt'o:bar",
    u8"okt'o:br*", u8"nov'embar", u8"nov'embr*", u8"dec'embar", u8"dec'embr*",
    u8"k^ahv*", u8"k^af*", u8"l^ahko", u8"m^ehk*",
};

template <size_t N>
const char* const* table(const char* const (&entries)[N], size_t& count) {
    count = N;
    return entries;
}

} // namespace

const char* const* lexicon_common(size_t& count) { return table(COMMON, count); }
const char* const* lexicon_croatian(size_t& count) { return table(CROATIAN, count); }
const char* const* lexicon_serbian(size_t& count) { return table(SERBIAN, count); }
const char* const* lexicon_bosnian(size_t& count) { return table(BOSNIAN, count); }

} // namespace formant
} // namespace laprdus
