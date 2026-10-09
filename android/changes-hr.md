# Laprdus za Android — Što je novo

## Verzija 2.0.0 (build 12) — u razvoju

- Tri nova glasa koji se u cijelosti sintetiziraju po pravilima, bez snimaka: Zvonko (hrvatski), Stojan (srpski) i Mirsad (bosanski). Ostaju čisti i razumljivi i pri vrlo velikim brzinama govora. Zadani su glas svakog jezika; glas koji ste sami odabrali nikada se ne zamjenjuje. Josip, Vlado, Detence, Baba i Đedo i dalje su dostupni.
- Bosanski je sada podržan jezik, a Mirsad je njegov glas.
- Naglasak riječi i rečenična melodija: svaka je riječ naglašena na pravom slogu (ugrađeni rječnik s nekoliko tisuća riječi i imena, pravila za česte nastavke i naglasni znakovi koje možete upisati u rječnik izgovora, na primjer `telèfon`), a svaka rečenica dobiva prirodnu intonaciju: porast na kraju pitanja, pad na točki, zadržani porast na zarezu. Engleske riječi i imena naglašeni su na slogu na kojem ih naglašava engleski (*Croàtian*, *Ábleton*), a riječ s velikim slovima u sredini čita se kao njezini dijelovi (*ElevenLabs* kao *Eleven Labs*).
- Josip, Vlado, Detence, Baba i Đedo imaju novi govorni mehanizam: isti naglasak riječi i istu melodiju kao novi glasovi, prirodan tempo, čujne glasove p, t, k, b, d i g pri velikim brzinama i više nikakvih klikova ni pucketanja između glasova. Promjena brzine više ne mijenja visinu, promjena visine više ne čini glas hrapavim, a izvedeni glasovi zvuče kao dijete, baka i djed, a ne kao ubrzana ili usporena snimka.
- Deset pjevajućih glasova: Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač i Zvonko Bećarac (hrvatski), Stojan Trubač, Stojan Harmonikaš i Stojan Pevač (srpski), Mirsad Sevdalija, Mirsad Sazlija i Mirsad Solist (bosanski). Svaki tekst pjevaju na narodnu pjesmu svoje zemlje; brzina govora određuje tempo, visina transponira pjesmu, a razina infleksije vibrato.
- Dvije nove postavke za nove glasove, prikazane samo kad je jedan od njih odabran: Razina infleksije (0% je monoton govor, 50% prirodna melodija, 100% udvostručuje svaki pomak) i Ubrzanje (množitelj brzine govora od 0,5 do 3, pa klizač brzine doseže veću ili manju najveću brzinu). Novi glasovi prihvaćaju brzinu i visinu govora od četvrtine do četverostruke uobičajene vrijednosti.
- Brzina i visina govora podešene u Laprdusu sada rade zajedno s onima čitača ekrana ili aplikacije. One su normalna brzina i visina glasa, a TalkBack ili druga aplikacija svojom brzinom i visinom govor od njih ubrzava, usporava, podiže ili spušta: dok čitač ekrana govori svojom normalnom brzinom, Laprdus govori upravo brzinom koju ste podesili. Prije su se koristile samo uz uključen odgovarajući prekidač za prisilno korištenje, pa pomicanje klizača brzine nije mijenjalo ništa što TalkBack izgovara. Prekidači za prisilno korištenje sada znače da Laprdus zanemaruje brzinu ili visinu koju aplikacija traži.
- Naglasni rječnik: novi korisnički rječnik `accents.json` ispravlja naglasak riječi odjednom za sve njezine oblike ili cijelog glagola. Sprema se uz ostale korisničke rječnike i učitava zajedno s njima (vidi odjeljak 5.11 korisničkog priručnika).
- Brojevi se čitaju na jeziku glasa (Zvonko: *tisuća*, *milijun*; Stojan i Mirsad: *hiljada*, *milion*), a jedan i dva slažu se s brojevnom riječi (*dvije tisuće*, *dvadeset jedna tisuća*).
- Ispravljeno: unosi dodani u korisnički rječnik slovkanja i rječnik emodžija nisu imali učinka.
- Ispravljeno: postavka pauze za novi red nije imala učinka, a retci objave ili popisa stapali su se u jednu rečenicu. Prijelom retka sada završava frazu tom pauzom, a sljedeći redak počinje novu rečenicu (redak koji počinje s „S ponosom” više ne počinje slovom *es*).
- Laprdus sada sustavu nudi i bosanski. Kad aplikacija zatraži jezik kojim vaš odabrani glas već govori, taj se glas zadržava umjesto da se zamijeni.
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
