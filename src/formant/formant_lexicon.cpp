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
//   |  stem|ending|ending: one exact entry per ending (an empty ending is the
//      bare stem); for paradigms whose accent moves or whose forms would be
//      taken by another word's stem
//
// An exact form always wins over a stem, and the longest stem over shorter
// ones. Where a spelling belongs to two words (obavijesti: the noun's cases
// and the verb's imperative), the noun gets it.

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
    u8"završ'e:tk*", u8"dod'a:tak", u8"dod'a:tk*", u8"pod'a:tak",
    u8"pod'a:tk*", u8"zad'a:tak", u8"zad'a:tk*", u8"ost'a:tak", u8"ost'a:tk*",
    u8"izuz'e:tak", u8"izuz'e:tk*", u8"tren'u:tak", u8"tren'u:tk*",
    u8"slob'od*", u8"zadov'oljstv*", u8"jedn'ostav*", u8"nem'oguć*",
    u8"prek'i:ni*", u8"prek'i:nu*", u8"intelig'ent*",

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

    // ---- Exceptions to the -ina rule (see StressRules::strong) ----
    u8"g^odin*", u8"^istin*", u8"c^arin*", u8"l/avin*", u8"st^otin*", u8"lj^etin*",
    u8"p/okrajin*", u8"/otadžbin*", u8"p/ostojbin*", u8"sudb'in*", u8"deset'in*",
    u8"sv^injetin*", u8"g/ovedin*", u8"p/iletin*", u8"j/anjetin*", u8"j/agnjetin*",
    u8"t/eletin*", u8"j/unetin*", u8"p^aučin*", u8"K^utin*", u8"Kr^apin*",
    u8"j/edina", u8"j/edine", u8"j/edini", u8"j/edinu", u8"j/edino", u8"j/edinom",
    u8"j/edinog", u8"j/edinoj", u8"j/edinih", u8"j/edinim", u8"j/edinoga",
    // loans with a long i
    u8"maš'i:n*", u8"kab'i:n*", u8"vitr'i:n*", u8"medic'i:n*", u8"vakc'i:n*",
    u8"benz'i:n*", u8"discipl'i:n*", u8"rut'i:n*", u8"kuž'i:n*", u8"terr'i:n*",

    // ---- Nouns in -ica with non-initial stress ----
    u8"učit'eljic*", u8"jed'inic*", u8"tipk'o:vnic*", u8"uči'onic*", u8"rad'ionic*",
    u8"lub'enic*", u8"gol'ubic*", u8"kob'asic*", u8"prodav'aonic*", u8"spav'aonic*",
    u8"čit'aonic*", u8"bolnič'a:rk*",

    // ---- Frequent words the rules get wrong ----
    u8"ob'i:telj*", u8"kal'enda:r*", u8"svej'edno", u8"bic'ikl*", u8"gosp'ođic*",
    u8"tak'o:đer", u8"stan'o:vni*", u8"stan'o:vnik*", u8"sveuč'ilišn*",
    u8"iz'u:zetn*", u8"pojed'in*", u8"pojed'inac", u8"pojed'inc*", u8"ist'ovremen*",
    u8"vjer'ojatn*", u8"ver'ovatn*", u8"nar'avn*",
    u8"objašnj'e:nj*", u8"obr'a:zovanj*", u8"obr'a:zovn*", u8"infor'ma:cij*",

    // ---- Nouns in -tak: the plural loses the t (podátak, podáci), the
    // genitive plural moves the accent to the front (pòdātākā) ----
    u8"pod'a:|ci|cima|tci|tcima|tče", u8"p/oda:ta:ka:",
    u8"dod'a:|ci|cima|tci|tcima|tče", u8"d/oda:ta:ka:",
    u8"zad'a:|ci|cima|tci|tcima|tče", u8"z/ada:ta:ka:",
    u8"ost'a:|ci|cima|tci|tcima|tče", u8"/osta:ta:ka:",
    u8"poč'e:|ci|cima|tci|tcima|tče", u8"p/oče:ta:ka:",
    u8"završ'e:|ci|cima|tci|tcima|tče", u8"zav'rše:ta:ka:",
    u8"izuz'e:|ci|cima|tci|tcima|tče", u8"iz'uze:ta:ka:",
    u8"tren'u:|ci|cima|tci|tcima|tče", u8"tr/enu:ta:ka:",
    // The adjectives and adverbs keep the first syllable (dȍdatno, trȅnutno).
    u8"d^odatan", u8"d^odatn*", u8"d^odatno*", u8"d^odatni*",
    u8"tr^enutan", u8"tr^enutn*", u8"tr^enutno*", u8"tr^enutni*",

    // ---- obavijest / obavest and the verbs beside it ----
    // Noun: ȍbavijēst in every case. "obavijesti" is also the verb's
    // imperative, aorist and third person; the noun has priority.
    u8"^obavijest*", u8"^obaviješću", u8"^obave:st*", u8"^obave:šću",
    // obavijéstiti: infinitive, future, imperative, aorist, participle
    u8"obav'ijest|iti|it|ite|io|ila|ilo|ili|ile|ih|ismo|iste|iše|ivši",
    u8"obav'ijest|iću|ićeš|iće|ićemo|ićete",
    u8"obav'e:st|iti|it|ite|io|ila|ilo|ili|ile|ih|ismo|iste|iše|ivši",
    u8"obav'e:st|iću|ićeš|iće|ićemo|ićete",
    // present obàvijēstīm, passive participle obàvijēšten
    u8"ob'avijest|i:m|i:š|i:mo|e:", u8"ob'aviješten*",
    u8"ob'ave:st|i:m|i:š|i:mo|e:", u8"ob'ave:šten*",
    u8"obavešt'e:nj|e|a|u|em|ima",
    // obavještávati follows the rules for verbs in -avati; only its passive
    // participle needs an entry
    u8"obavješt'a:van*", u8"obavešt'a:van*",

    // ---- Irregular verbs whose commands are everywhere on a screen ----
    // započeti, preuzeti, oduzeti...: zapòčni, preùzmi; pronaći: pronáđi;
    // prevesti: prevèla. (The present of these is in the Croatian table.)
    u8"zap'oč|eti|et|ni|nimo|nite", u8"pre'uz|eti|et|mi|mimo|mite",
    u8"od'uz|eti|et|mi|mimo|mite", u8"pod'uz|eti|et|mi|mimo|mite",
    u8"za'uz|eti|et|mi|mimo|mite", u8"pron'a:đ|i|imo|ite",
    u8"prev'el|a|o|i|e", u8"dov'el|a|o|i|e", u8"uv'el|a|o|i|e", u8"izv'el|a|o|i|e",
    u8"prov'el|a|o|i|e", u8"odv'el|a|o|i|e", u8"nav'el|a|o|i|e", u8"zav'el|a|o|i|e",

    // ---- Passive participles of prosljeđívati, nasljeđívati ----
    // (everything else of these verbs comes from the rules for -ivati and
    // -ujem; proslijediti, naslijediti, uslijediti from IJE_VERBS and VERBS)
    u8"prosljeđ'i:van*", u8"nasljeđ'i:van*", u8"prosleđ'i:van*", u8"nasleđ'i:van*",

    // Verbs in -ovati that keep the accent on the first syllable in the
    // present, against the rule for -ujem (nàpredujem, sùdjelujem,
    // sávjetujem), and nouns that look like such a present (olúja, kravàta)
    u8"n/apreduj*", u8"n/azaduj*", u8"s/udjeluj*", u8"s/a:vjetuj*", u8"p/o:sjeduj*",
    u8"/obraduj*", u8"p/ovjeruj*", u8"d/oručkuj*", u8"pr/isustvuj*",
    u8"ol'u:j*", u8"krav'at*", u8"samo'uprav*", u8"/obustav|a|e|i|u|om",
    u8"z/aborav|a|u|om",

    // Words that look like forms of a verb from VERBS: sprȅman (ready) is not
    // the participle of spremati; nàčin, nàčina are not forms of načiniti and
    // not nouns in -ina either
    u8"spr^eman", u8"n/ačin*",

    // "Too ..." adjectives that look like verbs from IJE_VERBS (prȅlijep)
    u8"pr^elijep*", u8"pr^etijesn*", u8"pr^elijen*", u8"pr^ebijel*", u8"pr^esvijetl*",

    // ---- Names of the voices ----
    // Zvónko and Vládo have a long rising accent (Stòjan, Mìrsad and Jòsip
    // are the default). "vlada", "vlade" stay with the noun.
    u8"Zv/o:nk*", u8"Zv/o:nkov*", u8"Vl/a:do", u8"Vl/a:din*",

    // ---- Nouns in -enje of three syllables ----
    // The rule for -enje starts at four syllables, because the short ones
    // go both ways: rođénje, rješénje, kršténje, pošténje, stvorénje,
    // snižénje, but ùčenje, mìšljenje, vȉđenje, which the default gets
    // right. (rȍđendan keeps the first syllable.)
    u8"rođ'e:nj*", u8"rješ'e:nj*", u8"reš'e:nj*", u8"kršt'e:nj*", u8"pošt'e:nj*",
    u8"stvor'e:nj*", u8"sniž'e:nj*", u8"zn/a:čenj*",

    // ---- Abstract nouns in -nost with a long rising accent ----
    u8"mog'u:ćno:st", u8"mog'u:ćnost*", u8"mog'u:ćnošću",
    u8"nemog'u:ćno:st", u8"nemog'u:ćnost*", u8"nemog'u:ćnošću",
    u8"sig'u:rno:st", u8"sig'u:rnost*", u8"sig'u:rnošću", u8"sig'u:rnosn*",
    u8"nesig'u:rno:st", u8"nesig'u:rnost*", u8"nesig'u:rnošću",
    u8"priv'a:tno:st", u8"priv'a:tnost*", u8"priv'a:tnošću",

    // ---- Nouns whose accent moves in the oblique cases ----
    // sìgnal, signála; telèfon, telefóna; novčànīk, novčaníka
    u8"s/ignal", u8"sign'a:l|a|u|om|e|i|ima",
    // telèfon, telefóna (the nominative comes from the rule for -fon)
    u8"telef'o:n|a|u|om|e|i|ima", u8"gramof'o:n|a|u|om|e|i|ima", u8"tel'efonsk*",
    u8"novč'ani:k", u8"novčan'i:|ka|ku|kom|ke|ci|cima", u8"n^ovčani:če",

    // ---- pòzadina against the -ina rule, kòntrolnī against kontróla ----
    u8"p/ozadin*", u8"p/ozadinsk*",
    u8"k/ontroln*", u8"k/ontrolno*", u8"k/ontrolni*",

    // ---- First names with non-initial stress ----
    u8"Aleks'a:ndar", u8"Aleks'a:ndr*", u8"Katar'i:n*", u8"Krist'i:n*", u8"Valent'i:n*",
    u8"Nikol'i:n*", u8"Karol'i:n*", u8"Paul'i:n*", u8"Jasm'i:n*", u8"Em'i:n*", u8"Am'i:n*",
    u8"Sab'i:n*", u8"Alb'i:n*", u8"Reg'i:n*", u8"Georg'i:n*", u8"Ir'e:n*", u8"Hel'e:n*",
    u8"mart'i:na", u8"mart'i:ne", u8"mart'i:ni", u8"mart'i:nu", u8"mart'i:nom",
    u8"mar'i:na", u8"mar'i:ne", u8"mar'i:ni", u8"mar'i:nu", u8"mar'i:nom",
    u8"marij'a:na", u8"marij'a:ne", u8"marij'a:ni", u8"marij'a:nu", u8"marij'a:nom",
    u8"kristij'a:na", u8"kristij'a:ne", u8"kristij'a:ni", u8"kristij'a:nu", u8"kristij'a:nom",
    u8"Daj'a:n*", u8"Tij'a:n*", u8"Dij'an*", u8"Mih'ovil*", u8"Muh'amed*", u8"Hus'ein*",
    u8"Ibr'a:him*", u8"Sul'ejman*", u8"Slob'odan*", u8"Sin'iš*", u8"Mih'ael*",
    u8"elv'i:ra", u8"elv'i:re", u8"elv'i:ri", u8"elv'i:ru", u8"elv'i:rom",
    u8"Ant'o:nio", u8"Ant'o:nij*", u8"Ant'o:nia", u8"Ant'o:nie", u8"Ant'o:niu",
    u8"Ren'a:t*", u8"Sand'r*", u8"Natal'ij*", u8"Vikt'o:rij*", u8"Dan'ijel*",

    // ---- Surnames: exceptions to the -ović/-ević rule ----
    u8"'ivanović*", u8"j'osipović*", u8"m'aksimović*", u8"dr'agović*", u8"v'idović*",

    // ---- More places ----
    u8"Var'aždin*", u8"Kr'agujevac", u8"Kr'agujevc*", u8"Mak'edo:nij*", u8"At'e:n*",
    u8"Kopenh'a:gen*", u8"Vuk'ova:r*", u8"Bjel'ova:r*", u8"Vir'ovitic*", u8"Crikv'enic*",
    u8"Og'uli:n*", u8"Iv'anec", u8"Iv'anc*", u8"S'ubotic*", u8"P'odgoric*", u8"K'oprivnic*",
    u8"Prij'e:dor*", u8"Vis'ok*", u8"Slav'o:nij*", u8"Dalm'a:cij*", u8"Mad'ri:d*",
    u8"Berl'i:n*", u8"Par'i:z*", u8"Lond'o:n*", u8"Ljublj'an*", u8"Beogr'a:đan*",
    u8"Zagrepč'an*", u8"Spl'ićan*", u8"Riječ'an*", u8"Sarajl'ij*",

    // ---- Food, places in town, everyday loans ----
    u8"rest'ora:n*", u8"aer'odrom*", u8"apot'e:k*", u8"ljek'a:rn*", u8"trg'ovin*",
    u8"kupa'onic*", u8"bibliot'e:k*", u8"ban'a:n*", u8"čokol'a:d*", u8"sal'a:t*",
    u8"par'adajz*", u8"fak'ulte:t*", u8"sveuč'ilišt*",
};

