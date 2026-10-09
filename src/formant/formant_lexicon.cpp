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
    // ---- Numbers (Hrvatski jezični portal; Wiktionary and the Serbian
    // dictionaries for the Serbian forms) ----
    u8"n^ul*", u8"j/edan", u8"j/edna", u8"j/edno", u8"d^va:", u8"dv^ije", u8"dv^e:",
    u8"t^ri:", u8"č/etiri", u8"p^e:t", u8"š^e:st", u8"s^edam", u8"^osam",
    u8"d^evet", u8"d^eset",
    // The teens and their ordinals (jedànaest, jedànaestī) share a stem; the
    // length of -naēst is in the Serbian and Bosnian tables.
    u8"jed'anaest*", u8"dv'a:naest*", u8"tr'i:naest*", u8"čet'rnaest*", u8"p'etnaest*",
    u8"š'esnaest*", u8"sed'amnaest*", u8"os'amnaest*", u8"dev'etnaest*",
    // Shared listening-based rendering of twenty and thirty: long falling
    // accents were chosen when a short vowel or a later pitch peak was
    // heard as two words ("dva deset"). The Croatian exact entries below
    // now restore dictionary dvádeset and its ordinal, and distinguish
    // dvadesétak. The remaining shared entries keep their existing tone.
    // Forty to ninety stress the "de"
    // (četrdèsēt); the Croatian voice has them without the final length
    // (CROATIAN), the Serbian and Bosnian ordinals are dvadèsētī.
    u8"dv^a:deset*", u8"tr^i:deset*",
    u8"četrd'ese:t*", u8"ped'ese:t*", u8"šezd'ese:t*",
    u8"sedamd'ese:t*", u8"osamd'ese:t*", u8"deved'ese:t*",
    u8"st^o:", u8"dvj^esto", u8"dvj^esta", u8"dv^esta", u8"tr^isto", u8"tr^ista",
    u8"č^etiristo", u8"p^e:tsto", u8"š^e:ststo", u8"s^edamsto", u8"^osamsto",
    u8"d^evetsto",
    // The ordinals of the hundreds keep the accent of the cardinal: stȏtī,
    // dvjȅstōtī, trȉstōtī, petstȏtī (the length of -ōtī in the Serbian and
    // Bosnian tables)
    u8"st^o:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", u8"dvj^estot*", u8"dv^estot*",
    u8"tr^istot*", u8"č^etiristot*", u8"p^e:tstot*", u8"š^e:ststot*", u8"s^edamstot*",
    u8"^osamstot*", u8"d^evetstot*",
    // tìsuća, tìsuću, tìsuće, tìsućītī; after a number the genitive plural
    // tȉsūćā is a phrase rule of the front end. hȉljada, hȉljaditī.
    u8"t/isuć*", u8"h^iljad*",
    // milìjūn but milijúna, like the other nouns with a long last stem
    // syllable (StressRules::long_stem); milìjūntī, milìōnitī; milìjārda
    // (its length after the accent in the Serbian and Bosnian tables)
    u8"mil'iju:n", u8"milij'u:n|a|u|om|e|i|ima|sk*", u8"mil'io:n", u8"mili'o:n|a|u|om|e|i|ima|sk*",
    u8"mil/ijunt*", u8"mil/ionit*", u8"mil/ijard*",
    u8"bil'iju:n", u8"bilij'u:n|a|u|om|e|i|ima", u8"bil'io:n", u8"bili'o:n|a|u|om|e|i|ima",
    // Ordinals one to ten: pȓvī, drȕgī, trȅćī, čètvr̄tī, pȇtī, šȇstī, sȇdmī,
    // ȏsmī, dèvētī, dèsētī; the cardinals (pȇt, dȅvet) keep their own exact
    // entries, so the ordinals are paradigms
    u8"p'rv*", u8"dr^ug*", u8"tr^eć*", u8"č/etvrt|i|a|o|e|u|og|oga|om|ome|oj|ih|im|ima",
    u8"p^e:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", u8"š^e:st|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"s^e:dm|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", u8"^o:sm|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"d/evet|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", u8"d/eset|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    // dvȍje, trȍje, ȍba, ȍbje, dvòjica, tròjica, četvòrica, petòrica
    u8"dv^oje", u8"tr^oje", u8"^oba", u8"^obje", u8"dv/ojic*", u8"tr/ojic*",
    u8"četv/oric*", u8"pet/oric*",
    u8"z^arez", u8"c^ije:l*", u8"c^e:l*",

    // ---- Very frequent words: tone and length ----
    u8"d^a:n", u8"d^obar", u8"d^obro", u8"d^obra", u8"hv/a:la", u8"m^oli:m",
    u8"n^o:ć", u8"j^utro", u8"v^eče:r", u8"zdr^avo", u8"b^o:g", u8"d^a", u8"n^e",
    u8"v/oda", u8"v/ode", u8"ž/ena", u8"ž/ene", u8"j/ezik*", u8"k^uć*", u8"gl/a:v*",
    u8"r/u:k*", u8"n/og*", u8"s/estr*", u8"z/emlj*", u8"gr^a:d", u8"s^i:n",
    u8"m^a:jk*", u8"^otac", u8"br^at", u8"lj^u:di", u8"d/ije:te", u8"sv^ije:t",
    u8"r^ije:č", u8"vr/ije:me", u8"l/ije:p*", u8"ml/ije:k*", u8"r/ije:k*",
    u8"b/ije:l*", u8"mj^esto", u8"p^osao", u8"r/a:di*", u8"r^a:d",
    u8"zn^a:m", u8"zn^a:š", u8"zn^a:", u8"zn^a:mo", u8"zn^a:te",
    u8"^ima:m", u8"^ima:š", u8"^ima:",
    u8"s^ada", u8"s^ad", u8"t^ada", u8"k^ada", u8"/o:vdje", u8"/o:vde",
    u8"t/a:mo", u8"/ovamo", u8"d^anas", u8"s^utra", u8"j^uče:r", u8"j^uče:",

    // ---- Words with non-initial stress ----
    u8"doviđ'e:nja", u8"dobrod'oš*", u8"izv'ol*", u8"izv'i:ni*", u8"opr'osti*",
    u8"zahv'a:lj*", u8"gosp'odin*", u8"gosp'odo", u8"međ'utim", u8"več'eras",
    u8"ov'ako", u8"on'ako", u8"kol'iko", u8"tol'iko", u8"ukol'iko", u8"odj'ednom",
    // ȉnāče against the rule for -ač (ináče); uòstālom, ionàkō
    u8"^inače", u8"u'ostalom", u8"ion'ako",
    u8"otpr'ilike", u8"zan'imljiv*", u8"izgl'e:da*", u8"post'oj*",
    u8"raz'umije*", u8"raz'ume*", u8"raz'umje*", u8"gov'orio", u8"gov'orila",
    u8"gov'orili", u8"gov'oril*", u8"poč'e:tak", u8"poč'e:tk*", u8"završ'e:tak",
    u8"završ'e:tk*", u8"dod'a:tak", u8"dod'a:tk*", u8"pod'a:tak",
    u8"pod'a:tk*", u8"zad'a:tak", u8"zad'a:tk*", u8"ost'a:tak", u8"ost'a:tk*",
    u8"izuz'e:tak", u8"izuz'e:tk*", u8"tren'u:tak", u8"tren'u:tk*",
    u8"slob'od*", u8"zadov'oljstv*", u8"nem'oguć*",
    // jȅdnostavan, jȅdnostavno, jednostàvniji, jednostávnōst
    u8"j^ednostav*", u8"j^ednostavn*", u8"jednost'avnij*",
    u8"jednost'a:vno:st", u8"jednost'a:vnost*", u8"jednost'a:vnošću",
    // lègalan, ȉlegalan (the rule for -alan gives the first syllable, these
    // set the tone) and legálnōst, ilegálnōst
    u8"l/egal*", u8"l/egaln*", u8"leg'a:lno:st", u8"leg'a:lnost*", u8"leg'a:lnošću",
    u8"^ilegal*", u8"^ilegaln*", u8"ileg'a:lno:st", u8"ileg'a:lnost*", u8"ileg'a:lnošću",
    // slobòda, but the adjective and adverb slȍbodan, slȍbodno; the name
    // Slobòdan keeps its cases (Slobòdana), the nominative goes to the
    // commoner adjective
    u8"sl^obodan", u8"sl^obodn*", u8"sl^obodnij*", u8"sl^obodn|oga|ome|ima",
    // snága, snázi, snážan, snážno
    u8"sn/a:g*", u8"sn/a:zi", u8"sn/a:žan", u8"sn/a:žn*", u8"sn/a:žnij*",
    u8"intelig'ent*",

    // ---- Abstract nouns in -ina ----
    u8"brz'in*", u8"vis'in*", u8"duž'in*", u8"šir'in*", u8"dub'in*", u8"dalj'in*",
    u8"topl'in*", u8"tiš'in*", u8"vruć'in*", u8"velič'in*", u8"količ'in*",
    u8"tež'in*", u8"sred'in*", u8"plan'in*", u8"dol'in*", u8"već'in*",
    u8"manj'in*", u8"cjel'in*", u8"cel'in*", u8"bliz'in*", u8"jač'in*",
    u8"glasn'oć*",

    // ---- Technology and screen reader vocabulary ----
    u8"sint'e:z*", u8"rač'unal*", u8"rač'una:r*", u8"kompj'u:ter*", u8"m/obitel*",
    u8"datot'e:k*", u8"pretraž'ivač*", u8"kontr'o:l*", u8"adr'es*", u8"tastat'u:r*",
    u8"'internet*", u8"^instagram*", u8"'auto", u8"'autor*", u8"aut'obus*", u8"autom'obi:l*",
    u8"aut'omat*", u8"telev'i:zor*", u8"kil'omet*", u8"cent'imet*", u8"mil'imet*",
    u8"sek'u:nd*", u8"min'u:t*", u8"r/a:dio", u8"L'aprdus*", u8"l'aprd*",
    // emòtikon in every case; not in the dictionaries, accented like
    // emòcija, against the rule for loans in -on (emotikóna)
    u8"em'otikon*",
    // ubrzánje (HJP), the participle and adverb ȕbrzan, ȕbrzano; the verb
    // ubr̀zati is in VERBS
    u8"ubrz'a:nj*", u8"^ubrzan*",
    // podèsiv, podèsivo like the other adjectives in -iv from prefixed
    // verbs (izvèdiv, dokàziv, podnòšljiv, prilagòdljiv in HJP; the word
    // itself is in no dictionary), podesívōst like održívōst
    u8"pod'esiv*", u8"nepod'esiv*", u8"podes'i:vost*", u8"podes'i:vošću",
    // mȍnoton, mȍnotono in every form (HJP); the noun monotònija and the
    // comparative monotòniji need their own stem, the shorter one would
    // take them
    u8"m^onoton*", u8"monot'onij*",
    // pàuza, sȁuna, fàuna (HJP): the long-stem rule had taken "au" for a
    // long u (paúza)
    u8"p/auz*", u8"s^aun*", u8"f/aun*",
    // rȍđendān, rȍđendāna (HJP, Školski rječnik, Wiktionary); the default
    // rules stressed the second syllable (rođèndan, rođendána)
    u8"r^ođendan*",
    // kòrisnīk, kòrisnica, kòrisnički (HJP) and the adjective kȍristan,
    // kȍrisno, the noun kȍrīst; the verb kòristiti is in VERBS, and
    // "koristi" is read as the noun
    u8"k/orisnik*", u8"k/orisnic*", u8"k/orisniče", u8"k/orisničk*",
    u8"k^oristan", u8"k^orisn|a|o|i|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"k^orist", u8"k^oristi",
    // rédak, rétka, réci, rȇdākā (HJP), also spelled redka, redci; "reci"
    // stays the imperative rèci, the far commoner word
    u8"r/e:dak", u8"r/e:tk|a|u|om|e|i|o|ih|im|ima|og|oj", u8"r/e:dk|a|u|om|e|i",
    u8"r/e:tc|i|ima", u8"r/e:dc|i|ima", u8"r/e:cima", u8"r/eci", u8"r^e:daka",

    // ---- Age and time ----
    // čèstitka, čèstitke, G pl čèstitākā/čèstitkā, DL čèstitki/čèstitci
    // (Školski rječnik and Rečnik Matice srpske; HJP čȅstitka); the rule for
    // a long last stem syllable had čestítka
    u8"č/estitk*", u8"č/estitaka", u8"č/estitci",
    // gòdišnjāk, gòdišnjāka (HJP, Školski rječnik, Rečnik Matice srpske);
    // its compounds (petogòdišnjāk) and the other compounds of -godišnji,
    // -mjesečni, -dnevni, -tjedni and -ljetan are a rule
    // (StressRules::time_compound)
    u8"g/odišnjak*", u8"g/odišnjac|i|ima", u8"g/odišnjače",

    // ---- Places ----
    u8"Z'a:greb*", u8"Be'ograd*", u8"S/arajev*", u8"Eur'o:p*",
    u8"Evr'o:p*", u8"Am'erik*", u8"H'rva:tsk*", u8"H'rva:t*",
    u8"S'rbij*", u8"s'rpsk*", u8"B^osn*", u8"b^osa:nsk*", u8"H/ercegovin*",
    u8"'Austrij*", u8"Austr'a:lij*", u8"C'rn*",
    // The adjectives of the counties and regions (Zagrebačka, Primorsko-
    // goranska, Ličko-senjska, Šibensko-kninska, Splitsko-dalmatinska,
    // Dubrovačko-neretvanska, Međimurska, Koprivničko-križevačka,
    // Karlovačka županija), HJP: zágrebačkī, kȃrlovačkī, kòprīvničkī,
    // kríževačkī, prímorskī, lȋčkī, knȋnskī, dalmàtīnskī, nerétvanskī,
    // međìmurskī
    u8"z/a:grebačk*", u8"k^a:rlovačk*", u8"k/oprivničk*", u8"kr/i:ževačk*", u8"pr/i:morsk*",
    u8"l^i:čk*", u8"kn^i:nsk*", u8"dalm/atinsk*", u8"ner/e:tvansk*", u8"međ/imursk*",
    // àustrījskī, aùstrālījskī (HJP): the rule for -ijski would take the
    // forms the stems above do not reach (austrijskog)
    u8"/austrijsk*", u8"a/ustralijsk*",

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
    u8"maš'i:n*", u8"kab'i:n|a|e|i|u|om|ama", u8"vitr'i:n*", u8"medic'i:n*", u8"vakc'i:n*",
    u8"benz'i:n*", u8"discipl'i:n*", u8"rut'i:n*", u8"kuž'i:n*", u8"terr'i:n*",

    // ---- Nouns in -ica with non-initial stress ----
    u8"učit'eljic*", u8"jed'inic*", u8"tipk'o:vnic*", u8"uči'onic*", u8"radi'onic*",
    u8"lub'enic*", u8"gol'ubic*", u8"kob'asic*", u8"prodav'aonic*", u8"spav'aonic*",
    u8"čit'aonic*", u8"bolnič'a:rk*",

    // ---- Frequent words the rules get wrong ----
    u8"ob'i:telj*", u8"kal'enda:r*", u8"svej'edno", u8"bic'ikl*", u8"gosp'ođic*",
    u8"tak'o:đer", u8"stan'o:vni*", u8"stan'o:vnik*", u8"sveuč'ilišn*",
    u8"iz'u:zetn*", u8"pojed'in*", u8"pojed'inac", u8"pojed'inc*", u8"ist'ovremen*",
    u8"vjer'ojatn*", u8"ver'ovatn*", u8"nar'avn*",
    u8"objašnj'e:nj*", u8"/obrazovanj*", u8"/obrazovn*", u8"/obrazov*", u8"/obrazuj*", u8"infor'ma:cij*",

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
    // nȅpregledan is an adjective of its own (vast; unreviewed), with the
    // first syllable in every form, unlike the negated participles
    // (nepròčitan, neprèslušan, nedòvršen; see StressRules::form_of);
    // the comparative is nepreglèdniji
    u8"n^epregledan*", u8"n^epregledn*", u8"nepregl'ednij*",
    // nȅpoznat the same (pòznat, but the negated adjective moves to ne-)
    u8"n^epoznat*",

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
    u8"za'uz|eti|mi|mimo|mite", u8"pron'a:đ|i|imo|ite",
    // zauzeti is the exception among them: the dictionaries' zȁuzēt (also
    // the adjective "busy"), zȁuzeo, zȁuzētōst on the prefix in every voice,
    // the verbal noun zauzéće (HJP, Školski rječnik); only the infinitive and
    // imperative keep zaùzēti, zaùzmi, and the plural adjective zȁuzēti
    // gives way to the infinitive
    u8"z^auzet*", u8"z^auz|eo|ela|elo|eli|ele", u8"zauz'e:ć*",
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
    // Zvȍnimīr in every case (the length after the accent is left out, as
    // in the other entries); the rule for loans in -ir would give Zvonimíra
    u8"Zv^onimir*",

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
    // pȍruka, prȅporuka, ȉsporuka: falling tone on the first syllable
    u8"p^oruk*", u8"p^oruci", u8"pr^eporuk*", u8"pr^eporuci", u8"^isporuk*", u8"^isporuci",
    // mȉkrofon, sȁksofon, mȅgafon keep the first syllable in every case
    u8"m^ikrofon*", u8"s^aksofon*", u8"m^egafon*",
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
    u8"marij/ana", u8"marij/ane", u8"marij/ani", u8"marij/anu", u8"marij/anom",
    u8"kristij'a:na", u8"kristij'a:ne", u8"kristij'a:ni", u8"kristij'a:nu", u8"kristij'a:nom",
    // Nàtaša like Mìrjana (the rule for -aš would give Natáša)
    u8"N/ataš*", u8"N/atašin*",
    u8"Daj'a:n*", u8"T/ijan*", u8"Dij'an*", u8"Mih'ovil*", u8"Muh'amed*", u8"Hus'ein*",
    u8"Ibr'a:him*", u8"Sul'ejman*", u8"Slob'odan*", u8"S/iniš*", u8"Mih'ael*",
    u8"elv'i:ra", u8"elv'i:re", u8"elv'i:ri", u8"elv'i:ru", u8"elv'i:rom",
    u8"Ant'o:nio", u8"Ant'o:nij*", u8"Ant'o:nia", u8"Ant'o:nie", u8"Ant'o:niu",
    u8"Ren'a:t*", u8"S^a:ndr*", u8"Natal'ij*", u8"Vikt'o:rij*", u8"D/anijel*", u8"D/aniel*",

    // ---- Surnames: exceptions to the -ović/-ević rule ----
    u8"'ivanović*", u8"j'osipović*", u8"m'aksimović*", u8"dr'agović*", u8"v'idović*",
    // Listener corrections: Ìsaković stresses the initial short i (a is
    // unstressed and short); Spàsojević stresses Spa, with short unstressed
    // o. Override the generic
    // surname rule in every language, including cases and possessives.
    u8"/Isaković||a|u|em|i|e|ima", u8"/Isakovićev*",
    u8"Sp/asojević||a|u|em|i|e|ima", u8"Sp/asojevićev*",
    // HJP onomastics under knez: Knȇzović and Knéžević, both long initial e.
    // Full surname stems avoid changing the noun plurals knȅzovi / knéževi.
    u8"Kn^e:zović||a|u|em|i|e|ima", u8"Kn^e:zovićev*",
    u8"Kn/e:žević||a|u|em|i|e|ima", u8"Kn/e:ževićev*",

    // ---- More places ----
    // Varàždin, Vukòvār and Kòprīvnica are the second forms of HJP (Vȁraždīn,
    // Vȕkovār); Bjȅlovār, Virovìtica, Ȍgulīn, Ivánec, Slàvōnija and Dàlmācija
    // as HJP has them. The place-name table (formant_proper_names.inc) has these
    // and some four thousand more with their case forms; the entries here
    // also serve lowercase text and the derived words their stems reach.
    // (visok* is the adjective vìsok, visòka, visòko; the town Vìsokō is in
    // the place-name table.)
    u8"Var'aždin*", u8"Kr'agujevac", u8"Kr'agujevc*", u8"Mak'edo:nij*", u8"At'e:n*",
    u8"Kopenh'a:gen*", u8"Vuk'ova:r*", u8"Bj^elovar*", u8"Virov/itic*", u8"Crikv'enic*",
    u8"^Ogulin*", u8"Iv/a:nec", u8"Iv/a:nc*", u8"S'ubotic*", u8"P'odgoric*", u8"K'oprivnic*",
    u8"Prij'e:dor*", u8"Vis'ok*", u8"Sl/avonij*", u8"D/almacij*", u8"Mad'ri:d*",
    u8"Berl'i:n*", u8"Par'i:z*", u8"Lond'o:n*", u8"Ljublj'an*", u8"Beogr'a:đan*",
    u8"Zagrepč'an*", u8"Spl'ićan*", u8"Riječ'an*", u8"Sarajl'ij*",

    // ---- Food, places in town, everyday loans ----
    u8"rest'ora:n*", u8"aer'odrom*", u8"apot'e:k*", u8"ljek'a:rn*", u8"trg'ovin*",
    u8"kupa'onic*", u8"bibliot'e:k*", u8"ban'a:n*", u8"čokol'a:d*", u8"sal'a:t*",
    u8"par'adajz*", u8"fak'ulte:t*", u8"sveuč'ilišt*", u8"ćev'a:p*",

    // ---- Words shaped like the loans with a long last stem syllable
    // (StressRules::long_stem) that keep their native accent ----
    // zákon, nágrada; spȍmenīk, zàmjenīk, ùdžbenīk, zlòčin next to zločínac;
    // vòjnīk but vojníka; mȅnadžer, hȁmburger
    u8"z/a:kon*", u8"n/a:grad*", u8"sp^omenik*", u8"z/amjenik*", u8"'udžbenik*",
    u8"zl/očin*", u8"zloč'inc*", u8"zloč'inac", u8"v/ojnik", u8"v/ojniče",
    u8"vojn'i:|ka|ku|kom|ci|cima|ke", u8"m^enadžer*", u8"h^amburger*",
    u8"čvrst'in*", u8"četvrt'in*", u8"pol'ovic*", u8"činj'enic*", u8"b'aterij*",
    u8"b'aterijsk*",
    u8"/ogledal*", u8"četvrt'i:nk*", u8"Dalmat'i:nk*", u8"b/ogat*", u8"pr^evar*", u8"s/udar*",
    u8"dom'aćin*", u8"dom'aćic*", u8"B/alkan", u8"Balk'a:n|a|u|om|e|i|ima", u8"balk'a:nsk*",
    // Three-syllable nouns in -ina (the rule needs four syllables, see
    // StressRules::strong): abstract nouns and loans with a long i
    u8"ravn'in*", u8"niz'in*", u8"treć'in*", u8"osm'in*", u8"pet'in*", u8"šest'in*",
    u8"sedm'in*", u8"devet'in*", u8"čist'in*", u8"led'in*", u8"vrl'in*", u8"tuđ'in*",
    u8"crn'in*", u8"bjel'in*", u8"bel'in*", u8"modr'in*", u8"vedr'in*", u8"puč'in*",
    u8"turb'i:n*", u8"doktr'i:n*", u8"kant'i:n*", u8"delf'i:n*", u8"dup'i:n*", u8"pingv'i:n*",
    u8"term'i:n|a|u|om|e|i|ima",
    // ùmire (umrijeti), not umíre (umiriti) and not papíre
    u8"'umir|e|em|eš|emo|ete|u|ući", u8"'izumir|e|em|eš|emo|ete|u|ući",
    // zȅmlja against zemljáka, bolèsnīk, bolesníka
    u8"z/emljak", u8"zemlj'a:|ka|ku|kom|ci|cima|ke|če|kov*", u8"bol'esnik", u8"bol'esniče",
    u8"bolesn'i:|ka|ku|kom|ci|cima|ke", u8"bol'esnic*",
    // Verbs in -ovati that keep the first syllable against the rule for
    // -ovati (vjȅrovati, vȅrovati, rȁdovati se, mìlovati, ljȅtovati), with
    // their present (vjȅrujēm, ljȅtujēm), verbal noun (vjȅrovānje, rȁdovānje,
    // ljȅtovānje) and the fused future forms the stems do not reach
    u8"vj^erov*", u8"v^erov*", u8"r^adov*", u8"m/ilov*", u8"lj^etov*", u8"l^etov*",
    u8"vj^eruj*", u8"v^eruj*", u8"lj^etuj*", u8"l^etuj*",
    u8"vj^erovanj*", u8"v^erovanj*", u8"r^adovanj*", u8"lj^etovanj*", u8"l^etovanj*",
    u8"vj^erov|aćeš|aćemo|aćete|avši", u8"v^erov|aćeš|aćemo|aćete|avši",
    u8"r^adov|aćeš|aćemo|aćete|avši", u8"m/ilov|aćeš|aćemo|aćete|avši",
    u8"lj^etov|aćeš|aćemo|aćete|avši", u8"l^etov|aćeš|aćemo|aćete|avši",

    // ---- Words checked in October 2026 against Školski rječnik
    // hrvatskoga jezika, Hrvatski mrežni rječnik, HJP and Wiktionary, with
    // their whole paradigms (the lengths after the accent are in the
    // Serbian and Bosnian tables) ----
    // The personal pronouns: jȃ, tȋ, ȏn, mȋ, vȋ, òni, òna, òno, òne and
    // their long forms; the clitics (me, ti, ga, nas, ...) stay unstressed
    u8"j^a:", u8"t^i:", u8"^o:n", u8"m^i:", u8"v^i:", u8"/oni", u8"/ona", u8"/ono", u8"/one",
    u8"mn^o:m", u8"nj^i:m", u8"nj^e:", u8"nj^o:j", u8"nj^u:", u8"n^a:s", u8"v^a:s",
    u8"nj^i:h", u8"nj^ima", u8"m^ene", u8"m^eni", u8"t^ebe", u8"t^ebi", u8"t^obom",
    u8"nj^ega", u8"nj^emu", u8"n^ama", u8"v^ama",
    // zȁjednički; vȋd, vȋda, u vídu, vȉdovi; iskústvo, ìskūstāvā; àlāt,
    // aláta; mòdel, modèla; progràmērskī; èkrān, ekrána; záslon; znánje;
    // modèrātor, modèrātorica; surádnja (Školski rječnik; HJP suràdnja),
    // saràdnja; sùdjelovati, ùčestvovati (the present sùdjelujēm,
    // ùčestvujēm with the verbs in -ovati below)
    u8"z^ajedničk*", u8"v^i:d", u8"v^i:da", u8"v^i:dom", u8"v/i:du", u8"v^idov|i|a|e|ima",
    u8"isk'u:stv*", u8"/iskustava", u8"/alat", u8"al'a:t|a|u|om|i|e|ima",
    u8"m/odel", u8"mod/el|a|u|om|i|e|ima", u8"progr/amersk*", u8"/ekran",
    u8"ekr'a:n|a|u|om|i|e|ima", u8"z/a:slon*", u8"zn/a:nj*", u8"mod/erator*",
    u8"mod/eratoric*", u8"mod/eratork*", u8"sur'a:dnj*", u8"sar/adnj*", u8"s/udjelov*",
    u8"s/udjelovanj*", u8"s/udelov*", u8"s/udeluj*", u8"s/udelovanj*", u8"/učestvov*",
    u8"/učestvuj*", u8"/učestvovanj*",
    // razvíjajū against the other persons (ràzvījām); razvìjen from
    // ràzviti; pòtaći, pòtakao, pòdstaći
    // pòtpun(o), potpùnijī; mȁslina, mȁslinov (Školski rječnik; HJP and the
    // Serbian dictionary also mà-, in the Serbian and Bosnian tables);
    // oáza; rakèta; krȃlj, králja, králjevi, králjevina; ùistinu;
    // higijéna; jȃvnī, jȃvno, jàvnijī; srȅdnjī; ekípa; žèljezničār,
    // žèljeznica, žèljeznīčkī; streljáštvo; dìrektan, dìrektno, dirèktnijī;
    // drȃg, drága, drȃgo, drȁžī (nȃjdražī by the rule for naj-);
    // međunárodnī; odr̀živ, odr̀živōst;
    // kampànja (HJP and Wiktionary; also kàmpānja)
    u8"p/otpun*", u8"potp/unij*", u8"m^aslin*", u8"m^aslinov*", u8"o/a:z*", u8"rak/et*",
    u8"kr^a:lj", u8"kr/a:lj|a|u|em|om", u8"kr/a:ljev*", u8"/uistinu", u8"/uistini",
    u8"higij'e:n|a|e|i|u|o|om|ama", u8"j^a:vn*", u8"j^a:van", u8"j/avnij*", u8"sr^ednj*",
    u8"ek'i:p*", u8"ž/eljezničar*", u8"ž/eljeznic*", u8"ž/eljezničk*", u8"ž/elezničar*",
    u8"ž/eleznic*", u8"ž/elezničk*", u8"strelj'a:štv*", u8"d/irektan", u8"d/irektn*",
    u8"dir/ektnij*", u8"dr^a:g", u8"dr/a:ga", u8"dr^a:gi", u8"dr^a:go",
    u8"dr^až|i|a|e|eg|em|oj|ih|im", u8"međun'a:rodn*", u8"međun'a:rodan",
    // Superlative adverbs outside the rule for naj- (StressRules::strong):
    // nȃjposlije, nȃjposlē, nȃjprē, nȁjzad; pònājprije, pònājviše
    u8"n^a:jposlije", u8"n^a:jposle", u8"n^a:jpre", u8"n^ajzad", u8"p/onajprije",
    u8"p/onajviše",
    u8"od/rživ*", u8"održ/ivij*", u8"kamp/anj*",
    // The nouns in -ost and the adjectives beside them, whose stems would
    // otherwise end the entries above after three letters (jȃvnī but
    // jávnōst, javnosti; pòtpunōst; odr̀živōst) or take the noun's accent
    // (iskùstven against iskústvo); srȅdnjoškolskī like srȅdnjī
    u8"j/a:vnost*", u8"j/a:vnošću", u8"p/otpunost*", u8"p/otpunošću", u8"od/rživost*",
    u8"od/rživošću", u8"isk/ustven*", u8"sr^ednjoškolsk*",
    // protéza in every form, protètika, protètičār; zȗbnī, òčnī (zubna,
    // očna proteza)
    u8"prot/e:z*", u8"prot/etik*", u8"prot/etičar*", u8"z^u:bn*", u8"/očn*",
    // inspirírati with the dictionaries' shifts (inspìrīrām, inspìrīrān,
    // inspirírajū), Serbian inspìrisati, inspìrišēm, inspìrisan
    u8"inspir'i:raju", u8"inspir'i:rajući",
    u8"razv'i:jaju", u8"razv'i:jajući", u8"razv/ijen*", u8"p/otaći", u8"p/otakao",
    u8"p/otakl|a|o|i|e", u8"p/odstaći", u8"p/odstakao", u8"p/odstakl|a|o|i|e",
    // Names (HJP, Wiktionary): Sȃndra, Sìniša, Dànijel(a), Dàniel(a),
    // Mìhājlo, Ànita, Ȇva, Mája, Máto (from Máte; no stem, mati and matu are
    // words), Mátić, Ìvanović, Aleksándar, Teodóra (the man's name is
    // Tèodor; "Teodora" is read as hers), Crnògorac with Crnogórci,
    // Ivànščica, bȑkljača. Without a dictionary entry, from names and words
    // of the same shape: Nàida, Adrijàna like
    // Dijàna and Marijàna, Mȉlosava like Mȉroslava, Márkovina from Mȃrko like
    // králjevina, Prȗgovečkī from Prȗgovac, Vládić from Vládo,
    // Tȉhić, Rȍtić, Lȅtić, Jȕsić
    u8"M/ihajl*", u8"/Anit*", u8"^E:v|a|i|u|om", u8"M/a:j|a|e|i|u|o|om", u8"M/a:to",
    u8"M/a:tić*", u8"Teod/o:r|a|e|i|u|o|om", u8"T/eodor", u8"Crn/ogorac",
    u8"Crn/ogorc|a|u|em", u8"Crn/ogorče", u8"Crnog'o:rc|i|ima|e", u8"Crn/ogoraca",
    u8"Crn/ogork*", u8"crn/ogorsk*", u8"Iv/anščic*", u8"Iv/ančic*", u8"B^rkljač*",
    // Enída: the listener's long stressed i replaces the earlier analogy
    // Ènida; the possessive needs its own stem for forms such as Enídinima.
    u8"En/i:d|a|e|i|u|o|om|ama", u8"En/i:din*",
    u8"N/aid*", u8"Adrij/an|a|e|i|u|o|om", u8"M^ilosav*", u8"M/a:rkovin*",
    // Listener correction: gre-BLIČ-ki, retaining the short vowel.
    u8"Pr^u:govečk*", u8"Grebl/ičk*", u8"Vl/a:dić*", u8"T^ihić*", u8"R^otić*",
    u8"L^etić*", u8"J^usić*",

    // Foreign app names after internal.json's pronunciation replacements.
    // Collins/Oxford: initial stress in Messenger and TikTok; falling tone
    // is our Croatian adaptation, not an English lexical pitch accent.
    // WinTalker: the user's vin-TO-ker, with short rising o.
    u8"m^esindžer||a|u|e|om", u8"t^iktok||a|u|e|om",
    u8"vint/oker||a|u|e|om",

    // ---- The listener's third October 2026 list (prijateljstvo,
    // nevidljiv, potencijal, snalaženje, simulacijski, doživjeti,
    // poštovanje, interesantno) and the families of these words, checked
    // against HJP, Školski rječnik, Mrežnik, Wiktionary, Rečnik Matice srpske
    // and Alić's Bosnian accent handbook. The classes behind them are rules
    // (StressRules: -teljstvo, -ljiv, -laziti, -antan/-entan, -ijski);
    // the lengths after the accent are in the Serbian and Bosnian tables ----
    // prȉjatelj and nèprijatelj in every case (Bosnian prìjatelj, in its
    // table); their derivatives have the accent of -telj-: prijatèljica,
    // prijatèljskī, neprijatèljica, neprijatèljskī, and prijatèljstvo by the
    // rule for -teljstvo, except the G pl neprijàtēljstāvā (HJP, Wiktionary)
    u8"pr^ijatelj*", u8"prijat/eljic*", u8"prijat/eljič*", u8"prijat/eljsk*",
    u8"prijat/eljuj*",
    u8"n/eprijatelj*", u8"neprijat/eljic*", u8"neprijat/eljsk*", u8"neprij/ateljstava",
    // potencìjāl by the rule for -al, potencijála in the other cases, like
    // the other nouns in -ijal (materìjāl, materijála; serìjāl, memorìjāl,
    // tutorìjāl, ceremonìjāl); the adjective pȍtencijālan, pȍtencijālno in
    // every form, but potencijálnōst
    u8"potencij'a:l|a|u|om|e|i|ima", u8"materij'a:l|a|u|om|e|i|ima",
    u8"serij'a:l|a|u|om|e|i|ima", u8"memorij'a:l|a|u|om|e|i|ima",
    u8"tutorij'a:l|a|u|om|e|i|ima", u8"ceremonij'a:l|a|u|om|e|i|ima",
    u8"p^otencijal|an|na|no|ni|ne|nu|nog|nom|noj|nih|nim|noga|nome|nomu|nima",
    u8"potencij'a:lno:st", u8"potencij'a:lnost*", u8"potencij'a:lnošću",
    // snȃći, snȃđēm, but snáđi, snáđoh and snàšao (Školski rječnik; HJP has
    // snȁšao); snàlaziti and snàlažēnje come from the rule for -laziti,
    // snalàžljiv from the rule for -ljiv
    u8"sn^a:ći", u8"sn^a:đ|em|eš|e|emo|ete|u", u8"sn/a:đ|i|imo|ite|oh|osmo|oste|oše",
    u8"sn/aš|ao|la|lo|li|le|avši",
    // simùlātor, simùlātorskī (HJP, Rečnik Matice srpske) against the rule
    // for -ator; simulácija and simulácījskī are the rule for -ij
    u8"sim/ulator*", u8"sim/ulatorsk*",
    // dožívjeti, dožívio, dožívjela, imperative dožívi, dožívite. The
    // Croatian dictionaries move the present to the prefix (dòžīvīm),
    // Rečnik Matice srpske and the Bosnian dictionary keep it (dožívīm), and
    // so does the Croatian voice, as with the verbs in VERBS. The passive
    // participle is dòžīvljen in all of them, the noun dȍživljāj. The same
    // for preživjeti, proživjeti, nadživjeti, poživjeti, and for žívjeti.
    u8"dož'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    u8"dož'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    u8"prež'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    u8"prež'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    u8"prož'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    u8"prož'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    u8"nadž'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    u8"nadž'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    u8"pož'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    u8"pož'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    u8"ž/i:v|jeti|jet|jela|jelo|jeli|jele|jeh|jesmo|jeste|ješe|jevši|io|im|iš|imo|ite|jeću|jećeš|jeće|jećemo|jećete",
    u8"ž/i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    // The participle as an adjective (prežívjelī, broj prežívjelīh): its
    // declension falls back to these stems
    u8"dož'i:vjel*", u8"dož'i:vel*", u8"prež'i:vjel*", u8"prež'i:vel*", u8"prož'i:vjel*",
    u8"prož'i:vel*", u8"nadž'i:vjel*", u8"nadž'i:vel*", u8"pož'i:vjel*", u8"pož'i:vel*",
    u8"ž/i:vjel*", u8"ž/i:vel*",
    // dòžīvljen, but the nouns preživljénje, doživljénje
    u8"d/oživljen*", u8"pr/eživljen*", u8"pr/oživljen*", u8"n/adživljen*",
    u8"doživlj'e:nj*", u8"preživlj'e:nj*", u8"proživlj'e:nj*", u8"nadživlj'e:nj*",
    u8"d^oživljaj*", u8"doživlj'a:jno:st", u8"doživlj'a:jnost*",
    // poštovánje, the noun (HJP, Školski rječnik, Rečnik Matice srpske),
    // nepoštovánje, samopoštovánje; the verb poštòvati, pòštujēm by the
    // rules for -ovati and -ujem; its participle pȍštovān, pȍštovānī;
    // poštòvatelj, poštòvalac
    u8"poštov'a:nj*", u8"nepoštov'a:nj*", u8"samopoštov'a:nj*", u8"p^oštovan*",
    u8"pošt/ovatelj*", u8"poštovat/eljic*", u8"pošt/ovalac", u8"pošt/ovalaca",
    u8"pošt/ovaoc*", u8"p/oštuj", u8"pošt/ova",
    // ȉnteres and ȉnterēsnī in every form (an exact paradigm: a stem would
    // take interesira); interesàntan is the rule for -antan, interesírati
    // the rule for -irati. The Serbian and Bosnian ȉnteresovati,
    // ȉnteresujēm, zàinteresovati, zàinteresujēm, nezàinteresovān (Rečnik
    // Matice srpske) against the rules for -ovati and -ujem
    u8"^interes||a|u|e|om|i|ima|ni|na|no|ne|nu|nog|nom|noj|nih|nim|nima|noga|nome|nomu",
    u8"^interesov*", u8"^interesovanj*", u8"^interesuj*", u8"^interesujući",
    // The adjectives of nouns in -ant, -ent keep the noun's accent against
    // the rule for -antan (fòrmant, fòrmantnī; pàtentnī, gàrantnī,
    // dijàmantan, cèmentnī, sègmentan, pìgmentnī, tàngentnī, prézentnī in
    // HJP and Školski rječnik), and so does mòmēntan
    u8"f/ormant*", u8"f/ormantn*", u8"p/atentn*", u8"g/arantn*", u8"dij/amant*",
    u8"dij/amantn*", u8"c/ementn*", u8"s/egmentan", u8"s/egmentn*", u8"p/igmentn*",
    u8"t/angentn*", u8"m/omentan", u8"m/omentn*", u8"pr/e:zentn*",
    u8"z/ainteresov*", u8"z/ainteresovan*", u8"z/ainteresovanost*", u8"z/ainteresuj*",
    u8"z/ainteresujući", u8"nez/ainteresovan*", u8"nez/ainteresovanost*",
    u8"^interesov|aćeš|aćemo|aćete|avši", u8"z/ainteresov|aćeš|aćemo|aćete|avši",

    // ---- The fourth October 2026 list (HJP, Školski rječnik, Wiktionary,
    // Rečnik Matice srpske; all agree) ----
    // Ègipat, Ègipta in every case against the rule for -at; ègipatskī,
    // Ègipćanin, Ègipćani, Ègipćānka
    u8"/egip*", u8"/egipatsk*", u8"/egipćan*", u8"/egipćank*",
    // proizvòdnja in every case; proìzvod, proìzvoda (an exact paradigm: a
    // stem would take proizvòditi, proizvòdio). The adjective stays on the
    // first syllable as in HJP (prȍizvodnī). The verbs are in VERBS, except
    // proìzvesti, proìzveo and the verbal noun proìzvođēnje
    u8"proizv'odnj*", u8"pro'izvod||a|u|om|e|i|ima|eći",
    u8"pro'izve|sti|st|o|la|lo|li|le|vši|šću|šćeš|šće|šćemo|šćete", u8"pro'izvođenj*",
    // vòdīč, but vodíča, vodíči in the other cases; poluvòdīč the same
    u8"v/odič", u8"vod'i:č|a|u|em|i|e|ima", u8"poluv'odič", u8"poluvod'i:č|a|u|em|i|e|ima",
    // nȁpomena in every case. nȁpomene, nȁpomenu and nȁpomeni are also
    // spelled like forms of napoménuti (VERBS): the noun has them, except
    // the imperative at the head of a clause (Napoméni mu ...)
    u8"n^apomen|a|e|i|u|o|om|ama",

    // ---- The fifth October 2026 list (Školski rječnik, HJP, Hrvatski
    // mrežni rječnik, Wiktionary, Rečnik Matice srpske; all agree). The
    // verbs' other forms are in VERBS ----
    // ȍdabīr in every case (an exact paradigm: a stem would take
    // odàbirati); "odabire" and "odabiru" are also forms of odàbirati, the
    // noun has them. odàbrati, odàberēm, odabèri (VERBS), but ȍdabrao,
    // ȍdabrān on the first syllable and the aorist odàbrah
    u8"^odabir||a|u|om|i|e|ima",
    u8"^odabra|o|la|lo|li|le", u8"^odabran||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"od/abr|a|ah|asmo|aste|aše|avši",
    // asìstent, asìstenta in every case, the genitive plural asìstenātā
    // against the rule for -at; asìstentica (HJP also asistȅnt, asistȅntica)
    u8"as/istenata", u8"as/istentic|a|e|i|u|o|om|ama",
    // ȕčenīk, ȕčenīka, ȕčenīci: the nouns in -nik of three syllables keep
    // the first syllable (StressRules::long_stem), these set its falling
    // tone; ȕčenica, ȕčeničkī
    u8"^učeni|k|ka|ku|kom|ke|ci|cima|če", u8"^učenic|a|e|u|om|ama", u8"^učeničk*",
    // ...but ugljènīk, ugljeníka (Rečnik Matice srpske) moves like the
    // longer ones
    u8"uglj/enik", u8"ugljen'i:|ka|ku|kom|ke|ci|cima",
    // ȉzglēd, ȉzglēda in every case. "izgleda" is also the third person of
    // izglédati (ìzglēdā): the noun has it. The other persons and the
    // imperative are ìzglēdām, ìzglēdāj; the rest of the verb keeps
    // izgléd- (above)
    u8"^izgled||a|u|om|i|e|ima", u8"/izgled|am|aš|amo|ate|aj|ajmo|ajte",
    u8"izgl'e:dajući",
    // žìvot, but živòta, živòte in the other cases; žìvotnī; živòtinja
    u8"ž/ivot", u8"živ/ot|a|u|om|i|e|ima", u8"ž/ivotan", u8"ž/ivotn*",
    u8"ž/ivotn|oga|ome|omu|ima", u8"živ/otinj*", u8"živ/otinjama", u8"živ/otinjsk*",
    // spávati, némati (VERBS), but the present and the imperative spȃvām,
    // spȃvā, spȃvāj, nȇmām, nȇmā, nȇmāj have the falling accent
    u8"sp^a:v|am|aš|a|amo|ate|aj|ajmo|ajte", u8"n^e:m|am|aš|a|amo|ate|aj|ajmo|ajte",
    // líce in every case
    u8"l/i:c|e|a|u|em|ima",
    // krèdīt, but kredíta in the other cases; krèdītnī
    u8"kr/edit", u8"kred'i:t|a|u|om|i|e|ima", u8"kr/editn*",
    // stȃvka, stȃvci, stȁvākā
    u8"st^a:vk|a|e|i|u|o|om|ama", u8"st^a:vci", u8"st^avaka",
    // krétati, kréći, krénuti, kréni (VERBS), but the present krȇćēm,
    // krȇćēte, krȇnēm, krȇnē
    u8"kr^e:ć|em|eš|e|emo|ete|u", u8"kr^e:n|em|eš|e|emo|ete|u",
    // dokumentárac, dokumentárca, dokumèntārācā (dȍkumentārnī keeps the
    // first syllable)
    u8"dokument'a:r|ac|ca|cu|cem|ci|ce|cima", u8"dokum/entaraca",
    // vlȃst, vlȃsti, vlȃšću, vlástima; the locative vlásti after a
    // preposition is a rule of the front end
    u8"vl^a:st||i", u8"vl^a:šću", u8"vl/a:stima",
    // nòvac, nóvca, nóvcem; nȏvci, nȍvācā
    u8"n/ovac", u8"n/o:vc|a|u|em", u8"n^o:vc|i|e|ima", u8"n^ovaca",
    // sústav, sústāvan, sústavno, but sustàvnijī
    u8"s/u:stav*", u8"s/u:stavn*", u8"sust/avnij*",
    // rádnja in every case
    u8"r/a:dnj|a|e|i|u|o|om|ama",
    // dȃr, dȃra, dȁrovi, dȁrōvā; the locative dáru after a preposition is a
    // rule of the front end
    u8"d^a:r||a|u|om|em|e", u8"d^arov|i|a|e|ima",
    // vážiti, vážio (VERBS), but the present vȃžīm, vȃžī, vȃžē; vážēćī
    u8"v^a:ž|im|iš|i|imo|ite|e", u8"v/a:žeć*",

    // ---- The sixth October 2026 list (Školski rječnik, HJP, Hrvatski
    // mrežni rječnik, Wiktionary, Rečnik Matice srpske, Alić's Bosnian
    // handbook). karijéra, premìjer, premijéra and the other loans in -ijer
    // are a rule (is_ije_diphthong, StressRules::long_rhyme), except
    // prȅmijērnī, hotelìjērskī, hijèrātskī, hijeròglīf, arhijèrej; zanímati, odmòriti and obèležiti are in
    // VERBS. The comparatives keep the accent of the rule for -ij against
    // the stems below (jedinstvènijī, zaobilàznijī, kompromìsnijī) ----
    u8"pr^emijern*", u8"hotel/ijersk*", u8"hotel/ijerstv*", u8"hij/eratsk*",
    u8"hijer/oglif*", u8"arhij/erej*",
    u8"jedinstv/enij*", u8"zaobil/aznij*", u8"nezaobil/aznij*", u8"komprom/isnij*",
    u8"beskomprom/isnij*", u8"neukrot/ivij*",
    // ùmjetnīk, ùmjetnica, ùmjetnōst, ùmjetničkī, ùmetnīk, ùmetnōst in every
    // form (not ùmjetan, umjetnína, which these stems do not reach)
    u8"/umjetnik*", u8"/umjetnic*", u8"/umjetniče", u8"/umjetničk*", u8"/umjetnost*",
    u8"/umjetnošću", u8"/umetnik*", u8"/umetnic*", u8"/umetniče", u8"/umetničk*",
    u8"/umetnost*", u8"/umetnošću",
    // àlbūm, but albúma in the other cases
    u8"/album", u8"alb'u:m|a|u|om|e|i|ima",
    // jedìnstven, jedìnstvenōst, but jedínstvo (HJP)
    u8"jed/instven*", u8"jed/instvenost*", u8"jed/i:nstv|o|a|u|om|ima",
    // kompròmis, kompròmisan (HJP, Rečnik Matice srpske, Wiktionary;
    // Školski rječnik kȍmpromis) and bèskompromisan (Školski rječnik,
    // Wiktionary; HJP and Rečnik Matice srpske bȅ-, in the Serbian and
    // Bosnian tables)
    u8"kompr/omis*", u8"kompr/omisn*", u8"b/eskompromis*", u8"b/eskompromisn*",
    // plȃćenīk, plȃćenica, plȃćeničkī in every form, also as written with č
    u8"pl^a:ćenik*", u8"pl^a:ćenic*", u8"pl^a:ćeniče", u8"pl^a:ćeničk*",
    u8"pl^a:čenik*", u8"pl^a:čenic*", u8"pl^a:čeniče",
    // pȍvijēst, pȍvijesnī: the falling accent keeps the long ije after it
    // unstressed
    u8"p^ovijest*", u8"p^oviješću", u8"p^ovijesn*",
    // ostávština (HJP, Rečnik Matice srpske; Školski rječnik òstāvština)
    u8"ost'a:vštin*",
    // odijélo, odélo in every case, and "odjelo" as some write it (not
    // odjela, odjelu: the cases of òdjel)
    u8"odij'e:l|o|a|u|om|ima", u8"od'e:l|o|a|u|om|ima", u8"odj'e:lo",
    // gubítak, gubíci, gùbītākā (HJP, Školski rječnik, Rečnik Matice
    // srpske), and the nouns in -itak, -utak of the same pattern in HJP:
    // dobítak, užítak, primítak, odbítak, razvítak, boljítak, zgodítak,
    // probítak, imútak, osnútak, privítak (the other cases are the rule for
    // -itk-)
    u8"gub'i:|tak|ci|cima|tci|tcima", u8"g/ubitaka",
    u8"dob'i:|tak|ci|cima|tci|tcima", u8"d/obitaka",
    u8"už'i:|tak|ci|cima|tci|tcima", u8"/užitaka",
    u8"prim'i:|tak|ci|cima|tci|tcima", u8"pr/imitaka",
    u8"odb'i:|tak|ci|cima|tci|tcima", u8"/odbitaka",
    u8"razv'i:|tak|ci|cima|tci|tcima", u8"r/azvitaka",
    u8"bolj'i:|tak|ci|cima|tci|tcima", u8"b/oljitaka",
    u8"zgod'i:|tak|ci|cima|tci|tcima", u8"zg/oditaka",
    u8"prob'i:|tak|ci|cima|tci|tcima", u8"pr/obitaka",
    u8"im'u:|tak|ci|cima|tci|tcima", u8"/imutaka",
    u8"osn'u:|tak|ci|cima|tci|tcima", u8"/osnutaka",
    // privítak, privíci, prìvītākā (HJP, Školski rječnik, Wiktionary); prìvijen
    u8"priv'i:|tak|ci|cima|tci|tcima", u8"pr/ivitaka", u8"pr/ivijen*",
    // debìtant, debìtantica, debìtantskī in every form, against the rules
    // for -ant
    u8"deb/itant*", u8"deb/itantic*", u8"deb/itantsk*", u8"deb/itantkinj*",
    // neukròtiv in every form
    u8"neukr/otiv*", u8"neukr/otivost*",
    // ljúbav, ljúbāvnī, ljúbāvnīk in every form
    u8"lj/u:bav*", u8"lj/u:bavn*", u8"lj/u:bavnik*", u8"lj/u:bavnic*",
    // dèbīl, but debíla in the other cases
    u8"d/ebil", u8"deb'i:l|a|u|om|e|i|ima",
    // izdánje in every case
    u8"izd'a:nj*",
    // nezaobìlazan, zaobìlazan in every form (Rečnik Matice srpske
    // nȅzaobilāzan, in the Serbian and Bosnian tables)
    u8"nezaob/ilaz*", u8"nezaob/ilazn*", u8"zaob/ilazan", u8"zaob/ilazn*", u8"zaob/ilaznic*",
    // bòja in every case; "boji", "boje" are also forms of bòjiti and
    // bȍjati se, the noun has them
    u8"b/oj|a|e|i|u|om|ama",
    // tȇško, tȇškī, but téžak, téška (HJP, Rečnik Matice srpske,
    // Wiktionary; Školski rječnik tȇžak); tȅžē, tȅžī. Not a stem: tèškoća
    u8"t^e:šk|o|i|og|oga|om|ome|omu|oj|ih|im|ima", u8"t/e:šk|a|e|u", u8"t/e:žak",
    u8"t^ež|e|i|a|u|eg|em|oj|ih|im|ega|emu|ima",
    // òdmor in every case; "odmori" is the imperative of odmòriti (VERBS)
    // only at the head of a clause
    u8"/odmor||a|u|om|i|e|ima",
    // bȏl, bȏla, bȏli, bȍlovi, bólima (Školski rječnik); the locatives bólu,
    // bóli after a preposition are a rule of the front end. "boli" is also
    // bòlī (it hurts): the noun has it. "bolju" is left to the comparative
    u8"b^o:l||a|i|om|u", u8"b/o:lima", u8"b^olov|i|a|e|ima",
    // bȏlan, bȏlno, bȏlnī, but bólna and bòlnijī (not a stem: bólnica)
    u8"b^o:lan", u8"b^o:ln|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", u8"b/o:lna",
    u8"b/olnij*",
    // neprikosnòven in every form, also as it is often misspelled
    u8"neprikosn/oven*", u8"neprekosn/oven*",
    // kr̀hotina (HJP, Wiktionary; Školski rječnik kȑhotina), against the
    // rule for -ina
    u8"k/rhotin*",
    // túga, túzi, tȗgo; túžan, túžna, but tȗžno, tȗžnī and tùžnijī
    u8"t/u:g|a|e|u|om|ama", u8"t/u:zi", u8"t^u:go",
    u8"t/u:žan", u8"t/u:žna", u8"t^u:žn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"t/užnij*",
    // dùga, the adjective (duga noć, dugog: dùga), far commoner than the
    // rainbow dúga and the debt's genitive dȗga
    u8"d/uga",
    // vážan, vážna, vážnōst, but vȃžno, vȃžnī and vàžnijī
    u8"v/a:žan", u8"v/a:žna", u8"v^a:žn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"v/ažnij*", u8"v/a:žnost*", u8"v/a:žnošću",
    // lȁžan, lȁžno, lȁžnī, lȁžnōst, but làžna and làžnijī; lȃž, lȁži,
    // làžima
    u8"l^ažan", u8"l/ažna", u8"l^ažn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"l/ažnij*", u8"l^ažnost*", u8"l^ažnošću", u8"l^a:ž", u8"l^až|i|ju", u8"l/ažima",
    // rúža in every case; not a stem: rȗžan, rȗžno
    u8"r/u:ž|a|e|i|u|om|ama",

    // ---- The seventh October 2026 list (HJP, Školski rječnik) ----
    // nȉkad, nȉkada, ȉkad, ȉkada, nȅkad, nȅkada, kȁtkad, against the rule
    // for loans in -ad (nikáda); òtkad, dòkad, dòsad, pònekad, kadìkad with
    // their forms in -a, svȁgda ("dosada" is also the noun dȍsada, left
    // as it was). The verbal nouns in -ánje (poboljšánje) are a rule
    // (StressRules::form_of), reproducírati is in VERBS
    u8"n^ikad", u8"n^ikada", u8"^ikad", u8"^ikada", u8"n^ekad", u8"n^ekada", u8"k^atkad",
    u8"k^atkada", u8"/otkad", u8"/otkada", u8"d/okad", u8"d/okada", u8"d/osad",
    u8"p/onekad", u8"p/onekada", u8"kad/ikad", u8"kad/ikada", u8"sv^agda",
    // rȅproduktīvan, rȅproduktīvnī
    u8"r^eproduktivan", u8"r^eproduktivn*",

    // ---- The eighth October 2026 list (HJP, Školski rječnik, Wiktionary,
    // Rečnik Matice srpske) ----
    // kȃrta, kȃrte in every case, G pl kȁrātā (the rule for loans in -at
    // had karáta); kàrtica. Not stems: kàrtōn, kàrtel
    u8"k^a:rt|a|e|i|u|o|om|ama", u8"k^arata", u8"k/artic|a|e|i|u|o|om|ama",
    // zdrȃvlje in every case; zdràvstvo, zdràvstvenī
    u8"zdr^a:vlj|e|a|u|em", u8"zdr/avstv|o|a|u|om", u8"zdr/avstven*",

    // ---- The ninth October 2026 list (HJP, Školski rječnik, Wiktionary,
    // Rečnik Matice srpske) ----
    // rázličit, rázličitōst in every form, but različìtijī
    u8"r/a:zličit*", u8"r/a:zličitost*", u8"r/a:zličitošću", u8"različ/itij*",
    // strána, stránē, stránama, G pl stránā; A strȃnu, N pl strȃne, D
    // strȃni (L stráni after a preposition is a rule of the front end).
    // "strane" is the genitive (s druge strane, od strane, dvije strane),
    // not the plural strȃne; "strani", "stranu", "strano", "stranom" are
    // also the adjective strȃn (strani jezik), which has the falling accent
    u8"str/a:n|a|e|ama", u8"str^a:n||i|u|o|om|og|oga|ome|omu|oj|ih|im|ima",
    // dokùment, dokùmenti (unchanged), G pl dokùmenātā
    u8"dok/umenata",
    // vijȇst, vijȇsti, vijȇšću, but vijéstima, G pl vijéstī (spelled as
    // the far commoner vijȇsti) and L sg vijésti after o, po, pri (a rule
    // of the front end); ekavian vȇst, vȇsti, véstima
    u8"v^ije:st||i", u8"v^ije:šću", u8"v/ije:stima", u8"v^e:st||i", u8"v^e:šću",
    u8"v/e:stima",
    // rázmak, rázmaci, G pl rázmākā; rázmaknica (HJP, Školski rječnik).
    // Exact forms: a stem would take razmàkne, razmàkni of razmàknuti (VERBS)
    u8"r/a:zmak||a|u|om|e", u8"r/a:zmac|i|ima", u8"r/a:zmaknic*",
    // ȉpsilōn (the name of Y, also when spelled), èpsilōn (HJP), against the
    // rule for loans in -on (ipsìlon)
    u8"^ipsilon*", u8"/epsilon*",
    // cȃrstvo, cȃrskī and králjevstvo in every case, G pl cȃrstāvā,
    // králjevstāvā (HJP, Školski rječnik, Wiktionary); the kraljev* stem
    // above does not reach them
    u8"c^a:rstv|o|a|u|om|ima", u8"c^a:rstava", u8"c^a:rsk*",
    u8"kr/a:ljevstv|o|a|u|om|ima", u8"kr/a:ljevstava",

    // ---- The tenth October 2026 list (HJP, Školski rječnik, Wiktionary) ----
    // ćȃr; čȃr, čȃri, čȃrju, but čárima, G pl čárī and the locative čári
    // after a preposition (a rule of the front end); čárati, čárao,
    // čárajū (VERBS), but the present čȃrām, čȃrā, čȃrāj; čȁroban, čȁrobnī
    // (čaròbnjāk is the rule for -njak). prìtisak, prìtiska, prìtisci,
    // prìtisākā; prìtisnuti and pritískati are in VERBS, and so are
    // ujedíniti, očárati, začárati
    u8"ć^a:r||a|u|om", u8"č^a:r||a|i|u|om|ju", u8"č/a:rima",
    u8"č^a:r|am|aš|amo|ate|aj|ajmo|ajte", u8"č^aroban",
    u8"č^arobn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"pr/itis|ak|ka|ku|kom|ke|ci|cima|aka",
    // minimálac, G pl minimálācā (HJP; the other cases are the rule for
    // -alc-, which cannot take the nominative: glȅdalac, čìtalac); the
    // adjective mȉnimālan, but minimálnōst
    u8"minim'a:lac", u8"minim'a:laca",
    u8"m^inimal|an|na|no|ni|ne|nu|nog|noga|nom|nome|nomu|noj|nih|nim|nima",
    u8"minim'a:lnost*", u8"minim'a:lnošću",
    // fȋrma in every case (HJP, Školski rječnik, Wiktionary)
    u8"f^i:rm|a|e|i|u|o|om|ama",
};

