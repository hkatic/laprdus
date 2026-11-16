/// The Croatian entries: they override the common table.
#[rustfmt::skip] // the entries stand in rows under the comments that give their sources
pub(crate) static CROATIAN: &[&str] = &[
    // Collected words, 14 October 2026: Školski rječnik, checked against
    // HJP (sources and variants in docs/formant.md). Exact paradigms keep
    // the accents out of derivatives; omit Croatian unstressed lengths.
    "kol/i:c|a|ima",
    "d/ubinsk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // ŠR tȉskara; HJP instead has tìskara. Prefer the Institute's entry.
    "t^iskar|a|e|i|u|o|om|ama",
    "p/odizanj|e|a|u|em|ima",
    "st/a:nj|e|a|u|em|ima",
    "raspol/oživ||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // Irregular doći: dȏći/dȏđēm, but dóđi/dóđoh and ŠR dòšao
    // (HJP dȍšao). Ambiguous dođe defaults to the present.
    "d^o:ći", "d^o:đ|em|eš|e|emo|ete|u",
    "d/o:đ|i|imo|ite|oh|osmo|oste|oše",
    "d/oš|ao|la|lo|li|le|avši",

    // Listener's ispuna / Hrvata / -ac plurals audit, 12 October 2026.
    // Školski rječnik and HJP; sources and homographs in docs/formant.md.
    // Exact forms keep these accents out of unrelated derivatives.
    // ȉspuna is falling; the imperative ìspuni uses the noun-twin verb
    // rule at the head of a clause, while u ȉspuni keeps the noun.
    "^ispun|a|e|i|u|o|om|ama",
    // Hr̀vāt has short rising syllabic r (HJP's legacy-font ẕ glyph),
    // but its obliques and plural have long rising a: Hrváta, Hrváti.
    // Hrvate defaults to A plural, not the singular vocative.
    "h/rvat", "hrv/a:t|a|u|om|i|e|ima",
    // Fleeting a disappears in the regular cases; G plural retracts
    // and shortens the accent, V singular has initial short falling.
    "vatrog/a:s|ac|ca|cu|cem|ci|ce|cima",
    "vatr/ogasaca", "v^atrogašče",
    "mušk/a:r|ac|ca|cu|cem|ci|ce|cima",
    "m/uškaraca", "m^uškarče",
    "polic/a:j|ac|ca|cu|cem|ci|ce|cima",
    "pol/icajaca", "p^olicajče",
    // HJP explicitly lists all six negative presents with long rising i.
    "n/i:|sam|si|je|smo|ste|su",
    // Listener-requested long stressed i; neither ŠR nor HJP has these
    // headwords. Rising tone follows the native non-initial convention;
    // case forms extend that preference, not a verified dictionary rule.
    "kafan/i:c|a|e|i|u|o|om|ama", "lagan/i:c|a|e|i|u|o|om|ama",

    // Drinks / ćevap audit, 12 October 2026 (ŠR/HJP, docs/formant.md).
    // Exact paradigms keep these accents out of longer derivatives.
    "p^i:v|o|a|u|om|ima",
    // Colloquial feminine piva follows the same long falling root.
    "p^i:v|e|i|ama",
    "v/i:n|o|a|u|om|ima",
    // ŠR sȏk, but sȍka, sȍkovi (HJP instead gives sòkovi).
    "s^o:k", "s^ok|a|u|om", "s^oče", "s^okov|i|a|e|ima",
    // HJP pèlīnkovac, G pèlīnkōvca; Croatian omits unstressed length.
    "p/elinkov|ac|ca|cu|cem|ci|ce|cima|aca|če",
    "m/edic|a|e|i|u|o|om|ama",
    // ćèvāp -> ćevápa / ćevápi. The shared stem incorrectly stresses
    // the last syllable of the nominative and lengthens ćevàpčić.
    // Ambiguous ćevape defaults to A plural rather than the vocative.
    "ć/evap", "ćev/a:p|a|u|om|i|e|ima",
    "ćev/apčić||a|u|em|i|e|ima",

    // ŠR/HJP svétac, svéci/svétci; V sg svȇče/svȇtče, G pl svȇtācā.
    // Exact cases keep the noun separate from svet, svetica, svečan, etc.
    "sv/e:tac", "sv/e:c|a|u|em|i|e|ima", "sv/e:tc|a|u|em|i|e|ima",
    "sv^e:če", "sv^e:tče", "sv^e:taca",
    // ŠR/Mrežnik nádati se, but nȃdām/nȃdāj. The regular rising
    // infinitive, active forms and third-person plural are in VERBS.
    "n^a:d|am|aš|amo|ate|aj|ajmo|ajte",
    // The noun náda keeps rising tone; adjacent se selects verbal nȃda
    // in the phrase pass. Do not use a stem that would capture nadati.
    "n/a:d|a|e|i|u|o|om|ama",
    // Listener-confirmed vȉšednevni (one short falling accent on VI).
    // ŠR has višèdnēvnī; Martinović/Pudić (2013) records both variants.
    // Exact forms override the time-compound rule only for this adjective.
    "v^išednevn|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // ŠR rázlikujēm / rázlikūj; HJP instead shortens the present's a.
    // The bounded -uj- stem covers presents, imperatives and -ujući;
    // the -ov- forms use the regular -ati class in VERBS below.
    "r/a:zlikuj*",
    // ŠR/HJP àpsurdnī; exact positives leave apsùrdnijī to its rule.
    "/apsurdan", "/apsurdn|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",

    // ŠR/HJP: sposóbnōst, sposóbnosti.
    // Croatian keeps stressed quantity but omits post-accent lengths.
    "spos/o:bnost||i|ima", "spos/o:bnošću",
    // Listener-requested initial stress (12 October 2026): pȍjedini.
    // ŠR/HJP instead give pojèdīnī; this is a pronunciation preference.
    // Exact adjective forms leave the separate noun pojedìnac alone.
    "p^ojedin|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // ŠR/HJP òstarjeti, òstarjela, òstarjeh. The bounded -je- stem
    // supplements /ostar=i in VERBS for the -jeti forms; it does not
    // capture longer derivatives such as ostarjelost.
    "/ostarje*",
    // vještìna / vještìnu, but G plural vještínā (phrase pass).
    "vješt/in|a|e|i|u|o|om|ama",
    "prv/enstveno",
    // The capitalized name lives in places.tsv. Also accept the requested
    // lowercase/all-capital spelling: it has no common-word homograph.
    "jug/oslavij|a|e|i|u|o|om",

    // ŠR: both regret and shore are žȃl. Ambiguous singular obliques
    // default to regret (G žȃla); the shore has G žála and plural žálovi.
    // Do not take žali/žale away from the much more frequent verb žaliti.
    "ž^a:l||a|u|om", "ž/a:lov|i|a|e|ima",
    // ŠR ulágānje / ulágānja: preserve stressed length, omit unstressed
    // length under the existing Croatian policy. Exact noun forms only.
    "ul/a:ganj|e|a|u|em|ima",

    // ŠR/HJP: vrȃg, mrȃk, zrȃk and their long falling singulars.
    // ŠR L vrágu/mráku/zráku and G pl vrágā use the phrase pass.
    "vr^a:g||a|u|om", "vr^a:že",
    "vr^agov|i|a|e|ima", "vr^a:zi",
    "mr^a:k||a|u|om",
    // HJP explicitly records both mrȁkovi and literary mrȃci.
    "mr^akov|i|a|e|ima", "mr^a:ci",
    // Ambiguous zraka/zraku/zrakom default to air. Clear feminine
    // modifiers select the short falling ray noun in the phrase pass.
    "zr^a:k||a|u|om",
    "zr^ak|e|o|ama", "zr^aci",

    // ŠR: pošìljatelj, pošìljatelja, pošìljatelji, G pl pošìljatēljā.
    // Exact noun forms preserve short rising i without reaching derivatives.
    "poš/iljatelj||a|u|em|i|e|ima",

    // ŠR: zvȗčan / zvúčna / zvȗčno, krȋv / kríva / krȋvo.
    // Isolated -a defaults to the indefinite feminine (also masculine G);
    // other ambiguous positive forms use the definite adjective accent.
    "zv^u:čan", "zv/u:čna",
    "zv^u:čn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "zv/učnij*",
    "kr^i:v", "kr/i:va",
    "kr^i:v|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "kr^ivlj|i|a|e|u|eg|ega|em|emu|oj|om|ih|im|ima",
    // ŠR ankèta, ankètē, ankète, but G plural ankétā (phrase pass).
    "ank/et|a|e|i|u|o|om|ama",
    // HJP also gives contracted active forms zàtēgao / zàtēgla.
    // Regular zatégnuo / zatégnula and shifted presents/passives use VERBS.
    "z/ateg|ao|la|lo|li|le",

    // ŠR/HJP: kriminálac, kriminálca/kriminálci, but V krȉminālče
    // and G plural krimìnālācā. Exact forms do not reach kriminalan.
    "krimin/a:l|ac|ca|cu|cem|ci|ce|cima",
    "kr^iminalče", "krim/inalaca",
    // RMS (2011), p. 294: dosìjē, G dosijèa. The Croatian dictionaries
    // list dosje instead; keep the requested spelling and three syllables.
    // As elsewhere in Croatian, omit the unstressed nominative length.
    "dos/ije", "dosij/e|a|u|om|i|e|ima",

    // Očajnički–povucite audit, 12 October 2026. Školski rječnik,
    // cross-checked with HJP; disagreements and sources: docs/formant.md.
    // Keep Croatian's existing omission of unstressed vowel lengths.
    "^očajničk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "bud/u:ćnost||i|ima", "bud/u:ćnošću",
    // ŠR razočaránje / razočaránja; HJP instead has razočáranje.
    "razočar/a:nj|e|a|u|em|ima",
    // Mixed -ati / -im / -j paradigm: póstojati, póstojīm, póstōj.
    // Exact forms override COMMON's post'oj* without reaching postojan.
    "p/o:stoj|ati|at|im|iš|i|imo|ite|e|ao|ala|alo|ali|ale|ah|asmo|aste|aše",
    "p/o:stoj||mo|te|eći|anje|anja|anju|anjem|anjima",
    // The regular -iti class does not generate -eći or verbal nouns.
    "v/i:reći", "v/i:renj|e|a|u|em|ima",
    "v^i:r||a|u|om", "v^irov|i|a|e|ima",
    // D vȋru / L víru, and G/N/A nȍći / L and G plural nòći:
    // the phrase pass recognizes locative and plural-count cues.
    "s^u:nc|e|a|u|em|ima", "s^unaca",
    "n^o:ć", "n^oć|i|u", "n/oćima",
    "tr^u:d", "tr/u:d|a|u|om", "tr/u:dov|i|a|e|ima", "tr/u:dima",
    // Irregular povući: pòvūći / pòvūkao, but povúčem, povúci and
    // short rising povùčen. Do not force these into a regular verb class.
    "p/ovući", "p/ovuk|ao|la|lo|li|le|avši",
    "pov/u:č|em|eš|e|emo|ete", "pov/u:ku",
    "pov/u:c|i|imo|ite", "pov/u:k|oh|osmo|oste|oše",
    "pov/učen||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",

    // Pacijentima–duša audit, 11 October 2026. Školski rječnik and
    // Mrežnik, cross-checked with HJP; evidence in docs/formant.md.
    // Keep exact paradigms and Croatian's omission of unstressed length.
    "pac/ijent||a|u|e|om|i|ima", "pac/ijenata",
    // The requested adjective vȅseli differs from vesèli (veseliti).
    // Nearby verbal cues restore the verb in the frontend's phrase pass.
    "v^eseo", "v^esel|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // ŠR zàštita (HJP zȃštita); G plural zȁštītā is contextual.
    "z/aštit|a|e|i|u|o|om|ama",
    "posjek/otin|a|e|i|u|o|om|ama",
    "neprav/i:lnost||i|ima", "neprav/i:lnošću",
    "nj^ežan", "nj/ežna",
    "nj^ežn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "nj/ežnij*",
    "polikl/inik|a|e|u|o|om|ama", "polikl/inici",
    // HRVATSKI! 1/2023, p. 56 marks short stressed i; its notation omits
    // tone. Rising tone here follows the standard internal-syllable rule.
    "cedev'it|a|e|i|u|o|om|ama",
    "dostoj/anstv|o|a|u|om|ima",
    // ŠR glȃdan (HJP gládan), but gládna, glȃdno and glȃdnī.
    "gl^a:dan", "gl/a:dna",
    "gl^a:dn|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "gl/adnij*",
    "gl^a:d", "gl^a:di", "gl^a:đu",
    // Isolated dana defaults to G singular dȃna; counts select G plural
    // dánā and locative prepositions select dánu in the phrase pass.
    "d^a:n||a|u|om|i|e|ima",
    "b/i:ć|e|a|u|em|ima",
    // di-jé-ta is not jat. Do not take dijete away from the child noun:
    // its spelling is ambiguous with the diet noun's G sg / N/A pl.
    "dij/e:t|a|i|u|o|om|ama",
    // Mrežnik: dúša, dúšē, dȗši, dȗšu, dȗšo, dúšōm; N/A/V pl dȗše,
    // G pl dúšā, D/L/I pl dúšama. Isolated duše defaults to N/A plural.
    "d/u:š|a|om|ama", "d^u:š|i|u|o|e",

    // Testament–nogomet audit, 11 October 2026. Školski rječnik,
    // cross-checked with HJP; HJP supplies pediker and pedikerski.
    // Sources, dictionary disagreements and homographs: docs/formant.md.
    // Exact paradigms avoid imposing noun accents on related adjectives.
    "test/ament||a|u|om|i|e|ima", "test/amenata",
    // ŠR bògatstvo (HJP bogàtstvo), including both G plural spellings.
    "b/ogatstv|o|a|u|om|ima", "b/ogatstava",
    // sažétak, sažétka, sažéci/sažétci, but G plural sàžētākā.
    "saž/e:t|ak|ka|ku|kom|ke|ci|cima", "saž/e:c|i|ima", "s/ažetaka",
    "s/alon", "sal/o:n|a|u|om|i|e|ima",
    "s/alonsk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // pedìkēr -> pedikéra, but adjective pedìkērskī. The spelling
    // pedikeru defaults to D/L pedikéru rather than V pȅdikēru.
    "ped/iker", "pedik/e:r|a|u|om|i|e|ima",
    "ped/ikersk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "^obzir||a|u|om|i|e|ima",
    // ŠR medìcīnskī/medìcīnskā (HJP mȅdicīnskī). The noun medicína
    // retains its existing long rising accent on the second i.
    "med/icinsk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "n^ogomet||a|u|om|i|e|ima",
    "n^ogometn|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // The player nouns differ from the sport: nogomètāš -> nogometáša,
    // nogometàšica, but nogomètāškī. D/L nogometášu is the default
    // for the spelling shared with V nȍgomētāšu.
    "nogom/etaš", "nogomet/a:š|a|u|em|i|e|ima",
    "nogomet/ašic|a|e|i|u|o|om|ama",
    "nogom/etašk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",

    // Stablo / zelenilo / tobogan, 11 October 2026 (ŠR/HJP).
    // Prefer ŠR stàblo over HJP stáblo; G pl stabálā / stáblā differs.
    // The shorter G plural is selected by the phrase-level case cues.
    "st/abl|o|a|u|om|ima", "stab/a:la",
    "zel/enil|o|a|u|om|ima", "t^obogan||a|u|e|om|i|ima",

    // Stanovnik–godišnjakinja audit, 11 October 2026. Školski rječnik
    // principal forms, cross-checked with HJP; citations in docs/formant.md.
    // Croatian retains its existing omission of post-accent lengths.
    "stan/ovnik", "stanovn/i:k|a|u|om|e", "stanovn/i:c|i|ima",
    // stanovnici defaults to masculine N pl, not feminine D/L sg.
    "st^anovniče", "stan/ovnic|a|e|u|o|om|ama",
    "stanovn/i:štv|o|a|u|om|ima",
    "st/anar", "stan/a:r|a|u|om|i|e|ima",
    // stanaru/stanare default to D/L sg and A pl, not their vocatives.
    "t^u:", "č^e:sto", "č^ešće",
    "m^agazin||a|u|e|om|i|ima",
    "n^eb|o|a|u|om", "neb/es|a|ima",
    // događaju defaults to the verb dogáđajū; D/L dȍgađāju is contextual.
    "d^ogađaj||a|em|i|e|ima",
    // pada defaults to the frequent verb pȁdā; G pȃda is contextual.
    "p^a:d||u|om|e", "p^adov|i|a|e|ima",
    // User preference: short mnȍm, although ŠR gives dictionary mnȏm.
    "mn^om",
    "r^a:dnik||a|u|om|e", "r^a:dniče",
    "r^a:dnic|a|e|i|u|o|om|ama|ima",
    "dok/ument||a|u|e|om|i|ima", "dok/umenata",
    "z/a:vod||a|u|e|om|i|ima", "n/a:vod||a|u|e|om|i|ima",
    "im/ovin|a|e|i|u|o|om|ama", "mir/ovin|a|e|i|u|o|om|ama",
    // Prefer ŠR ministárstvo over HJP ministàrstvo. G plural shifts.
    "minist/a:rstv|o|a|u|om|ima", "min/istarstava",
    // The bare feminine component follows attested petogodišnjàkinja
    // by analogy; compounds already use StressRules::time_compound().
    "godišnj/akinj|a|e|i|u|o|om|ama",
    // Keep the noun ȍsjećāj separate from the verb òsjećati.
    "^osjećaj||a|em|i|e|ima",

    // ŠR presvláčēnje / presvláčēnja: root á, not the generic -énje.
    // Exact noun forms leave the related verb to its own accent rules.
    "presvl/a:čenj|e|a|u|em|ima",

    // -ivac audit, 11 October 2026 (ŠR/HJP; sources in docs/formant.md).
    // Exact noun paradigms: long rising í in these loans, but short
    // rising on the previous vowel in G pl. No blanket *ivac rule:
    // -ljivac/-njivac, short native words and vocatives differ.
    "pozit/i:vac", "pozit/i:vc|a|u|em|i|e|ima", "poz/itivaca",
    "negat/i:vac", "negat/i:vc|a|u|em|i|e|ima", "neg/ativaca",
    "depres/i:vac", "depres/i:vc|a|u|em|i|e|ima", "depr/esivaca",
    "emot/i:vac", "emot/i:vc|a|u|em|i|e|ima", "em/otivaca",
    "agres/i:vac", "agres/i:vc|a|u|em|i|e|ima", "agr/esivaca",
    "kreat/i:vac", "kreat/i:vc|a|u|em|i|e|ima", "kre/ativaca",
    "primit/i:vac", "primit/i:vc|a|u|em|i|e|ima", "prim/itivaca",
    "operat/i:vac", "operat/i:vc|a|u|em|i|e|ima", "oper/ativaca",
    "konzervat/i:vac", "konzervat/i:vc|a|u|em|i|e|ima", "konzerv/ativaca",
    "rekreat/i:vac", "rekreat/i:vc|a|u|em|i|e|ima", "rekre/ativaca",
    "na/i:vac", "na/i:vc|a|u|em|i|e|ima", "n/aivaca",
    // Prescribe only vocatives explicitly accented in the references.
    // ŠR prȉmitīvče takes precedence over HJP's unshifted -vče.
    "pozit/i:vče", "d^epresivče", "kre/ativče",
    "pr^imitivče", "k^onzervativče", "n^aivče",
    // Native two-syllable nouns have long falling G pl. Prefer ŠR
    // žívac/žívca over HJP žìvac/žìvca; both have G pl žȋvācā.
    "kr/i:vac", "kr/i:vc|a|u|em|i|e|ima", "kr^i:vaca", "kr^i:vče",
    "s/i:vac", "s/i:vc|a|u|em|i|e|ima", "s^i:vaca", "s/i:vče",
    "ž/i:vac", "ž/i:vc|a|u|em|i|e|ima", "ž^i:vaca",
    // Short rising root accents persist in these -ljivac/-njivac nouns.
    "s/umnjiv|ac|ca|cu|cem|ci|ce|cima|aca|če",
    "pl/ašljiv|ac|ca|cu|cem|ci|ce|cima|aca|če",
    "š/utljiv|ac|ca|cu|cem|ci|ce|cima|aca|če",
    "st/idljiv|ac|ca|cu|cem|ci|ce|cima|aca|če",
    "sram/ežljiv|ac|ca|cu|cem|ci|ce|cima|aca|če",
    "zav/idljiv|ac|ca|cu|cem|ci|ce|cima|aca",

    // Household words, 11 October 2026: Školski rječnik principal forms;
    // HJP for bȃjan/bȃjno. Sources and homographs: docs/formant.md.
    // Explicit paradigms keep noun accents out of verbs and derivatives.
    "kont/ejner||a|u|om|i|e|ima",
    // ŠR komòda, G pl komódā (context); HJP instead has komóda throughout.
    "kom/od|a|e|i|u|o|om|ama",
    // L stánu is contextual; stane stays the common verb, not V stȃne.
    "st^a:n||a|u|om", "st^anov|i|a|e|ima",
    // ìspust and ìspustē share the accent. The singular command ispùsti
    // still wins at clause head; the verb's shifting forms use VERBS.
    "/ispust||a|u|om|i|e|ima",
    "s/a:stav||a|u|om|i|e|ima",
    // ŠR ràstava, G pl rȁstāvā (context); HJP instead has rȃstava.
    "r/astav|a|e|i|u|o|om|ama",
    "t^a:jn|a|e|i|u|o|om|ama|og|oga|ome|omu|oj|ih|im|ima",
    // Indefinite sjájan/sjájna/sjájno and trȃjan/trájna/trȃjno differ
    // from definite sjȃjnī/trȃjnī. Ambiguous short endings default to
    // the indefinite; explicitly written accents can select the other.
    "sj/a:jan", "sj/a:jn|a|e|u|o|oj",
    "sj^a:jn|i|og|oga|om|ome|omu|ih|im|ima",
    "sj/ajnij|i|a|e|u|eg|ega|em|emu|oj|om|ih|im|ima",
    "tr^a:jan", "tr/a:jn|a|e|u|oj",
    "tr^a:jn|o|i|og|oga|om|ome|omu|ih|im|ima",
    "tr/ajnij|i|a|e|u|eg|ega|em|emu|oj|om|ih|im|ima",
    "b^a:jan", "b^a:jn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "j^a:jn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // jáje/jája but the expanded singular stem is short: jàjeta.
    "j/a:j|e|a|u|em|ima", "j/ajet|a|u|om",

    // ŠR/HJP sȉgūran: short falling i, also in the positive adjective
    // forms and sȉgūrno. Exact forms leave sigúrnōst and sigùrnijī alone.
    // Croatian continues to omit unstressed vowel lengths.
    "s^iguran", "s^igurn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // Listener-selected HJP variant dirèktan / dirèktno (11 October 2026),
    // short rising e. ŠR and COMMON use dìrektan / dìrektno instead.
    "dir/ektan", "dir/ektn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",

    // Collected nouns and names, 11 October 2026 (Školski rječnik / HJP).
    // Sources, dictionary variants and name capitalization: docs/formant.md.
    // Exact forms keep the noun accents out of verbs and derivatives.
    "kom/a:rac", "kom/a:rc|a|u|em|i|e|ima",
    "k^omarče", "k/omaraca",
    // Prefer ŠR žȁlac / žȁlācā (HJP žálac / žȃlācā); both have žálca.
    "ž^alac", "ž/a:lc|a|u|em|i|e|ima", "ž^alaca",
    "s/a:mac", "s/a:mc|a|u|em|i|e|ima", "s^a:maca",
    "š/a:rac", "š/a:rc|a|u|em|i|e|ima", "š^a:raca",
    "s/okak", "sok/a:k|a|u|om|e", "sok/a:c|i|ima",
    // planinárēnje, not the generic -énje; leave planináriti / planìnārīm
    // to the verb system. Croatian continues to omit unstressed lengths.
    "planin/a:renj|e|a|u|em|ima",
    // Rímac is already in the proper-name table. Also accept the supplied
    // lowercase spelling; no common Croatian noun shares these forms.
    "r/i:mac", "r/i:mc|a|u|em",
    "opć/enit||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "medijat/e:k|a|e|i|u|o|om|ama", "medijat/e:ci",
    "r^o:tor||a|u|e|om|i|ima",
    "akt/i:vnost||i|ima", "akt/i:vnošću",
    "el/ement||a|u|e|om|i|ima", "el/emenata",
    // Prefer ŠR snȉmka (HJP snȋmka); G pl snȉmākā / snȋmkā / snȉmkī.
    // The homographic snȋmkā is selected by the existing case-cue pass.
    "sn^imk|a|e|i|u|o|om|ama", "sn^imci", "sn^imaka",
    // dȍdīra, not the loanword rule's dodíra; do not capture dodírati.
    "d^odir||a|u|e|om|i|ima",
    "n/a:redb|a|e|i|u|o|om|ama", "n/a:redaba",
    // zvȗk, L zvúku, but short falling zvȕkovi; G plural zvúkā.
    "zv^u:k||a|u|om", "zv^u:če", "zv^u:c|i|ima",
    "zv^ukov|i|a|e|ima",
    "gl/avn|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",

    // Collected-word audit, 11 October 2026: Školski rječnik and HJP.
    // Full citations, disagreements and ambiguous cases: docs/formant.md.
    // Exact paradigms keep these accents out of unrelated derivatives.
    "sl^a:vlj|e|a|u|em|ima",
    // Listener-preferred evènata: short second e, no long a. The checked
    // dictionaries do not accent this inflection, so keep it exact.
    "ev/enata",
    "fest/ival", "festiv/a:l|a|u|e|om|i|ima",
    "r/a:zlog||a|u|om|e", "r/a:zlo|zi|zima|že",
    "zahv/a:lnost||i|ima", "zahv/a:lnošću",
    "n/asmiješen||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "putov/a:nj|e|a|u|em|ima",
    // Prefer ŠR kráfna, nàbava, kònobār where HJP differs.
    "kr/a:fn|a|e|i|u|o|om|ama",
    "n/abav|a|e|i|u|o|om|ama",
    "k/onobar||a|u|om|i|ima", "k^onobare",
    // Listener chose HJP drúštvo (12 October), over ŠR/Mrežnik drùštvo.
    // Keep the separately attested long falling G plural drȗštāvā.
    "dr/u:štv|o|a|u|om|ima", "dr^u:štava",
    "gl^a:s||a|u|e|om|i", "gl^asov|i|a|e|ima",
    "strplj/e:nj|e|a|u|em|ima",
    "p/ažnj|a|e|i|u|o|om|ama",
    "hr/a:brost||i|ima", "hr/a:brošću",
    "samopouzd/a:nj|e|a|u|em|ima",
    "samod/opadan", "samod/opadn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "samod/opadnost||i|ima", "samod/opadnošću",
    // Irregular -vesti participles differ from the present and passive:
    // ìzvela but izvèdē/izvèden, and likewise od-/za-/do-.
    "/izvesti", "/izv|eo|ela|elo|eli|ele",
    "/odvesti", "/odv|eo|ela|elo|eli|ele",
    "z/avesti", "z/av|eo|ela|elo|eli|ele",
    "d/ovesti", "d/ov|eo|ela|elo|eli|ele",
    // Irregular passives: ukràden, obìjen, ubìjen, razbìjen, rèčen. The
    // i-je in the -biti participles is hiatus, handled by letter-to-sound.
    "ukr/aden||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "ob/ijen||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "ub/ijen||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "razb/ijen||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "r/ečen||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "poč/initelj||a|u|e|em|i|ima",
    // Lowercase alias for the requested Zagreb neighbourhood; the proper
    // name and its cases also live in places.tsv. HJP onomastics: Špánsko.
    "šp/a:nsk|o|og|oga|om|ome|omu|im",
    "pr/oblem", "probl/e:m|a|u|e|om|i|ima",
    "tr^a:vnj|ak|aka|aku|akom|ače|aci|acima|ake",
    "t/eren", "ter/e:n|a|u|e|om|i|ima",
    "tr/e:ner||a|u|e|om|i|ima",
    "sm^i:sao", "sm^i:sl|a|u|om|e", "sm/islov|i|a|e|ima",
    "st^a:nk|a|e|i|u|o|om|ama", "st^a:nci",
    // stvȃri defaults to N/A plural or G/D singular. Clear G plural and
    // L singular contexts are resolved in Frontend; DLI plural is stvárima.
    "stv^a:r||i|ju", "stv/a:rima",
    // HJP néću, but nȇćeš/nȇće/nȇćemo/nȇćete; néće (3pl) is
    // homographic with 3sg, which is the unmarked default.
    "n/e:ću", "n^e:ć|eš|e|emo|ete",
    "pr/i:lik|a|e|u|o|om|ama", "pr/i:lici",
    // ŠR komponènta, DL komponènti, but G plural kompònentī.
    // An isolated komponenti keeps the G plural reading.
    "kompon/ent|a|e|u|o|om|ama",
    "komp/onenti", "komp/onenata",
    "kol/ačić||a|u|e|em|i|ima",
    "r^e:p||a|u|e|om", "r^epov|i|a|e|ima",
    // ŠR keeps krokòdil's accent in the cases (HJP G krokodíla).
    "krok/odil||a|u|e|om|i|ima",
    "z^e:c||a|u|om", "z^e:če", "z^ečev|i|a|e|ima",
    "z^ecov|i|a|e|ima",
    "v^u:k||a|u|om", "v^u:če", "v^u:ci", "v^ukov|i|a|e|ima",
    "žir/af|a|e|i|u|o|om|ama", "pant/e:r|a|e|i|u|o|om|ama",
    // Listener's tiflo- compounds keep two component accents (Frontend),
    // including loans whose dictionary accent would be on -flo-.
    // The short initial accent of tiflo is a requested pronunciation;
    // tèhnika/tèhnici are confirmed by Školski rječnik and HJP.
    "t^iflo", "t/ehnik|a|e|i|u|o|om|ama", "t/ehnici",
    // Listener's 11 October 2026 cycle, checked against Školski rječnik
    // and HJP; sources, variants and homographs are in docs/formant.md.
    // Exact paradigms avoid extending a noun's accent to other derivatives.
    // Prefer ŠR pomágalo and kòntakātā (HJP pomàgalo and kȍntakātā).
    // As elsewhere in Croatian, omit unstressed lengths.
    "pom/a:gal|o|a|u|om|ima",
    "k/ontakt||a|u|e|om|i|ima", "k/ontakata",
    "odgov/o:rnost||i|ima", "odgov/o:rnošću",
    "str^učnj|ak|aka|aku|akom|ače|aci|acima|ake",
    // HJP Rijéčanin, Rijéčānka: the first (ije) nucleus, not -čan-.
    // Exact entries also cover lowercase input and the longer case forms.
    "r/ije:čanin||a|u|e|om", "r/ije:čan|i|a|ima|e",
    "r/ije:čank|a|e|i|u|o|om|ama", "r/ije:čanaka",
    // ŠR logòpēd / logopédi and pedìjātar / pedìjātri. The spelling
    // logopede defaults to A plural; rare V singular lȍgopēde can be marked.
    "log/oped", "logop/e:d|a|u|om|i|ima|e",
    "ped/ijatar", "ped/ijatr|a|u|e|om|i|ima", "ped/ijatara",
    "prib/ad|ača|ače|ači|aču|ačo|ačom|ačama",
    "tum/a:čenj|e|a|u|em|ima",
    // Adjectival cases of nastrádao; the regular verb is in VERBS.
    "nastr/a:dal|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // Irregular prònāći / prònāđē; keep COMMON's imperative pronáđi.
    "pr/onaći", "pr/onađ|em|eš|e|emo|ete|u",
    "pr/onaš|ao|la|lo|li|le|avši",
    // ŠR's indefinite svijetlopláva/svijetloplávo versus masculine
    // svijétloplāv and definite svijétloplāvī. Ambiguous a/o/e/u forms
    // default to the indefinite reading; written accents can select either.
    "svijetlopl/a:v|a|o|e|u|om",
    "sv/ije:tloplav||i|og|oga|ome|omu|oj|ih|im|ima",
    // Listener requests short A in unmarked nas/vas, including isolated
    // words. Dictionaries distinguish emphatic nȃs/vȃs from clitic nas/vas;
    // explicit accent marks still select the long emphatic pronunciation.
    "n^as", "v^as",
    // ŠR komplìcīrān / komplìcīrāna / komplìcīrāno; HJP also has the
    // adverb komplìcīrano and noun komplìcīranōst. Croatian omits unstressed
    // length. Exact positive forms leave komplicírati and komplicirànijī
    // to their separate verb and comparative rules.
    // https://rjecnik.hr/search/?q=kompliciran&strict=yes
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eltuXhM%3D
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eltuXhQ%3D
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eltuXhU%3D
    "kompl/iciran||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "kompl/icirano|st|sti|stima|šću",
    // HJP: pòbjednīk, pòbjedničkī (feminine pòbjedničkā); keep initial
    // short rising o in every case, including -nici/-nicima/-niče.
    // Croatian omits unstressed lengths. Exact paradigms avoid derivatives.
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxmXxU%3D
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eVxmXxM%3D
    "p/objedn|ik|ika|iku|ikom|ici|icima|ike|iče",
    "p/objedničk|i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // Mrežnik: dvádesēt, dvádesētī, but dvadesétak; Školski rječnik:
    // dvádesetero. Restore the long rising a in Croatian, replacing the
    // older falling-accent rendering override only for these exact forms.
    // https://rjecnik.hr/mreznik/dvadeset/
    // https://rjecnik.hr/mreznik/dvadeseti/
    // https://rjecnik.hr/mreznik/dvadesetak/
    // https://rjecnik.hr/?letter=d&page=28
    "dv/a:deset||i|a|o|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "dv/a:desetero", "dvades/e:tak",
    // Mrežnik / HJP: idéja, bèsplatan (checked 2026-10-08).
    // Exact paradigms keep unrelated derivatives out; Croatian omits the
    // dictionaries' post-accent lengths, as elsewhere in this table.
    // https://rjecnik.hr/mreznik/ideja/
    // https://rjecnik.hr/mreznik/besplatan/
    "id/e:j|a|e|i|u|o|om|ama",
    "b/esplatan", "b/esplatn|a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // HJP súčēlje; HJP / Školski rječnik òtīći, òtišao, òtišla.
    // Keep Croatian's usual omission of unstressed length. Exact noun
    // and irregular verb forms avoid catching sučèliti or otíđi.
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=d1pjWBU%3D
    // https://hjp.znanje.hr/index.php?show=search_by_id&id=eFdmXhI%3D
    "s/u:čelj|e|a|u|em|ima",
    "/otići", "/otiš|ao|la|lo|li|le|avši",
    // Školski rječnik / HJP: sȃt, rȃd, but L sátu/rádu, G pl sátī,
    // and short falling sȁtovi/rȁdovi. Frontend resolves the contextual
    // forms; isolated satu/radu and sati use D sg and N pl respectively.
    // Kapović, Filologija 54 (2010), also records DLI pl sátima.
    // https://rjecnik.hr/search/?q=sat&strict=yes
    // https://rjecnik.hr/search/?q=rad&strict=yes
    // https://hrcak.srce.hr/file/77788
    "s^a:t||a|u|e|om|i", "s/a:tima", "s^atov|i|a|e|ima",
    "r^a:d||a|u|e|om", "r^adov|i|a|e|ima",
    // Forty to ninety without the length of the last syllable (četrdèset,
    // pedèset for the dictionaries' četrdèsēt, pedèsēt), as the other
    // entries for this voice do. Ordinals and -ak forms follow the stem.
    "četrd'eset", "četrd'eset*", "ped'eset", "ped'eset*", "šezd'eset", "šezd'eset*",
    "sedamd'eset", "sedamd'eset*", "osamd'eset", "osamd'eset*",
    "deved'eset", "deved'eset*",
    "pr/ofesor*", "pr/ocent*", "d'irektor*", "V^ojvodin*", "'ukrajin*",
    "s^iječ*", "v/eljač*", "/ožuj*", "tr/a:v*", "sv/i:b*", "l/i:p*",
    "s'rp*", "k^olovoz*", "r/u:j*", "l^istopad*", "st^uden*",
    "pr^osin*",
    // Croatian is commonly spoken with the long e of obavijestiti stressed in
    // the present and the passive participle as well (obavijéstim,
    // obavijéšten for the dictionaries' obàvijēstīm, obàvijēšten), like the
    // verbs in IJE_VERBS; so is razumijévam.
    "obav'ijest|i:m|i:š|i:mo|e:", "obav'iješten*", "razum'ijev*",
    // The same for the present of započeti and the verbs in -uzeti.
    "zap'očn|em|eš|e|emo|ete|u", "pre'uzm|em|eš|e|emo|ete|u",
    "od'uzm|em|eš|e|emo|ete|u", "pod'uzm|em|eš|e|emo|ete|u",
    "za'uzm|em|eš|e|emo|ete|u",
    // Bèla, the Croatian name of the kings Béla (Zlatna bula Bele IV.);
    // in Serbian "bela, bele" is the adjective
    "B/el|a|e|i|u|om",
    // HJP: Bȅograd in Croatian (Serbian Beògrad, COMMON)
    "B^eograd*",

    // Listener's second October 2026 list. Školski rječnik hrvatskoga
    // jezika, cross-checked with HJP; sources and homographs in
    // docs/formant.md. As elsewhere in this table, omit unstressed length.
    // The currency stays on the first syllable in every case (dȍlāra,
    // dȍlārā), unlike the productive -ar rule. ŠR ȅuro, HJP èuro: use ŠR.
    "d^olar*", "^eur|o|a|u|om|i|ima|e",
    "n/a:ravan", "n/a:ravn*",
    // Exact noun forms: a priča stem would also swallow príčati/príčao.
    // The present prȋčā shares the noun's tone; Croatian drops its final
    // length. Other present/imperative forms are falling, príčajū rising.
    "pr^i:č|a|e|i|u|o|om|ama|am|aš|amo|ate|aj|ajmo|ajte",
    "pr^i:čan*",
    "dj^elomičan", "dj^elomičn*", "č^injenic*",
    "n^emoguć*",   // nemogúćnost keeps its own longer entry
    "pop/o:dne*", "pop/o:dnevima", "pop/o:dnevn*", "pop/o:dnevan",
    // èuropskī differs from Európa; do not change the place name's stem.
    "/europsk*", "osigur/a:nj*",
    // The noun shifts onto its long i in the oblique cases; the adjective
    // retains the nominative's accent (automòbīlskī).
    "autom/obil", "automob/i:l*", "autom/obilsk*",
    "tek/ućin*", "k/afić", "kaf/i:ć*",
    // pomòćnīk / pomoćníka / pomoćníci, but pomòćnica and pomòćničkī.
    // "pomoćnici" defaults to the masculine plural, not the feminine D/L.
    "pom/oćnik", "pomoćn/i:k*", "pomoćn/i:c|i|ima",
    "p^omoćniče", "pom/oćnic*", "pom/oćničk*",
    "t^amo", "progn/o:z*", "kab/anic*",
    // Mrežnik and HJP: kȉšobrān, kȉšobrāna, kȉšobrāni. Exact noun
    // forms keep the short falling i without imposing it on derivatives.
    "k^išobran||a|u|e|om|i|ima",
    "sloj/evit||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    "sloj/evitost*", "sloj/evitošću",
    // Derivatives checked separately so verb/noun stems cannot take them.
    // proslavi/proslave default to the noun, as other exact homographs do.
    "pr^oslav|a|e|i|u|o|om|ama", "ves/e:lj*",
    // HJP zàštīćenōst, like the participle (zaštititi shifts in VERBS).
    "z/aštićenost*", "z/aštićenošću",

    // Školski rječnik and Mrežnik: batèrija (HJP has bàtērija).
    "bat/erij|a|e|i|u|o|om|ama", "bat/erijsk*",
    // ŠR and HJP: pročìtati/pročìtām, but prȍčitān. The passive needs
    // a separate falling-tone paradigm; the negative stays nepròčitan.
    "pr^očitan||a|o|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima",
    // Školski rječnik: the adverb túžno, like the neuter túžno (HJP, Rečnik
    // Matice srpske and Wiktionary tȗžno, kept for Stojan and Mirsad)
    "t/u:žno",
];
