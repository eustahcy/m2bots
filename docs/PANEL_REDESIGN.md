# Przeprojektowanie panelu WWW (pulpit + nawigacja)

Dotyczy `files/admin_panel.py` i jego kopii `linux-port/docker/panel/app/admin_panel.py`
(obie bajtowo zgodne). Zakres: nowy `BASE` (styl + powłoka z nawigacją) oraz
`TPL_DASH` przepisany z menu na pulpit. Pozostałe 15 szablonów **nie zostało
tknięte** — dziedziczą nową powłokę bez zmian w treści.

Kopia sprzed zmian: `files/admin_panel.py.bak-redesign` (nie jest na allowliście
aktualizacji, więc nie trafi do paczki — to tylko siatka bezpieczeństwa).

## Co się zmieniło

1. **Pulpit przestał być menu.** Było dziewięć kart „nagłówek + zdanie +
   przycisk gdzie indziej". Teraz: wstążka stanu → cztery liczby → siatka
   (tabela graczy | szyna z „Wymaga ręki", aktywnością botów i skrótami) →
   sekcja instalacyjna. Odnośniki do funkcji poszły do nawigacji.
2. **Jest nawigacja.** Sześć pozycji, tłumaczonych, z zaznaczoną bieżącą.
   Pokazuje się tylko przy `is_admin` — strona frontowa należy do graczy.
3. **Naprawiony błąd i18n.** Jedyny dotychczasowy skrót w nagłówku miał tekst
   `{{'Mapa na żywo' if curlang == 'pl' else 'Live map'}}` wpisany na sztywno,
   więc `de` i `tr` dostawały angielski mimo pełnego tłumaczenia panelu.
4. **Szerokość idzie za treścią.** `.wrap` to 820 px, `.wrap.wide` 1200 px —
   nakładane w `BASE` po `request.endpoint` dla `dash`, `live_map` i `season`.
5. **Rozdzielone znaczenia koloru.** Złoto = marka i interakcja. Stan dostał
   własny zestaw (`--green`, `--amber`, `--red`), więc „Wymaga ręki" wyróżnia
   się bez udawania nagłówka.

## Ograniczenia, których trzeba pilnować

**Nazwy tokenów CSS są kontraktem.** `--bg --card --card2 --line --txt --muted
--gold --gold2 --green --red --glow` — wszystkie jedenaście zostało z nazwami,
bo mapa na żywo, rankingi i okna ekwipunku mają własne arkusze czytające
`var(--gold)`, `var(--line)` itd. Zmiana nazwy któregokolwiek rozwala strony,
których ten plik nie dotyka. Nowe tokeny dołożono obok (`--card3`, `--ink2`,
`--faint`, `--amber`, `--cat-1..3`, `--cat-rest`, `--r`, `--r-sm`).

**Klasy z `BASE` też są kontraktem.** Pozostałe szablony używają `card`, `btn`,
`big`, `btn-sm`, `badge`, `help`, `muted`, `dot`, `on`, `steps`, `steps3`, `s3`,
`s3bar`, `now`, `done`, `playways`, `playcard`, `play`, `glow`, `onboard`,
`orsep`, `hintline`, `ok-t`, `bad-t`, `about`, `md`, `cmd`, `sect`, `sect-hint`,
`row`, `flash`, `err`, `wrap`. Wszystkie przestylowane, żadnej nie usunięto.

**Barwy danych są zwalidowane, nie dobrane na oko.** `--cat-1/2/3` przeszły
walidator kontrastu i daltonizmu (`dataviz/scripts/validate_palette.js`) na tle
`--card`, we **wszystkich** parach, w obu trybach. Pierwsza dobrana ręcznie
paleta oblała cztery testy z pięciu. Przy dokładaniu czwartej kategorii trzeba
walidator uruchomić ponownie — cztery barwy z tego zestawu **nie** przechodzą
`--pairs all`, dlatego kubełki są trzy plus neutralna „reszta".

## Rozbudowa: strona „Świat" (`/world`)

Wszystko na niej czyta tabele, **które serwer już zapisuje, a panel nigdy nie
czytał**: `log.levellog`, `log.refinelog`, `log.loginlog`, plus `player.player`.
Sekcje: skuteczność ulepszeń wg stopnia, rozkład poziomów, logowania wg godziny,
populacja map, gospodarka (yang w obiegu + najbogatsi) i ostatnie awanse.
Na pulpit doszedł mały kanał „Właśnie w świecie" z awansami.

**Dwie pułapki w danych, na które trzeba uważać:**

1. **`refinelog.is_success` nie jest logiczne.** Ma wartości `-1`, `0` i `1`
   (zejście w dół, zwykła porażka, sukces). `AVG(is_success)` daje bzdurę —
   na tym świecie wychodziło „−31% sukcesu" dla +8. Liczy się wyłącznie
   `SUM(is_success=1)/COUNT(*)`; poprawna krzywa to 90% → 60% → 33% → 12%.
2. **`levellog.name` jest puste** na tym buildzie. Nazwę trzeba brać joinem
   po `pid` do `player.player`.

**Koszt zmierzony na żywej bazie** (2507 postaci, 198 tys. wierszy ulepszeń
z doby): `world_stats()` 161 ms na zimno i 0 ms z cache (TTL 60 s),
`dash_overview()` 80 ms, `recent_levelups()` 32 ms.

**Bez bibliotek z CDN.** Wykresy to zwykły HTML i CSS (`.bars`, `.bar`), bo ten
panel musi działać na maszynie bez internetu. Jedyna zewnętrzna zależność to
Google Fonts w `BASE` — ma pełny fallback na `system-ui`, więc offline panel
wygląda jak wcześniej, tylko bez IBM Plex.

## Nowy kod poza szablonami

- `dash_overview()` — liczby na wstążkę i kafelki. Nigdy nie rzuca; brak
  odpowiedzi z bazy daje `None`, a kafelek pokazuje kreskę zamiast pewnego zera.
- `dash_attention(overview, upd)` — krótka lista rzeczy wymagających ręki jako
  pary `(klucz, wartość)`. Pusta lista oznacza, że panel w ogóle się nie renderuje.
- `BOT_ACTIVITY_BUCKETS` — mapowanie `action_id` z `playerbot_status.tsv` na trzy
  kubełki (walka / wędrówka / miasto i handel) plus resztę.

## Weryfikacja

- `py_compile` obu kopii — OK.
- **Render wszystkich 16 szablonów offline** (Jinja + atrapy danych):
  13 przechodzi, 3 wymagają danych trasy, których atrapa nie dostarcza
  (`TPL_AI`, `TPL_SEASON`, `TPL_LIVE_MAP`). Te same trzy padają **identycznie**
  na kopii sprzed przeprojektowania — czyli to artefakt testu, nie regresja.
- **Każdy klucz `t()` użyty w pliku ma definicję** (sprawdzone przez AST);
  wszystkie 35 nowych kluczy obecne, w tym cztery rozwiązywane dynamicznie
  (`t('b_' + a.key)`).
- Render produkcyjnego `TPL_DASH` z prawdziwymi polskimi etykietami — poprawny.

**Nie wykonano: uruchomienia panelu.** Docker Desktop był wyłączony przez całą
tę turę. Obraz `panel` wymaga przebudowy (`docker compose build panel`), tak samo
jak `game` z poprzedniej tury.