const char* const CROATIAN[] = {
    // HJP: pòbjednīk, pòbjedničkī (feminine pòbjedničkā); keep initial
    // short rising o in every case, including -nici/-nicima/-niče.
    // Croatian omits unstressed lengths. Exact paradigms avoid derivatives.
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxmXxU%3D
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxmXxM%3D
    u8"p/objedn|ik|ika|iku|ikom|ici|icima|ike|iče",
    u8"p/objedničk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // Mrežnik: dvádesēt, dvádesētī, but dvadesétak; Školski rječnik:
    // dvádesetero. Restore the long rising a in Croatian, replacing the
    // older falling-accent rendering override only for these exact forms.
    // https://rjecnik.hr/mreznik/dvadeset/
    // https://rjecnik.hr/mreznik/dvadeseti/
    // https://rjecnik.hr/mreznik/dvadesetak/
    // https://rjecnik.hr/?letter=d&page=28
    u8"dv/a:deset||i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"dv/a:desetero", u8"dvades/e:tak",
    // Mrežnik / HJP: idéja, bèsplatan (checked 2026-10-08).
    // Exact paradigms keep unrelated derivatives out; Croatian omits the
    // dictionaries' post-accent lengths, as elsewhere in this table.
    // https://rjecnik.hr/mreznik/ideja/
    // https://rjecnik.hr/mreznik/besplatan/
    u8"id/e:j|a|e|i|u|o|om|ama",
    u8"b/esplatan", u8"b/esplatn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // HJP súčēlje; HJP / Školski rječnik òtīći, òtišao, òtišla.
    // Keep Croatian's usual omission of unstressed length. Exact noun
    // and irregular verb forms avoid catching sučèliti or otíđi.
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=d1pjWBU%3D
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eFdmXhI%3D
    u8"s/u:čelj|e|a|u|em|ima",
    u8"/otići", u8"/otiš|ao|la|lo|li|le|avši",
    // Školski rječnik / HJP: sȃt, rȃd, but L sátu/rádu, G pl sátī,
    // and short falling sȁtovi/rȁdovi. Frontend resolves the contextual
    // forms; isolated satu/radu and sati use D sg and N pl respectively.
    // Kapović, Filologija 54 (2010), also records DLI pl sátima.
    // https://rjecnik.hr/search/?q=sat&strict=yes
    // https://rjecnik.hr/search/?q=rad&strict=yes
    // https://hrcak.srce.hr/file/77788
    u8"s^a:t||a|u|e|om|i", u8"s/a:tima", u8"s^atov|i|a|e|ima",
    u8"r^a:d||a|u|e|om", u8"r^adov|i|a|e|ima",
    // Forty to ninety without the length of the last syllable (četrdèset,
    // pedèset for the dictionaries' četrdèsēt, pedèsēt), as the other
    // entries for this voice do. Ordinals and -ak forms follow the stem.
    u8"četrd'eset", u8"četrd'eset*", u8"ped'eset", u8"ped'eset*", u8"šezd'eset", u8"šezd'eset*",
    u8"sedamd'eset", u8"sedamd'eset*", u8"osamd'eset", u8"osamd'eset*",
    u8"deved'eset", u8"deved'eset*",
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
    // Bèla, the Croatian name of the kings Béla (Zlatna bula Bele IV.);
    // in Serbian "bela, bele" is the adjective
    u8"B/el|a|e|i|u|om",
    // HJP: Bȅograd in Croatian (Serbian Beògrad, COMMON)
    u8"B^eograd*",

    // Listener's second October 2026 list. Školski rječnik hrvatskoga
    // jezika, cross-checked with HJP; sources and homographs in
    // docs/formant.md. As elsewhere in this table, omit unstressed length.
    // The currency stays on the first syllable in every case (dȍlāra,
    // dȍlārā), unlike the productive -ar rule. ŠR ȅuro, HJP èuro: use ŠR.
    u8"d^olar*", u8"^eur|o|a|u|om|i|ima|e",
    u8"n/a:ravan", u8"n/a:ravn*",
    // Exact noun forms: a priča stem would also swallow príčati/príčao.
    // The present prȋčā shares the noun's tone; Croatian drops its final
    // length. Other present/imperative forms are falling, príčajū rising.
    u8"pr^i:č|a|e|i|u|o|om|ama|am|aš|amo|ate|aj|ajmo|ajte",
    u8"pr^i:čan*",
    u8"dj^elomičan", u8"dj^elomičn*", u8"č^injenic*",
    u8"n^emoguć*",   // nemogúćnost keeps its own longer entry
    u8"pop/o:dne*", u8"pop/o:dnevima", u8"pop/o:dnevn*", u8"pop/o:dnevan",
    // èuropskī differs from Európa; do not change the place name's stem.
    u8"/europsk*", u8"osigur/a:nj*",
    // The noun shifts onto its long i in the oblique cases; the adjective
    // retains the nominative's accent (automòbīlskī).
    u8"autom/obil", u8"automob/i:l*", u8"autom/obilsk*",
    u8"tek/ućin*", u8"k/afić", u8"kaf/i:ć*",
    // pomòćnīk / pomoćníka / pomoćníci, but pomòćnica and pomòćničkī.
    // "pomoćnici" defaults to the masculine plural, not the feminine D/L.
    u8"pom/oćnik", u8"pomoćn/i:k*", u8"pomoćn/i:c|i|ima",
    u8"p^omoćniče", u8"pom/oćnic*", u8"pom/oćničk*",
    u8"t^amo", u8"progn/o:z*", u8"kab/anic*",
    // Mrežnik and HJP: kȉšobrān, kȉšobrāna, kȉšobrāni. Exact noun
    // forms keep the short falling i without imposing it on derivatives.
    u8"k^išobran||a|u|e|om|i|ima",
    u8"sloj/evit||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    u8"sloj/evitost*", u8"sloj/evitošću",
    // Derivatives checked separately so verb/noun stems cannot take them.
    // proslavi/proslave default to the noun, as other exact homographs do.
    u8"pr^oslav|a|e|i|u|o|om|ama", u8"ves/e:lj*",
    // HJP zàštīćenōst, like the participle (zaštititi shifts in VERBS).
    u8"z/aštićenost*", u8"z/aštićenošću",

    // Školski rječnik and Mrežnik: batèrija (HJP has bàtērija).
    u8"bat/erij|a|e|i|u|o|om|ama", u8"bat/erijsk*",
    // ŠR and HJP: pročìtati/pročìtām, but prȍčitān. The passive needs
    // a separate falling-tone paradigm; the negative stays nepròčitan.
    u8"pr^očitan||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
};

