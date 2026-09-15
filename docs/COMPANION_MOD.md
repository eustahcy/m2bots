# Mod „Towarzysz" (companion) — dokumentacja techniczna

Gracz wybiera **do 10** istniejących PlayerBotów na stałych towarzyszy. Bot nadal
jest normalną postacią prowadzoną przez system PlayerBot — ma swoje konto,
ekwipunek, skille i postęp — ale zamiast samodzielnie przemierzać świat zostaje
przy właścicielu i pomaga mu w grze.

Limit to `PLAYERBOT_COMPANION_MAX_PER_OWNER` w `playerbot_types.h`. Podnieść go
jest tanio w kodzie, ale nie za darmo w świecie: każdy towarzysz to pełna postać,
którą rdzeń trzyma wczytaną i tickuje.

Zaimplementowane na drzewie **2.0.34 / mt2009**, na bazie rozwiązania z
`companion-pack-2x` (paczka dla 2.0.14/r41023). Paczki **nie** nałożono —
jej README tego zabrania na nowszych źródłach, a różnice są opisane w sekcji
„Czym to się różni od companion-pack-2x".

## Jak używać

Trzy drogi: przycisk w oknie postaci, szept do bota i komenda.

### 1. Przez Szept (myszką, bez wpisywania komend)

Klikasz bota → **Szept** → jedno słowo:

| Napisz | Efekt |
| --- | --- |
| `kompan`, `towarzysz`, `companion`, `za mna`, `chodz ze mna` | ten bot dołącza do Twojej drużyny towarzyszy |
| `zwolnij`, `odejdz`, `idz sobie`, `stop`, `dismiss` | **ten** bot przestaje nim być (reszta zostaje) |

