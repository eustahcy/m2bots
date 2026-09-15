# Tuike

Panel administracyjny i obserwacyjny dla serwera Metin2 z Playerbots. Działa
w osobnym kontenerze na porcie **7799**, obok panelu klasycznego (7788)
i Seban Panelu (7790). Wszystkie trzy mogą chodzić jednocześnie.

Tuike powstał jako przebudowa Seban Panelu: ta sama praca, napisana od nowa
jako pakiet Pythona zamiast jednego pliku, z własnym interfejsem i własnymi
tabelami. Nie wymaga migracji i nie rusza danych żadnego innego panelu.

## Co potrafi

- **Mapa świata na żywo** — pozycje botów prosto z rdzenia, odświeżane co 1,5 s,
  z filtrami poziomu, wyszukiwarką, widokiem grup, rankingiem na mapie i
  oznaczeniami botów zawieszonych oraz walczących z Metinem.
- **Mapy cieplne** — zgony, rozbite Metiny i zabici bossowie z ostatnich 24 h,
  naniesieni na faktyczne współrzędne z logu gry.
- **Postacie** — profil z ekwipunkiem, magazynem konta, tooltipami w stylu gry
  (bonusy, kamienie duszy), umiejętnościami ze stopniami M/G/P, paskami PŻ, PM
  i doświadczenia oraz historią zdarzeń.
- **Rankingi** — trzynaście zestawień: poziom, broń, zbroja, broń 30 lv (po
  średnich obrażeniach, obrażeniach umiejętności albo ulepszeniu), przedmiot +9,
  Yang, przedmioty, koń, Biolog, umiejętności, otwarte stragany, czas gry, bossy.
- **Gospodarka** — stan wszystkich przedmiotów w świecie z historią oraz wykres
  Yang w obiegu.
- **Sezon** — ranking tygodnia i rekordy świata z ostatnich 7 dni.
- **Wydajność** — CPU, RAM i dysk hosta z historią doby.
- **Zarządzanie** — mnożniki, respawny map, zachowanie botów na żywo, restart
  z potwierdzeniem i odizolowany aktualizator.
- **Masowe nadawanie przedmiotów** — po VNUM, ilości i warunkach (poziom, klasa,
  koń, jeździectwo, czas gry), z podglądem odbiorców i historią wyników.
- **Konta i GM** — tworzenie kont oraz postaci game mastera razem z wpisem
  w `common.gmlist`.
- **Wygląd** — cztery motywy (Midnight, Ember, Forest, Dawn), dwa zagęszczenia
  widoku i pełna obsługa telefonu.

## Uruchomienie w tym stacku

Usługi są już w `docker-compose.yml`. Wystarczy:

```bash
cd linux-port/docker
docker compose up -d --build tuike tuike-collector tuike-grants
```

Panel czeka pod `http://adres-serwera:7799`.

Trzy kontenery, każdy z własnym zadaniem:

| Usługa | Rola |
| --- | --- |
| `tuike` | strony i API |
| `tuike-collector` | migawki obciążenia, map, pozycji botów i gospodarki |
| `tuike-grants` | przekazuje zlecone nadania do kolejki gry |

Port zmienia się przez `M2_TUIKE_PANEL_PORT` w `.env`; pozostałe zmienne opisuje
`.env.example` w tym samym katalogu.

## Uruchomienie obok cudzego serwera

Gdy Tuike ma stać osobno, skopiuj `tuike.env.example` do `tuike.env`, uzupełnij
dane bazy i nazwy wolumenów, po czym podłącz kontener do tej samej sieci Dockera,
co MariaDB i gra.

Panel potrzebuje:

- konta bazy z dostępem do `player`, `account`, `common` i `log`, z prawem do
  utworzenia tabel `player.web_tuike_*` oraz `player.web_admin_queue`;
- wolumenu z `/opt/metin2/var` (tylko do odczytu — pliki `playerbot_status.tsv`);
- wolumenu kolejki mnożników i restartu (u nas `rates-spool`);
- wolumenu `update-spool`, jeśli przycisk aktualizacji ma być czynny.

## Dane w bazie

Tuike trzyma swoje dane pod prefiksem `web_tuike_`:

| Tabela | Zawartość |
| --- | --- |
| `player.web_tuike_settings` | ustawienia panelu i skrót hasła |
| `player.web_tuike_system_snapshot` | historia CPU, RAM i dysku |
| `player.web_tuike_map_snapshot` | historia zaludnienia map |
| `player.web_tuike_bot_position_snapshot` | pozycje botów (wykrywanie zawieszeń) |
| `player.web_tuike_item_snapshot` | stan przedmiotów w świecie |
| `player.web_tuike_metric_snapshot` | Yang w obiegu |
| `player.web_tuike_grants` | zlecone nadania i ich wyniki |

Jedyna tabela dzielona z innymi panelami to `player.web_admin_queue` — kolejka
samej gry, do której dopisuje każdy panel, a opróżnia ją quest `web_admin`.

