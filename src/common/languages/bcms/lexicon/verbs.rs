/// Verbs whose root has the long "ije" (podijéliti, promijéniti, zalijévati).
/// After a prefix the spelling "ije" marks the verb: the nouns beside it have
/// the short "je" (podjela, promjena, zamjena). The front end puts the accent
/// on the "ije", or shifts it in the present and passive participle
/// (StressRules::verb_form); nothing else tells it that "podijeli", "podijelio"
/// and "podijeljen" belong to "podijeliti".
///
///   root:classes[:prefixes]
///   i  verb in -iti (podijeliti, podijelim, podijeli, podijelio, podijeljen)
///   a  verb in -ati (pomiješati, pomiješam, pomiješaj, pomiješao, pomiješan)
///   t  verb in -ati whose present has another root (dolijetati)
///   e  that present root (dolijećem, podliježem)
///   !  Croatian also shifts the present and passive accent (pòdijelim)
///   prefixes: the only ones the root takes, where any prefix would also
///   match nouns (povijest, pripovijest, zapovijed)
#[rustfmt::skip] // the entries stand in rows under the comments that give their sources
pub(crate) static IJE_VERBS: &[&str] = &[
    // HJP podijéliti, prez. pòdijēlīm, prid. trp. pòdijēljen.
    // Keep this restricted entry before the generic root: other prefixes
    // retain their existing Croatian treatment (e.g. raspodijelim).
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxlXhY%3D
    "dijel:i!:po",
    // HJP proslijéditi, prez. pròslijēdīm, prid. trp. pròslijēđen.
    // The restricted prefix preserves the treatment of naslijediti etc.
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=dl5lWRA%3D
    "slijed:i!:pro",
    "dijel:i", "mijen:i", "lijep:i", "cijen:i", "bijed:i", "slijed:i",
    "vrijed:i", "zlijed:i", "trijeb:i", "riješ:i", "liječ:i", "slijep:i",
    "bijel:i", "cijel:i", "mijet:i", "svijetl:i", "rijed:i", "prijet:i",
    "cijed:i", "korijen:i", "plijen:i", "lijen:i", "smiješ:i", "tijesn:i",
    "strijel:i", "nijem:i", "bijesn:i", "griješ:i", "mijes:i", "cijep:ia",
    "prijed:i:una", "vijest:i:iz na nago", "svijest:i:o po",
    "miješ:a", "lijev:a", "grijev:a", "spijev:a", "dijev:a", "zrijev:a",
    "gorijev:a", "pijev:a", "sijec:a", "drijem:a", "mijer:a",
    "htijev:a:za", "umijev:a:raz sporaz", "olijev:a:od", "starijev:a:za",
    "povijed:a:pri za is pro",
    "lijet:t", "lijeć:e", "lijeg:t", "lijež:e",
];