Bot odpowiada szeptem („Jasne, ide z toba! Prowadz."), a przy odmowie —
zdaniem z managera, np. że należy już do kogoś innego. Zwolnić może tylko bot,
który faktycznie jest Twój: szept „odejdz" do cudzego bota nie ruszy towarzysza
zostawionego gdzie indziej.

Obsługa siedzi w `HandlePlayerBotCompanionWhisper` i jest wołana **przed**
parserem handlu i przed limitem odpowiedzi — to komenda, nie pogawędka, więc
odpowiedź nie może zostać połknięta przez throttling.

### 2. Komendą (poziom `GM_PLAYER`, każdy ma dostęp)

| Wywołanie | Efekt |
| --- | --- |
| `/companion` | wypisuje całą drużynę w kolejności marszu |
| `/companion <nazwa>` | dodaje bota po nazwie postaci |
| `/companion <pid>` | to samo po ID — dla bota, który nie jest wczytany |
| `/companion clear <nazwa\|pid>` | zwalnia jednego |
| `/companion clear` | zwalnia wszystkich |

Obie drogi działają od razu: zapisują wiersz, a potem wołają tę samą ścieżkę co
logowanie (`OnOwnerEnteredGame`), więc bot zostaje przywołany albo zespawnowany
bez czekania na relog. Zwolnienie **nie** usuwa ani nie despawnuje postaci.

### 3. Przyciskiem „Kompan" w oknie postaci

Klikasz gracza → w oknie celu, obok „Zapr. Grupy", jest **Kompan**. Jeden
przycisk robi oba działania, bo klient nie ma skąd wiedzieć, który to przypadek:
czy cel w ogóle jest PlayerBotem i czyim jest towarzyszem, wie wyłącznie serwer.
Przycisk wysyła więc `/companion_toggle <nazwa>`, a serwer albo bierze bota, albo
zwalnia go, jeśli już jest Twój — i odpowiada zdaniem na czacie.

Przycisk pokazuje się przy każdym graczu **tego samego królestwa** (kompan
z innego i tak nie wszedłby do grupy). Przy zwykłym graczu kliknięcie zwróci „to
nie jest PlayerBot" — klient nie zna rejestru seeda, więc tę odpowiedź musi dać
serwer.

#### Jak to zbudowano i jak powtórzyć

`uiTarget.py` siedzi w zaszyfrowanym packu `Klient\pack\root.data`
(`root.index` ma magiczne `MCOZ`). W `Klient\pack\` są tylko
`PackMakerLite.json` z kluczami XTEA i `.bat`-y; samego narzędzia tam nie ma —
użyto `PackMakerLite-RS` (`packmakerlite-cli.exe`), uruchamianego z katalogu
`pack`, bo stamtąd czyta `PackMakerLite.json` z kluczami.

```powershell
cd "…\Klient\pack"
packmakerlite-cli --unpack root      # powstaje folder root\
#   … edycja root\uitarget.py …
packmakerlite-cli --pack root
packmakerlite-cli --verify root      # 237/237 OK
```

**Oryginalny pack leży w `Klient\pack\_backup-przed-kompanem\`** (`root.data`,
`root.index`). Przy jakimkolwiek problemie z klientem wystarczy skopiować te dwa
pliki z powrotem. Rozpakowany folder `Klient\pack\root\` zostaje jako kopia
robocza pod kolejne zmiany UI — gra go nie czyta, czyta `root.data`/`.index`.

Zmiany w `uitarget.py`: `"Kompan"` w `BUTTON_NAME_LIST`, `SAFE_SetEvent` na
`__OnCompanion`, sama metoda, i `__ShowButton("Kompan")` w `RefreshButton`.

## Panel WWW

Na stronie postaci (`/player/<pid>`) jest karta **Towarzysze**: lista w kolejności
marszu, „Zwolnij" przy każdym, „Zwolnij wszystkich" oraz pole do dodania po
nazwie albo id. Licznik pokazuje `użyte / 10`, a towarzysze od ósmego w górę mają
odznakę „poza grupą" (patrz limit `PARTY_MAX_MEMBER`).

Panel pisze do `common.playerbot_companion` **wprost**, a nie przez kolejkę
questa w grze — i może, bo wiersz *jest* relacją: rdzeń odczytuje tę tabelę co
minutę, więc zmiana z panelu trafia do działającego świata bez restartu i bez
obecności gracza.

Panel powtarza te same dwa sprawdzenia co rdzeń (czy PID jest zaseedowanym
PlayerBotem — `playerbot_seed_state` + prefiks konta `playerbot_`; i czy bot nie
należy już do kogoś innego), żeby administrator dostał odpowiedź od razu, zamiast
znajdować później linię w `syserr`. Rdzeń i tak waliduje każdy wiersz przy
odczycie — panel jest uprzejmością, nie granicą bezpieczeństwa.

Klucze tłumaczeń dodano dla `en` i `pl`; `de` i `tr` schodzą na angielski przez
zwykły fallback w `t()`. Obie kopie panelu (`files/admin_panel.py`
i `linux-port/docker/panel/app/admin_panel.py`) są zsynchronizowane.

## Granica bezpieczeństwa — najważniejsza rzecz w tym modzie

Towarzyszem może zostać **wyłącznie identyfikator, za który ręczy rejestr
seeda** (`m_setRegisteredBots`). To ten sam warunek, który chroni `Spawn()`:
zwykła postać gracza wczytana jako bot zawiesza jej właścicielowi kolejne
logowanie — rozdział „Cykl życia Playerbota i krytyczna granica bezpieczeństwa"
w `CLAUDE_HANDOFF_2026-09-01.md` opisuje ten incydent.

Sprawdzenie jest w dwóch miejscach, celowo zduplikowane:

- `SetCompanion()` — odmawia przy wyborze, ze zdaniem dla gracza;
- `LoadCompanionRegistry()` — odrzuca wiersz przy **każdym** odczycie tabeli,
  więc ręcznie dopisany rekord albo skurczony seed nic nie zmieniają.

Dzięki temu tabela towarzyszy nie jest obejściem `PLAYERBOT_AUTH`. Imperium
towarzysza nadal pochodzi z rejestru wewnątrz `Spawn()`, nigdy od wołającego.

## Pliki

Źródłem prawdy jest overlay; kopia w kontekście builda musi być bajtowo
identyczna (obie są w allowliście aktualizacji przez wzorzec `playerbot_*`).

| Plik | Zmiana |
| --- | --- |
| `linux-port/overlays/playerbot/src/game/src/playerbot_manager.h` | API towarzysza + stan rejestru |
| `.../playerbot_manager.cpp` | rejestr, haki właściciela, gałąź w `Update()` |
| `.../playerbot_combat.h` | `ManagePlayerBotCompanionOwnerBuffs` |
| `.../playerbot_types.h` | stałe `PLAYERBOT_COMPANION_*` |
| `.../playerbot_chat_trade.h` | wybór/zwolnienie towarzysza przez Szept |
| `linux-port/docker/game/src/server/game/src/playerbot_*` | kopie runtime (sync z overlayem) |
| `.../cmd.cpp`, `.../cmd_gm.cpp` | komendy `/companion` i `/companion_toggle` |
| `Klient/pack/root.data`, `root.index` | przycisk „Kompan" (przepakowane; kopia w `_backup-przed-kompanem/`) |
| `files/admin_panel.py` + `linux-port/docker/panel/app/admin_panel.py` | karta „Towarzysze" na stronie postaci |
| `.../input_login.cpp` | `OnOwnerEnteredGame` po `SetPhase(PHASE_GAME)` |
| `.../input_main.cpp` | `OnOwnerAttack` przed `ch->Attack(...)` |
| `.../exchange.cpp` | auto-akceptacja handlu po stronie bota |
| `.../char.cpp` | auto-akceptacja zaproszenia do grupy + wyjście ze starej PT |
| `.../messenger_manager.cpp` | auto-akceptacja zaproszenia do znajomych |
| `linux-port/docker/mariadb/playerbot/companion_schema.sql` | tabela (nowy plik) |
| `.../playerbot/apply.sh` | nakładanie schematu na każdym starcie |
| `launcher/server-update-files.mt2009.txt` | `exchange.cpp`, `messenger_manager.cpp`, `companion_schema.sql` |

## Formacja

Dziesięciu towarzyszy idących do „350 jednostek za właścicielem" to dziesięciu
botów stojących w sobie nawzajem. Dlatego każdy ma **slot** — swoje miejsce
w klinie za właścicielem:

- slot 0 idzie dokładnie z tyłu, kolejne rozchodzą się na przemian w prawo
  i w lewo co `PLAYERBOT_COMPANION_ARC_STEP` (24°),
- po pięciu (`PLAYERBOT_COMPANION_ROW_SIZE`) zaczyna się drugi szereg,
  `PLAYERBOT_COMPANION_ROW_STEP` (220) dalej.

Dzięki rozkładowi „0, +1, −1, +2, −2" pojedynczy towarzysz stoi tam, gdzie stał
przed wprowadzeniem wielu — a klin rozrasta się symetrycznie.

**Slot to kolejność wzięcia**, nie kolejność w bazie. Zapytanie sortuje po
`created_at, companion_pid`, a `SetCompanion` dopisuje na koniec — nowy
towarzysz nigdy nie przestawia tych, którzy już maszerują. Zwolnienie jednego
usuwa go z wektora, więc szeregi się zwierają i w klinie nie zostaje dziura.

## Grupa i znajomi

Bot nie ma klienta, więc **nie naciśnie żadnego okienka potwierdzenia**. To był
powód zgłoszenia „kompan odrzuca":

- **Grupa.** `CHARACTER::PartyInvite` wysyła `HEADER_GC_PARTY_INVITE` i zakłada
  zdarzenie z 10-sekundowym timeoutem. Bot nigdy nie odpowiadał, więc timeout
  odmawiał każdego zaproszenia. Teraz `PartyInvite` rozpoznaje własnego
  towarzysza i po utworzeniu zaproszenia woła `PartyInviteAccept` — czyli tę
  samą ścieżkę i te same `IsPartyJoinableMutableCondition`, co gracz klikający
  „Tak". Nic nie jest pomijane, tylko odpowiedź przychodzi od razu.
- **Stara grupa bota.** Bot wzięty z pola zwykle jest już w botowej PT, a to
  `PERR_ALREADYJOIN` — odmowa, na którą właściciel nic nie poradzi. Dlatego
  towarzysz najpierw wychodzi ze swojej poprzedniej grupy (`Quit`). Tamtej nie
  wybierał, tę owszem.
- **Znajomi.** `MessengerManager::RequestToAdd` wysyła `messenger_auth` do
  klienta celu. Dla własnego towarzysza wołane jest `AuthToAdd(cel, proszący,
  false)` — dokładnie to, co robi `do_messenger_auth` po kliknięciu „Tak".
  Kolejność argumentów jest istotna: klucz żądania buduje się z CRC nazw
  w kolejności `(companion=proszący, account=cel)`.

**Towarzysza nie wyrzuca już botowe AI.** `ManagePlayerBotParty` rotuje grupy co
5–15 minut, wypisuje bota spoza ~10% kohorty PT i rozwiązuje grupę przy różnicy
poziomów > 6 albo zbyt dużym dystansie. Siedzi w pętli `Update()` **za** gałęzią
towarzysza, która zawsze kończy tick `continue`, więc dla kompana nie wykona się
nigdy.

### Czego to nie obchodzi silnika

Dwie reguły zostają nienaruszone, bo są regułami gry, nie skutkiem braku klienta:

- **Królestwo.** `IsPartyJoinableCondition` odrzuca `PERR_DIFFEMPIRE` na wejściu.
  Towarzysz z innego królestwa będzie podążał i walczył, ale do grupy nie
  wejdzie. `SetCompanion` mówi o tym **przy wyborze**, zamiast zostawiać gracza
  z zaproszeniem, które po prostu nie działa.
- **Różnica poziomów ±30.** Zwykły limit party. Świadomie nieobchodzony —
  wpuszczenie kompana 105 lvl do grupy gracza 5 lvl zmieniłoby rozdział
  doświadczenia w całej grze. To decyzja balansowa, nie techniczna.
- **Rozmiar grupy.** `PARTY_MAX_MEMBER` = 8, czyli właściciel + 7. Przy limicie
  10 towarzyszy trzej ostatni **nie wejdą do grupy** — będą normalnie podążać
  i walczyć, ale zaproszenie dla nich zwróci silnikowe „grupa pełna".
  `SetCompanion` mówi o tym przy braniu ósmego i dalszych, zamiast zostawiać to
  jako odmowę wyglądającą na błąd.

## Baza

```sql
common.playerbot_companion(
    PRIMARY KEY (owner_pid, companion_pid),
    UNIQUE KEY (companion_pid),
    enabled, created_at)
```

Do dziesięciu wierszy na właściciela (liczbę pilnuje rdzeń, tabela ma je tylko
przechowywać). Klucz złożony pozwala jednemu graczowi trzymać kilka botów;
`companion_pid` zostaje UNIQUE samo w sobie, więc jeden bot nigdy nie będzie
towarzyszem dwóch osób. `created_at` nie jest ozdobą — to po nim idzie
sortowanie, a ta kolejność jest formacją.

Świadomie **bez** klucza obcego do `player.player` — te tabele są MyISAM, a więz,
który MyISAM przyjmuje i ignoruje, jest gorszy niż jego brak. Walidację robi
rdzeń, przy każdym odczycie.

### Migracja istniejących serwerów

Świat założony wersją jednokompanową ma `PRIMARY KEY (owner_pid)`, a
`CREATE TABLE IF NOT EXISTS` nic takiej tabeli nie robi — bez migracji baza
odrzucałaby drugiego towarzysza, podczas gdy rdzeń pozwalałby na dziesięciu.
Dlatego plik zawiera warunkowy `ALTER` (przez `information_schema` +
`PREPARE`/`EXECUTE`), który odpala się raz i przy kolejnych startach nic nie
robi. Istniejące wiersze zostają: stary klucz i tak gwarantował jeden wiersz na
właściciela, więc poszerzenie go nie może kolidować.

Schemat nakłada `apply.sh` kontem gry (`db < ...`), tak jak `log_schema.sql`
i inaczej niż schemat ItemShopu — konto gry ma już `common.*`, więc nie trzeba
hasła roota i działa to zarówno na świeżym, jak i istniejącym świecie. Błąd jest
ostrzeżeniem, nie fatalem: serwer wstaje, mod jest wyłączony, w logu jest linia
dlaczego.

## Jak działa pętla

Gałąź towarzysza w `CPlayerBotManager::Update()` stoi **przed** całą autonomiczną
logiką bota (cele, wędrówka, errandy, targ, gildie, podróż) i zawsze kończy tick
przez `continue`. To, co z normalnego życia bota nadal obowiązuje, wołają
w środku te same helpery co główny przebieg — dlatego towarzysz zostaje
PlayerBotem, a nie petem:

1. `HandleDeath` — śmierć i odrodzenie normalną ścieżką;
2. `PersistPlayerBot` — zapis postępu;
3. `ClosePlayerBotShop` — jeśli bot trzymał stragan, zamyka go;
4. `ManagePlayerBotSkills` + `PrepareWeapon` — rozwój i broń pod profesję;
5. buffy właściciela (Szaman) — leczenie, gdy właścicielowi spadnie HP,
   inaczej brakujący buff z jego builda;
6. `HandleLoot` — podnoszenie przedmiotów;
7. przywołanie, gdy zmieniła się mapa albo dystans > `RECALL_RANGE`;
8. walka: cel właściciela → cel już związany walką → dobicie paczki;
9. podążanie ~350 jednostek za właścicielem.

Gałąź chodzi w **każdym** ticku, nie co drugi: stagger istnieje dla trzystu
botów robiących skany celów naraz, a towarzyszy są pojedyncze sztuki —
follower reagujący sekundę za późno wygląda na zepsutego.

### Wybór celu

`OnOwnerAttack` zapisuje VID tego, w co właściciel uderzył, i czas. Towarzysz
bierze ten cel przez `PLAYERBOT_COMPANION_ASSIST_WINDOW` (10 s), potem wpis
wygasa — VID zostaje przydzielony czemuś, co się później zrodzi. Gdy celu nie
ma, bierze to, co już z nim (albo z PT) walczy, a dopiero na końcu dociąga
sąsiedniego moba — ograniczonego do `PLAYERBOT_LOCAL_CHAIN_RANGE` **jednocześnie
od towarzysza i od właściciela**, żeby „pomoc z paczką" nie zamieniła się
w samodzielne polowanie.

## Czym to się różni od `companion-pack-2x`

Portując, poprawiłem trzy rzeczy i jedną usunąłem.

**1. Przywołanie sprawdza, czy rdzeń hostuje mapę.** Paczka wołała
`Show(owner->GetMapIndex(), ...)` bez sprawdzenia. Jedna mapa żyje na jednym
rdzeniu i postać nie przechodzi między nimi (bot nie ma klienta, któremu można
kazać się przelogować) — `playerbot_travel.h` opisuje to jako kształt, na którym
ten plik już raz się przejechał: dziesięć tysięcy odrzuconych warpów na minutę.
`RecallPlayerBotCompanion` pyta `IsPlayerBotMapHostedHere` i przy odmowie loguje
raz, przez `PlayerBotErrThrottled`.

**2. Rejestr odświeża się co minutę, nie raz na proces.** Ten świat ma trzy
rdzenie. `/companion` obsługuje ten rdzeń, na którym stoi gracz; rdzeń hostujący
mapę, na którą gracz zaraz wejdzie, o wyborze nigdy by się nie dowiedział przed
restartem — towarzysz chodziłby za graczem po jednej części świata i znikał na
każdej granicy. Kadencja jak w rejestrze banów (`PLAYERBOT_TOPUP_INTERVAL`),
mapy podmieniane dopiero po udanym zapytaniu, log tylko przy realnej zmianie.

**3. Towarzysz trafia do `m_setScheduledBots`.** `TopUpMissingBots` przywraca
to, co zaplanowane i nieobecne, więc towarzysz, który zgubi deskryptor, wraca
sam — zamiast czekać, aż właściciel się przeloguje.

**4. Usunięty hardcode PID 2505 (Mija).** Paczka na stałe wymuszała grupę
umiejętności 1 i `ClearSkill()` dla jednej konkretnej postaci; jej własne README
nazywa to buildem pod użytkownika, nie konfiguracją. Tutaj towarzysz używa
zwykłego `ManagePlayerBotSkills` i `PrepareWeapon`, więc działa dla każdej
profesji bez wpisywania numerów.

Poza tym paczka pozwalała, by wiersz w tabeli towarzyszy przepuścił **dowolny**
PID przez `IsRegistered()` — u nas nie, patrz „Granica bezpieczeństwa".

## Weryfikacja

- `docker compose build game` — **PASS**, exit 0. Etapy `deps`/`libs` z cache,
  przekompilował się `builder`.
- **Schemat przetestowany na jednorazowej MariaDB 11.8** (kontener `--rm`, bez
  dotykania bazy serwera), sześć scenariuszy, wszystkie zielone: migracja starej
  tabeli 1→2 kolumny klucza z zachowaniem wiersza; drugi przebieg jako no-op
  (idempotencja); czterech towarzyszy jednego właściciela; odmowa przy próbie
  przypisania bota drugiemu właścicielowi (`ERROR 1062` na `companion_pid`);
  stabilna kolejność slotów po `created_at, companion_pid`; świeża instalacja od
  razu z kluczem złożonym.
- Sprawdzone w binarce nowego obrazu (`metin2/game:mt2009`), że wszystkie trzy
  zmienione obszary faktycznie się skompilowały: `PLAYERBOT_COMPANION: owner buff`
  (nagłówek walki), `companion could not be saved` (manager), `is now your
  companion` (komenda).
- Overlay i kopia w kontekście builda — bajtowo identyczne (SHA-256 wszystkich
  plików `playerbot_*`).
- Końce linii zachowane: overlay LF, pliki silnika CRLF, `apply.sh` LF, żadnego
  BOM-u (paczka 2.x dodawała BOM-y — tutaj nie).

### Stan po dodaniu przycisku i panelu (do dokończenia)

- Klient: **przepakowany i zweryfikowany** (`--verify root` → 237/237 CRC OK,
  przycisk obecny w spakowanym `uitarget.py`). Kopia oryginału zachowana.
- Panel: obie kopie kompilują się (`py_compile`), karta przechodzi parsowanie
  Jinja i render z danymi testowymi — granica „poza grupą" sprawdzona dla 1, 7,
  8 i 10 towarzyszy (odznaka pojawia się dokładnie od ósmego).
- **Obrazy `game` i `panel` NIE zostały przebudowane po tej turze.** Docker
  Desktop zatrzymał się w trakcie budowy (brak procesów, usługi i działających
  dystrybucji WSL). `/companion_toggle` i karta panelu wymagają przebudowy:
  `docker compose build game panel`, potem `docker compose up -d game panel`.

**Nie wykonano: testu w grze.** Wolumenów nie wolno ruszać; `down -v` jest
zakazane.

### Co przetestować w grze

Przycisk „Kompan" w oknie postaci: bierze bota, a drugie kliknięcie go zwalnia;
przy zwykłym graczu ma odpowiedzieć „to nie jest PlayerBot"; nie pokazuje się
przy graczu z innego królestwa. Karta „Towarzysze" w panelu: dodanie po nazwie
i po id, zwolnienie jednego i wszystkich, odmowa przy niebocie i przy cudzym
bocie, oraz to, że zmiana z panelu wchodzi w życie w grze **bez restartu**
(do minuty).

Dalej: wzięcie kilku towarzyszy pod rząd (sprawdzić klin — nie mogą stać w sobie),
odmowę przy jedenastym, `/companion` jako listę, zwolnienie jednego z kilku
(szept „odejdz" do jednego nie może rozpuścić reszty), zwarcie szeregów po
zwolnieniu ze środka, zachowanie kolejności slotów po restarcie serwera.

Dalej: wybór przez Szept („kompan") i przez `/companion`, spawn i podążanie, zmiana
mapy, oddalenie się ponad `RECALL_RANGE`, wspólna walka z paczką, handel
z towarzyszem, **zaproszenie do grupy** (także gdy bot był w botowej PT —
powinien z niej wyjść i wejść do Twojej), **dodanie do znajomych**, sprawdzenie
że grupa się utrzymuje dłużej niż 15 minut (rotacja botowych PT nie może jej
rozbić), śmierć i powrót, podnoszenie łupu, zwolnienie
(szept „zwolnij" oraz `/companion clear` — bot musi wrócić do normalnego AI),
szept „odejdz" do **cudzego** bota (nie może zwolnić Twojego towarzysza), oraz —
przy towarzyszu Szamanie — buffy i leczenie rzucane na właściciela.

## Uwaga o `server-update-files.mt2009.txt`

Nagłówek tego pliku mówi, że renderuje go `linux-port-mt2009/port/listify.py`,
którego w tej paczce nie ma. Dopisałem dwie pozycje ręcznie
(`exchange.cpp`, `companion_schema.sql`). Przy następnym renderowaniu listy
w repozytorium źródłowym trzeba dodać je również do generatora, inaczej znikną,
a aktualizacja dowiozłaby rdzeń z modem bez tabeli, której on potrzebuje.