const char* const CROATIAN[] = {
    u8"pr/ofesor*", u8"pr/ocent*", u8"d'irektor*", u8"V^ojvodin*", u8"'ukrajin*",
    u8"s^iječ*", u8"v/eljač*", u8"/ožuj*", u8"tr/a:v*", u8"sv/i:b*", u8"l/i:p*",
    u8"s'rp*", u8"k^olovoz*", u8"r/u:j*", u8"l^istopad*", u8"st^uden*",
    u8"pr^osin*",
    // Croatian is commonly spoken with the long e of obavijestiti stressed in
    // the present and the passive participle as well (obavijéstim,
    // obavijéšten for the dictionaries' obàvijēstīm, obàvijēšten), like the
    // verbs in IJE_VERBS; so is razumijévam.
    u8"obav'ijest|i:m|i:š|i:mo|e:", u8"obav'iješten*", u8"razum'ijev*",
    // The same for the present of započeti and the verbs in -uzeti.
    u8"zap'očn|em|eš|e|emo|ete|u", u8"pre'uzm|em|eš|e|emo|ete|u",
    u8"od'uzm|em|eš|e|emo|ete|u", u8"pod'uzm|em|eš|e|emo|ete|u",
    u8"za'uzm|em|eš|e|emo|ete|u",
};

