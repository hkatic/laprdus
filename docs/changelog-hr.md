# Laprdus — Povijest inačica

Ova datoteka navodi što se za korisnike promijenilo između objavljenih inačica Laprdusa, na svim platformama. Promjene učinjene tijekom razvoja jedne inačice (alfa, beta i RC izdanja) ne navode se zasebno: svaki unos opisuje objavljenu inačicu u cjelini.

## Inačica 2.0

### Sve platforme

- Tri nova glasa koji se u cijelosti sintetiziraju po pravilima, bez snimaka: Zvonko (hrvatski), Stojan (srpski) i Mirsad (bosanski). Ostaju čisti i razumljivi i pri vrlo velikim brzinama govora, jer se brzina i visina ne dobivaju naknadnom obradom snimljenog zvuka.
- Bosanski je sada podržan jezik, a Mirsad je njegov glas.
- Novi glasovi zadani su glas svojega jezika na svim platformama. Glas koji ste sami odabrali nikada se ne zamjenjuje. Josip, Vlado, Detence, Baba i Đedo i dalje su dostupni.
- Naglasak riječi: novi glasovi mjesto naglaska svake riječi određuju iz ugrađenog rječnika s nekoliko tisuća riječi i imena, prema pravilima za česte nastavke i vrste riječi (posuđenice i izvedenice s dugim zadnjim slogom osnove kao *telefóna, programéra, prodaváča, učeníka*; glagoli s prefiksom u svim oblicima, *poglèdala, napràvio*; glagoli na *-ovati*, imenice na *-ica* i *-anin*) te iz naglasnih znakova koje možete upisati u rječnik izgovora (na primjer `telèfon`). Stari glasovi nisu naglašavali ništa.
- Rečenična melodija: svaka rečenica dobiva prirodnu intonaciju, s pomakom na svakoj naglašenoj riječi, porastom na kraju pitanja (i iza upitne riječi, kao u „Kako si ti?”), padom na točki, zadržanim porastom na zarezu i širim rasponom kod uskličnika.
- Josip, Vlado i glasovi izvedeni iz njih imaju novi govorni mehanizam. Sada imaju isti naglasak riječi i istu rečeničnu melodiju kao novi glasovi, koriste naglasni rječnik i naglasne znakove iz rječnika izgovora, govore prirodnim tempom (nestali su Vladini razvučeni samoglasnici) i zadržavaju čujne praskove glasova p, t, k, b, d i g pri velikim brzinama. Nestali su klikovi i pucketanje između glasova.
- Promjena brzine govora Josipa i Vlade više ne mijenja visinu glasa, a promjena visine više ne čini glas hrapavim ili prigušenim. Detence, Baba i Đedo sada zvuče kao dijete, baka i djed, a ne kao ubrzana ili usporena snimka.
- Deset pjevajućih glasova: Zvonko Orguljaš, Klapa Zvonko, Zvonko Pjevač i Zvonko Bećarac (hrvatski), Stojan Trubač, Stojan Harmonikaš i Stojan Pevač (srpski), Mirsad Sevdalija, Mirsad Sazlija i Mirsad Solist (bosanski). Svaki tekst pjevaju na narodnu pjesmu svoje zemlje, slog po slog na jednu notu; brzina govora određuje tempo, visina transponira pjesmu, a razina infleksije dubinu vibrata.
- Dvije nove postavke za nove glasove, prikazane samo kad je jedan od njih odabran: Razina infleksije (0% je monoton govor, 50% prirodna melodija, 100% udvostručuje svaki pomak) i Ubrzanje (množitelj brzine govora od 0,5 do 3, pa klizač brzine može dosegnuti veću ili manju najveću brzinu).
- Novi glasovi prihvaćaju širi raspon brzine i visine govora: od četvrtine do četverostruke uobičajene vrijednosti.
- Naglasni rječnik: novi korisnički rječnik `accents.json` omogućuje ispravak naglaska riječi odjednom za sve njezine oblike ili cijelog glagola sa svim licima i vremenima. Sprema se uz ostale korisničke rječnike i učitava zajedno s njima. Vidi odjeljak 5.11 korisničkog priručnika.
- Brojevi se čitaju na jeziku glasa: Zvonko kaže *tisuća* i *milijun*, Stojan i Mirsad *hiljada* i *milion*. Jedan i dva sada se slažu s brojevnom riječi (*dvije tisuće*, *dvadeset jedna tisuća*, *dvije milijarde*).
- Ispravljeno: unosi dodani u korisnički rječnik slovkanja i rječnik emodžija nisu imali učinka; sada mijenjaju kako se znakovi i emodžiji čitaju.
- Ispravljeno: u naredbenoj liniji i pod Speech Dispatcherom Detence, Baba i Đedo zvučali su potpuno jednako kao Josip i Vlado.

### Windows (SAPI5 i NVDA)

- Novi glasovi, pjevajući glasovi, postavke Razina infleksije i Ubrzanje te naglasni rječnik dostupni su u SAPI5 programima, u Laprdus Konfiguratoru i u NVDA dodatku. NVDA dodatak kao zadani uzima novi glas NVDA-ova jezika i ponovno učitava naglasni rječnik kad se datoteka promijeni.
- Ispravljeno: kad je program od SAPI5 zatražio slovkanje samo dijela teksta, Laprdus je slovkao cijeli tekst, a zatražena tišina izgovarala se kao razmak.

### Linux

- Novi glasovi, pjevajući glasovi i naglasni rječnik dostupni su u alatu naredbene linije i u Speech Dispatcheru. Alat naredbene linije ima nove opcije `-I` (infleksija, 0-100) i `-a` (ubrzanje); pod Speech Dispatcherom razinu infleksije određuje postavka raspona visine.
- Speech Dispatcher sada nudi Laprdus i za bosanski, uz hrvatski i srpski; paketi dodaju tu postavku i pri nadogradnji.

### Android

- Novi glasovi i pjevajući glasovi dostupni su kao glasovi sustava, a postavke Razina infleksije i Ubrzanje nalaze se u aplikaciji.
- Laprdus sada sustavu nudi i bosanski. Kad aplikacija zatraži jezik kojim vaš odabrani glas već govori, taj se glas zadržava umjesto da se zamijeni.
- Ispravljeno: rječnik slovkanja i rječnik emodžija uređeni u aplikaciji govorna jedinica nije koristila; sva tri korisnička rječnika sada vrijede čim se glas učita. Naglasni rječnik čita se iz iste mape.

### iPhone, iPad i Mac

- Novi glasovi i pjevajući glasovi pojavljuju se među glasovima sustava, a postavke Razina infleksije i Ubrzanje nalaze se u aplikaciji. Naglasni rječnik čita se iz mape rječnika aplikacije.
- Ispravljeno: kad je VoiceOver čitao stavku koja sadrži jedan znak za slovkanje, na primjer gumb s brojem na znački, cijela se stavka slovkala slovo po slovo, a naziv i vrsta kontrole stapali su se u jednu rečenicu. Svaki se dio sada čita onako kako VoiceOver traži: tekst kao tekst, znak slovkano, stanke kao stanke.
- Sada se čuje VoiceOverova promjena visine za velika slova, zahtjev koji VoiceOver otkaže više se ne izgovara, a rječnik slovkanja i rječnik emodžija učitavaju se zajedno s rječnikom izgovora.