const char* const SERBIAN[] = {
    // Numbers: the lengths after the accent (dȅvēt, dȅsēt, jedànaēst,
    // milìjārda, milìjūntī, dvjȅstōtī) and the ordinals dvadèsētī, tridèsētī,
    // dèvētī, dèsētī
    u8"d^eve:t", u8"d^ese:t",
    u8"jed'anae:st*", u8"dv'a:nae:st*", u8"tr'i:nae:st*", u8"čet'rnae:st*", u8"p'etnae:st*",
    u8"š'esnae:st*", u8"sed'amnae:st*", u8"os'amnae:st*", u8"dev'etnae:st*",
    u8"dvad'ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"trid'ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"dvj^esto:t*", u8"dv^esto:t*", u8"tr^isto:t*", u8"č^etiristo:t*", u8"p^e:tsto:t*",
    u8"š^e:ststo:t*", u8"s^edamsto:t*", u8"^osamsto:t*", u8"d^evetsto:t*",
    u8"mil/ija:rd*", u8"mil/iju:nt*", u8"mil/io:nit*",
    u8"d/eve:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", u8"d/ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"prof'esor*", u8"proc'enat", u8"proc'ent*", u8"Vojv'odin*", u8"Ukraj'i:n*",
    u8"j^anua:r*", u8"f^ebrua:r*", u8"m^art*", u8"/apri:l*", u8"m^a:j", u8"j^u:n",
    u8"j^u:l", u8"^avgust*", u8"sept'embar", u8"sept'embr*", u8"okt'o:bar",
    u8"okt'o:br*", u8"nov'embar", u8"nov'embr*", u8"dec'embar", u8"dec'embr*",
    u8"d/e:te", u8"vr/e:me", u8"l/e:p*", u8"ml/e:k*", u8"r/e:k*", u8"b/e:l*",
    u8"sv^e:t", u8"r^e:č",
    u8"m^onoto:no", u8"r^e:da:ka:", u8"k/orisni:k*", u8"k/orisni:ci", u8"k/orisni:cima",
    u8"k/orisni:če", u8"k^ori:stan", u8"k^ori:st",
    u8"r^ođenda:n*", u8"z^auze:t*",
    // The words and names checked in October 2026 (see COMMON) with their
    // length after the accent
    u8"/obrazova:nj*", u8"/obrazo:vn*", u8"/isku:sta:va:", u8"/ala:t",
    u8"progr/ame:rsk*", u8"/ekra:n", u8"mod/era:tor*", u8"mod/era:toric*",
    u8"mod/era:tork*", u8"razv'i:jaju:", u8"t^obo:m", u8"M/iha:jl*",
    u8"Crn/ogo:rc|a|u|em", u8"Crn/ogo:rče", u8"Crn/ogora:ca:", u8"Crn/ogo:rk*",
    u8"n^a:jposle:", u8"n^a:jpre:", u8"p/ona:jprije", u8"p/ona:jviše",
    u8"j/a:vno:st", u8"p/otpuno:st",
    u8"crn/ogo:rsk*", u8"prot/etiča:r*", u8"ž/eljezniča:r*", u8"ž/elezniča:r*", u8"ž/eljezni:čk*",
    u8"ž/elezni:čk*", u8"od/rživo:st", u8"inspir'i:raju:",
    // màslina, màslinov and rakéta are the first forms of the Serbian
    // dictionary (Rečnik Matice srpske: ма̀слина и ма̏слина, раке́та и ракѐта)
    u8"m/aslin*", u8"m/aslinov*", u8"rak/e:t*",
    // The third October 2026 list (see COMMON). Rečnik Matice srpske has
    // prijatéljstvo, neprijatéljstvo (first form) and snáći, where the
    // Croatian dictionaries have prijatèljstvo and snȃći
    u8"prijat/e:ljstv|o|a|u|om|ima", u8"prijat/e:ljsta:va:",
    u8"neprijat/e:ljstv|o|a|u|om|ima", u8"neprij/ate:ljsta:va:", u8"sn/a:ći",
    u8"p^otencija:l|an|na|no|ni|ne|nu|nog|nom|noj|nih|nim|noga|nome|nomu|nima",
    u8"sn/alaže:nj|e|a|u|em|ima", u8"simul'a:ci:jsk*", u8"sim/ula:tor*", u8"sim/ula:torsk*",
    u8"dož'i:v|i:m|i:š|e:", u8"prež'i:v|i:m|i:š|e:", u8"prož'i:v|i:m|i:š|e:",
    u8"nadž'i:v|i:m|i:š|e:", u8"pož'i:v|i:m|i:š|e:", u8"ž/i:v|i:m|i:š",
    u8"d/oži:vljen*", u8"pr/eži:vljen*", u8"pr/oži:vljen*", u8"n/adži:vljen*",
    u8"d^oživlja:j*", u8"nev/idljivo:st", u8"nesn/alaže:nj|e|a|u|em|ima",
    u8"p^oštova:n*", u8"^interesova:n*", u8"^interesuj|e:m|e:š|e:|e:mo|e:te|u:",
    u8"z/ainteresuj|e:m|e:š|e:|e:mo|e:te|u:", u8"z/ainteresova:n*", u8"nez/ainteresova:n*",
    // The fourth list (see COMMON): vòdīč, poluvòdīč, Ègipćānka
    u8"v/odi:č", u8"poluv'odi:č", u8"/egipća:nk*",
    // gòdišnjāk (see COMMON)
    u8"g/odišnja:k*", u8"g/odišnja:c|i|ima", u8"g/odišnja:če",
    // The fifth October 2026 list (see COMMON) with the length after the
    // accent: ȍdabīr, ȍdabrāno, asìstenātā, ȕčenīk, ȉzglēd, ìzglēdām,
    // spȃvām, nȇmām, krèdīt, stȁvākā, krȇćēm, krȇnēm, dokumèntārācā,
    // nȍvācā, sústāvan, dȁrōvā, vȃžīm
    u8"^odabi:r||a|u|om|i|e|ima", u8"^odabra:|la|lo|li|le",
    u8"^odabra:n||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", u8"as/istena:ta:",
    u8"^učeni:|k|ka|ku|kom|ke|ci|cima|če",
    u8"uglj/eni:k",
    u8"^izgle:d||a|u|om|i|e|ima", u8"/izgle:d|a:m|a:š|a:mo|a:te|a:j|a:jmo|a:jte",
    u8"izgl'e:daju:", u8"izgl'e:daju:ći", u8"sp^a:v|a:m|a:š|a:|a:mo|a:te|a:j|a:jmo|a:jte", u8"sp/a:vaju:",
    u8"n^e:m|a:m|a:š|a:|a:mo|a:te|a:j|a:jmo|a:jte", u8"n/e:maju:",
    u8"kr/edi:t", u8"kr/edi:tn*", u8"st^ava:ka:",
    u8"kr^e:ć|e:m|e:š|e:|e:mo|e:te|u:", u8"kr^e:n|e:m|e:š|e:|e:mo|e:te|u:",
    u8"dokum/enta:ra:ca:", u8"n^ova:ca:", u8"s/u:sta:van", u8"d^aro:va:",
    u8"v^a:ž|i:m|i:š|i:|i:mo|i:te|e:", u8"v/a:že:ć*",
    // The sixth October 2026 list (see COMMON) with the length after the
    // accent: ùmjetnīk, ùmetnōst, àlbūm, jedìnstvenōst, plȃćenīk, dèbīl,
    // debìtanātā, ljúbāvnī, ljúbāvnīk, bòjōm, túgōm; and Rečnik Matice
    // srpske's bȅskompromisan, nȅzaobilāzan and tȗžan, tȗžna (Croatian
    // bèskompromisan, nezaobìlazan, túžan)
    u8"/umjetni:|k|ka|ku|kom|ke|ci|cima|če", u8"/umetni:|k|ka|ku|kom|ke|ci|cima|če",
    u8"/umjetno:st", u8"/umetno:st", u8"/albu:m", u8"jed/instveno:st",
    u8"pl^a:ćeni:|k|ka|ku|kom|ke|ci|cima|če", u8"pl^a:čeni:|k|ka|ku|kom|ke|ci|cima|če",
    u8"d/ebi:l", u8"deb/itana:ta:", u8"lj/u:ba:vn*", u8"lj/u:ba:vnic*",
    u8"lj/u:ba:vni:|k|ka|ku|kom|ke|ci|cima|če", u8"b/ojo:m", u8"t/u:go:m",
    u8"b^eskompromis*", u8"b^eskompromisn*", u8"n^ezaobila:zan", u8"n^ezaobilazn*",
    u8"t^u:žan", u8"t^u:žna",
    // The eighth list: kȁrātā; the ninth: rázličitōst, dokùmenātā
    u8"k^ara:ta:", u8"r/a:zličito:st", u8"dok/umena:ta:",
    // rázmākā, ȉpsilōn, èpsilōn
    u8"r/a:zma:ka:", u8"^ipsilo:n*", u8"/epsilo:n*", u8"c^a:rsta:va:",
    u8"kr/a:ljevsta:va:", u8"č^a:r|a:m|a:š|a:mo|a:te|a:j|a:jmo|a:jte", u8"č/a:raju:",
    u8"minim'a:la:ca:", u8"minim'a:lno:st", u8"m^inima:l|an|na|no|ni|ne|nu|nog|noga|nom|nome|nomu|noj|nih|nim|nima",
    // gùbītākā, dòbītākā ... (Rečnik Matice srpske, HJP)
    u8"g/ubi:ta:ka:", u8"d/obi:ta:ka:", u8"/uži:ta:ka:", u8"pr/imi:ta:ka:", u8"/odbi:ta:ka:",
    u8"r/azvi:ta:ka:", u8"b/olji:ta:ka:", u8"zg/odi:ta:ka:", u8"pr/obi:ta:ka:", u8"/imu:ta:ka:",
    u8"/osnu:ta:ka:", u8"pr/ivi:ta:ka:",
};

