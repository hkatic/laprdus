# Laprdus za Android — Što je novo

## Verzija 2.0.0 (build 12) — u razvoju

- Tri nova glasa koji se u cijelosti sintetiziraju po pravilima, bez snimaka: Zvonko, Stojan i Mirsad. Ostaju čisti i razumljivi i pri vrlo velikim brzinama govora. Zvonko je zadani glas, za svaki jezik. Josip, Vlado, Detence, Baba i Đedo i dalje su dostupni.
- Svaki glas govori svaki jezik. Jezik (hrvatski, srpski ili bosanski) bira se odvojeno od glasa, kao u eSpeaku ili Eloquenceu: nova postavka Jezik ispod glasa određuje jezik gumba „Laprdaj!”, a TalkBack i druge aplikacije biraju jezik svojega govora dok glas ostaje onaj koji ste odabrali. Jezik određuje pravila izgovora, naglasak riječi, imena slova pri slovkanju i brojevne riječi (*tisuća* i *milijun* na hrvatskom, *hiljada* i *milion* na srpskom i bosanskom, *dvjesto* / *dvesta*). Sustav sada popisuje svaki glas na sva tri jezika, ukupno 54 glasa, s imenima poput `zvonko-hr`, `zvonko-sr` ili `josip-bs`.
- Bosanski je sada podržan jezik kojim govori svaki glas; Mirsad je njegov izvorni glas.
- Prisilno korištenje jezika sada znači: uvijek govori jezikom odabranim u Laprdusovim postavkama, koji god jezik aplikacija zatražila. Prije je ponovno učitavalo spremljeni glas. Glas kojim aplikacija govori uvijek je onaj odabran u Laprdusovim postavkama.
- Preimenovani glasovi. Imena sada dolaze iz govorne jedinice, bez prefiksa „Laprdus” i bez jezika u zagradi, a pjevajući glasovi nazvani su po sviraču i pjesmi ili po kraju svoje pjesme: Zvonko Orguljaš sada je *Orguljaš - Vila Velebita*, Klapa Zvonko *Dalmacija Klapa*, Zvonko Pjevač *Lika Velebit*, Zvonko Bećarac *Bećarac*, Stojan Trubač *Trubač - Kreće se lađa francuska*, Stojan Harmonikaš *Harmonikaš - Kreće se lađa francuska*, Stojan Pevač *Solunac*, Mirsad Sevdalija *Sevdalija - Kad ja pođoh na Bembašu*, Mirsad Sazlija *Sazlija - Kad ja pođoh na Bembašu*, a Mirsad Solist *Sarajevo Bembaša*; djedovo ime piše se *Đedo*. Glas spremljen pod starim imenom (`djed`, `klapa`, `pjevac`, `pevac`, `solist`) i dalje se učitava.
- Naglasak riječi i rečenična melodija: svaka je riječ naglašena na pravom slogu (ugrađeni rječnik s nekoliko tisuća riječi i imena, pravila za česte nastavke i naglasni znakovi koje možete upisati u rječnik izgovora, na primjer `telèfon`), a svaka rečenica dobiva prirodnu intonaciju: porast na kraju pitanja, pad na točki, zadržani porast na zarezu. Engleske riječi i imena naglašeni su na slogu na kojem ih naglašava engleski (*Croàtian*, *Ábleton*), a riječ s velikim slovima u sredini čita se kao njezini dijelovi (*ElevenLabs* kao *Eleven Labs*).
- Josip, Vlado, Detence, Baba i Đedo imaju novi govorni mehanizam: isti naglasak riječi i istu melodiju kao novi glasovi, prirodan tempo, čujne glasove p, t, k, b, d i g pri velikim brzinama i više nikakvih klikova ni pucketanja između glasova. Promjena brzine više ne mijenja visinu, promjena visine više ne čini glas hrapavim, a izvedeni glasovi zvuče kao dijete, baka i djed, a ne kao ubrzana ili usporena snimka.
- Deset pjevajućih glasova: Orguljaš - Vila Velebita, Dalmacija Klapa, Lika Velebit i Bećarac (Zvonkovim glasom), Trubač - Kreće se lađa francuska, Harmonikaš - Kreće se lađa francuska i Solunac (Stojanovim), Sevdalija - Kad ja pođoh na Bembašu, Sazlija - Kad ja pođoh na Bembašu i Sarajevo Bembaša (Mirsadovim). Svaki tekst, na bilo kojem jeziku, pjevaju na narodnu pjesmu; brzina govora određuje tempo, visina transponira pjesmu, a razina infleksije vibrato.
- Dvije nove postavke za nove glasove, prikazane samo kad je jedan od njih odabran: Razina infleksije (0% je monoton govor, 50% prirodna melodija, 100% udvostručuje svaki pomak) i Ubrzanje (množitelj brzine govora od 0,5 do 3, pa klizač brzine doseže veću ili manju najveću brzinu). Novi glasovi prihvaćaju brzinu i visinu govora od četvrtine do četverostruke uobičajene vrijednosti.
- Brzina i visina govora podešene u Laprdusu sada rade zajedno s onima čitača ekrana ili aplikacije. One su normalna brzina i visina glasa, a TalkBack ili druga aplikacija svojom brzinom i visinom govor od njih ubrzava, usporava, podiže ili spušta: dok čitač ekrana govori svojom normalnom brzinom, Laprdus govori upravo brzinom koju ste podesili. Prije su se koristile samo uz uključen odgovarajući prekidač za prisilno korištenje, pa pomicanje klizača brzine nije mijenjalo ništa što TalkBack izgovara. Prekidači za prisilno korištenje sada znače da Laprdus zanemaruje brzinu ili visinu koju aplikacija traži.
- Naglasni rječnik: novi korisnički rječnik `accents.json` ispravlja naglasak riječi odjednom za sve njezine oblike ili cijelog glagola. Uređuje se na zaslonu Rječnici kao i ostali rječnici (vrsta rječnika Naglasni rječnik), s poljima za unos, vrstu (riječ ili glagol), jezik i komentar; aplikacija provjerava svaki unos istim pravilima kojima ga Laprdus čita i neispravan odbija uz razlog. Sprema se uz ostale korisničke rječnike i učitava zajedno s njima (vidi odjeljak 5.11 korisničkog priručnika).
- Brojevi se čitaju na odabranom jeziku (hrvatski: *tisuća*, *milijun*; srpski i bosanski: *hiljada*, *milion*), svakim glasom, a jedan i dva slažu se s brojevnom riječi (*dvije tisuće*, *dvadeset jedna tisuća*).
- Ispravljeno: unosi dodani u korisnički rječnik slovkanja i rječnik emodžija nisu imali učinka.
- Ispravljeno: postavka pauze za novi red nije imala učinka, a retci objave ili popisa stapali su se u jednu rečenicu. Prijelom retka sada završava frazu tom pauzom, a sljedeći redak počinje novu rečenicu (redak koji počinje s „S ponosom” više ne počinje slovom *es*).
- Laprdus sada sustavu nudi i bosanski, a nijedna aplikacija više ne može zamijeniti vaš glas: zahtjev aplikacije za jezikom mijenja samo jezik.
- Ispravljeno: rječnik slovkanja i rječnik emodžija uređeni u aplikaciji govorna jedinica nije koristila; sva tri korisnička rječnika sada vrijede čim se glas učita. Naglasni rječnik čita se iz iste mape.
- Ispravljeno: aplikacija i govorna jedinica dijelile su jedan mehanizam, pa je gumb „Laprdaj!” govorio brzinom, visinom i glasnoćom posljednjeg TalkBackova zahtjeva, a odabir glasa u aplikaciji ostavljao je TalkBack bez vaših unosa iz rječnika dok se glas ponovno ne učita. Sada svaka ima svoj: gumb „Laprdaj!” govori vašim postavkama i s vašim rječnicima, a TalkBack prelazi na glas koji odaberete, s rječnicima na mjestu.
- Unos u rječniku koji dodate, promijenite ili obrišete u aplikaciji, kao i uključivanje ili isključivanje korisničkih rječnika, u TalkBacku sada vrijedi odmah. Prije je čekalo da se glas ponovno učita.

