# Laprdus za iPhone, iPad i Mac — Što je novo

## Verzija 2.0.0 (build 10) — u razvoju

- Tri nova glasa koji se u cijelosti sintetiziraju po pravilima, bez snimaka: Zvonko (hrvatski), Stojan (srpski) i Mirsad (bosanski). Ostaju čisti i razumljivi i pri vrlo velikim brzinama govora. Zadani su glas svakog jezika; glas koji ste sami odabrali nikada se ne zamjenjuje. Josip, Vlado, Detence, Baba i Đedo i dalje su dostupni.
- Bosanski je sada podržan jezik, a Mirsad je njegov glas.
- Naglasak riječi i rečenična melodija: svaka je riječ naglašena na pravom slogu (ugrađeni rječnik s nekoliko tisuća riječi i imena, pravila za česte nastavke i naglasni znakovi koje možete upisati u rječnik izgovora, na primjer `telèfon`), a svaka rečenica dobiva prirodnu intonaciju: porast na kraju pitanja, pad na točki, zadržani porast na zarezu. Engleske riječi i imena naglašeni su na slogu na kojem ih naglašava engleski (*Croàtian*, *Ábleton*), a riječ s velikim slovima u sredini čita se kao njezini dijelovi (*ElevenLabs* kao *Eleven Labs*).
- Josip, Vlado, Detence, Baba i Đedo imaju novi govorni mehanizam: isti naglasak riječi i istu melodiju kao novi glasovi, prirodan tempo, čujne glasove p, t, k, b, d i g pri velikim brzinama i više nikakvih klikova ni pucketanja između glasova. Promjena brzine više ne mijenja visinu, promjena visine više ne čini glas hrapavim, a izvedeni glasovi zvuče kao dijete, baka i djed, a ne kao ubrzana ili usporena snimka.
- Deset pjevajućih glasova: Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač i Zvonko Bećarac (hrvatski), Stojan Trubač, Stojan Harmonikaš i Stojan Pevač (srpski), Mirsad Sevdalija, Mirsad Sazlija i Mirsad Solist (bosanski). Svaki tekst pjevaju na narodnu pjesmu svoje zemlje; brzina govora određuje tempo, visina transponira pjesmu, a razina infleksije vibrato.
- Dvije nove postavke za nove glasove, prikazane samo kad je jedan od njih odabran: Razina infleksije (0% je monoton govor, 50% prirodna melodija, 100% udvostručuje svaki pomak) i Ubrzanje (množitelj brzine govora od 0,5 do 3, pa klizač brzine doseže veću ili manju najveću brzinu). Novi glasovi prihvaćaju brzinu i visinu govora od četvrtine do četverostruke uobičajene vrijednosti.
- Brzina i visina govora podešene u Laprdusu sada rade zajedno s onima čitača ekrana ili aplikacije. One su normalna brzina i visina glasa, a VoiceOver ili druga aplikacija svojom brzinom i visinom govor od njih ubrzava, usporava, podiže ili spušta: dok čitač ekrana govori svojom normalnom brzinom, Laprdus govori upravo brzinom koju ste podesili. Prije su se koristile samo uz uključen odgovarajući prekidač za prisilno korištenje, pa pomicanje klizača brzine nije mijenjalo ništa što VoiceOver izgovara. Prekidači za prisilno korištenje sada znače da Laprdus zanemaruje brzinu ili visinu koju aplikacija traži.
- Naglasni rječnik: novi korisnički rječnik `accents.json` ispravlja naglasak riječi odjednom za sve njezine oblike ili cijelog glagola. Sprema se uz ostale korisničke rječnike i učitava zajedno s njima (vidi odjeljak 5.11 korisničkog priručnika).
- Brojevi se čitaju na jeziku glasa (Zvonko: *tisuća*, *milijun*; Stojan i Mirsad: *hiljada*, *milion*), a jedan i dva slažu se s brojevnom riječi (*dvije tisuće*, *dvadeset jedna tisuća*).
- Ispravljeno: unosi dodani u korisnički rječnik slovkanja i rječnik emodžija nisu imali učinka.
- Ispravljeno: postavka pauze za novi red nije imala učinka, a retci objave ili popisa stapali su se u jednu rečenicu. Prijelom retka sada završava frazu tom pauzom, a sljedeći redak počinje novu rečenicu (redak koji počinje s „S ponosom” više ne počinje slovom *es*).
- Naglasni rječnik čita se iz mape rječnika aplikacije, zajedno s ostalim korisničkim rječnicima.
- Ispravljeno: kad je VoiceOver čitao stavku koja sadrži jedan znak za slovkanje, na primjer gumb s brojem na znački, cijela se stavka slovkala slovo po slovo, a naziv i vrsta kontrole stapali su se u jednu rečenicu. Svaki se dio sada čita onako kako VoiceOver traži: tekst kao tekst, znak slovkano, stanke kao stanke.
- Sada se čuje VoiceOverova promjena visine za velika slova, a zahtjev koji VoiceOver otkaže više se ne izgovara.

