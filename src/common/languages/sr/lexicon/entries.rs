/// The Serbian accent entries. A later entry overrides an earlier one of the
/// same spelling: the forms that are Serbian's own stand last, and correct
/// what the entries before them say of the words the language shares with
/// its neighbours.
#[rustfmt::skip] // the entries stand in rows under the comments that give their sources
pub(super) static ENTRIES: &[&str] = &[
    // Preserve verbal readings outside Croatian's exact ispuna paradigm.
    // The noun-twin verb below handles clause-initial imperative ìspuni.
    "/ispun|i|e:",
    // ---- Numbers (Hrvatski jezični portal; Wiktionary and the Serbian
    // dictionaries for the Serbian forms) ----
    "n^ul*", "j/edan", "j/edna", "j/edno", "d^va:", "dv^ije", "dv^e:",
    "t^ri:", "č/etiri", "p^e:t", "š^e:st", "s^edam", "^osam",
    "d^evet", "d^eset",
    // The teens and their ordinals (jedànaest, jedànaestī) share a stem; the
    // length of -naēst is in the Serbian and Bosnian tables.
    "jed'anaest*", "dv'a:naest*", "tr'i:naest*", "čet'rnaest*", "p'etnaest*",
    "š'esnaest*", "sed'amnaest*", "os'amnaest*", "dev'etnaest*",
    // Shared listening-based rendering of twenty and thirty: long falling
    // accents were chosen when a short vowel or a later pitch peak was
    // heard as two words ("dva deset"). The Croatian exact entries below
    // now restore dictionary dvádeset and its ordinal, and distinguish
    // dvadesétak. The remaining shared entries keep their existing tone.
    // Forty to ninety stress the "de"
    // (četrdèsēt); the Croatian voice has them without the final length
    // (in its own lexicon), the Serbian and Bosnian ordinals are dvadèsētī.
    "dv^a:deset*", "tr^i:deset*",
    "četrd'ese:t*", "ped'ese:t*", "šezd'ese:t*",
    "sedamd'ese:t*", "osamd'ese:t*", "deved'ese:t*",
    "st^o:", "dvj^esto", "dvj^esta", "dv^esta", "tr^isto", "tr^ista",
    "č^etiristo", "p^e:tsto", "š^e:ststo", "s^edamsto", "^osamsto",
    "d^evetsto",
    // The ordinals of the hundreds keep the accent of the cardinal: stȏtī,
    // dvjȅstōtī, trȉstōtī, petstȏtī (the length of -ōtī in the Serbian and
    // Bosnian tables)
    "st^o:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", "dvj^estot*", "dv^estot*",
    "tr^istot*", "č^etiristot*", "p^e:tstot*", "š^e:ststot*", "s^edamstot*",
    "^osamstot*", "d^evetstot*",
    // tìsuća, tìsuću, tìsuće, tìsućītī; after a number the genitive plural
    // tȉsūćā is a phrase rule of the front end. hȉljada, hȉljaditī.
    "t/isuć*", "h^iljad*",
    // milìjūn but milijúna, like the other nouns with a long last stem
    // syllable (StressRules::long_stem); milìjūntī, milìōnitī; milìjārda
    // (its length after the accent in the Serbian and Bosnian tables)
    "mil'iju:n", "milij'u:n|a|u|om|e|i|ima|sk*", "mil'io:n", "mili'o:n|a|u|om|e|i|ima|sk*",
    "mil/ijunt*", "mil/ionit*", "mil/ijard*",
    "bil'iju:n", "bilij'u:n|a|u|om|e|i|ima", "bil'io:n", "bili'o:n|a|u|om|e|i|ima",
    // Ordinals one to ten: pȓvī, drȕgī, trȅćī, čètvr̄tī, pȇtī, šȇstī, sȇdmī,
    // ȏsmī, dèvētī, dèsētī; the cardinals (pȇt, dȅvet) keep their own exact
    // entries, so the ordinals are paradigms
    "p'rv*", "dr^ug*", "tr^eć*", "č/etvrt|i|a|o|e|u|og|oga|om|ome|oj|ih|im|ima",
    "p^e:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", "š^e:st|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    "s^e:dm|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", "^o:sm|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    "d/evet|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", "d/eset|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    // dvȍje, trȍje, ȍba, ȍbje, dvòjica, tròjica, četvòrica, petòrica
    "dv^oje", "tr^oje", "^oba", "^obje", "dv/ojic*", "tr/ojic*",
    "četv/oric*", "pet/oric*",
    "z^arez", "c^ije:l*", "c^e:l*",

    // ---- Very frequent words: tone and length ----
    "d^a:n", "d^obar", "d^obro", "d^obra", "hv/a:la", "m^oli:m",
    "n^o:ć", "j^utro", "v^eče:r", "zdr^avo", "b^o:g", "d^a", "n^e",
    "v/oda", "v/ode", "ž/ena", "ž/ene", "j/ezik*", "k^uć*", "gl/a:v*",
    "r/u:k*", "n/og*", "s/estr*", "z/emlj*", "gr^a:d", "s^i:n",
    "m^a:jk*", "^otac", "br^at", "lj^u:di", "d/ije:te", "sv^ije:t",
    "r^ije:č", "vr/ije:me", "l/ije:p*", "ml/ije:k*", "r/ije:k*",
    "b/ije:l*", "mj^esto", "p^osao", "r/a:di*", "r^a:d",
    "zn^a:m", "zn^a:š", "zn^a:", "zn^a:mo", "zn^a:te",
    "^ima:m", "^ima:š", "^ima:",
    "s^ada", "s^ad", "t^ada", "k^ada", "/o:vdje", "/o:vde",
    "t/a:mo", "/ovamo", "d^anas", "s^utra", "j^uče:r", "j^uče:",

    // ---- Words with non-initial stress ----
    "doviđ'e:nja", "dobrod'oš*", "izv'ol*", "izv'i:ni*", "opr'osti*",
    "zahv'a:lj*", "gosp'odin*", "gosp'odo", "međ'utim", "več'eras",
    "ov'ako", "on'ako", "kol'iko", "tol'iko", "ukol'iko", "odj'ednom",
    // ȉnāče against the rule for -ač (ináče); uòstālom, ionàkō
    "^inače", "u'ostalom", "ion'ako",
    "otpr'ilike", "zan'imljiv*", "izgl'e:da*", "post'oj*",
    "raz'umije*", "raz'ume*", "raz'umje*", "gov'orio", "gov'orila",
    "gov'orili", "gov'oril*", "poč'e:tak", "poč'e:tk*", "završ'e:tak",
    "završ'e:tk*", "dod'a:tak", "dod'a:tk*", "pod'a:tak",
    "pod'a:tk*", "zad'a:tak", "zad'a:tk*", "ost'a:tak", "ost'a:tk*",
    "izuz'e:tak", "izuz'e:tk*", "tren'u:tak", "tren'u:tk*",
    "slob'od*", "zadov'oljstv*", "nem'oguć*",
    // jȅdnostavan, jȅdnostavno, jednostàvniji, jednostávnōst
    "j^ednostav*", "j^ednostavn*", "jednost'avnij*",
    "jednost'a:vno:st", "jednost'a:vnost*", "jednost'a:vnošću",
    // lègalan, ȉlegalan (the rule for -alan gives the first syllable, these
    // set the tone) and legálnōst, ilegálnōst
    "l/egal*", "l/egaln*", "leg'a:lno:st", "leg'a:lnost*", "leg'a:lnošću",
    "^ilegal*", "^ilegaln*", "ileg'a:lno:st", "ileg'a:lnost*", "ileg'a:lnošću",
    // slobòda, but the adjective and adverb slȍbodan, slȍbodno; the name
    // Slobòdan keeps its cases (Slobòdana), the nominative goes to the
    // commoner adjective
    "sl^obodan", "sl^obodn*", "sl^obodnij*", "sl^obodn|oga|ome|ima",
    // snága, snázi, snážan, snážno
    "sn/a:g*", "sn/a:zi", "sn/a:žan", "sn/a:žn*", "sn/a:žnij*",
    "intelig'ent*",

    // ---- Abstract nouns in -ina ----
    "brz'in*", "vis'in*", "duž'in*", "šir'in*", "dub'in*", "dalj'in*",
    "topl'in*", "tiš'in*", "vruć'in*", "velič'in*", "količ'in*",
    "tež'in*", "sred'in*", "plan'in*", "dol'in*", "već'in*",
    "manj'in*", "cjel'in*", "cel'in*", "bliz'in*", "jač'in*",
    "glasn'oć*",

    // ---- Technology and screen reader vocabulary ----
    "sint'e:z*", "rač'unal*", "rač'una:r*", "kompj'u:ter*", "m/obitel*",
    "datot'e:k*", "pretraž'ivač*", "kontr'o:l*", "adr'es*", "tastat'u:r*",
    "'internet*", "^instagram*", "'auto", "'autor*", "aut'obus*", "autom'obi:l*",
    "aut'omat*", "telev'i:zor*", "kil'omet*", "cent'imet*", "mil'imet*",
    "sek'u:nd*", "min'u:t*", "r/a:dio", "L'aprdus*", "l'aprd*",
    // emòtikon in every case; not in the dictionaries, accented like
    // emòcija, against the rule for loans in -on (emotikóna)
    "em'otikon*",
    // ubrzánje (HJP), the participle and adverb ȕbrzan, ȕbrzano; the verb
    // ubr̀zati is in VERBS
    "ubrz'a:nj*", "^ubrzan*",
    // podèsiv, podèsivo like the other adjectives in -iv from prefixed
    // verbs (izvèdiv, dokàziv, podnòšljiv, prilagòdljiv in HJP; the word
    // itself is in no dictionary), podesívōst like održívōst
    "pod'esiv*", "nepod'esiv*", "podes'i:vost*", "podes'i:vošću",
    // mȍnoton, mȍnotono in every form (HJP); the noun monotònija and the
    // comparative monotòniji need their own stem, the shorter one would
    // take them
    "m^onoton*", "monot'onij*",
    // pàuza, sȁuna, fàuna (HJP): the long-stem rule had taken "au" for a
    // long u (paúza)
    "p/auz*", "s^aun*", "f/aun*",
    // rȍđendān, rȍđendāna (HJP, Školski rječnik, Wiktionary); the default
    // rules stressed the second syllable (rođèndan, rođendána)
    "r^ođendan*",
    // kòrisnīk, kòrisnica, kòrisnički (HJP) and the adjective kȍristan,
    // kȍrisno, the noun kȍrīst; the verb kòristiti is in VERBS, and
    // "koristi" is read as the noun
    "k/orisnik*", "k/orisnic*", "k/orisniče", "k/orisničk*",
    "k^oristan", "k^orisn|a|o|i|e|u|og|om|oj|ih|im|ima|oga|ome",
    "k^orist", "k^oristi",
    // rédak, rétka, réci, rȇdākā (HJP), also spelled redka, redci; "reci"
    // stays the imperative rèci, the far commoner word
    "r/e:dak", "r/e:tk|a|u|om|e|i|o|ih|im|ima|og|oj", "r/e:dk|a|u|om|e|i",
    "r/e:tc|i|ima", "r/e:dc|i|ima", "r/e:cima", "r/eci", "r^e:daka",

    // ---- Age and time ----
    // čèstitka, čèstitke, G pl čèstitākā/čèstitkā, DL čèstitki/čèstitci
    // (Školski rječnik and Rečnik Matice srpske; HJP čȅstitka); the rule for
    // a long last stem syllable had čestítka
    "č/estitk*", "č/estitaka", "č/estitci",
    // gòdišnjāk, gòdišnjāka (HJP, Školski rječnik, Rečnik Matice srpske);
    // its compounds (petogòdišnjāk) and the other compounds of -godišnji,
    // -mjesečni, -dnevni, -tjedni and -ljetan are a rule
    // (StressRules::time_compound)
    "g/odišnjak*", "g/odišnjac|i|ima", "g/odišnjače",

    // ---- Places ----
    "Z'a:greb*", "Be'ograd*", "S/arajev*", "Eur'o:p*",
    "Evr'o:p*", "Am'erik*", "H'rva:tsk*", "H'rva:t*",
    "S'rbij*", "s'rpsk*", "B^osn*", "b^osa:nsk*", "H/ercegovin*",
    "'Austrij*", "Austr'a:lij*", "C'rn*",
    // The adjectives of the counties and regions (Zagrebačka, Primorsko-
    // goranska, Ličko-senjska, Šibensko-kninska, Splitsko-dalmatinska,
    // Dubrovačko-neretvanska, Međimurska, Koprivničko-križevačka,
    // Karlovačka županija), HJP: zágrebačkī, kȃrlovačkī, kòprīvničkī,
    // kríževačkī, prímorskī, lȋčkī, knȋnskī, dalmàtīnskī, nerétvanskī,
    // međìmurskī
    "z/a:grebačk*", "k^a:rlovačk*", "k/oprivničk*", "kr/i:ževačk*", "pr/i:morsk*",
    "l^i:čk*", "kn^i:nsk*", "dalm/atinsk*", "ner/e:tvansk*", "međ/imursk*",
    // àustrījskī, aùstrālījskī (HJP): the rule for -ijski would take the
    // forms the stems above do not reach (austrijskog)
    "/austrijsk*", "a/ustralijsk*",

    // ---- Days ----
    "pon'edjelj*", "pon'edelj*", "^utor*", "sr/ije:d*", "sr/e:d*",
    "četv'rtak", "četv'rtk*", "p/e:tak", "p/e:tk*", "s^ubot*", "n^edjelj*", "n^edelj*",

    // ---- Exceptions to the -ina rule (see StressRules::strong) ----
    "g^odin*", "^istin*", "c^arin*", "l/avin*", "st^otin*", "lj^etin*",
    "p/okrajin*", "/otadžbin*", "p/ostojbin*", "sudb'in*", "deset'in*",
    "sv^injetin*", "g/ovedin*", "p/iletin*", "j/anjetin*", "j/agnjetin*",
    "t/eletin*", "j/unetin*", "p^aučin*", "K^utin*", "Kr^apin*",
    "j/edina", "j/edine", "j/edini", "j/edinu", "j/edino", "j/edinom",
    "j/edinog", "j/edinoj", "j/edinih", "j/edinim", "j/edinoga",
    // loans with a long i
    "maš'i:n*", "kab'i:n|a|e|i|u|om|ama", "vitr'i:n*", "medic'i:n*", "vakc'i:n*",
    "benz'i:n*", "discipl'i:n*", "rut'i:n*", "kuž'i:n*", "terr'i:n*",

    // ---- Nouns in -ica with non-initial stress ----
    "učit'eljic*", "jed'inic*", "tipk'o:vnic*", "uči'onic*", "radi'onic*",
    "lub'enic*", "gol'ubic*", "kob'asic*", "prodav'aonic*", "spav'aonic*",
    "čit'aonic*", "bolnič'a:rk*",

    // ---- Frequent words the rules get wrong ----
    "ob'i:telj*", "kal'enda:r*", "svej'edno", "bic'ikl*", "gosp'ođic*",
    "tak'o:đer", "stan'o:vni*", "stan'o:vnik*", "sveuč'ilišn*",
    "iz'u:zetn*", "pojed'in*", "pojed'inac", "pojed'inc*", "ist'ovremen*",
    "vjer'ojatn*", "ver'ovatn*", "nar'avn*",
    "objašnj'e:nj*", "/obrazovanj*", "/obrazovn*", "/obrazov*", "/obrazuj*", "infor'ma:cij*",

    // ---- Nouns in -tak: the plural loses the t (podátak, podáci), the
    // genitive plural moves the accent to the front (pòdātākā; Zvonko
    // without the lengths after the accent, as gùbitaka below; the
    // lengths are in the Serbian and Bosnian tables) ----
    "pod'a:|ci|cima|tci|tcima|tče", "p/odataka",
    "dod'a:|ci|cima|tci|tcima|tče", "d/odataka",
    "zad'a:|ci|cima|tci|tcima|tče", "z/adataka",
    "ost'a:|ci|cima|tci|tcima|tče", "/ostataka",
    "poč'e:|ci|cima|tci|tcima|tče", "p/očetaka",
    "završ'e:|ci|cima|tci|tcima|tče", "zav'ršetaka",
    "izuz'e:|ci|cima|tci|tcima|tče", "iz'uzetaka",
    "tren'u:|ci|cima|tci|tcima|tče", "tr/enutaka",
    // The adjectives and adverbs keep the first syllable (dȍdatno, trȅnutno).
    "d^odatan", "d^odatn*", "d^odatno*", "d^odatni*",
    "tr^enutan", "tr^enutn*", "tr^enutno*", "tr^enutni*",
    // nȅpregledan is an adjective of its own (vast; unreviewed), with the
    // first syllable in every form, unlike the negated participles
    // (nepròčitan, neprèslušan, nedòvršen; see StressRules::form_of);
    // the comparative is nepreglèdniji
    "n^epregledan*", "n^epregledn*", "nepregl'ednij*",
    // nȅpoznat the same (pòznat, but the negated adjective moves to ne-)
    "n^epoznat*",

    // ---- obavijest / obavest and the verbs beside it ----
    // Noun: ȍbavijēst in every case. "obavijesti" is also the verb's
    // imperative, aorist and third person; the noun has priority.
    "^obavijest*", "^obaviješću", "^obave:st*", "^obave:šću",
    // obavijéstiti: infinitive, future, imperative, aorist, participle
    "obav'ijest|iti|it|ite|io|ila|ilo|ili|ile|ih|ismo|iste|iše|ivši",
    "obav'ijest|iću|ićeš|iće|ićemo|ićete",
    "obav'e:st|iti|it|ite|io|ila|ilo|ili|ile|ih|ismo|iste|iše|ivši",
    "obav'e:st|iću|ićeš|iće|ićemo|ićete",
    // present obàvijēstīm, passive participle obàvijēšten
    "ob'avijest|i:m|i:š|i:mo|e:", "ob'aviješten*",
    "ob'ave:st|i:m|i:š|i:mo|e:", "ob'ave:šten*",
    "obavešt'e:nj|e|a|u|em|ima",
    // obavještávati follows the rules for verbs in -avati; only its passive
    // participle needs an entry
    "obavješt'a:van*", "obavešt'a:van*",

    // ---- Irregular verbs whose commands are everywhere on a screen ----
    // započeti, preuzeti, oduzeti...: zapòčni, preùzmi; pronaći: pronáđi;
    // prevesti: prevèla. (The present of these is in the Croatian table.)
    "zap'oč|eti|et|ni|nimo|nite", "pre'uz|eti|et|mi|mimo|mite",
    "od'uz|eti|et|mi|mimo|mite", "pod'uz|eti|et|mi|mimo|mite",
    "za'uz|eti|mi|mimo|mite", "pron'a:đ|i|imo|ite",
    // zauzeti is the exception among them: the dictionaries' zȁuzēt (also
    // the adjective "busy"), zȁuzeo, zȁuzētōst on the prefix in every voice,
    // the verbal noun zauzéće (HJP, Školski rječnik); only the infinitive and
    // imperative keep zaùzēti, zaùzmi, and the plural adjective zȁuzēti
    // gives way to the infinitive
    "z^auzet*", "z^auz|eo|ela|elo|eli|ele", "zauz'e:ć*",
    "prev'el|a|o|i|e", "dov'el|a|o|i|e", "uv'el|a|o|i|e", "izv'el|a|o|i|e",
    "prov'el|a|o|i|e", "odv'el|a|o|i|e", "nav'el|a|o|i|e", "zav'el|a|o|i|e",

    // ---- Passive participles of prosljeđívati, nasljeđívati ----
    // (everything else of these verbs comes from the rules for -ivati and
    // -ujem; proslijediti, naslijediti, uslijediti from IJE_VERBS and VERBS)
    "prosljeđ'i:van*", "nasljeđ'i:van*", "prosleđ'i:van*", "nasleđ'i:van*",

    // Verbs in -ovati that keep the accent on the first syllable in the
    // present, against the rule for -ujem (nàpredujem, sùdjelujem,
    // sávjetujem), and nouns that look like such a present (olúja, kravàta)
    "n/apreduj*", "n/azaduj*", "s/udjeluj*", "s/a:vjetuj*", "p/o:sjeduj*",
    "/obraduj*", "p/ovjeruj*", "d/oručkuj*", "pr/isustvuj*",
    "ol'u:j*", "krav'at*", "samo'uprav*", "/obustav|a|e|i|u|om",
    "z/aborav|a|u|om",

    // Words that look like forms of a verb from VERBS: sprȅman (ready) is not
    // the participle of spremati; nàčin, nàčina are not forms of načiniti and
    // not nouns in -ina either
    "spr^eman", "n/ačin*",

    // "Too ..." adjectives that look like verbs from IJE_VERBS (prȅlijep)
    "pr^elijep*", "pr^etijesn*", "pr^elijen*", "pr^ebijel*", "pr^esvijetl*",

    // ---- Names of the voices ----
    // Zvónko and Vládo have a long rising accent (Stòjan, Mìrsad and Jòsip
    // are the default). "vlada", "vlade" stay with the noun.
    "Zv/o:nk*", "Zv/o:nkov*", "Vl/a:do", "Vl/a:din*",
    // Zvȍnimīr in every case (the length after the accent is left out, as
    // in the other entries); the rule for loans in -ir would give Zvonimíra
    "Zv^onimir*",

    // ---- Nouns in -enje of three syllables ----
    // The rule for -enje starts at four syllables, because the short ones
    // go both ways: rođénje, rješénje, kršténje, pošténje, stvorénje,
    // snižénje, but ùčenje, mìšljenje, vȉđenje, which the default gets
    // right. (rȍđendan keeps the first syllable.)
    "rođ'e:nj*", "rješ'e:nj*", "reš'e:nj*", "kršt'e:nj*", "pošt'e:nj*",
    "stvor'e:nj*", "sniž'e:nj*", "zn/a:čenj*",

    // ---- Abstract nouns in -nost with a long rising accent ----
    "mog'u:ćno:st", "mog'u:ćnost*", "mog'u:ćnošću",
    "nemog'u:ćno:st", "nemog'u:ćnost*", "nemog'u:ćnošću",
    "sig'u:rno:st", "sig'u:rnost*", "sig'u:rnošću", "sig'u:rnosn*",
    "nesig'u:rno:st", "nesig'u:rnost*", "nesig'u:rnošću",
    "priv'a:tno:st", "priv'a:tnost*", "priv'a:tnošću",

    // ---- Nouns whose accent moves in the oblique cases ----
    // sìgnal, signála; telèfon, telefóna; novčànīk, novčaníka
    "s/ignal", "sign'a:l|a|u|om|e|i|ima",
    // telèfon, telefóna (the nominative comes from the rule for -fon)
    "telef'o:n|a|u|om|e|i|ima", "gramof'o:n|a|u|om|e|i|ima", "tel'efonsk*",
    // pȍruka, prȅporuka, ȉsporuka: falling tone on the first syllable
    "p^oruk*", "p^oruci", "pr^eporuk*", "pr^eporuci", "^isporuk*", "^isporuci",
    // mȉkrofon, sȁksofon, mȅgafon keep the first syllable in every case
    "m^ikrofon*", "s^aksofon*", "m^egafon*",
    "novč'ani:k", "novčan'i:|ka|ku|kom|ke|ci|cima", "n^ovčani:če",

    // ---- pòzadina against the -ina rule, kòntrolnī against kontróla ----
    "p/ozadin*", "p/ozadinsk*",
    "k/ontroln*", "k/ontrolno*", "k/ontrolni*",

    // ---- First names with non-initial stress ----
    "Aleks'a:ndar", "Aleks'a:ndr*", "Katar'i:n*", "Krist'i:n*", "Valent'i:n*",
    "Nikol'i:n*", "Karol'i:n*", "Paul'i:n*", "Jasm'i:n*", "Em'i:n*", "Am'i:n*",
    "Sab'i:n*", "Alb'i:n*", "Reg'i:n*", "Georg'i:n*", "Ir'e:n*", "Hel'e:n*",
    "mart'i:na", "mart'i:ne", "mart'i:ni", "mart'i:nu", "mart'i:nom",
    "mar'i:na", "mar'i:ne", "mar'i:ni", "mar'i:nu", "mar'i:nom",
    "marij/ana", "marij/ane", "marij/ani", "marij/anu", "marij/anom",
    "kristij'a:na", "kristij'a:ne", "kristij'a:ni", "kristij'a:nu", "kristij'a:nom",
    // Nàtaša like Mìrjana (the rule for -aš would give Natáša)
    "N/ataš*", "N/atašin*",
    "Daj'a:n*", "T/ijan*", "Dij'an*", "Mih'ovil*", "Muh'amed*", "Hus'ein*",
    "Ibr'a:him*", "Sul'ejman*", "Slob'odan*", "S/iniš*", "Mih'ael*",
    "elv'i:ra", "elv'i:re", "elv'i:ri", "elv'i:ru", "elv'i:rom",
    "Ant'o:nio", "Ant'o:nij*", "Ant'o:nia", "Ant'o:nie", "Ant'o:niu",
    "Ren'a:t*", "S^a:ndr*", "Natal'ij*", "Vikt'o:rij*", "D/anijel*", "D/aniel*",

    // ---- Surnames: exceptions to the -ović/-ević rule ----
    "'ivanović*", "j'osipović*", "m'aksimović*", "dr'agović*", "v'idović*",
    // Listener corrections: Ìsaković stresses the initial short i (a is
    // unstressed and short); Spàsojević stresses Spa, with short unstressed
    // o. Override the generic
    // surname rule in every language, including cases and possessives.
    "/Isaković||a|u|em|i|e|ima", "/Isakovićev*",
    "Sp/asojević||a|u|em|i|e|ima", "Sp/asojevićev*",
    // HJP onomastics under knez: Knȇzović and Knéžević, both long initial e.
    // Full surname stems avoid changing the noun plurals knȅzovi / knéževi.
    "Kn^e:zović||a|u|em|i|e|ima", "Kn^e:zovićev*",
    "Kn/e:žević||a|u|em|i|e|ima", "Kn/e:ževićev*",

    // ---- More places ----
    // Varàždin, Vukòvār and Kòprīvnica are the second forms of HJP (Vȁraždīn,
    // Vȕkovār); Bjȅlovār, Virovìtica, Ȍgulīn, Ivánec, Slàvōnija and Dàlmācija
    // as HJP has them. The place-name table (formant_proper_names.inc) has these
    // and some four thousand more with their case forms; the entries here
    // also serve lowercase text and the derived words their stems reach.
    // (visok* is the adjective vìsok, visòka, visòko; the town Vìsokō is in
    // the place-name table.)
    "Var'aždin*", "Kr'agujevac", "Kr'agujevc*", "Mak'edo:nij*", "At'e:n*",
    "Kopenh'a:gen*", "Vuk'ova:r*", "Bj^elovar*", "Virov/itic*", "Crikv'enic*",
    "^Ogulin*", "Iv/a:nec", "Iv/a:nc*", "S'ubotic*", "P'odgoric*", "K'oprivnic*",
    "Prij'e:dor*", "Vis'ok*", "Sl/avonij*", "D/almacij*", "Mad'ri:d*",
    "Berl'i:n*", "Par'i:z*", "Lond'o:n*", "Ljublj'an*", "Beogr'a:đan*",
    "Zagrepč'an*", "Spl'ićan*", "Riječ'an*", "Sarajl'ij*",

    // ---- Food, places in town, everyday loans ----
    "rest'ora:n*", "aer'odrom*", "apot'e:k*", "ljek'a:rn*", "trg'ovin*",
    "kupa'onic*", "bibliot'e:k*", "ban'a:n*", "čokol'a:d*", "sal'a:t*",
    "par'adajz*", "fak'ulte:t*", "sveuč'ilišt*", "ćev'a:p*",

    // ---- Words shaped like the loans with a long last stem syllable
    // (StressRules::long_stem) that keep their native accent ----
    // zákon, nágrada; spȍmenīk, zàmjenīk, ùdžbenīk, zlòčin next to zločínac;
    // vòjnīk but vojníka; mȅnadžer, hȁmburger
    "z/a:kon*", "n/a:grad*", "sp^omenik*", "z/amjenik*", "'udžbenik*",
    "zl/očin*", "zloč'inc*", "zloč'inac", "v/ojnik", "v/ojniče",
    "vojn'i:|ka|ku|kom|ci|cima|ke", "m^enadžer*", "h^amburger*",
    "čvrst'in*", "četvrt'in*", "pol'ovic*", "činj'enic*", "b'aterij*",
    "b'aterijsk*",
    "/ogledal*", "četvrt'i:nk*", "Dalmat'i:nk*", "b/ogat*", "pr^evar*", "s/udar*",
    "dom'aćin*", "dom'aćic*", "B/alkan", "Balk'a:n|a|u|om|e|i|ima", "balk'a:nsk*",
    // Three-syllable nouns in -ina (the rule needs four syllables, see
    // StressRules::strong): abstract nouns and loans with a long i
    "ravn'in*", "niz'in*", "treć'in*", "osm'in*", "pet'in*", "šest'in*",
    "sedm'in*", "devet'in*", "čist'in*", "led'in*", "vrl'in*", "tuđ'in*",
    "crn'in*", "bjel'in*", "bel'in*", "modr'in*", "vedr'in*", "puč'in*",
    "turb'i:n*", "doktr'i:n*", "kant'i:n*", "delf'i:n*", "dup'i:n*", "pingv'i:n*",
    "term'i:n|a|u|om|e|i|ima",
    // ùmire (umrijeti), not umíre (umiriti) and not papíre
    "'umir|e|em|eš|emo|ete|u|ući", "'izumir|e|em|eš|emo|ete|u|ući",
    // zȅmlja against zemljáka, bolèsnīk, bolesníka
    "z/emljak", "zemlj'a:|ka|ku|kom|ci|cima|ke|če|kov*", "bol'esnik", "bol'esniče",
    "bolesn'i:|ka|ku|kom|ci|cima|ke", "bol'esnic*",
    // Verbs in -ovati that keep the first syllable against the rule for
    // -ovati (vjȅrovati, vȅrovati, rȁdovati se, mìlovati, ljȅtovati), with
    // their present (vjȅrujēm, ljȅtujēm), verbal noun (vjȅrovānje, rȁdovānje,
    // ljȅtovānje) and the fused future forms the stems do not reach
    "vj^erov*", "v^erov*", "r^adov*", "m/ilov*", "lj^etov*", "l^etov*",
    "vj^eruj*", "v^eruj*", "lj^etuj*", "l^etuj*",
    "vj^erovanj*", "v^erovanj*", "r^adovanj*", "lj^etovanj*", "l^etovanj*",
    "vj^erov|aćeš|aćemo|aćete|avši", "v^erov|aćeš|aćemo|aćete|avši",
    "r^adov|aćeš|aćemo|aćete|avši", "m/ilov|aćeš|aćemo|aćete|avši",
    "lj^etov|aćeš|aćemo|aćete|avši", "l^etov|aćeš|aćemo|aćete|avši",

    // ---- Words checked in October 2026 against Školski rječnik
    // hrvatskoga jezika, Hrvatski mrežni rječnik, HJP and Wiktionary, with
    // their whole paradigms (the lengths after the accent are in the
    // Serbian and Bosnian tables) ----
    // The personal pronouns: jȃ, tȋ, ȏn, mȋ, vȋ, òni, òna, òno, òne and
    // their long forms; the clitics (me, ti, ga, nas, ...) stay unstressed
    "j^a:", "t^i:", "^o:n", "m^i:", "v^i:", "/oni", "/ona", "/ono", "/one",
    "mn^o:m", "nj^i:m", "nj^e:", "nj^o:j", "nj^u:", "n^a:s", "v^a:s",
    "nj^i:h", "nj^ima", "m^ene", "m^eni", "t^ebe", "t^ebi", "t^obom",
    "nj^ega", "nj^emu", "n^ama", "v^ama",
    // zȁjednički; vȋd, vȋda, u vídu, vȉdovi; iskústvo, ìskūstāvā; àlāt,
    // aláta; mòdel, modèla; progràmērskī; èkrān, ekrána; záslon; znánje;
    // modèrātor, modèrātorica; surádnja (Školski rječnik; HJP suràdnja),
    // saràdnja; sùdjelovati, ùčestvovati (the present sùdjelujēm,
    // ùčestvujēm with the verbs in -ovati below)
    "z^ajedničk*", "v^i:d", "v^i:da", "v^i:dom", "v/i:du", "v^idov|i|a|e|ima",
    "isk'u:stv*", "/iskustava", "/alat", "al'a:t|a|u|om|i|e|ima",
    "m/odel", "mod/el|a|u|om|i|e|ima", "progr/amersk*", "/ekran",
    "ekr'a:n|a|u|om|i|e|ima", "z/a:slon*", "zn/a:nj*", "mod/erator*",
    "mod/eratoric*", "mod/eratork*", "sur'a:dnj*", "sar/adnj*", "s/udjelov*",
    "s/udjelovanj*", "s/udelov*", "s/udeluj*", "s/udelovanj*", "/učestvov*",
    "/učestvuj*", "/učestvovanj*",
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
    "p/otpun*", "potp/unij*", "m^aslin*", "m^aslinov*", "o/a:z*", "rak/et*",
    "kr^a:lj", "kr/a:lj|a|u|em|om", "kr/a:ljev*", "/uistinu", "/uistini",
    "higij'e:n|a|e|i|u|o|om|ama", "j^a:vn*", "j^a:van", "j/avnij*", "sr^ednj*",
    "ek'i:p*", "ž/eljezničar*", "ž/eljeznic*", "ž/eljezničk*", "ž/elezničar*",
    "ž/eleznic*", "ž/elezničk*", "strelj'a:štv*", "d/irektan", "d/irektn*",
    "dir/ektnij*", "dr^a:g", "dr/a:ga", "dr^a:gi", "dr^a:go",
    "dr^až|i|a|e|eg|em|oj|ih|im", "međun'a:rodn*", "međun'a:rodan",
    // Superlative adverbs outside the rule for naj- (StressRules::strong):
    // nȃjposlije, nȃjposlē, nȃjprē, nȁjzad; pònājprije, pònājviše
    "n^a:jposlije", "n^a:jposle", "n^a:jpre", "n^ajzad", "p/onajprije",
    "p/onajviše",
    "od/rživ*", "održ/ivij*", "kamp/anj*",
    // The nouns in -ost and the adjectives beside them, whose stems would
    // otherwise end the entries above after three letters (jȃvnī but
    // jávnōst, javnosti; pòtpunōst; odr̀živōst) or take the noun's accent
    // (iskùstven against iskústvo); srȅdnjoškolskī like srȅdnjī
    "j/a:vnost*", "j/a:vnošću", "p/otpunost*", "p/otpunošću", "od/rživost*",
    "od/rživošću", "isk/ustven*", "sr^ednjoškolsk*",
    // protéza in every form, protètika, protètičār; zȗbnī, òčnī (zubna,
    // očna proteza)
    "prot/e:z*", "prot/etik*", "prot/etičar*", "z^u:bn*", "/očn*",
    // inspirírati with the dictionaries' shifts (inspìrīrām, inspìrīrān,
    // inspirírajū), Serbian inspìrisati, inspìrišēm, inspìrisan
    "inspir'i:raju", "inspir'i:rajući",
    "razv'i:jaju", "razv'i:jajući", "razv/ijen*", "p/otaći", "p/otakao",
    "p/otakl|a|o|i|e", "p/odstaći", "p/odstakao", "p/odstakl|a|o|i|e",
    // Names (HJP, Wiktionary): Sȃndra, Sìniša, Dànijel(a), Dàniel(a),
    // Mìhājlo, Ànita, Ȇva, Mája, Máto (from Máte; no stem, mati and matu are
    // words), Mátić, Ìvanović, Aleksándar, Teodóra (the man's name is
    // Tèodor; "Teodora" is read as hers), Crnògorac with Crnogórci,
    // Ivànščica, bȑkljača. Without a dictionary entry, from names and words
    // of the same shape: Nàida, Adrijàna like
    // Dijàna and Marijàna, Mȉlosava like Mȉroslava, Márkovina from Mȃrko like
    // králjevina, Prȗgovečkī from Prȗgovac, Vládić from Vládo,
    // Tȉhić, Rȍtić, Lȅtić, Jȕsić
    "M/ihajl*", "/Anit*", "^E:v|a|i|u|om", "M/a:j|a|e|i|u|o|om", "M/a:to",
    "M/a:tić*", "Teod/o:r|a|e|i|u|o|om", "T/eodor", "Crn/ogorac",
    "Crn/ogorc|a|u|em", "Crn/ogorče", "Crnog'o:rc|i|ima|e", "Crn/ogoraca",
    "Crn/ogork*", "crn/ogorsk*", "Iv/anščic*", "Iv/ančic*", "B^rkljač*",
    // Enída: the listener's long stressed i replaces the earlier analogy
    // Ènida; the possessive needs its own stem for forms such as Enídinima.
    "En/i:d|a|e|i|u|o|om|ama", "En/i:din*",
    "N/aid*", "Adrij/an|a|e|i|u|o|om", "M^ilosav*", "M/a:rkovin*",
    // Listener correction: gre-BLIČ-ki, retaining the short vowel.
    "Pr^u:govečk*", "Grebl/ičk*", "Vl/a:dić*", "T^ihić*", "R^otić*",
    "L^etić*", "J^usić*",

    // Foreign app names after internal.json's pronunciation replacements.
    // Collins/Oxford: initial stress in Messenger and TikTok; falling tone
    // is our Croatian adaptation, not an English lexical pitch accent.
    // WinTalker: the user's vin-TO-ker, with short rising o.
    "m^esindžer||a|u|e|om", "t^iktok||a|u|e|om",
    "vint/oker||a|u|e|om",

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
    "pr^ijatelj*", "prijat/eljic*", "prijat/eljič*", "prijat/eljsk*",
    "prijat/eljuj*",
    "n/eprijatelj*", "neprijat/eljic*", "neprijat/eljsk*", "neprij/ateljstava",
    // potencìjāl by the rule for -al, potencijála in the other cases, like
    // the other nouns in -ijal (materìjāl, materijála; serìjāl, memorìjāl,
    // tutorìjāl, ceremonìjāl); the adjective pȍtencijālan, pȍtencijālno in
    // every form, but potencijálnōst
    "potencij'a:l|a|u|om|e|i|ima", "materij'a:l|a|u|om|e|i|ima",
    "serij'a:l|a|u|om|e|i|ima", "memorij'a:l|a|u|om|e|i|ima",
    "tutorij'a:l|a|u|om|e|i|ima", "ceremonij'a:l|a|u|om|e|i|ima",
    "p^otencijal|an|na|no|ni|ne|nu|nog|nom|noj|nih|nim|noga|nome|nomu|nima",
    "potencij'a:lno:st", "potencij'a:lnost*", "potencij'a:lnošću",
    // snȃći, snȃđēm, but snáđi, snáđoh and snàšao (Školski rječnik; HJP has
    // snȁšao); snàlaziti and snàlažēnje come from the rule for -laziti,
    // snalàžljiv from the rule for -ljiv
    "sn^a:ći", "sn^a:đ|em|eš|e|emo|ete|u", "sn/a:đ|i|imo|ite|oh|osmo|oste|oše",
    "sn/aš|ao|la|lo|li|le|avši",
    // simùlātor, simùlātorskī (HJP, Rečnik Matice srpske) against the rule
    // for -ator; simulácija and simulácījskī are the rule for -ij
    "sim/ulator*", "sim/ulatorsk*",
    // dožívjeti, dožívio, dožívjela, imperative dožívi, dožívite. The
    // Croatian dictionaries move the present to the prefix (dòžīvīm),
    // Rečnik Matice srpske and the Bosnian dictionary keep it (dožívīm), and
    // so does the Croatian voice, as with the verbs in VERBS. The passive
    // participle is dòžīvljen in all of them, the noun dȍživljāj. The same
    // for preživjeti, proživjeti, nadživjeti, poživjeti, and for žívjeti.
    "dož'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    "dož'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    "prež'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    "prež'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    "prož'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    "prož'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    "nadž'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    "nadž'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    "pož'i:v|jeti|jet|jela|jelo|jeli|jele|jeh|je|jesmo|jeste|ješe|jevši|io|i|imo|ite|im|iš|e|jeću|jećeš|jeće|jećemo|jećete",
    "pož'i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    "ž/i:v|jeti|jet|jela|jelo|jeli|jele|jeh|jesmo|jeste|ješe|jevši|io|im|iš|imo|ite|jeću|jećeš|jeće|jećemo|jećete",
    "ž/i:v|eti|et|ela|elo|eli|ele|eh|esmo|este|eše|evši|eo|eću|ećeš|eće|ećemo|ećete",
    // The participle as an adjective (prežívjelī, broj prežívjelīh): its
    // declension falls back to these stems
    "dož'i:vjel*", "dož'i:vel*", "prež'i:vjel*", "prež'i:vel*", "prož'i:vjel*",
    "prož'i:vel*", "nadž'i:vjel*", "nadž'i:vel*", "pož'i:vjel*", "pož'i:vel*",
    "ž/i:vjel*", "ž/i:vel*",
    // dòžīvljen, but the nouns preživljénje, doživljénje
    "d/oživljen*", "pr/eživljen*", "pr/oživljen*", "n/adživljen*",
    "doživlj'e:nj*", "preživlj'e:nj*", "proživlj'e:nj*", "nadživlj'e:nj*",
    "d^oživljaj*", "doživlj'a:jno:st", "doživlj'a:jnost*",
    // poštovánje, the noun (HJP, Školski rječnik, Rečnik Matice srpske),
    // nepoštovánje, samopoštovánje; the verb poštòvati, pòštujēm by the
    // rules for -ovati and -ujem; its participle pȍštovān, pȍštovānī;
    // poštòvatelj, poštòvalac
    "poštov'a:nj*", "nepoštov'a:nj*", "samopoštov'a:nj*", "p^oštovan*",
    "pošt/ovatelj*", "poštovat/eljic*", "pošt/ovalac", "pošt/ovalaca",
    "pošt/ovaoc*", "p/oštuj", "pošt/ova",
    // ȉnteres and ȉnterēsnī in every form (an exact paradigm: a stem would
    // take interesira); interesàntan is the rule for -antan, interesírati
    // the rule for -irati. The Serbian and Bosnian ȉnteresovati,
    // ȉnteresujēm, zàinteresovati, zàinteresujēm, nezàinteresovān (Rečnik
    // Matice srpske) against the rules for -ovati and -ujem
    "^interes||a|u|e|om|i|ima|ni|na|no|ne|nu|nog|nom|noj|nih|nim|nima|noga|nome|nomu",
    "^interesov*", "^interesovanj*", "^interesuj*", "^interesujući",
    // The adjectives of nouns in -ant, -ent keep the noun's accent against
    // the rule for -antan (fòrmant, fòrmantnī; pàtentnī, gàrantnī,
    // dijàmantan, cèmentnī, sègmentan, pìgmentnī, tàngentnī, prézentnī in
    // HJP and Školski rječnik), and so does mòmēntan
    "f/ormant*", "f/ormantn*", "p/atentn*", "g/arantn*", "dij/amant*",
    "dij/amantn*", "c/ementn*", "s/egmentan", "s/egmentn*", "p/igmentn*",
    "t/angentn*", "m/omentan", "m/omentn*", "pr/e:zentn*",
    "z/ainteresov*", "z/ainteresovan*", "z/ainteresovanost*", "z/ainteresuj*",
    "z/ainteresujući", "nez/ainteresovan*", "nez/ainteresovanost*",
    "^interesov|aćeš|aćemo|aćete|avši", "z/ainteresov|aćeš|aćemo|aćete|avši",

    // ---- The fourth October 2026 list (HJP, Školski rječnik, Wiktionary,
    // Rečnik Matice srpske; all agree) ----
    // Ègipat, Ègipta in every case against the rule for -at; ègipatskī,
    // Ègipćanin, Ègipćani, Ègipćānka
    "/egip*", "/egipatsk*", "/egipćan*", "/egipćank*",
    // proizvòdnja in every case; proìzvod, proìzvoda (an exact paradigm: a
    // stem would take proizvòditi, proizvòdio). The adjective stays on the
    // first syllable as in HJP (prȍizvodnī). The verbs are in VERBS, except
    // proìzvesti, proìzveo and the verbal noun proìzvođēnje
    "proizv'odnj*", "pro'izvod||a|u|om|e|i|ima|eći",
    "pro'izve|sti|st|o|la|lo|li|le|vši|šću|šćeš|šće|šćemo|šćete", "pro'izvođenj*",
    // vòdīč, but vodíča, vodíči in the other cases; poluvòdīč the same
    "v/odič", "vod'i:č|a|u|em|i|e|ima", "poluv'odič", "poluvod'i:č|a|u|em|i|e|ima",
    // nȁpomena in every case. nȁpomene, nȁpomenu and nȁpomeni are also
    // spelled like forms of napoménuti (VERBS): the noun has them, except
    // the imperative at the head of a clause (Napoméni mu ...)
    "n^apomen|a|e|i|u|o|om|ama",

    // ---- The fifth October 2026 list (Školski rječnik, HJP, Hrvatski
    // mrežni rječnik, Wiktionary, Rečnik Matice srpske; all agree). The
    // verbs' other forms are in VERBS ----
    // ȍdabīr in every case (an exact paradigm: a stem would take
    // odàbirati); "odabire" and "odabiru" are also forms of odàbirati, the
    // noun has them. odàbrati, odàberēm, odabèri (VERBS), but ȍdabrao,
    // ȍdabrān on the first syllable and the aorist odàbrah
    "^odabir||a|u|om|i|e|ima",
    "^odabra|o|la|lo|li|le", "^odabran||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "od/abr|a|ah|asmo|aste|aše|avši",
    // asìstent, asìstenta in every case, the genitive plural asìstenātā
    // against the rule for -at; asìstentica (HJP also asistȅnt, asistȅntica)
    "as/istenata", "as/istentic|a|e|i|u|o|om|ama",
    // ȕčenīk, ȕčenīka, ȕčenīci: the nouns in -nik of three syllables keep
    // the first syllable (StressRules::long_stem), these set its falling
    // tone; ȕčenica, ȕčeničkī
    "^učeni|k|ka|ku|kom|ke|ci|cima|če", "^učenic|a|e|u|om|ama", "^učeničk*",
    // ...but ugljènīk, ugljeníka (Rečnik Matice srpske) moves like the
    // longer ones
    "uglj/enik", "ugljen'i:|ka|ku|kom|ke|ci|cima",
    // ȉzglēd, ȉzglēda in every case. "izgleda" is also the third person of
    // izglédati (ìzglēdā): the noun has it. The other persons and the
    // imperative are ìzglēdām, ìzglēdāj; the rest of the verb keeps
    // izgléd- (above)
    "^izgled||a|u|om|i|e|ima", "/izgled|am|aš|amo|ate|aj|ajmo|ajte",
    "izgl'e:dajući",
    // žìvot, but živòta, živòte in the other cases; žìvotnī; živòtinja
    "ž/ivot", "živ/ot|a|u|om|i|e|ima", "ž/ivotan", "ž/ivotn*",
    "ž/ivotn|oga|ome|omu|ima", "živ/otinj*", "živ/otinjama", "živ/otinjsk*",
    // spávati, némati (VERBS), but the present and the imperative spȃvām,
    // spȃvā, spȃvāj, nȇmām, nȇmā, nȇmāj have the falling accent
    "sp^a:v|am|aš|a|amo|ate|aj|ajmo|ajte", "n^e:m|am|aš|a|amo|ate|aj|ajmo|ajte",
    // líce in every case
    "l/i:c|e|a|u|em|ima",
    // krèdīt, but kredíta in the other cases; krèdītnī
    "kr/edit", "kred'i:t|a|u|om|i|e|ima", "kr/editn*",
    // stȃvka, stȃvci, stȁvākā
    "st^a:vk|a|e|i|u|o|om|ama", "st^a:vci", "st^avaka",
    // krétati, kréći, krénuti, kréni (VERBS), but the present krȇćēm,
    // krȇćēte, krȇnēm, krȇnē
    "kr^e:ć|em|eš|e|emo|ete|u", "kr^e:n|em|eš|e|emo|ete|u",
    // dokumentárac, dokumentárca, dokumèntārācā (dȍkumentārnī keeps the
    // first syllable)
    "dokument'a:r|ac|ca|cu|cem|ci|ce|cima", "dokum/entaraca",
    // vlȃst, vlȃsti, vlȃšću, vlástima; the locative vlásti after a
    // preposition is a rule of the front end
    "vl^a:st||i", "vl^a:šću", "vl/a:stima",
    // nòvac, nóvca, nóvcem; nȏvci, nȍvācā
    "n/ovac", "n/o:vc|a|u|em", "n^o:vc|i|e|ima", "n^ovaca",
    // sústav, sústāvan, sústavno, but sustàvnijī
    "s/u:stav*", "s/u:stavn*", "sust/avnij*",
    // rádnja in every case
    "r/a:dnj|a|e|i|u|o|om|ama",
    // dȃr, dȃra, dȁrovi, dȁrōvā; the locative dáru after a preposition is a
    // rule of the front end
    "d^a:r||a|u|om|em|e", "d^arov|i|a|e|ima",
    // vážiti, vážio (VERBS), but the present vȃžīm, vȃžī, vȃžē; vážēćī
    "v^a:ž|im|iš|i|imo|ite|e", "v/a:žeć*",

    // ---- The sixth October 2026 list (Školski rječnik, HJP, Hrvatski
    // mrežni rječnik, Wiktionary, Rečnik Matice srpske, Alić's Bosnian
    // handbook). karijéra, premìjer, premijéra and the other loans in -ijer
    // are a rule (is_ije_diphthong, StressRules::long_rhyme), except
    // prȅmijērnī, hotelìjērskī, hijèrātskī, hijeròglīf, arhijèrej; zanímati, odmòriti and obèležiti are in
    // VERBS. The comparatives keep the accent of the rule for -ij against
    // the stems below (jedinstvènijī, zaobilàznijī, kompromìsnijī) ----
    "pr^emijern*", "hotel/ijersk*", "hotel/ijerstv*", "hij/eratsk*",
    "hijer/oglif*", "arhij/erej*",
    "jedinstv/enij*", "zaobil/aznij*", "nezaobil/aznij*", "komprom/isnij*",
    "beskomprom/isnij*", "neukrot/ivij*",
    // ùmjetnīk, ùmjetnica, ùmjetnōst, ùmjetničkī, ùmetnīk, ùmetnōst in every
    // form (not ùmjetan, umjetnína, which these stems do not reach)
    "/umjetnik*", "/umjetnic*", "/umjetniče", "/umjetničk*", "/umjetnost*",
    "/umjetnošću", "/umetnik*", "/umetnic*", "/umetniče", "/umetničk*",
    "/umetnost*", "/umetnošću",
    // àlbūm, but albúma in the other cases
    "/album", "alb'u:m|a|u|om|e|i|ima",
    // jedìnstven, jedìnstvenōst, but jedínstvo (HJP)
    "jed/instven*", "jed/instvenost*", "jed/i:nstv|o|a|u|om|ima",
    // kompròmis, kompròmisan (HJP, Rečnik Matice srpske, Wiktionary;
    // Školski rječnik kȍmpromis) and bèskompromisan (Školski rječnik,
    // Wiktionary; HJP and Rečnik Matice srpske bȅ-, in the Serbian and
    // Bosnian tables)
    "kompr/omis*", "kompr/omisn*", "b/eskompromis*", "b/eskompromisn*",
    // plȃćenīk, plȃćenica, plȃćeničkī in every form, also as written with č
    "pl^a:ćenik*", "pl^a:ćenic*", "pl^a:ćeniče", "pl^a:ćeničk*",
    "pl^a:čenik*", "pl^a:čenic*", "pl^a:čeniče",
    // pȍvijēst, pȍvijesnī: the falling accent keeps the long ije after it
    // unstressed
    "p^ovijest*", "p^oviješću", "p^ovijesn*",
    // ostávština (HJP, Rečnik Matice srpske; Školski rječnik òstāvština)
    "ost'a:vštin*",
    // odijélo, odélo in every case, and "odjelo" as some write it (not
    // odjela, odjelu: the cases of òdjel)
    "odij'e:l|o|a|u|om|ima", "od'e:l|o|a|u|om|ima", "odj'e:lo",
    // gubítak, gubíci, gùbītākā (HJP, Školski rječnik, Rečnik Matice
    // srpske), and the nouns in -itak, -utak of the same pattern in HJP:
    // dobítak, užítak, primítak, odbítak, razvítak, boljítak, zgodítak,
    // probítak, imútak, osnútak, privítak (the other cases are the rule for
    // -itk-)
    "gub'i:|tak|ci|cima|tci|tcima", "g/ubitaka",
    "dob'i:|tak|ci|cima|tci|tcima", "d/obitaka",
    "už'i:|tak|ci|cima|tci|tcima", "/užitaka",
    "prim'i:|tak|ci|cima|tci|tcima", "pr/imitaka",
    "odb'i:|tak|ci|cima|tci|tcima", "/odbitaka",
    "razv'i:|tak|ci|cima|tci|tcima", "r/azvitaka",
    "bolj'i:|tak|ci|cima|tci|tcima", "b/oljitaka",
    "zgod'i:|tak|ci|cima|tci|tcima", "zg/oditaka",
    "prob'i:|tak|ci|cima|tci|tcima", "pr/obitaka",
    "im'u:|tak|ci|cima|tci|tcima", "/imutaka",
    "osn'u:|tak|ci|cima|tci|tcima", "/osnutaka",
    // privítak, privíci, prìvītākā (HJP, Školski rječnik, Wiktionary); prìvijen
    "priv'i:|tak|ci|cima|tci|tcima", "pr/ivitaka", "pr/ivijen*",
    // debìtant, debìtantica, debìtantskī in every form, against the rules
    // for -ant
    "deb/itant*", "deb/itantic*", "deb/itantsk*", "deb/itantkinj*",
    // neukròtiv in every form
    "neukr/otiv*", "neukr/otivost*",
    // ljúbav, ljúbāvnī, ljúbāvnīk in every form
    "lj/u:bav*", "lj/u:bavn*", "lj/u:bavnik*", "lj/u:bavnic*",
    // dèbīl, but debíla in the other cases
    "d/ebil", "deb'i:l|a|u|om|e|i|ima",
    // izdánje in every case
    "izd'a:nj*",
    // nezaobìlazan, zaobìlazan in every form (Rečnik Matice srpske
    // nȅzaobilāzan, in the Serbian and Bosnian tables)
    "nezaob/ilaz*", "nezaob/ilazn*", "zaob/ilazan", "zaob/ilazn*", "zaob/ilaznic*",
    // bòja in every case; "boji", "boje" are also forms of bòjiti and
    // bȍjati se, the noun has them
    "b/oj|a|e|i|u|om|ama",
    // tȇško, tȇškī, but téžak, téška (HJP, Rečnik Matice srpske,
    // Wiktionary; Školski rječnik tȇžak); tȅžē, tȅžī. Not a stem: tèškoća
    "t^e:šk|o|i|og|oga|om|ome|omu|oj|ih|im|ima", "t/e:šk|a|e|u", "t/e:žak",
    "t^ež|e|i|a|u|eg|em|oj|ih|im|ega|emu|ima",
    // òdmor in every case; "odmori" is the imperative of odmòriti (VERBS)
    // only at the head of a clause
    "/odmor||a|u|om|i|e|ima",
    // bȏl, bȏla, bȏli, bȍlovi, bólima (Školski rječnik); the locatives bólu,
    // bóli after a preposition are a rule of the front end. "boli" is also
    // bòlī (it hurts): the noun has it. "bolju" is left to the comparative
    "b^o:l||a|i|om|u", "b/o:lima", "b^olov|i|a|e|ima",
    // bȏlan, bȏlno, bȏlnī, but bólna and bòlnijī (not a stem: bólnica)
    "b^o:lan", "b^o:ln|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", "b/o:lna",
    "b/olnij*",
    // neprikosnòven in every form, also as it is often misspelled
    "neprikosn/oven*", "neprekosn/oven*",
    // kr̀hotina (HJP, Wiktionary; Školski rječnik kȑhotina), against the
    // rule for -ina
    "k/rhotin*",
    // túga, túzi, tȗgo; túžan, túžna, but tȗžno, tȗžnī and tùžnijī
    "t/u:g|a|e|u|om|ama", "t/u:zi", "t^u:go",
    "t/u:žan", "t/u:žna", "t^u:žn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "t/užnij*",
    // dùga, the adjective (duga noć, dugog: dùga), far commoner than the
    // rainbow dúga and the debt's genitive dȗga
    "d/uga",
    // vážan, vážna, vážnōst, but vȃžno, vȃžnī and vàžnijī
    "v/a:žan", "v/a:žna", "v^a:žn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "v/ažnij*", "v/a:žnost*", "v/a:žnošću",
    // lȁžan, lȁžno, lȁžnī, lȁžnōst, but làžna and làžnijī; lȃž, lȁži,
    // làžima
    "l^ažan", "l/ažna", "l^ažn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "l/ažnij*", "l^ažnost*", "l^ažnošću", "l^a:ž", "l^až|i|ju", "l/ažima",
    // rúža in every case; not a stem: rȗžan, rȗžno
    "r/u:ž|a|e|i|u|om|ama",

    // ---- The seventh October 2026 list (HJP, Školski rječnik) ----
    // nȉkad, nȉkada, ȉkad, ȉkada, nȅkad, nȅkada, kȁtkad, against the rule
    // for loans in -ad (nikáda); òtkad, dòkad, dòsad, pònekad, kadìkad with
    // their forms in -a, svȁgda ("dosada" is also the noun dȍsada, left
    // as it was). The verbal nouns in -ánje (poboljšánje) are a rule
    // (StressRules::form_of), reproducírati is in VERBS
    "n^ikad", "n^ikada", "^ikad", "^ikada", "n^ekad", "n^ekada", "k^atkad",
    "k^atkada", "/otkad", "/otkada", "d/okad", "d/okada", "d/osad",
    "p/onekad", "p/onekada", "kad/ikad", "kad/ikada", "sv^agda",
    // rȅproduktīvan, rȅproduktīvnī
    "r^eproduktivan", "r^eproduktivn*",

    // ---- The eighth October 2026 list (HJP, Školski rječnik, Wiktionary,
    // Rečnik Matice srpske) ----
    // kȃrta, kȃrte in every case, G pl kȁrātā (the rule for loans in -at
    // had karáta); kàrtica. Not stems: kàrtōn, kàrtel
    "k^a:rt|a|e|i|u|o|om|ama", "k^arata", "k/artic|a|e|i|u|o|om|ama",
    // zdrȃvlje in every case; zdràvstvo, zdràvstvenī
    "zdr^a:vlj|e|a|u|em", "zdr/avstv|o|a|u|om", "zdr/avstven*",

    // ---- The ninth October 2026 list (HJP, Školski rječnik, Wiktionary,
    // Rečnik Matice srpske) ----
    // rázličit, rázličitōst in every form, but različìtijī
    "r/a:zličit*", "r/a:zličitost*", "r/a:zličitošću", "različ/itij*",
    // strána, stránē, stránama, G pl stránā; A strȃnu, N pl strȃne, D
    // strȃni (L stráni after a preposition is a rule of the front end).
    // "strane" is the genitive (s druge strane, od strane, dvije strane),
    // not the plural strȃne; "strani", "stranu", "strano", "stranom" are
    // also the adjective strȃn (strani jezik), which has the falling accent
    "str/a:n|a|e|ama", "str^a:n||i|u|o|om|og|oga|ome|omu|oj|ih|im|ima",
    // dokùment, dokùmenti (unchanged), G pl dokùmenātā
    "dok/umenata",
    // vijȇst, vijȇsti, vijȇšću, but vijéstima, G pl vijéstī (spelled as
    // the far commoner vijȇsti) and L sg vijésti after o, po, pri (a rule
    // of the front end); ekavian vȇst, vȇsti, véstima
    "v^ije:st||i", "v^ije:šću", "v/ije:stima", "v^e:st||i", "v^e:šću",
    "v/e:stima",
    // rázmak, rázmaci, G pl rázmākā; rázmaknica (HJP, Školski rječnik).
    // Exact forms: a stem would take razmàkne, razmàkni of razmàknuti (VERBS)
    "r/a:zmak||a|u|om|e", "r/a:zmac|i|ima", "r/a:zmaknic*",
    // ȉpsilōn (the name of Y, also when spelled), èpsilōn (HJP), against the
    // rule for loans in -on (ipsìlon)
    "^ipsilon*", "/epsilon*",
    // cȃrstvo, cȃrskī and králjevstvo in every case, G pl cȃrstāvā,
    // králjevstāvā (HJP, Školski rječnik, Wiktionary); the kraljev* stem
    // above does not reach them
    "c^a:rstv|o|a|u|om|ima", "c^a:rstava", "c^a:rsk*",
    "kr/a:ljevstv|o|a|u|om|ima", "kr/a:ljevstava",

    // ---- The tenth October 2026 list (HJP, Školski rječnik, Wiktionary) ----
    // ćȃr; čȃr, čȃri, čȃrju, but čárima, G pl čárī and the locative čári
    // after a preposition (a rule of the front end); čárati, čárao,
    // čárajū (VERBS), but the present čȃrām, čȃrā, čȃrāj; čȁroban, čȁrobnī
    // (čaròbnjāk is the rule for -njak). prìtisak, prìtiska, prìtisci,
    // prìtisākā; prìtisnuti and pritískati are in VERBS, and so are
    // ujedíniti, očárati, začárati
    "ć^a:r||a|u|om", "č^a:r||a|i|u|om|ju", "č/a:rima",
    "č^a:r|am|aš|amo|ate|aj|ajmo|ajte", "č^aroban",
    "č^arobn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "pr/itis|ak|ka|ku|kom|ke|ci|cima|aka",
    // minimálac, G pl minimálācā (HJP; the other cases are the rule for
    // -alc-, which cannot take the nominative: glȅdalac, čìtalac); the
    // adjective mȉnimālan, but minimálnōst
    "minim'a:lac", "minim'a:laca",
    "m^inimal|an|na|no|ni|ne|nu|nog|noga|nom|nome|nomu|noj|nih|nim|nima",
    "minim'a:lnost*", "minim'a:lnošću",
    // fȋrma in every case (HJP, Školski rječnik, Wiktionary)
    "f^i:rm|a|e|i|u|o|om|ama",

    // ---- The eleventh October 2026 list (Školski rječnik, HJP, Rečnik
    // Matice srpske, Wiktionary; all agree unless noted) ----
    // trènūtākā is in the -tak block above. sitnìca in every case
    "sitn/ic*",
    // nezàpažen, zàpažen: the participles of zàpaziti (VERBS), like òpaziti
    // zànāt, zanáta, zanáti, G pl zanátā; zànātskī
    "z/anat", "zan'a:t|a|u|om|i|e|ima", "z/anatsk*",
    // písmo, písma, písmima, G pl pȋsāmā (not pȉsmen, pȉsmeno)
    "p/i:sm|o|a|u|om|ima", "p^i:sama",
    // stȁr, but the definite stȃrī, stȃrōg, stȃrā, stȃrō in every other
    // form (stari grad, stara lozinka; the indefinite stàra, stȁro is
    // the rarer predicate); stàrijī is the rule for -iji
    "st^a:r|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // mlȃd, mlȃdī, mlȃdōg, mlȃdo, but mláda (also the bride)
    "ml^a:d||i|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima", "ml/a:da",
    // dètālj, detálja, detáljem, detálji, G pl detáljā; dètāljan,
    // dètāljna, but the adverb dȅtāljno and detàljnijī (the rule for -iji)
    "d/etalj", "det'a:lj|a|u|em|i|e|ima",
    "d/etaljan", "d/etaljn|a|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", "d^etaljno",

    // ---- Serbian's own forms; they override the entries above ----
    // Numbers: the lengths after the accent (dȅvēt, dȅsēt, jedànaēst,
    // milìjārda, milìjūntī, dvjȅstōtī) and the ordinals dvadèsētī, tridèsētī,
    // dèvētī, dèsētī
    "d^eve:t", "d^ese:t",
    "jed'anae:st*", "dv'a:nae:st*", "tr'i:nae:st*", "čet'rnae:st*", "p'etnae:st*",
    "š'esnae:st*", "sed'amnae:st*", "os'amnae:st*", "dev'etnae:st*",
    "dvad'ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    "trid'ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    "dvj^esto:t*", "dv^esto:t*", "tr^isto:t*", "č^etiristo:t*", "p^e:tsto:t*",
    "š^e:ststo:t*", "s^edamsto:t*", "^osamsto:t*", "d^evetsto:t*",
    "mil/ija:rd*", "mil/iju:nt*", "mil/io:nit*",
    "d/eve:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome", "d/ese:t|i|a|o|e|u|og|om|oj|ih|im|ima|oga|ome",
    "prof'esor*", "proc'enat", "proc'ent*", "Vojv'odin*", "Ukraj'i:n*",
    "j^anua:r*", "f^ebrua:r*", "m^art*", "/apri:l*", "m^a:j", "j^u:n",
    "j^u:l", "^avgust*", "sept'embar", "sept'embr*", "okt'o:bar",
    "okt'o:br*", "nov'embar", "nov'embr*", "dec'embar", "dec'embr*",
    "d/e:te", "vr/e:me", "l/e:p*", "ml/e:k*", "r/e:k*", "b/e:l*",
    "sv^e:t", "r^e:č",
    "m^onoto:no", "r^e:da:ka:", "k/orisni:k*", "k/orisni:ci", "k/orisni:cima",
    "k/orisni:če", "k^ori:stan", "k^ori:st",
    "r^ođenda:n*", "z^auze:t*",
    // The words and names checked in October 2026 (see above) with their
    // length after the accent
    "/obrazova:nj*", "/obrazo:vn*", "/isku:sta:va:", "/ala:t",
    "progr/ame:rsk*", "/ekra:n", "mod/era:tor*", "mod/era:toric*",
    "mod/era:tork*", "razv'i:jaju:", "t^obo:m", "M/iha:jl*",
    "Crn/ogo:rc|a|u|em", "Crn/ogo:rče", "Crn/ogora:ca:", "Crn/ogo:rk*",
    "n^a:jposle:", "n^a:jpre:", "p/ona:jprije", "p/ona:jviše",
    "j/a:vno:st", "p/otpuno:st",
    "crn/ogo:rsk*", "prot/etiča:r*", "ž/eljezniča:r*", "ž/elezniča:r*", "ž/eljezni:čk*",
    "ž/elezni:čk*", "od/rživo:st", "inspir'i:raju:",
    // màslina, màslinov and rakéta are the first forms of the Serbian
    // dictionary (Rečnik Matice srpske: ма̀слина и ма̏слина, раке́та и ракѐта)
    "m/aslin*", "m/aslinov*", "rak/e:t*",
    // The third October 2026 list (see above). Rečnik Matice srpske has
    // prijatéljstvo, neprijatéljstvo (first form) and snáći, where the
    // Croatian dictionaries have prijatèljstvo and snȃći
    "prijat/e:ljstv|o|a|u|om|ima", "prijat/e:ljsta:va:",
    "neprijat/e:ljstv|o|a|u|om|ima", "neprij/ate:ljsta:va:", "sn/a:ći",
    "p^otencija:l|an|na|no|ni|ne|nu|nog|nom|noj|nih|nim|noga|nome|nomu|nima",
    "sn/alaže:nj|e|a|u|em|ima", "simul'a:ci:jsk*", "sim/ula:tor*", "sim/ula:torsk*",
    "dož'i:v|i:m|i:š|e:", "prež'i:v|i:m|i:š|e:", "prož'i:v|i:m|i:š|e:",
    "nadž'i:v|i:m|i:š|e:", "pož'i:v|i:m|i:š|e:", "ž/i:v|i:m|i:š",
    "d/oži:vljen*", "pr/eži:vljen*", "pr/oži:vljen*", "n/adži:vljen*",
    "d^oživlja:j*", "nev/idljivo:st", "nesn/alaže:nj|e|a|u|em|ima",
    "p^oštova:n*", "^interesova:n*", "^interesuj|e:m|e:š|e:|e:mo|e:te|u:",
    "z/ainteresuj|e:m|e:š|e:|e:mo|e:te|u:", "z/ainteresova:n*", "nez/ainteresova:n*",
    // The fourth list (see above): vòdīč, poluvòdīč, Ègipćānka
    "v/odi:č", "poluv'odi:č", "/egipća:nk*",
    // gòdišnjāk (see above)
    "g/odišnja:k*", "g/odišnja:c|i|ima", "g/odišnja:če",
    // The fifth October 2026 list (see above) with the length after the
    // accent: ȍdabīr, ȍdabrāno, asìstenātā, ȕčenīk, ȉzglēd, ìzglēdām,
    // spȃvām, nȇmām, krèdīt, stȁvākā, krȇćēm, krȇnēm, dokumèntārācā,
    // nȍvācā, sústāvan, dȁrōvā, vȃžīm
    "^odabi:r||a|u|om|i|e|ima", "^odabra:|la|lo|li|le",
    "^odabra:n||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", "as/istena:ta:",
    "^učeni:|k|ka|ku|kom|ke|ci|cima|če",
    "uglj/eni:k",
    "^izgle:d||a|u|om|i|e|ima", "/izgle:d|a:m|a:š|a:mo|a:te|a:j|a:jmo|a:jte",
    "izgl'e:daju:", "izgl'e:daju:ći", "sp^a:v|a:m|a:š|a:|a:mo|a:te|a:j|a:jmo|a:jte", "sp/a:vaju:",
    "n^e:m|a:m|a:š|a:|a:mo|a:te|a:j|a:jmo|a:jte", "n/e:maju:",
    "kr/edi:t", "kr/edi:tn*", "st^ava:ka:",
    "kr^e:ć|e:m|e:š|e:|e:mo|e:te|u:", "kr^e:n|e:m|e:š|e:|e:mo|e:te|u:",
    "dokum/enta:ra:ca:", "n^ova:ca:", "s/u:sta:van", "d^aro:va:",
    "v^a:ž|i:m|i:š|i:|i:mo|i:te|e:", "v/a:že:ć*",
    // The sixth October 2026 list (see above) with the length after the
    // accent: ùmjetnīk, ùmetnōst, àlbūm, jedìnstvenōst, plȃćenīk, dèbīl,
    // debìtanātā, ljúbāvnī, ljúbāvnīk, bòjōm, túgōm; and Rečnik Matice
    // srpske's bȅskompromisan, nȅzaobilāzan and tȗžan, tȗžna (Croatian
    // bèskompromisan, nezaobìlazan, túžan)
    "/umjetni:|k|ka|ku|kom|ke|ci|cima|če", "/umetni:|k|ka|ku|kom|ke|ci|cima|če",
    "/umjetno:st", "/umetno:st", "/albu:m", "jed/instveno:st",
    "pl^a:ćeni:|k|ka|ku|kom|ke|ci|cima|če", "pl^a:čeni:|k|ka|ku|kom|ke|ci|cima|če",
    "d/ebi:l", "deb/itana:ta:", "lj/u:ba:vn*", "lj/u:ba:vnic*",
    "lj/u:ba:vni:|k|ka|ku|kom|ke|ci|cima|če", "b/ojo:m", "t/u:go:m",
    "b^eskompromis*", "b^eskompromisn*", "n^ezaobila:zan", "n^ezaobilazn*",
    "t^u:žan", "t^u:žna",
    // The eighth list: kȁrātā; the ninth: rázličitōst, dokùmenātā
    "k^ara:ta:", "r/a:zličito:st", "dok/umena:ta:",
    // rázmākā, ȉpsilōn, èpsilōn
    "r/a:zma:ka:", "^ipsilo:n*", "/epsilo:n*", "c^a:rsta:va:",
    "kr/a:ljevsta:va:", "č^a:r|a:m|a:š|a:mo|a:te|a:j|a:jmo|a:jte", "č/a:raju:",
    "minim'a:la:ca:", "minim'a:lno:st", "m^inima:l|an|na|no|ni|ne|nu|nog|noga|nom|nome|nomu|noj|nih|nim|nima",
    // gùbītākā, dòbītākā ... (Rečnik Matice srpske, HJP)
    "g/ubi:ta:ka:", "d/obi:ta:ka:", "/uži:ta:ka:", "pr/imi:ta:ka:", "/odbi:ta:ka:",
    "r/azvi:ta:ka:", "b/olji:ta:ka:", "zg/odi:ta:ka:", "pr/obi:ta:ka:", "/imu:ta:ka:",
    "/osnu:ta:ka:", "pr/ivi:ta:ka:",
    // The eleventh list: pòdātākā ... trènūtākā (the -tak block above),
    // zànāt, zànātskī, pȋsāmā, dètālj, dètāljan, dȅtāljno
    "p/oda:ta:ka:", "d/oda:ta:ka:", "z/ada:ta:ka:", "/osta:ta:ka:",
    "p/oče:ta:ka:", "zav'rše:ta:ka:", "iz'uze:ta:ka:", "tr/enu:ta:ka:",
    "z/ana:t", "z/ana:tsk*", "p^i:sa:ma:", "d/eta:lj", "d/eta:ljan",
    "d/eta:ljn|a|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", "d^eta:ljno",
];
