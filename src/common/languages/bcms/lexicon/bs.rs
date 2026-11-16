/// The Bosnian entries: they override the common table.
#[rustfmt::skip] // the entries stand in rows under the comments that give their sources
pub(crate) static BOSNIAN: &[&str] = &[
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
    "j^u:l", "^august*", "sept'embar", "sept'embr*", "okt'o:bar",
    "okt'o:br*", "nov'embar", "nov'embr*", "dec'embar", "dec'embr*",
    "k^ahv*", "k^af*", "l^ahko", "m^ehk*",
    "m^onoto:no", "r^e:da:ka:", "k/orisni:k*", "k/orisni:ci", "k/orisni:cima",
    "k/orisni:če", "k^ori:stan", "k^ori:st",
    "r^ođenda:n*", "z^auze:t*",
    // The words and names checked in October 2026 (see COMMON) with their
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
    // The third October 2026 list (see COMMON). Alić's handbook gives
    // prìjatelj as the main Bosnian form (prȉjatelj in the Croatian and
    // Serbian dictionaries) and dožívīm as Rječnik bosanskoga jezika has it
    "pr/ijatelj*", "prijat/eljsta:va:", "neprij/ate:ljsta:va:",
    "sn^a:đ|e:m|e:š|e:|e:mo|e:te|u:",
    "p^otencija:l|an|na|no|ni|ne|nu|nog|nom|noj|nih|nim|noga|nome|nomu|nima",
    "sn/alaže:nj|e|a|u|em|ima", "simul'a:ci:jsk*", "sim/ula:tor*", "sim/ula:torsk*",
    "dož'i:v|i:m|i:š|e:", "prež'i:v|i:m|i:š|e:", "prož'i:v|i:m|i:š|e:",
    "nadž'i:v|i:m|i:š|e:", "pož'i:v|i:m|i:š|e:", "ž/i:v|i:m|i:š",
    "d/oži:vljen*", "pr/eži:vljen*", "pr/oži:vljen*", "n/adži:vljen*",
    "d^oživlja:j*", "nev/idljivo:st", "nesn/alaže:nj|e|a|u|em|ima",
    "p^oštova:n*", "^interesova:n*", "^interesuj|e:m|e:š|e:|e:mo|e:te|u:",
    "z/ainteresuj|e:m|e:š|e:|e:mo|e:te|u:", "z/ainteresova:n*", "nez/ainteresova:n*",
    // The fourth list (see COMMON): vòdīč, poluvòdīč, Ègipćānka
    "v/odi:č", "poluv'odi:č", "/egipća:nk*",
    // gòdišnjāk (see COMMON)
    "g/odišnja:k*", "g/odišnja:c|i|ima", "g/odišnja:če",
    // The fifth October 2026 list (see COMMON) with the length after the
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
    // The sixth October 2026 list (see COMMON) with the length after the
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
    // Alić: the nouns like gubítak keep the accent in every case, the
    // genitive plural too (gubítākā)
    "gub'i:ta:ka:", "dob'i:ta:ka:", "už'i:ta:ka:", "prim'i:ta:ka:", "odb'i:ta:ka:",
    "razv'i:ta:ka:", "bolj'i:ta:ka:", "zgod'i:ta:ka:", "prob'i:ta:ka:", "im'u:ta:ka:",
    "osn'u:ta:ka:", "priv'i:ta:ka:",
    // ... and the nouns in -tak of the same type: podátākā ... trenútākā
    "pod'a:ta:ka:", "dod'a:ta:ka:", "zad'a:ta:ka:", "ost'a:ta:ka:",
    "poč'e:ta:ka:", "završ'e:ta:ka:", "izuz'e:ta:ka:", "tren'u:ta:ka:",
    // The eleventh list: zànāt, zànātskī, pȋsāmā, dètālj, dètāljan, dȅtāljno
    "z/ana:t", "z/ana:tsk*", "p^i:sa:ma:", "d/eta:lj", "d/eta:ljan",
    "d/eta:ljn|a|i|e|u|og|oga|om|ome|omu|oj|ih|im|ima", "d^eta:ljno",
];