const char* const SERBIAN[] = {
    u8"prof'esor*", u8"proc'enat", u8"proc'ent*", u8"Vojv'odin*", u8"Ukraj'i:n*",
    u8"j^anua:r*", u8"f^ebrua:r*", u8"m^art*", u8"/apri:l*", u8"m^a:j", u8"j^u:n",
    u8"j^u:l", u8"^avgust*", u8"sept'embar", u8"sept'embr*", u8"okt'o:bar",
    u8"okt'o:br*", u8"nov'embar", u8"nov'embr*", u8"dec'embar", u8"dec'embr*",
    u8"d/e:te", u8"vr/e:me", u8"l/e:p*", u8"ml/e:k*", u8"r/e:k*", u8"b/e:l*",
    u8"sv^e:t", u8"r^e:č",
};

const char* const BOSNIAN[] = {
    u8"prof'esor*", u8"proc'enat", u8"proc'ent*", u8"Vojv'odin*", u8"Ukraj'i:n*",
    u8"j^anua:r*", u8"f^ebrua:r*", u8"m^art*", u8"/apri:l*", u8"m^a:j", u8"j^u:n",
    u8"j^u:l", u8"^august*", u8"sept'embar", u8"sept'embr*", u8"okt'o:bar",
    u8"okt'o:br*", u8"nov'embar", u8"nov'embr*", u8"dec'embar", u8"dec'embr*",
    u8"k^ahv*", u8"k^af*", u8"l^ahko", u8"m^ehk*",
};