const char* const BOSNIAN[] = {
    // Numbers: the lengths after the accent (dȅvēt, dȅsēt, jedànaēst,
    // milìjārda, milìjūntī, dvjȅstōtī) and the ordinals dvadèsētī, tridèsētī,
    // dèvētī, dèsētī
    u8"d^eve:t", u8"d^ese:t",
    u8"jed'anae:st*", u8"dv'a:nae:st*", u8"tr'i:nae:st*", u8"čet'rnae:st*", u8"p'etnae:st*",
    u8"š'esnae:st*", u8"sed'amnae:st*", u8"os'amnae:st*", u8"dev'etnae:st*",
    u8"dvad'ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"trid'ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"dvj^esto:t*", u8"dv^esto:t*", u8"tr^isto:t*", u8"č^etiristo:t*", u8"p^e:tsto:t*",
    u8"š^e:ststo:t*", u8"s^edamsto:t*", u8"^osamsto:t*", u8"d^evetsto:t*",
    u8"mil/ija:rd*", u8"mil/iju:nt*", u8"mil/io:nit*",
    u8"d/eve:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", u8"d/ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    u8"prof'esor*", u8"proc'enat", u8"proc'ent*", u8"Vojv'odin*", u8"Ukraj'i:n*",
    u8"j^anua:r*", u8"f^ebrua:r*", u8"m^art*", u8"/apri:l*", u8"m^a:j", u8"j^u:n",
    u8"j^u:l", u8"^august*", u8"sept'embar", u8"sept'embr*", u8"okt'o:bar",
    u8"okt'o:br*", u8"nov'embar", u8"nov'embr*", u8"dec'embar", u8"dec'embr*",
    u8"k^ahv*", u8"k^af*", u8"l^ahko", u8"m^ehk*",
    u8"m^onoto:no", u8"r^e:da:ka:", u8"k/orisni:k*", u8"k/orisni:ci", u8"k/orisni:cima",
    u8"k/orisni:če", u8"k^ori:stan", u8"k^ori:st",
    u8"r^ođenda:n*", u8"z^auze:t*",
    // The words and names checked in October 2026 (see COMMON) with their
    // length after the accent
    u8"/obrazova:nj*", u8"/obrazo:vn*", u8"/isku:sta:va:", u8"/ala:t",
    u8"progr/ame:rsk*", u8"/ekra:n", u8"mod/era:tor*", u8"mod/era:toric*",
    u8"mod/era:tork*", u8"razv'i:jaju:", u8"t^obo:m", u8"M/iha:jl*",
    u8"Crn/ogo:rc|a|u|em", u8"Crn/ogo:rče", u8"Crn/ogora:ca:", u8"Crn/ogo:rk*",
    u8"n^a:jposle:", u8"n^a:jpre:", u8"p/ona:jprije", u8"p/ona:jviše",
    u8"j/a:vno:st", u8"p/otpuno:st",
    u8"crn/ogo:rsk*", u8"prot/etiča:r*", u8"ž/eljezniča:r*", u8"ž/elezniča:r*", u8"ž/eljezni:čk*",
    u8"ž/elezni:čk*", u8"od/rživo:st", u8"inspir'i:raju:",
    // màslina, màslinov and rakéta are the first forms of the Serbian
    // dictionary (Rečnik Matice srpske: ма̀слина и ма̏слина, раке́та и ракѐта)
    u8"m/aslin*", u8"m/aslinov*", u8"rak/e:t*",
    // The third October 2026 list (see COMMON). Alić's handbook gives
    // prìjatelj as the main Bosnian form (prȉjatelj in the Croatian and
    // Serbian dictionaries) and dožívīm as Rječnik bosanskoga jezika has it
    u8"pr/ijatelj*", u8"prijat/eljsta:va:", u8"neprij/ate:ljsta:va:",
    u8"sn^a:đ|e:m|e:š|e:|e:mo|e:te|u:",
    u8"p^otencija:l|an|na|no|ni|ne|nu|nog|nom|noj|nih|nim|noga|nome|nomu|nima",
    u8"sn/alaže:nj|e|a|u|em|ima", u8"simul'a:ci:jsk*", u8"sim/ula:tor*", u8"sim/ula:torsk*",
    u8"dož'i:v|i:m|i:š|e:", u8"prež'i:v|i:m|i:š|e:", u8"prož'i:v|i:m|i:š|e:",
    u8"nadž'i:v|i:m|i:š|e:", u8"pož'i:v|i:m|i:š|e:", u8"ž/i:v|i:m|i:š",
    u8"d/oži:vljen*", u8"pr/eži:vljen*", u8"pr/oži:vljen*", u8"n/adži:vljen*",
    u8"d^oživlja:j*", u8"nev/idljivo:st", u8"nesn/alaže:nj|e|a|u|em|ima",
    u8"p^oštova:n*", u8"^interesova:n*", u8"^interesuj|e:m|e:š|e:|e:mo|e:te|u:",
    u8"z/ainteresuj|e:m|e:š|e:|e:mo|e:te|u:", u8"z/ainteresova:n*", u8"nez/ainteresova:n*",
    // The fourth list (see COMMON): vòdīč, poluvòdīč, Ègipćānka
    u8"v/odi:č", u8"poluv'odi:č", u8"/egipća:nk*",
    // gòdišnjāk (see COMMON)
    u8"g/odišnja:k*", u8"g/odišnja:c|i|ima", u8"g/odišnja:če",
    // The fifth October 2026 list (see COMMON) with the length after the
    // accent: ȍdabīr, ȍdabrāno, asìstenātā, ȕčenīk, ȉzglēd, ìzglēdām,
    // spȃvām, nȇmām, krèdīt, stȁvākā, krȇćēm, krȇnēm, dokumèntārācā,
    // nȍvācā, sústāvan, dȁrōvā, vȃžīm
    u8"^odabi:r||a|u|om|i|e|ima", u8"^odabra:|la|lo|li|le",
    u8"^odabra:n||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", u8"as/istena:ta:",
    u8"^učeni:|k|ka|ku|kom|ke|ci|cima|če",
    u8"uglj/eni:k",
    u8"^izgle:d||a|u|om|i|e|ima", u8"/izgle:d|a:m|a:š|a:mo|a:te|a:j|a:jmo|a:jte",
    u8"izgl'e:daju:", u8"izgl'e:daju:ći", u8"sp^a:v|a:m|a:š|a:|a:mo|a:te|a:j|a:jmo|a:jte", u8"sp/a:vaju:",
    u8"n^e:m|a:m|a:š|a:|a:mo|a:te|a:j|a:jmo|a:jte", u8"n/e:maju:",
    u8"kr/edi:t", u8"kr/edi:tn*", u8"st^ava:ka:",
    u8"kr^e:ć|e:m|e:š|e:|e:mo|e:te|u:", u8"kr^e:n|e:m|e:š|e:|e:mo|e:te|u:",
    u8"dokum/enta:ra:ca:", u8"n^ova:ca:", u8"s/u:sta:van", u8"d^aro:va:",
    u8"v^a:ž|i:m|i:š|i:|i:mo|i:te|e:", u8"v/a:že:ć*",
    // The sixth October 2026 list (see COMMON) with the length after the
    // accent: ùmjetnīk, ùmetnōst, àlbūm, jedìnstvenōst, plȃćenīk, dèbīl,
    // debìtanātā, ljúbāvnī, ljúbāvnīk, bòjōm, túgōm; and Rečnik Matice
    // srpske's bȅskompromisan, nȅzaobilāzan and tȗžan, tȗžna (Croatian
    // bèskompromisan, nezaobìlazan, túžan)
    u8"/umjetni:|k|ka|ku|kom|ke|ci|cima|če", u8"/umetni:|k|ka|ku|kom|ke|ci|cima|če",
    u8"/umjetno:st", u8"/umetno:st", u8"/albu:m", u8"jed/instveno:st",
    u8"pl^a:ćeni:|k|ka|ku|kom|ke|ci|cima|če", u8"pl^a:čeni:|k|ka|ku|kom|ke|ci|cima|če",
    u8"d/ebi:l", u8"deb/itana:ta:", u8"lj/u:ba:vn*", u8"lj/u:ba:vnic*",
    u8"lj/u:ba:vni:|k|ka|ku|kom|ke|ci|cima|če", u8"b/ojo:m", u8"t/u:go:m",
    u8"b^eskompromis*", u8"b^eskompromisn*", u8"n^ezaobila:zan", u8"n^ezaobilazn*",
    u8"t^u:žan", u8"t^u:žna",
    // The eighth list: kȁrātā; the ninth: rázličitōst, dokùmenātā
    u8"k^ara:ta:", u8"r/a:zličito:st", u8"dok/umena:ta:",
    // rázmākā, ȉpsilōn, èpsilōn
    u8"r/a:zma:ka:", u8"^ipsilo:n*", u8"/epsilo:n*", u8"c^a:rsta:va:",
    u8"kr/a:ljevsta:va:", u8"č^a:r|a:m|a:š|a:mo|a:te|a:j|a:jmo|a:jte", u8"č/a:raju:",
    u8"minim'a:la:ca:", u8"minim'a:lno:st", u8"m^inima:l|an|na|no|ni|ne|nu|nog|noga|nom|nome|nomu|noj|nih|nim|nima",
    // Alić: the nouns like gubítak keep the accent in every case, the
    // genitive plural too (gubítākā)
    u8"gub'i:ta:ka:", u8"dob'i:ta:ka:", u8"už'i:ta:ka:", u8"prim'i:ta:ka:", u8"odb'i:ta:ka:",
    u8"razv'i:ta:ka:", u8"bolj'i:ta:ka:", u8"zgod'i:ta:ka:", u8"prob'i:ta:ka:", u8"im'u:ta:ka:",
    u8"osn'u:ta:ka:", u8"priv'i:ta:ka:",
};