## Verzija 1.0.0 (build 9) — 28. kolovoza 2026.

- Aplikacija sada ima standardni raspored s karticama: Glavno, Postavke, Rječnici i O aplikaciji, pa je sve lakše dostupno.
- Kartica Glavno sadrži ogledni tekst i gumb za reprodukciju.
- Rječnici su dobili vlastitu karticu, s uobičajenom alatnom trakom za uređivanje, potezima za brisanje i gumbom + za dodavanje novih unosa. Dodavanje i uređivanje unosa sada se otvara u standardnom obrascu s gumbima Odustani i Spremi.
- Uklonjen je gumb za otvaranje postavki govora sustava.
- Uklonjena je postavka „Prisilno koristi jezik”. Na iPhoneu, iPadu i Macu sustav uvijek traži određeni glas, pa je ta postavka samo poništavala vaš izbor; Laprdus sada uvijek govori glasom koji je zatražila aplikacija ili VoiceOver.
- Postavke su sada grupirane u odjeljke Glas, Govor, Nadjačavanje postavki aplikacije, Napredno, Pauze prilikom čitanja i Rječnici, pa čitač zaslona može skočiti izravno na željenu skupinu.
- Kretanje VoiceOverom kroz postavke znatno je kraće: svaki klizač i prekidač sada je jedno mjesto zaustavljanja. Brzina, visina i glasnoća govora te klizači pauza izgovaraju svoj naziv i vrijednost samo jednom, umjesto da su bila potrebna tri prelaska prstom do same kontrole.
- Klizači se sada pomiču u koracima koji odgovaraju izgovorenoj vrijednosti, pa je vrijednost koju čujete točno ona koja se sprema.
- Objašnjenja ispod prekidača sada se izgovaraju kao VoiceOver savjeti umjesto da se čitaju prije samog prekidača.
- Prilikom uređivanja unosa u rječniku, naziv svakog polja sada je stalno vidljiv iznad njega. Ranije su nazivi postojali samo kao rezervirani tekst, pa su se kod uređivanja unosa vidjela dva neoznačena okvira.
- Unosi u rječniku sada s desne strane imaju strelicu, pa je jasno da se dodirom otvaraju za uređivanje.
- Cijela je aplikacija provjerena pri najvećim veličinama teksta za pristupačnost i u tamnom načinu rada. Nazivi postavki i njihove vrijednosti sada se pri velikom tekstu slažu jedno ispod drugoga umjesto da se stišću jedno uz drugo, a opisi prekidača više se ne odsijecaju.
- Na Macu klizači postavki i polja rječnika sada stoje uz svoj naziv u jednom retku, onako kako Mac postavke inače izgledaju. Ranije su bili gurnuti uz desni rub, daleko od naziva kojemu pripadaju, a tekst upisan u polje bio je priljubljen uz njegov desni rub.
- Na Macu u tamnom načinu rada okvir za ogledni tekst bio je neprimjetan na pozadini prozora. Sada ima vidljiv obrub.
- Aplikacija sada radi na iPhone i iPad uređajima s iOS-om 16 ili novijim te na Mac računalima s macOS-om 13 Ventura ili novijim.
- U popisu glasova sustava Laprdusovi glasovi sada su svrstani pod „Laprdus” umjesto pod ime autora.
- Glasovi djeteta, bake i djeda sada se posvuda zovu Detence, Baba i Đedo, jednako kao na ostalim platformama.
- Unosi izgovora koje dodate ili promijenite u aplikaciji sada odmah vrijede i za glasove sustava. Ranije su VoiceOver i Izgovoreni sadržaj zadržavali stari izgovor sve dok ne biste promijenili glas.
- Ispravljena je pojava da su se brzina i visina govora same mijenjale pri čitanju teksta koji sadrži oznake, primjerice izvornog koda web stranice.
- Zaslon O aplikaciji sada upozorava ako aplikacija i glasovi sustava prestanu dijeliti postavke i rječnike, umjesto da to prođe nezapaženo.
- Prerađen je način na koji se sintetizirani zvuk predaje sustavu, čime je uklonjen rijedak uzrok isprekidanog zvuka.