/// Other verbs, as whole stems with the accent of the infinitive marked as in
/// the lexicon above: uréditi, otvòriti, pročìtati, pokrénuti. Their forms
/// keep that accent on the root (urédi, urédio, otvòri, pročìtaj, pokréni);
/// StressRules::verb_form() builds the forms.
///
///   st'e:m=class and flags (' automatic tone, / rising, ^ falling)
///   i  -iti, -im      (urediti, uredim, uredi, uredio, uređen)
///   a  -ati, -am      (pročitati, pročitam, pročitaj, pročitao, pročitan)
///   u  -nuti, -nem    (pokrenuti, pokrenem, pokreni, pokrenuo, pokrenut)
///   t  -ati, where the present has another stem (pokazati, pokazao, pokazan)
///   e  that stem, or any present in -em (pokažem, pokaži; unesem, unesi)
///   <  the dictionaries move the accent one syllable back in the present
///      (ùrēdīm, òtvorīm, pòkāžēm); Stojan and Mirsad do, Zvonko does not
///   p  ... and in the passive participle (ùrēđen);  P: to the first syllable
///   !  Zvonko moves it too, as the dictionaries do (ùključen, raspòrēđen)
///   n  the imperative is spelled like a form of a noun or adjective (potvrdi,
///      uredi, otvori, načini, napomeni; for classes a and u the third
///      person: oprema, proba, napomene, napomenu).
///      It is read as the verb only at the head of a clause ("Potvrdi",
///      "Ne zaboravi"); elsewhere the other word has priority.
///
/// The accents are those of Hrvatski jezični portal (October 2026); the
/// stems were generated from its entries, the noun twins from its answers
/// for the imperative forms. A verb that is not here is right only in the
/// infinitive or if the first syllable carries its accent anyway.
#[rustfmt::skip] // the entries stand in rows under the comments that give their sources
pub(crate) static VERBS: &[&str] = &[
    // ŠR/HJP perfective pògledati, pògledām, pògledāj, pògledao.
    // The less common imperfective poglédati needs a written accent or
    // a user entry; the ordinary perfective is the unmarked default.
    "p/ogled=a",
    // pòdizati / pòdižēm / pòdiži / pòdizao / pòdizān.
    "p/odiz=t", "p/odiž=e",
    // ŠR/Mrežnik nádati/nádao/nádajū. Croatian falling present and
    // imperative forms are exact entries; náda (noun) wins over nada.
    "n/a:d=an",
    // ŠR/HJP ponádati/ponádao, but pònādām/pònādāj in Croatian too;
    // third-person plural ponádajū keeps the infinitive's long rising a.
    "pon/a:d=a<!",
    // ŠR/HJP rázlikovati / rázlikovao; ŠR rázlikovān.
    // -ov- + the existing -ati class; Croatian -uj- forms are above.
    "r/a:zlikov=t",
    // ŠR/HJP: zatégnuti, rastégnuti, protégnuti; long rising e in
    // infinitives/imperatives/active forms, but zàtēgnēm / zàtēgnūt,
    // ràstēgnēm / ràstēgnūt, pròtēgnēm / pròtēgnūt in Croatian too.
    "zat/e:g=u<p!", "rast/e:g=u<p!", "prot/e:g=u<p!",
    // ŠR/HJP: ìspraviti / ìspravio / ìspravljen, pòmisliti / pòmislio,
    // víriti / vírīm / víri / vírio. Whole stems avoid other derivatives.
    "/isprav=i", "p/omisl=i", "v/i:r=i",
    // ŠR/HJP òstarīm, òstarē, òstari, òstario: short rising o, short a.
    // Also covers òstariti; Croatian -jeti forms are in CROATIAN above.
    "/ostar=i",
    // ŠR/HJP pòklanjati, pòklanjām, pòklanjajū, pòklanjāj,
    // pòklanjao, pòklanjān, pòklanjānje: one regular -ati paradigm.
    "p/oklanj=a",
    // ŠR/HJP: explicit short rising accents also matter on the first
    // syllable; generic suffix rules had moved these active forms.
    "pretp/ostav=i", "/osjeć=a", "/ostav=i", "p/okuš=a",
    "s/usret=t", "s/usreć=e", "p^ad=a",
    // dogáđati / dogáđajū, but dògāđām, in Croatian too.
    "dog/a:đ=a<!",
    // ŠR/HJP nàmignuti, nàmignēm, nàmigni, nàmignuo.
    "n/amig=u",
    "ur'e:d=i<pn", "odr'e:d=i<pn", "nar'e:d=i<p", "sr'e:d=i", "preur'e:d=i<p",
    "uklj'u:č=i<p!", "isklj'u:č=i<p!", "zaklj'u:č=i<p!", "priklj'u:č=i<p!",
    "odl'u:č=i<p", "na'uč=i<p", "pro'uč=i<p", "po'uč=i<p", "ur'u:č=i<p",
    "nar'u:č=i<p", "por'u:č=i<p", "prepor'u:č=i<p", "ispor'u:č=i<p",
    "potv'r:d=i<pn", "utv'r:d=i<pn", "tv'r:d=i", "ob'uhvat=i", "pretr'a:ž=i<p",
    "istr'a:ž=i<p", "potr'a:ž=i<", "tr'a:ž=i", "ozn'a:č=i<p", "zn'a:č=i",
    "spr'e:m=i", "pripr'e:m=i<pn", "opr'e:m=i<pn", "otv'or=i<pn",
    "zatv'or=i<pn", "pretv'or=i<p", "ostv'a:r=i<p", "ukl'on=i<p",
    "pokl'on=i<pn", "pon'ov=i<p", "obn'ov=i<pn", "odgov'o:r=i<pn",
    "dogov'or=i<pn", "izgov'or=i<pn", "progov'or=i<", "gov'or=i<n",
    "dozv'ol=i<pn", "omog'u:ć=i<p", "onemog'u:ć=i<p", "za'ustav=in",
    "usp'ostav=in", "zab'orav=in", "obj'a:v=i<pn", "prij'a:v=i<pn",
    "odj'a:v=i<pn", "naj'a:v=i<pn", "izj'a:v=i<pn", "j'a:v=i", "poj'a:v=i<n",
    "ispr'a:zn=i<pn", "uč'in=i<p", "nač'in=i<pn", "za'ustavlj=a",
    "pr'esluš=a", "p/osluš=a", "pr'egled=a", "vr'a:t=i", "povr'a:t=i<p",
    "svr'a:t=i", "navr'a:t=i<n", "odvr'a:t=i<p", "obr'a:t=i<p", "pl'a:t=i",
    "upl'a:t=i<pn", "ispl'a:t=i<pn", "napl'a:t=i<pn", "dopl'a:t=i<pn",
    "k'u:p=i", "otk'u:p=i<pn", "proš'i:r=i<p", "raš'i:r=i<p",
    "sm'a:nj=i", "um'a:nj=i<p", "pov'eć=ap", "uv'eć=ap", "poj'ač=ap",
    "oj'ač=ap", "ut'iš=ap", "pribl'i:ž=i<p", "ud'a:lj=i<p", "dop'ust=i<pn",
    // ŠR/HJP ìspustīm, ìspušten in Croatian too; ispùsti/ispùstio stay.
    "isp'ust=i<pn!", "prop'ust=i<pn", "nap'ust=i<p", "otp'ust=i<pn",
    // sàstavi at clause head versus the noun plural sástavi.
    "s/astav=in",
    "zabr'a:n=i<pn", "obr'a:n=i<pn", "br'a:n=i", "od'obr=i<p", "izb'a:c=i<p",
    "ub'a:c=i<p", "preb'a:c=i<p", "odb'a:c=i<p", "nab'a:c=i<p", "zab'a:c=i<p",
    "b'a:c=i", "izv'r:š=i<p", "zav'r:š=i<p", "dov'r:š=i<p", "izr'a:d=i<pn",
    "obr'a:d=i<pn", "ur'a:d=i<p", "zar'a:d=i<pn", "odr'a:d=i<p",
    "prer'a:d=i<pn", "nagr'a:d=i<pn", "ugr'a:d=i<p", "izgr'a:d=i<p",
    "sagr'a:d=i<p", "gr'a:d=i", "pohr'a:n=i<pn", "sahr'a:n=i<pn", "hr'a:n=i",
    // HJP: zaštítiti, but zàštītīm and zàštīćen in Croatian too.
    "nahr'a:n=i<p", "zašt'i:t=i<pn!", "odg'od=i<pn", "dog'od=i<", "pog'od=i<p",
    "ug'od=i<pn", "prilag'od=i<p", "pohv'a:l=i<pn", "hv'a:l=i", "zahv'a:l=i<n",
    "up'a:l=i<pn", "zap'a:l=i<p", "ug'a:s=i<p", "g'a:s=i", "ispr'o:b=a<p",
    "p'odes=i", "k'orist=in", "ub'rz=a<",
    "pr'o:b=a", "proč'it=ap", "oč'it=ap", "izrač'un=aPn", "rač'un=an",
    "obrač'un=aPn", "zaklj'uč=ap", "otklj'uč=ap", "p'i:t=a", "up'i:t=a<p",
    "zap'i:t=a<p", "pr/i:č=a", "ispr'i:č=a<p", "sv'i:r=a", "m'o:r=a",
    "up'ozn=ap", "prep'ozn=ap", "prid'od=aP", "k'a:z=t", "k'a:ž=e",
    "pok'a:z=tp", "pok'a:ž=e<", "prik'a:z=tp", "prik'a:ž=e<", "dok'a:z=tp",
    "dok'a:ž=e<", "otk'a:z=tp", "otk'a:ž=e<", "zak'a:z=tp", "zak'a:ž=e<",
    "isk'a:z=tp", "isk'a:ž=e<", "uk'a:z=tp", "uk'a:ž=e<", "p'i:s=t",
    "p'i:š=e", "nap'i:s=tp", "nap'i:š=e<", "up'i:s=tp", "up'i:š=e<",
    "isp'i:s=tp", "isp'i:š=e<", "op'i:s=tp", "op'i:š=e<", "zap'i:s=tp",
    "zap'i:š=e<", "potp'i:s=tp", "potp'i:š=e<", "prep'i:s=tp", "prep'i:š=e<",
    "otp'i:s=tp", "otp'i:š=e<", "pop'i:s=tp", "pop'i:š=e<", "dop'i:s=tp",
    "dop'i:š=e<", "prop'i:s=tp", "prop'i:š=e<", "v'e:z=t", "v'e:ž=e",
    "pov'e:z=tp", "pov'e:ž=e<", "zav'e:z=tp", "zav'e:ž=e<", "priv'e:z=tp",
    "priv'e:ž=e<", "odv'e:z=tp", "odv'e:ž=e<", "sv'e:z=t", "sv'e:ž=e",
    "obv'e:z=tp", "obv'e:ž=e<", "pokr'e:=u<p", "okr'e:=u<p", "zakr'e:=u<p",
    "skr'e:=u", "preokr'e:=u<p", "pom'ak=u<p", "odm'ak=u<p", "prim'ak=u<p",
    "razm'ak=u<p!", "dod'i:r=u<p", "dot'ak=u<p", "spom'e:=u<p", "napom'e:=u<pn",
    "iz'ostav=i", "up'u:t=i<pn", "stv'a:r=a", "otv'a:r=a<", "zatv'a:r=a<",
    "pretv'a:r=a<", "odgov'a:r=a<", "razgov'a:r=a<", "dogov'a:r=a<",
    "izgov'a:r=a<", "pon'a:vlj=a<", "obn'a:vlj=a<", "pripr'e:m=a<n",
    "spr'e:m=a", "opr'e:m=a<n", "premj'e:št=a<", "namj'e:št=a<", "pre'usmjer=i",
    "z'a:mjer=i", "iskor'ist=i<p", "sl'u:ž=i", "posl'u:ž=i<p", "zasl'u:ž=i<p",
    "zam'ol=i<p", "don'os=i<n", "odn'os=i<n", "pren'os=i<", "un'os=i<n",
    "izn'os=i<n", "podn'os=i<", "uv'od=i<n", "izv'od=i<n", "prev'od=i<",
    "prov'od=i<n", "dov'od=i<n", "nav'od=i<n", "h'o:d=a", "pron'alaz=i",
    "sn'i:m=i", "sn'i:m=a", "presn'i:m=i<p", "izg'ub=i<p", "prob'u:d=i<p",
    "razl'ož=i<p", "ul'ož=i<p", "pol'ož=i<p", "predl'ož=i<p", "izl'ož=i<p",
    "zal'ož=i<p", "odl'ož=i<p", "pril'ož=i<p", "nal'ož=i<p", "upoz'or=i<p",
    "pom'i:r=i<p", "sm'i:r=i", "um'i:r=i<p", "pož'u:r=i<", "prov'a:l=i<pn",
    "nav'a:l=i<pn", "pres'el=i<p", "dos'el=i<p", "us'el=i<p", "nas'el=i<p",
    "razves'el=i<p", "pozv'on=i<", "zar'on=i<p", "ur'on=i<p", "pob'u:n=i<pn",
    "zb'u:n=i", "nast'u:p=i<n", "prist'u:p=i<n", "ist'u:p=i<pn",
    "odst'u:p=i<pn", "post'u:p=i<", "zast'u:p=i<p", "ust'u:p=i<pn", "st'u:p=i",
    "ut'op=i<p", "rast'op=i<p", "pot'op=i<pn", "polj'u:b=i<p", "lj'u:b=i",
    "prel'om=i<p", "p'a:mt=i", "skr'a:t=i", "poč'ast=i<pn", "opr'ost=i<pn",
    "uv'r:st=i<p", "razv'rst=ap", "prem'ost=i<p", "izab'er=e", "odab'er=e<!",
    "od'ust=t", "od'ustan=e", "poš'alj=e<", "poz'ov=e", "naz'ov=e",
    "sač'u:v=a<p", "č'u:v=a", "oč'u:v=a<", "udr'u:ž=i<p", "pridr'u:ž=i<p",
    "zdr'u:ž=i", "dr'u:ž=i", "zatr'a:ž=i<p", "razm'otr=i<p", "prom'otr=i<p",
    "nasl'on=ipn", "osl'on=i<p", "prisl'on=i<p", "označ'a:v=a<", "ob'iljež=i", "ob'elež=i",
    "zab'iljež=i", "ub'iljež=i", "prib'iljež=i", "zab'elež=i", "ub'elež=i",
    "prib'elež=i", "nagl'a:s=i<p", "gl'a:s=i",
    "ogl'a:s=i<pn", "progl'a:s=i<pn", "prod'u:ž=i<p", "prod'u:lj=i<p",
    "od'u:ž=i<p", "zad'u:ž=i<p", "razd'u:ž=i<p", "ubl'a:ž=i<p", "ol'akš=ap",
    "ot'ež=ap", "pojednost'a:v=i<p", "pob'oljš=ap", "pog'orš=ap", "usp'or=i<p",
    "ub'rz=ap", "zak'asn=i<", "zat'a:j=i<pn", "t'a:j=i", "ut'a:j=i<pn",
    "ob'eć=ap", "op'er=e", "isp'er=e", "sap'er=e", "up'r:lj=a<p",
    // Short verbs in -avati and -ivati, whose third person the rule for
    // -avam leaves alone (rješava, uživa)
    "rješ'a:v=a<", "deš'a:v=a<", "už'i:v=a<", "dob'i:v=a<", "zadob'i:v=a<",
    "pokr'i:v=a<", "otkr'i:v=a<", "sakr'i:v=a<", "prekr'i:v=a<", "poč'i:v=a<",
    "um'i:v=a<", "izaz'i:v=a<",
    // držati and its family: -ati with a present in -im (zadr̀žati, zàdržīm)
    "zad'rž=ti<p", "od'rž=ti<p", "sad'rž=ti<p", "pod'rž=ti<p", "izd'rž=ti<p",
    "prid'rž=ti<p",
    // -nijeti and -vesti: unèsem, unèsi; prevèdem, prevèdi
    "un'es=e", "don'es=e", "pren'es=e", "izn'es=e", "odn'es=e", "podn'es=e",
    "pon'es=e", "nan'es=e", "zan'es=e", "prev'ed=e", "dov'ed=e", "uv'ed=e",
    "izv'ed=e", "prov'ed=e", "odv'ed=e", "nav'ed=e", "zav'ed=e",
    // Ekavian: the counterparts of IJE_VERBS have no spelling to know them
    // by, so the common ones are listed (podéliti, proméniti, poméšati)
    "prosl'e:d=i<p", "zal'e:p=i<p", "nal'e:p=i<p", "pril'e:p=i<p",
    "odl'e:p=i<p", "prec'e:n=i<p", "potc'e:n=i<p", "ub'e:d=i<p", "istr'e:b=i<p",
    "izl'e:č=i<p", "zal'e:č=i<p", "prim'e:t=i<p", "osv'e:tl=i<p",
    "rasv'e:tl=i<p", "prosv'e:tl=i<p", "razr'e:š=i<p", "odr'e:š=i<p",
    "unapr'e:d=i<p", "iskor'e:n=i<p", "nagov'e:st=i<p", "nasl'e:d=i<pn",
    "usl'e:d=i<pn", "pod'e:l=i<pn", "razd'e:l=i<pn", "dod'e:l=i<pn",
    "ud'e:l=i<pn", "raspod'e:l=i<pn", "prom'e:n=i<pn", "izm'e:n=i<pn",
    "zam'e:n=i<pn", "prim'e:n=i<pn", "razm'e:n=i<pn", "nam'e:n=i<pn",
    "oc'e:n=i<pn", "proc'e:n=i<pn", "uc'e:n=i<pn", "pob'e:d=i<pn",
    "uvr'e:d=i<pn", "povr'e:d=i<pn", "upotr'e:b=i<pn", "izv'e:st=i<pn",
    "pom'e:š=a<p", "um'e:š=a<p", "izm'e:š=a<p", "zam'e:š=a<p", "prom'e:š=a<p",
    "zaht'e:v=a<p", "usp'e:v=a<p", "dosp'e:v=a<p", "razum'e:v=a<p",
    "zal'e:v=a<p", "prel'e:v=a<p",
    // Verbs whose forms are spelled like the case forms of the nouns with
    // a long last stem syllable (StressRules::long_stem): ùdara is not
    // udára, ìspune not račúne, ìskače not koláče, spȁda not čokoláda;
    // odmárati, zamárati follow their accent. (The present of umrijeti,
    // ùmire, is in the lexicon: as a stem it would take umíri, umíriti.)
    // HJP ìspuniti; ìspuni is also the D/L of Croatian ȉspuna.
    "'udar=a", "/ispun=in", "n'apun=i", "d'opun=i", "p'opun=i",
    "'iskak=t", "'iskač=e", "'uskak=t", "'uskač=e",
    "z'aplak=t", "z'aplač=e", "sp'ad=a", "odm'a:r=a<", "zam'a:r=a<",
    "um'a:r=a<", "raz'a:r=a<",
    // Checked against Školski rječnik hrvatskoga jezika, Hrvatski mrežni
    // rječnik and HJP in October 2026, with the shifts of the present and
    // the participle in every voice: rasporéditi, raspòrēdīm, raspòrēđen;
    // osnážiti, òsnāžīm, òsnāžen; potàknuti, pòtaknēm, potàkni, pòtaknūt
    // (Serbian and Bosnian podstàknuti); razvíjati, ràzvījām (razvíjajū is
    // in the lexicon); prèdstaviti, nàstaviti and their imperfectives keep
    // the accent on the prefix.
    "raspor'e:d=i<p!", "osn'a:ž=i<p!", "pot'ak=u<p!", "podst'ak=u<p!",
    "razv'i:j=a<!", "pr'edstav=i", "pr'edstavlj=a", "n'astav=i", "n'astavlj=a",
    "p'otic=t", "p'otič=e", "p'odstic=t", "p'odstič=e",
    "inspir'i:r=a<p!", "insp'iris=t", "insp'iriš=e",
    // ŠR and HJP: pròslaviti/pròslavīm/pròslavljen; vesèliti/vesèlīm.
    "pr/oslav=i", "ves'el=i",
    // Školski rječnik, HJP and Rečnik Matice srpske: čestítati, čèstītām,
    // čestítajū, čèstītāj, čestítao, čèstītān, in every voice (the noun
    // čèstitka is in the lexicon)
    "čest'i:t=a<p!",
    // ŠR and HJP: sprijatèljiti, sprijàteljīm, sprijàteljen (Rečnik Matice
    // srpske also sprijatèljīm)
    "sprijat'elj=i<p",
    // HJP, Školski rječnik: napòminjati, napòminjēm, napòminji, napòminjao;
    // proizvòditi, proìzvodīm, proizvòdio, proìzvođen (proìzvodi is the
    // noun's in the lexicon); proizvèdēm, proizvèdi, proizvèden
    "nap'ominj=te", "proizv'od=i<p", "proizv'ed=e",
    // The fifth October 2026 list (Školski rječnik, HJP, Hrvatski mrežni
    // rječnik; Rečnik Matice srpske): prèkinuti, prèkinēm, prèkini,
    // prèkinūt; prekídati, prèkīdām, prèkīdān (the present and the passive
    // participle move in the Serbian and Bosnian voices); odàbirati,
    // odàbirēm, izàbirati; prìkvačiti, zàkvačiti, òtkvačiti and prìkačiti,
    // zàkačiti, òtkačiti, against the rule for -ač; the rising accent of
    // spávati, némati, krétati, kréći, krénuti, kréni, vážiti (their
    // falling present is in the lexicon). odàbrati moves its present in
    // every voice (odàberēm, odabèri)
    "pr/eki=u", "prek'i:d=a<pn", "od'abir=t", "od'abir=e", "iz'abir=t",
    "iz'abir=e", "pr/ikvač=i", "z/akvač=i", "/otkvač=i", "pr/ikač=i",
    "z/akač=i", "/otkač=i", "sp/a:v=a", "n/e:m=a", "kr/e:t=t", "kr/e:ć=e",
    "kr/e:=u", "v/a:ž=i",
    // The sixth October 2026 list, in every voice as the dictionaries
    // have them (Školski rječnik, HJP, Rečnik Matice srpske): zanímati,
    // zànīmām, zànīmā, zanímajū, zànīmāj, zanímao, zànīmān, zanímānje;
    // odmòriti, òdmorīm, odmòri, odmòrio, òdmoren ("odmori" is the noun's
    // plural òdmori except at the head of a clause)
    "zan'i:m=a<p!", "odm'or=i<pn!",
    // HJP: reproducírati, reprodùcīrām, reproducírānje, and the passive
    // reprodùcīrān like inspìrīrān, in every voice as for inspirírati
    "reproduc'i:r=a<p!",
    // HJP, Školski rječnik: razmàknuti, razmàkni, razmàknuo, but ràzmaknēm,
    // ràzmaknūt in every voice (`!` on its entry above); ràzmicati,
    // ràzmičēm
    "r/azmic=t", "r/azmič=e",
    // The tenth list, in every voice as the dictionaries have them:
    // prìtisnuti, prìtisnēm, prìtisni, prìtisnuo, prìtisnūt; pritískati,
    // pritískao, but prìtiskām and prìtīšćēm; ujedíniti, ujedíni,
    // ujedínio, but ujèdīnīm, ujèdīnjen; čárati (its falling present is in
    // the lexicon); očárati, začárati, but òčārām, òčārān, zàčārām
    "pr/itis=u", "prit'i:sk=a<!", "pr/itišć=e", "ujed'i:n=i<p!",
    "č/a:r=a", "oč'a:r=a<p!", "zač'a:r=a<p!",
    // The eleventh list (nezapaženo): zàpaziti, zàpazīm, zàpazio, zàpažen
    // (so nezàpažen), and òpaziti the same; zapážati, zapážao, zapážānje,
    // but zàpāžām, zàpāžān in every voice, and opážati, òpāžām the same
    "z/apaz=i", "/opaz=i", "zap'a:ž=a<p!", "op'a:ž=a<p!",
    // HJP / Školski rječnik: nastrádati, nastrádao, nastrádajū, but
    // nàstrādām and imperative nàstrādāj; pròstirati, pròstirēm,
    // pròstiri. The adjectival cases of nastrádao are in CROATIAN.
    // https://rjecnik.hr/search/?q=nastradati&strict=yes
    // https://rjecnik.hr/search/?q=prostirati&strict=yes
    "nastr/a:d=a<!", "pr/ostir=te",
];