// Verbs whose root has the long "ije" (podijéliti, promijéniti, zalijévati).
// After a prefix the spelling "ije" marks the verb: the nouns beside it have
// the short "je" (podjela, promjena, zamjena). The front end puts the accent
// on the "ije", or shifts it in the present and passive participle
// (StressRules::verb_form); nothing else tells it that "podijeli", "podijelio"
// and "podijeljen" belong to "podijeliti".
//
//   root:classes[:prefixes]
//   i  verb in -iti (podijeliti, podijelim, podijeli, podijelio, podijeljen)
//   a  verb in -ati (pomiješati, pomiješam, pomiješaj, pomiješao, pomiješan)
//   t  verb in -ati whose present has another root (dolijetati)
//   e  that present root (dolijećem, podliježem)
//   !  Croatian also shifts the present and passive accent (pòdijelim)
//   prefixes: the only ones the root takes, where any prefix would also
//   match nouns (povijest, pripovijest, zapovijed)
const char* const IJE_VERBS[] = {
    // HJP podijéliti, prez. pòdijēlīm, prid. trp. pòdijēljen.
    // Keep this restricted entry before the generic root: other prefixes
    // retain their existing Croatian treatment (e.g. raspodijelim).
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxlXhY%3D
    u8"dijel:i!:po",
    // HJP proslijéditi, prez. pròslijēdīm, prid. trp. pròslijēđen.
    // The restricted prefix preserves the treatment of naslijediti etc.
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=dl5lWRA%3D
    u8"slijed:i!:pro",
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
//   st'e:m=class and flags (' automatic tone, / rising, ^ falling)
//   i  -iti, -im      (urediti, uredim, uredi, uredio, uređen)
//   a  -ati, -am      (pročitati, pročitam, pročitaj, pročitao, pročitan)
//   u  -nuti, -nem    (pokrenuti, pokrenem, pokreni, pokrenuo, pokrenut)
//   t  -ati, where the present has another stem (pokazati, pokazao, pokazan)
//   e  that stem, or any present in -em (pokažem, pokaži; unesem, unesi)
//   <  the dictionaries move the accent one syllable back in the present
//      (ùrēdīm, òtvorīm, pòkāžēm); Stojan and Mirsad do, Zvonko does not
//   p  ... and in the passive participle (ùrēđen);  P: to the first syllable
//   !  Zvonko moves it too, as the dictionaries do (ùključen, raspòrēđen)
//   n  the imperative is spelled like a form of a noun or adjective (potvrdi,
//      uredi, otvori, načini, napomeni; for classes a and u the third
//      person: oprema, proba, napomene, napomenu).
//      It is read as the verb only at the head of a clause ("Potvrdi",
//      "Ne zaboravi"); elsewhere the other word has priority.
//
// The accents are those of Hrvatski jezični portal (October 2026); the
// stems were generated from its entries, the noun twins from its answers
// for the imperative forms. A verb that is not here is right only in the
// infinitive or if the first syllable carries its accent anyway.
const char* const VERBS[] = {
    u8"ur'e:d=i<pn", u8"odr'e:d=i<pn", u8"nar'e:d=i<p", u8"sr'e:d=i", u8"preur'e:d=i<p",
    u8"uklj'u:č=i<p!", u8"isklj'u:č=i<p!", u8"zaklj'u:č=i<p!", u8"priklj'u:č=i<p!",
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
    u8"ispr'a:zn=i<pn", u8"uč'in=i<p", u8"nač'in=i<pn", u8"za'ustavlj=a",
    u8"pr'esluš=a", u8"p/osluš=a", u8"pr'egled=a", u8"vr'a:t=i", u8"povr'a:t=i<p",
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
    // HJP: zaštítiti, but zàštītīm and zàštīćen in Croatian too.
    u8"nahr'a:n=i<p", u8"zašt'i:t=i<pn!", u8"odg'od=i<pn", u8"dog'od=i<", u8"pog'od=i<p",
    u8"ug'od=i<pn", u8"prilag'od=i<p", u8"pohv'a:l=i<pn", u8"hv'a:l=i", u8"zahv'a:l=i<n",
    u8"up'a:l=i<pn", u8"zap'a:l=i<p", u8"ug'a:s=i<p", u8"g'a:s=i", u8"ispr'o:b=a<p",
    u8"p'odes=i", u8"k'orist=in", u8"ub'rz=a<",
    u8"pr'o:b=a", u8"proč'it=ap", u8"oč'it=ap", u8"izrač'un=aPn", u8"rač'un=an",
    u8"obrač'un=aPn", u8"zaklj'uč=ap", u8"otklj'uč=ap", u8"p'i:t=a", u8"up'i:t=a<p",
    u8"zap'i:t=a<p", u8"pr/i:č=a", u8"ispr'i:č=a<p", u8"sv'i:r=a", u8"m'o:r=a",
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
    u8"razm'ak=u<p!", u8"dod'i:r=u<p", u8"dot'ak=u<p", u8"spom'e:=u<p", u8"napom'e:=u<pn",
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
    u8"uv'r:st=i<p", u8"razv'rst=ap", u8"prem'ost=i<p", u8"izab'er=e", u8"odab'er=e<!",
    u8"od'ust=t", u8"od'ustan=e", u8"poš'alj=e<", u8"poz'ov=e", u8"naz'ov=e",
    u8"sač'u:v=a<p", u8"č'u:v=a", u8"oč'u:v=a<", u8"udr'u:ž=i<p", u8"pridr'u:ž=i<p",
    u8"zdr'u:ž=i", u8"dr'u:ž=i", u8"zatr'a:ž=i<p", u8"razm'otr=i<p", u8"prom'otr=i<p",
    u8"nasl'on=ipn", u8"osl'on=i<p", u8"prisl'on=i<p", u8"označ'a:v=a<", u8"ob'iljež=i", u8"ob'elež=i",
    u8"zab'iljež=i", u8"ub'iljež=i", u8"prib'iljež=i", u8"zab'elež=i", u8"ub'elež=i",
    u8"prib'elež=i", u8"nagl'a:s=i<p", u8"gl'a:s=i",
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
    // Verbs whose forms are spelled like the case forms of the nouns with
    // a long last stem syllable (StressRules::long_stem): ùdara is not
    // udára, ìspune not račúne, ìskače not koláče, spȁda not čokoláda;
    // odmárati, zamárati follow their accent. (The present of umrijeti,
    // ùmire, is in the lexicon: as a stem it would take umíri, umíriti.)
    u8"'udar=a", u8"'ispun=i", u8"n'apun=i", u8"d'opun=i", u8"p'opun=i",
    u8"'iskak=t", u8"'iskač=e", u8"'uskak=t", u8"'uskač=e",
    u8"z'aplak=t", u8"z'aplač=e", u8"sp'ad=a", u8"odm'a:r=a<", u8"zam'a:r=a<",
    u8"um'a:r=a<", u8"raz'a:r=a<",
    // Checked against Školski rječnik hrvatskoga jezika, Hrvatski mrežni
    // rječnik and HJP in October 2026, with the shifts of the present and
    // the participle in every voice: rasporéditi, raspòrēdīm, raspòrēđen;
    // osnážiti, òsnāžīm, òsnāžen; potàknuti, pòtaknēm, potàkni, pòtaknūt
    // (Serbian and Bosnian podstàknuti); razvíjati, ràzvījām (razvíjajū is
    // in the lexicon); prèdstaviti, nàstaviti and their imperfectives keep
    // the accent on the prefix.
    u8"raspor'e:d=i<p!", u8"osn'a:ž=i<p!", u8"pot'ak=u<p!", u8"podst'ak=u<p!",
    u8"razv'i:j=a<!", u8"pr'edstav=i", u8"pr'edstavlj=a", u8"n'astav=i", u8"n'astavlj=a",
    u8"p'otic=t", u8"p'otič=e", u8"p'odstic=t", u8"p'odstič=e",
    u8"inspir'i:r=a<p!", u8"insp'iris=t", u8"insp'iriš=e",
    // ŠR and HJP: pròslaviti/pròslavīm/pròslavljen; vesèliti/vesèlīm.
    u8"pr/oslav=i", u8"ves'el=i",
    // Školski rječnik, HJP and Rečnik Matice srpske: čestítati, čèstītām,
    // čestítajū, čèstītāj, čestítao, čèstītān, in every voice (the noun
    // čèstitka is in the lexicon)
    u8"čest'i:t=a<p!",
    // ŠR and HJP: sprijatèljiti, sprijàteljīm, sprijàteljen (Rečnik Matice
    // srpske also sprijatèljīm)
    u8"sprijat'elj=i<p",
    // HJP, Školski rječnik: napòminjati, napòminjēm, napòminji, napòminjao;
    // proizvòditi, proìzvodīm, proizvòdio, proìzvođen (proìzvodi is the
    // noun's in the lexicon); proizvèdēm, proizvèdi, proizvèden
    u8"nap'ominj=te", u8"proizv'od=i<p", u8"proizv'ed=e",
    // The fifth October 2026 list (Školski rječnik, HJP, Hrvatski mrežni
    // rječnik; Rečnik Matice srpske): prèkinuti, prèkinēm, prèkini,
    // prèkinūt; prekídati, prèkīdām, prèkīdān (the present and the passive
    // participle move in the Serbian and Bosnian voices); odàbirati,
    // odàbirēm, izàbirati; prìkvačiti, zàkvačiti, òtkvačiti and prìkačiti,
    // zàkačiti, òtkačiti, against the rule for -ač; the rising accent of
    // spávati, némati, krétati, kréći, krénuti, kréni, vážiti (their
    // falling present is in the lexicon). odàbrati moves its present in
    // every voice (odàberēm, odabèri)
    u8"pr/eki=u", u8"prek'i:d=a<pn", u8"od'abir=t", u8"od'abir=e", u8"iz'abir=t",
    u8"iz'abir=e", u8"pr/ikvač=i", u8"z/akvač=i", u8"/otkvač=i", u8"pr/ikač=i",
    u8"z/akač=i", u8"/otkač=i", u8"sp/a:v=a", u8"n/e:m=a", u8"kr/e:t=t", u8"kr/e:ć=e",
    u8"kr/e:=u", u8"v/a:ž=i",
    // The sixth October 2026 list, in every voice as the dictionaries
    // have them (Školski rječnik, HJP, Rečnik Matice srpske): zanímati,
    // zànīmām, zànīmā, zanímajū, zànīmāj, zanímao, zànīmān, zanímānje;
    // odmòriti, òdmorīm, odmòri, odmòrio, òdmoren ("odmori" is the noun's
    // plural òdmori except at the head of a clause)
    u8"zan'i:m=a<p!", u8"odm'or=i<pn!",
    // HJP: reproducírati, reprodùcīrām, reproducírānje, and the passive
    // reprodùcīrān like inspìrīrān, in every voice as for inspirírati
    u8"reproduc'i:r=a<p!",
    // HJP, Školski rječnik: razmàknuti, razmàkni, razmàknuo, but ràzmaknēm,
    // ràzmaknūt in every voice (`!` on its entry above); ràzmicati,
    // ràzmičēm
    u8"r/azmic=t", u8"r/azmič=e",
    // The tenth list, in every voice as the dictionaries have them:
    // prìtisnuti, prìtisnēm, prìtisni, prìtisnuo, prìtisnūt; pritískati,
    // pritískao, but prìtiskām and prìtīšćēm; ujedíniti, ujedíni,
    // ujedínio, but ujèdīnīm, ujèdīnjen; čárati (its falling present is in
    // the lexicon); očárati, začárati, but òčārām, òčārān, zàčārām
    u8"pr/itis=u", u8"prit'i:sk=a<!", u8"pr/itišć=e", u8"ujed'i:n=i<p!",
    u8"č/a:r=a", u8"oč'a:r=a<p!", u8"zač'a:r=a<p!",
};

// Towns, villages and regions (PLACES_*) and personal names and surnames
// (PERSONS_*) of Croatia, Bosnia and Herzegovina, Serbia, Montenegro, North
// Macedonia and Slovenia, generated by tools/formant/proper_names.py from
// tools/formant/places.tsv and tools/formant/persons.tsv (the accented forms
// and their sources).
#include "formant_proper_names.inc"

// English words whose stress the rules for English words (see "English
// words" in formant_frontend.cpp) do not find, generated by
// tools/formant/english_words.py from the Carnegie Mellon University
// Pronouncing Dictionary.
#include "formant_english.inc"

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
const char* const* lexicon_places_common(size_t& count) { return table(PLACES_COMMON, count); }
const char* const* lexicon_places_croatian(size_t& count) { return table(PLACES_CROATIAN, count); }
const char* const* lexicon_places_serbian(size_t& count) { return table(PLACES_SERBIAN, count); }
const char* const* lexicon_places_bosnian(size_t& count) { return table(PLACES_BOSNIAN, count); }
const char* const* lexicon_persons_common(size_t& count) { return table(PERSONS_COMMON, count); }
const char* const* lexicon_persons_croatian(size_t& count) { return table(PERSONS_CROATIAN, count); }
const char* const* lexicon_persons_serbian(size_t& count) { return table(PERSONS_SERBIAN, count); }
const char* const* lexicon_persons_bosnian(size_t& count) { return table(PERSONS_BOSNIAN, count); }
const char* const* lexicon_ije_verbs(size_t& count) { return table(IJE_VERBS, count); }
const char* const* lexicon_verbs(size_t& count) { return table(VERBS, count); }
const char* const* lexicon_english(size_t& count) { return table(ENGLISH, count); }

} // namespace formant
} // namespace laprdus