## Verzija 1.0.0 (build 11) — 2. rujna 2026.

- Laprdus sada radi na zaključanom zaslonu odmah nakon ponovnog pokretanja telefona, prije unosa PIN-a, uzorka ili lozinke. Korisnici TalkBacka mogu otključati svoj uređaj uz Laprdusov govor, koristeći odabrani glas, brzinu, visinu, stanke, čitanje brojeva i rječnike.
- Postavke i rječnici spremljeni u ranijim verzijama automatski se prenose pri prvom pokretanju aplikacije ili govorne jedinice nakon nadogradnje. Nije potrebna nikakva radnja.
- Ako se Laprdusovi glasovi uopće ne mogu učitati, govorna jedinica sada prijavljuje grešku Androidu umjesto da ostane bez glasa, pa sustav može pokušati s drugom govornom jedinicom kada je dostupna.
- Da bi govorio na zaključanom zaslonu, Laprdus mora biti postavljen kao preferirani mehanizam za pretvaranje teksta u govor u postavkama sustava.
- Ispravljen je problem zbog kojeg je razmak bio bez zvuka pri čitanju znak po znak (na primjer pri kretanju kroz tekst TalkBackom); sada se izgovara kao „razmak”.
- Laprdus više ne zapisuje izgovoreni tekst u sistemski zapisnik, gdje je prije mogao završiti u zapisima uređaja i izvješćima o pogreškama — uključujući znakove izgovorene pri unosu PIN-a na zaključanom zaslonu.

## Verzija 1.0.0 (build 10) — 27. kolovoza 2026.

- Nadograđene su unutarnje komponente aplikacije na najnovije verzije radi bolje pouzdanosti i kompatibilnosti s najnovijim izdanjima Androida.

## Verzija 1.0.0 (build 9) — 1. kolovoza 2026.

- Ispravljen je problem zbog kojeg se Laprdus prikazivao na popisu sustavskih govornih jedinica, ali nakon odabira ne bi govorio na nekim uređajima (prijavljeno na Honor telefonima s MagicOS-om 10).
- Opcija "Poslušaj primjer" u Androidovim postavkama pretvaranja teksta u govor sada radi.
- Laprdus više ne ostaje bez glasa kada jezik uređaja nije postavljen na hrvatski ili srpski — sada uvijek govori odabranim glasom, bez obzira na jezik sustava.
- U korisnički vodič dodani su savjeti za rješavanje problema na Honor i Huawei uređajima.

## Verzija 1.0.0 (build 8) — 10. ožujka 2026.

- Prvo javno izdanje.