// Verbs whose root has the long "ije" (podijéliti, promijéniti, zalijévati).
// After a prefix the spelling "ije" marks the verb: the nouns beside it have
// the short "je" (podjela, promjena, zamjena). The front end puts the accent
// on the "ije" in every form built from a root listed here
// (StressRules::ije_verb); nothing else tells it that "podijeli", "podijelio"
// and "podijeljen" belong to "podijeliti".
//
//   root:classes[:prefixes]
//   i  verb in -iti (podijeliti, podijelim, podijeli, podijelio, podijeljen)
//   a  verb in -ati (pomiješati, pomiješam, pomiješaj, pomiješao, pomiješan)
//   t  verb in -ati whose present has another root (dolijetati)
//   e  that present root (dolijećem, podliježem)
//   prefixes: the only ones the root takes, where any prefix would also
//   match nouns (povijest, pripovijest, zapovijed)
const char* const IJE_VERBS[] = {
    u8"dijel:i", u8"mijen:i", u8"lijep:i", u8"cijen:i", u8"bijed:i", u8"slijed:i",
    u8"vrijed:i", u8"zlijed:i", u8"trijeb:i", u8"riješ:i", u8"liječ:i", u8"slijep:i",
    u8"bijel:i", u8"cijel:i", u8"mijet:i", u8"svijetl:i", u8"rijed:i", u8"prijet:i",
    u8"cijed:i", u8"korijen:i", u8"plijen:i", u8"lijen:i", u8"smiješ:i", u8"tijesn:i",
    u8"strijel:i", u8"nijem:i", u8"bijesn:i", u8"griješ:i", u8"mijes:i", u8"cijep:ia",
    u8"prijed:i:una", u8"vijest:i:iz na nago", u8"svijest:i:o po",
    u8"miješ:a", u8"lijev:a", u8"grijev:a", u8"spijev:a", u8"dijev:a", u8"zrijev:a",
    u8"gorijev:a", u8"pijev:a", u8"sijec:a", u8"drijem:a", u8"mijer:a",
    u8"htijev:a:za", u8"umijev:a:raz sporaz", u8"olijev:a:od", u8"starijev:a:za",
    u8"povijed:a:pri za is pro",
    u8"lijet:t", u8"lijeć:e", u8"lijeg:t", u8"lijež:e",
};