## Jak panel rozmawia z grą

Tuike nigdy nie tworzy przedmiotów i nie ma dostępu do socketu Dockera ani do
portu administracyjnego gry (ten przyjmuje też `SHUTDOWN`). Rozmawia przez dwa
katalogi z małymi plikami `klucz=wartość`:

- `/opt/m2spool` — mnożniki, respawny, restart i wagi zachowania botów. Panel
  zapisuje zlecenie, helper w kontenerze gry je wykonuje i odpisuje status.
- `/opt/m2update` — aktualizator. Panel zostawia zlecenie o stałej treści;
  osobny kontener `updater` je podejmuje.

Każde zlecenie jest publikowane atomowo i oznaczone prefiksem `tuike-`, więc
w logu helpera widać, który panel o nie poprosił.

Przedmioty tworzy wyłącznie gra: panel zapisuje, komu się co należy, worker
podaje nazwy do kolejki po kilka naraz, a odpowiedź questa wraca tą samą drogą.
Dzięki temu przedmiot powstaje z regułami silnika, a zlecenie przeżywa restart
panelu.

### Czego nie zrobi bez integracji gry

Zmiana czasów respawnu wymaga helpera `m2-server-settings` w kontenerze gry
(pliki w `../seban-panel/integration/`). Bez niego panel mówi o tym wprost
i odmawia zlecenia zamiast wystawiać je w nieskończoność. Restart serwera
i zmiana mnożników działają zawsze — gra od zawsze obserwuje kolejkę mnożników.

Masowe nadania wymagają aktywnego `web_admin.quest` (kopia leży w tym katalogu).
Bez niego panel przyjmie zlecenie, ale gra go nie wykona; strona nadawania mówi,
czy worker żyje, czy kolejka gry jest odbierana i jak stare są oczekujące wpisy.

## Bezpieczeństwo

- Włącz ochronę hasłem w Zarządzaniu, jeśli panel jest dostępny spoza tej maszyny.
  Hasło jest przechowywane jako skrót; wyłączenie ochrony ten skrót usuwa.
- Przycisk aktualizacji działa **tylko** przy włączonej ochronie hasłem.
- Każdy formularz zmieniający stan serwera ma token CSRF.
- Wystawiaj port 7799 przez reverse proxy z HTTPS, gdy panel ma być w Internecie.
- Nie wystawiaj MariaDB publicznie.
- Przed pierwszym masowym nadaniem przetestuj je na jednej postaci.

Jedyne połączenie wychodzące, jakie panel nawiązuje sam z siebie, to zapytanie
do GitHuba o najnowsze wydanie Playerbots, raz na kwadrans. `TUIKE_UPDATE_CHECK=0`
wyłącza je całkowicie; nic więcej się nie zmienia.

## Układ kodu

```
tuike/
├── wsgi.py                 gunicorn wsgi:app
├── collector.py            wejście kolektora
├── grants_worker.py        wejście workera nadań
└── tuike/
    ├── config.py           wszystko, co panel czyta ze środowiska
    ├── db.py               połączenie i trzy pomocniki zapytań
    ├── schema.py           tabele web_tuike_* i wartości domyślne
    ├── engine.py           różnice między mt2009 a r40250, w jednym miejscu
    ├── settings.py         ustawienia panelu i walidacja formularza
    ├── security.py         bramka hasła i CSRF
    ├── text.py             dekodowanie CP1250 i formatowanie liczb
    ├── live.py             odczyt playerbot_status.tsv
    ├── spool.py            kolejka plikowa do kontenera gry
    ├── updater.py          kolejka aktualizatora i sprawdzanie wydań
    ├── grants.py           logika nadań, wspólna dla strony i workera
    ├── gamedata/           stałe z nagłówków, questów i plików klienta
    ├── queries/            wszystkie zapytania SQL, pogrupowane tematycznie
    ├── views/              blueprinty Flaska, jeden na obszar panelu
    ├── templates/          szablony Jinja
    └── static/             jeden arkusz stylów, moduły JS i grafika gry
```

Zasada podziału: `gamedata/` i `queries/` nie importują Flaska, więc workery
korzystają z tych samych zapytań co strony. Kolory żyją wyłącznie jako tokeny
CSS w `static/css/tuike.css` — wykresy czytają je stamtąd, zamiast nieść drugą
paletę. Lista map, ich tła i proporcje są tylko w `gamedata/maps.py`; szablon
wstawia je w `<option>`, a JavaScript stamtąd odczytuje, więc nie ma drugiej
listy, która mogłaby się rozjechać.

## Rozwój lokalny

```bash
cd linux-port/docker/tuike
python -m venv .venv && . .venv/bin/activate
pip install -r requirements.txt
DB_USER=metin2 DB_PASSWORD=... DB_HOST=127.0.0.1 python wsgi.py
```

Panel wystartuje na 7799 z włączonym debugiem Flaska.