// Other verbs, as whole stems with the accent of the infinitive marked as in
// the lexicon above: uréditi, otvòriti, pročìtati, pokrénuti. Their forms
// keep that accent on the root (urédi, urédio, otvòri, pročìtaj, pokréni);
// StressRules::verb_form() builds the forms.
//
//   st'e:m=class and flags
//   i  -iti, -im      (urediti, uredim, uredi, uredio, uređen)
//   a  -ati, -am      (pročitati, pročitam, pročitaj, pročitao, pročitan)
//   u  -nuti, -nem    (pokrenuti, pokrenem, pokreni, pokrenuo, pokrenut)
//   t  -ati, where the present has another stem (pokazati, pokazao, pokazan)
//   e  that stem, or any present in -em (pokažem, pokaži; unesem, unesi)
//   <  the dictionaries move the accent one syllable back in the present
//      (ùrēdīm, òtvorīm, pòkāžēm); Stojan and Mirsad do, Zvonko does not
//   p  ... and in the passive participle (ùrēđen);  P: to the first syllable
//   n  the imperative is spelled like a form of a noun or adjective (potvrdi,
//      uredi, otvori, načini; for class a the third person: oprema, proba).
//      It is read as the verb only at the head of a clause ("Potvrdi",
//      "Ne zaboravi"); elsewhere the other word has priority.
//
// The accents are those of Hrvatski jezični portal (October 2026); the
// stems were generated from its entries, the noun twins from its answers
// for the imperative forms. A verb that is not here is right only in the
// infinitive or if the first syllable carries its accent anyway.
const char* const VERBS[] = {
    u8"ur'e:d=i<pn", u8"odr'e:d=i<pn", u8"nar'e:d=i<p", u8"sr'e:d=i", u8"preur'e:d=i<p",
    u8"uklj'u:č=i<p", u8"isklj'u:č=i<p", u8"zaklj'u:č=i<p", u8"priklj'u:č=i<p",
    u8"odl'u:č=i<p", u8"na'uč=i<p", u8"pro'uč=i<p", u8"po'uč=i<p", u8"ur'u:č=i<p",
    u8"nar'u:č=i<p", u8"por'u:č=i<p", u8"prepor'u:č=i<p", u8"ispor'u:č=i<p",
    u8"potv'r:d=i<pn", u8"utv'r:d=i<pn", u8"tv'r:d=i", u8"ob'uhvat=i", u8"pretr'a:ž=i<p",
    u8"istr'a:ž=i<p", u8"potr'a:ž=i<", u8"tr'a:ž=i", u8"ozn'a:č=i<p", u8"zn'a:č=i",
    u8"spr'e:m=i", u8"pripr'e:m=i<pn", u8"opr'e:m=i<pn", u8"otv'or=i<pn",
    u8"zatv'or=i<pn", u8"pretv'or=i<p", u8"ostv'a:r=i<p", u8"ukl'on=i<p",
    u8"pokl'on=i<pn", u8"pon'ov=i<p", u8"obn'ov=i<pn", u8"odgov'o:r=i<pn",
    u8"dogov'or=i<pn", u8"izgov'or=i<pn", u8"progov'or=i<", u8"gov'or=i<n",
    u8"dozv'ol=i<pn", u8"omog'u:ć=i<p", u8"onemog'u:ć=i<p", u8"za'ustav=in",
    u8"usp'ostav=in", u8"zab'orav=in", u8"obj'a:v=i<pn", u8"prij'a:v=i<pn",
    u8"odj'a:v=i<pn", u8"naj'a:v=i<pn", u8"izj'a:v=i<pn", u8"j'a:v=i", u8"poj'a:v=i<n",
    u8"ispr'a:zn=i<pn", u8"uč'in=i<p", u8"nač'in=i<pn", u8"vr'a:t=i", u8"povr'a:t=i<p",
    u8"svr'a:t=i", u8"navr'a:t=i<n", u8"odvr'a:t=i<p", u8"obr'a:t=i<p", u8"pl'a:t=i",
    u8"upl'a:t=i<pn", u8"ispl'a:t=i<pn", u8"napl'a:t=i<pn", u8"dopl'a:t=i<pn",
    u8"k'u:p=i", u8"otk'u:p=i<pn", u8"proš'i:r=i<p", u8"raš'i:r=i<p",
    u8"sm'a:nj=i", u8"um'a:nj=i<p", u8"pov'eć=ap", u8"uv'eć=ap", u8"poj'ač=ap",
    u8"oj'ač=ap", u8"ut'iš=ap", u8"pribl'i:ž=i<p", u8"ud'a:lj=i<p", u8"dop'ust=i<pn",
    u8"isp'ust=i<pn", u8"prop'ust=i<pn", u8"nap'ust=i<p", u8"otp'ust=i<pn",
    u8"zabr'a:n=i<pn", u8"obr'a:n=i<pn", u8"br'a:n=i", u8"od'obr=i<p", u8"izb'a:c=i<p",
    u8"ub'a:c=i<p", u8"preb'a:c=i<p", u8"odb'a:c=i<p", u8"nab'a:c=i<p", u8"zab'a:c=i<p",
    u8"b'a:c=i", u8"izv'r:š=i<p", u8"zav'r:š=i<p", u8"dov'r:š=i<p", u8"izr'a:d=i<pn",
    u8"obr'a:d=i<pn", u8"ur'a:d=i<p", u8"zar'a:d=i<pn", u8"odr'a:d=i<p",
    u8"prer'a:d=i<pn", u8"nagr'a:d=i<pn", u8"ugr'a:d=i<p", u8"izgr'a:d=i<p",
    u8"sagr'a:d=i<p", u8"gr'a:d=i", u8"pohr'a:n=i<pn", u8"sahr'a:n=i<pn", u8"hr'a:n=i",
    u8"nahr'a:n=i<p", u8"zašt'i:t=i<pn", u8"odg'od=i<pn", u8"dog'od=i<", u8"pog'od=i<p",
    u8"ug'od=i<pn", u8"prilag'od=i<p", u8"pohv'a:l=i<pn", u8"hv'a:l=i", u8"zahv'a:l=i<n",
    u8"up'a:l=i<pn", u8"zap'a:l=i<p", u8"ug'a:s=i<p", u8"g'a:s=i", u8"ispr'o:b=a<p",
    u8"pr'o:b=a", u8"proč'it=ap", u8"oč'it=ap", u8"izrač'un=aP", u8"rač'un=a",
    u8"obrač'un=aP", u8"zaklj'uč=ap", u8"otklj'uč=ap", u8"p'i:t=a", u8"up'i:t=a<p",
    u8"zap'i:t=a<p", u8"pr'i:č=a", u8"ispr'i:č=a<p", u8"sv'i:r=a", u8"m'o:r=a",
    u8"up'ozn=ap", u8"prep'ozn=ap", u8"prid'od=aP", u8"k'a:z=t", u8"k'a:ž=e",
    u8"pok'a:z=tp", u8"pok'a:ž=e<", u8"prik'a:z=tp", u8"prik'a:ž=e<", u8"dok'a:z=tp",
    u8"dok'a:ž=e<", u8"otk'a:z=tp", u8"otk'a:ž=e<", u8"zak'a:z=tp", u8"zak'a:ž=e<",
    u8"isk'a:z=tp", u8"isk'a:ž=e<", u8"uk'a:z=tp", u8"uk'a:ž=e<", u8"p'i:s=t",
    u8"p'i:š=e", u8"nap'i:s=tp", u8"nap'i:š=e<", u8"up'i:s=tp", u8"up'i:š=e<",
    u8"isp'i:s=tp", u8"isp'i:š=e<", u8"op'i:s=tp", u8"op'i:š=e<", u8"zap'i:s=tp",
    u8"zap'i:š=e<", u8"potp'i:s=tp", u8"potp'i:š=e<", u8"prep'i:s=tp", u8"prep'i:š=e<",
    u8"otp'i:s=tp", u8"otp'i:š=e<", u8"pop'i:s=tp", u8"pop'i:š=e<", u8"dop'i:s=tp",
    u8"dop'i:š=e<", u8"prop'i:s=tp", u8"prop'i:š=e<", u8"v'e:z=t", u8"v'e:ž=e",
    u8"pov'e:z=tp", u8"pov'e:ž=e<", u8"zav'e:z=tp", u8"zav'e:ž=e<", u8"priv'e:z=tp",
    u8"priv'e:ž=e<", u8"odv'e:z=tp", u8"odv'e:ž=e<", u8"sv'e:z=t", u8"sv'e:ž=e",
    u8"obv'e:z=tp", u8"obv'e:ž=e<", u8"pokr'e:=u<p", u8"okr'e:=u<p", u8"zakr'e:=u<p",
    u8"skr'e:=u", u8"preokr'e:=u<p", u8"pom'ak=u<p", u8"odm'ak=u<p", u8"prim'ak=u<p",
    u8"razm'ak=u<p", u8"dod'i:r=u<p", u8"dot'ak=u<p", u8"spom'e:=u<p", u8"napom'e:=u<p",
    u8"iz'ostav=i", u8"up'u:t=i<pn", u8"stv'a:r=a", u8"otv'a:r=a<", u8"zatv'a:r=a<",
    u8"pretv'a:r=a<", u8"odgov'a:r=a<", u8"razgov'a:r=a<", u8"dogov'a:r=a<",
    u8"izgov'a:r=a<", u8"pon'a:vlj=a<", u8"obn'a:vlj=a<", u8"pripr'e:m=a<n",
    u8"spr'e:m=a", u8"opr'e:m=a<n", u8"premj'e:št=a<", u8"namj'e:št=a<", u8"pre'usmjer=i",
    u8"z'a:mjer=i", u8"iskor'ist=i<p", u8"sl'u:ž=i", u8"posl'u:ž=i<p", u8"zasl'u:ž=i<p",
    u8"zam'ol=i<p", u8"don'os=i<n", u8"odn'os=i<n", u8"pren'os=i<", u8"un'os=i<n",
    u8"izn'os=i<n", u8"podn'os=i<", u8"uv'od=i<n", u8"izv'od=i<n", u8"prev'od=i<",
    u8"prov'od=i<n", u8"dov'od=i<n", u8"nav'od=i<n", u8"h'o:d=a", u8"pron'alaz=i",
    u8"sn'i:m=i", u8"sn'i:m=a", u8"presn'i:m=i<p", u8"izg'ub=i<p", u8"prob'u:d=i<p",
    u8"razl'ož=i<p", u8"ul'ož=i<p", u8"pol'ož=i<p", u8"predl'ož=i<p", u8"izl'ož=i<p",
    u8"zal'ož=i<p", u8"odl'ož=i<p", u8"pril'ož=i<p", u8"nal'ož=i<p", u8"upoz'or=i<p",
    u8"pom'i:r=i<p", u8"sm'i:r=i", u8"um'i:r=i<p", u8"pož'u:r=i<", u8"prov'a:l=i<pn",
    u8"nav'a:l=i<pn", u8"pres'el=i<p", u8"dos'el=i<p", u8"us'el=i<p", u8"nas'el=i<p",
    u8"razves'el=i<p", u8"pozv'on=i<", u8"zar'on=i<p", u8"ur'on=i<p", u8"pob'u:n=i<pn",
    u8"zb'u:n=i", u8"nast'u:p=i<n", u8"prist'u:p=i<n", u8"ist'u:p=i<pn",
    u8"odst'u:p=i<pn", u8"post'u:p=i<", u8"zast'u:p=i<p", u8"ust'u:p=i<pn", u8"st'u:p=i",
    u8"ut'op=i<p", u8"rast'op=i<p", u8"pot'op=i<pn", u8"polj'u:b=i<p", u8"lj'u:b=i",
    u8"prel'om=i<p", u8"p'a:mt=i", u8"skr'a:t=i", u8"poč'ast=i<pn", u8"opr'ost=i<pn",
    u8"uv'r:st=i<p", u8"razv'rst=ap", u8"prem'ost=i<p", u8"izab'er=e", u8"odab'er=e<",
    u8"od'ust=t", u8"od'ustan=e", u8"poš'alj=e<", u8"poz'ov=e", u8"naz'ov=e",
    u8"sač'u:v=a<p", u8"č'u:v=a", u8"oč'u:v=a<", u8"udr'u:ž=i<p", u8"pridr'u:ž=i<p",
    u8"zdr'u:ž=i", u8"dr'u:ž=i", u8"zatr'a:ž=i<p", u8"razm'otr=i<p", u8"prom'otr=i<p",
    u8"nasl'on=ipn", u8"osl'on=i<p", u8"prisl'on=i<p", u8"označ'a:v=a<", u8"ob'iljež=i",
    u8"zab'iljež=i", u8"ub'iljež=i", u8"prib'iljež=i", u8"nagl'a:s=i<p", u8"gl'a:s=i",
    u8"ogl'a:s=i<pn", u8"progl'a:s=i<pn", u8"prod'u:ž=i<p", u8"prod'u:lj=i<p",
    u8"od'u:ž=i<p", u8"zad'u:ž=i<p", u8"razd'u:ž=i<p", u8"ubl'a:ž=i<p", u8"ol'akš=ap",
    u8"ot'ež=ap", u8"pojednost'a:v=i<p", u8"pob'oljš=ap", u8"pog'orš=ap", u8"usp'or=i<p",
    u8"ub'rz=ap", u8"zak'asn=i<", u8"zat'a:j=i<pn", u8"t'a:j=i", u8"ut'a:j=i<pn",
    u8"ob'eć=ap", u8"op'er=e", u8"isp'er=e", u8"sap'er=e", u8"up'r:lj=a<p",
    // Short verbs in -avati and -ivati, whose third person the rule for
    // -avam leaves alone (rješava, uživa)
    u8"rješ'a:v=a<", u8"deš'a:v=a<", u8"už'i:v=a<", u8"dob'i:v=a<", u8"zadob'i:v=a<",
    u8"pokr'i:v=a<", u8"otkr'i:v=a<", u8"sakr'i:v=a<", u8"prekr'i:v=a<", u8"poč'i:v=a<",
    u8"um'i:v=a<", u8"izaz'i:v=a<",
    // držati and its family: -ati with a present in -im (zadr̀žati, zàdržīm)
    u8"zad'rž=ti<p", u8"od'rž=ti<p", u8"sad'rž=ti<p", u8"pod'rž=ti<p", u8"izd'rž=ti<p",
    u8"prid'rž=ti<p",
    // -nijeti and -vesti: unèsem, unèsi; prevèdem, prevèdi
    u8"un'es=e", u8"don'es=e", u8"pren'es=e", u8"izn'es=e", u8"odn'es=e", u8"podn'es=e",
    u8"pon'es=e", u8"nan'es=e", u8"zan'es=e", u8"prev'ed=e", u8"dov'ed=e", u8"uv'ed=e",
    u8"izv'ed=e", u8"prov'ed=e", u8"odv'ed=e", u8"nav'ed=e", u8"zav'ed=e",
    // Ekavian: the counterparts of IJE_VERBS have no spelling to know them
    // by, so the common ones are listed (podéliti, proméniti, poméšati)
    u8"prosl'e:d=i<p", u8"zal'e:p=i<p", u8"nal'e:p=i<p", u8"pril'e:p=i<p",
    u8"odl'e:p=i<p", u8"prec'e:n=i<p", u8"potc'e:n=i<p", u8"ub'e:d=i<p", u8"istr'e:b=i<p",
    u8"izl'e:č=i<p", u8"zal'e:č=i<p", u8"prim'e:t=i<p", u8"osv'e:tl=i<p",
    u8"rasv'e:tl=i<p", u8"prosv'e:tl=i<p", u8"razr'e:š=i<p", u8"odr'e:š=i<p",
    u8"unapr'e:d=i<p", u8"iskor'e:n=i<p", u8"nagov'e:st=i<p", u8"nasl'e:d=i<pn",
    u8"usl'e:d=i<pn", u8"pod'e:l=i<pn", u8"razd'e:l=i<pn", u8"dod'e:l=i<pn",
    u8"ud'e:l=i<pn", u8"raspod'e:l=i<pn", u8"prom'e:n=i<pn", u8"izm'e:n=i<pn",
    u8"zam'e:n=i<pn", u8"prim'e:n=i<pn", u8"razm'e:n=i<pn", u8"nam'e:n=i<pn",
    u8"oc'e:n=i<pn", u8"proc'e:n=i<pn", u8"uc'e:n=i<pn", u8"pob'e:d=i<pn",
    u8"uvr'e:d=i<pn", u8"povr'e:d=i<pn", u8"upotr'e:b=i<pn", u8"izv'e:st=i<pn",
    u8"pom'e:š=a<p", u8"um'e:š=a<p", u8"izm'e:š=a<p", u8"zam'e:š=a<p", u8"prom'e:š=a<p",
    u8"zaht'e:v=a<p", u8"usp'e:v=a<p", u8"dosp'e:v=a<p", u8"razum'e:v=a<p",
    u8"zal'e:v=a<p", u8"prel'e:v=a<p",
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
const char* const* lexicon_ije_verbs(size_t& count) { return table(IJE_VERBS, count); }
const char* const* lexicon_verbs(size_t& count) { return table(VERBS, count); }

} // namespace formant
} // namespace laprdus
