# Changelog

## 17.09.2026 · 1.8.0
- Mapa na żywo jest teraz naprawdę na żywo: każdy bot ma własny punkt, który płynnie przesuwa się do kolejnej zgłoszonej pozycji, zamiast przerysowywania całej mapy co półtorej sekundy. Panel pobiera przy tym tylko wybraną mapę (kilka kilobajtów zamiast 600 kB na każde odświeżenie), więc mapa nie obciąża już serwera przy każdej otwartej karcie. Gdy rdzeń przestaje odpowiadać, mapa mówi o tym wprost zamiast zastygać.
- Zakładka „Zachowanie botów” przebudowana: każde ustawienie to osobna karta z ikoną, suwakiem z zaznaczoną wartością domyślną, aktualną liczbą i przyciskiem przywrócenia. Ustawienia odbiegające od domyślnych są podświetlone i policzone, a przycisk zapisu jedzie razem ze stroną, więc nie trzeba go szukać na dole. Przełączniki są przełącznikami, a nie zwykłymi „ptaszkami”; długie objaśnienia schowane pod „Co to zmienia w świecie?”.
- Nowa karta konta (`/account/<id>`): postacie, pełna historia logowań z adresem IP i identyfikatorem maszyny, konta logujące się z tych samych adresów, blokada konta i ustawienie nowego hasła. Blokada zatrzymuje kolejne logowanie — trwającej sesji gra nie przerywa, bo nie ma takiej komendy, i panel to mówi.
- Nowy dziennik działań operatora na dole zakładki Konta i GM: blokady, zmiany haseł i zakładanie kont, z datą.
- Nowa zakładka „Zdrowie botów”: boty, których rdzeń przestał zgłaszać, stojące w miejscu, bez broni i bez zbroi, zgony z doby, mapy bez botów, rozkład poziomów na każdej mapie oraz porównanie ruchu w świecie sprzed i po ostatnim restarcie — czyli sposób na wychwycenie cichej regresji po aktualizacji.
- Nowa strona dla graczy pod `/serwer` (domyślnie wyłączona, włącznik w Zarządzanie → Wygląd i dostęp): stan serwera, mnożniki, rankingi, ostatnie wydarzenia i ceny na straganach — bez logowania i bez żadnych danych kont.

## 17.09.2026 · 1.7.0
- Wszystkie zakładki dostały wygląd strony głównej: każda otwiera się tym samym banerem z nocnym krajobrazem i ikoną, jaką ma w menu, a liczby nad treścią są takimi samymi kolorowymi kafelkami jak na pulpicie.
- Nowe kafelki tam, gdzie ich nie było: Postacie i boty (postacie, playerboty, gracze, w grze teraz), Gildie (gildie, członkowie, najwyższy poziom, punkty rangi), Rankingi (kategoria, lider, liczba sklasyfikowanych, kto się liczy), Aktywność map (postacie w świecie, mapy z ruchem, najbardziej zatłoczona), Baza przedmiotów (przedmioty, kategorie, pokazane), Konta (konta na liście, botów, graczy, ostatnie logowanie), Komendy GM i Changelog.
- Zarządzanie: dotychczasowy wąski pasek stanu zastąpiony pięcioma kafelkami — stan serwera, boty w świecie, mnożniki, język gry i wersja Playerbots.
- Profil postaci, stragany, gildia, stragan i karta przedmiotu: powrót do listy jest teraz w banerze, a nie osobnym linkiem nad nagłówkiem. Profil pokazuje portret postaci w banerze.
- Naprawiona zakładka Sezon: pokazywała „Internal Server Error”, bo MariaDB odrzuca nazwę sumy użytą w sortowaniu („Reference 'metins' not supported”). Zakładka znów działa.
- Panele na wszystkich stronach mają tę samą oprawę co karty pulpitu, a ich nagłówki ten sam niebieski znacznik.

## 17.09.2026 · 1.6.0
- Nowy wygląd całego panelu w stylu „Control Center”: pasek boczny z ikonami i aktywną pozycją jako wypełniona pigułka, karta serwera u dołu (stan, boty w świecie, czas działania liczony od startu rdzeni gry), górny pasek z wyszukiwarką postaci (Ctrl+K), przełącznikiem jasny/ciemny, powiadomieniami ze świata, skrótem do Zarządzania i zegarem.
- Pulpit przebudowany: baner powitalny, pięć kafelków z liczbami (z trendem Yang z ostatniej doby i wykresikiem obecności botów), mapa na żywo obok stanu serwera i pierścieni obciążenia hosta, Top 5, ostatnie logowania, karuzela rankingów, boty/stragany według map, wykres aktywności z 24 godzin (logowania, boty w świecie, sprzedaże) i lista ostatnich zdarzeń.
- Karuzela rankingów sama przewija się co 8 sekund; kliknięcie strzałki lub kropki zaczyna odliczanie od nowa. Kafelek „Boty według map” co 8 sekund przełącza się na „Stragany według map”.
- Nowy ranking „Skuteczność ulepszeń” (procent udanych, minimum 20 prób) — w Rankingach i w karuzeli. Porażka liczona jako `REMOVE (REFINE FAIL)`, bo tak silnik ją zapisuje.
- Nowy przełącznik w Zarządzaniu → Wygląd i dostęp: rankingi i karuzela mogą liczyć także prawdziwych graczy. Domyślnie wyłączony; postacie testowe instalatora nigdy nie trafiają do rankingu.
- „Yang w obiegu” nie liczy już kont testowych instalatora (Admin, AdminNinja, AdminSura, AdminSzaman — razem 2 mld Yang).
- Ranking „Przedmiot +9” liczy tylko przedmioty noszone i w plecaku. W magazynie `owner_id` to numer konta, więc cudzy przedmiot trafiał do postaci o tym samym numerze.
- Profil postaci: znaczniki z czasem gry, ostatnim logowaniem, Smoczymi Monetami, małżeństwem i gildią; osiągnięcia z dziennika (Metiny, bossy, zgony, PvP, ulepszenia udane i spalone, skuteczność); umiejętności jako ikony z rangą i podpowiedzią, plus umiejętności pasywne. Ikony umiejętności są teraz serwowane przez Tuike, a nie z adresu 127.0.0.1:7788, który w przeglądarce operatora wskazywał jego własny komputer.
- Profil postaci: sekcja „Sklep offline” (towar, ceny, mapa i współrzędne straganu) z przyciskiem „Teleportuj moją postać do sklepu”, działającym także, gdy właściciel jest offline.
- Profil postaci: „Historia ekwipunku” zamiast surowego logu — założone, ulepszone, spalone, wzmocnienia i zaczarowania, sprzedaże, magazyn — z nazwami przedmiotów. Surowy log zostaje pod rozwijanym przyciskiem.
- Profil postaci: „Dziennik rdzenia na żywo” — linie z syslogu kanałów gry o tej postaci, uruchamiane same, dopisywane zamiast podmieniane, kolorowane, z pauzą i kopiowaniem.
- Magazyn ma strony I/II/III jak w grze, a przedmioty zajmują tyle pól w pionie, ile w grze. Wcześniej przedmiot ze strony II rysował się na przedmiocie ze strony I.
- Księga Umiejętności pokazuje, jakiej umiejętności uczy, zamiast doklejać do podpowiedzi bonusy przypadkowego przedmiotu.
- Stragany: kafelek „Transakcji łącznie” (z liczbą z ostatnich 24 h), wykres tempa sprzedaży z trendem średniej ceny, stragany według map, ostatnie sprzedaże na żywo i tabela ksiąg umiejętności (na ladach teraz i sprzedane w 7 dni).
- Nowe mapy: Las, Czerwony Las i Wieża Demonów (historia map i mapy cieplne; bez mapy na żywo, bo nie ma ich grafik).
- Ranking „Ryby” w karuzeli wreszcie coś pokazuje i nie spowalnia pulpitu: liczy wyłowione ryby, a nie szukał słowa „ryb” w numerach przedmiotów (35 s przy każdym otwarciu pulpitu i zawsze pusty wynik).
- Naprawiono wydarzenia ze świata (lista na pulpicie i dzwonek): z ostatnich 12 godzin zostawał najstarszy fragment zamiast najnowszego.

## 14.09.2026 · 1.5.0
- Zakładka Zarządzanie przebudowana: jeden długi scroll zamieniony na sześć zakładek (Świat i restart, Aktualizacje, Zachowanie botów, Narzędzia, Wygląd i dostęp, Populacja), przełączanych bez przeładowania strony. Stare zakładki w przeglądarce (`/manage#behavior` itp.) nadal działają.
- Sekcja "Zachowanie botów" rozbita na czytelne podsekcje (Gospodarka straganów, Społeczne i widoczność, Nauka, Eventy) zamiast jednego gęstego gridu ~18 kontrolek.
- Nowa karta "Powiązane panele" w zakładce Narzędzia — odnośniki do Seban Panelu, panelu klasycznego, ItemShopu i panelu VPS (jeśli skonfigurowany).

## 14.09.2026 · 1.4.0
- Nowy suwak w zakładce Zachowanie botów: "Wrogość między królestwami" (`KINGDOMPVP`, domyślnie 0%) — mechanizm już istniał w silniku, ale nie miał interfejsu w żadnym panelu. Dotyczy tylko światów z trzema włączonymi królestwami.

## 14.09.2026 · 1.3.0
- Naprawiona ocena przedmiotów na straganach: nisko ulepszona zbroja/broń, która i tak jest warta yangów (rodzina wyceniana ręcznie) albo jest dla innej klasy niż bota, nie jest już traktowana jak złom — trafia na ladę w realnej cenie, zamiast być pomijana albo wyceniana jak fanty do przetopienia.
- Szyldy straganów zgadzają się teraz z tą samą oceną: taki przedmiot liczy się do "prawdziwego ekwipunku", nie do złomu, więc mniej straganów dostaje nazwę pokroju "Zlom do palenia".
- Nowe pole w zakładce Zachowanie botów: "Próg prawdziwego ekwipunku" (`SCRAPFLOOR`, domyślnie +4) — operator sam decyduje, od jakiego ulepszenia przedmiot liczy się jako towar, a nie złom.

## 14.09.2026 · 1.2.0
- Zakładka Gospodarka odświeża się teraz sama, bez przeładowania strony: liczniki i ostatnie transakcje co 6 sekund, ranking sprzedaży i lista straganów co 45 sekund (cięższe zapytanie po całym logu, stąd rzadziej).
- Nowy suwak w zakładce Zachowanie botów: "Handlarze okazjami" (`PROFIT`) — procent botów, które kupują przedmiot na straganie z myślą o zarobku, nie o własnym użytku, jeśli cena jest wyraźnie poniżej godziwej wartości rynkowej. Działa od ręki, tak jak reszta suwaków.

## 14.09.2026 · 1.1.0
- Przebudowana zakładka Gospodarka: liczniki na żywo (aktywne stragany, wystawione przedmioty, transakcje i obrót Yang z ostatnich 24h), lista ostatnich transakcji i ranking najlepiej sprzedających się przedmiotów - wszystko czytane wprost z logu Ikarus Shop (`log.ikarusshop_log`), bez potrzeby dodatkowego kolektora.
- Nowa podstrona Stragany: przegląd wszystkich otwartych sklepów (`player.ikashop_offlineshop`) oraz widok pojedynczego straganu z jego towarem i cenami, odczytanymi z `player.item`/`ikashop_data`.
- Karta przedmiotu pokazuje teraz też aktualne oferty sprzedaży na straganach oraz historię ostatnich sprzedaży tego przedmiotu, obok istniejącego wykresu liczebności.

## 14.09.2026 · 1.0.0
- Pierwsze wydanie Tuike: panel napisany od nowa na bazie Seban Panelu, działający na porcie 7799 obok panelu klasycznego (7788) i Seban Panelu (7790).
- Aplikacja rozbita na pakiet `tuike/`: konfiguracja, dostęp do bazy, dane gry, odczyt świata na żywo, kolejki plikowe, zapytania i blueprinty Flaska w osobnych modułach zamiast jednego pliku.
- Własne tabele `player.web_tuike_*` — ustawienia, historia telemetrii, gospodarki, pozycji botów i nadań. Oba panele mogą działać równolegle bez nadpisywania sobie danych.
- Nowy interfejs: jeden arkusz stylów oparty na tokenach, cztery motywy (Midnight, Ember, Forest, Dawn), dwa zagęszczenia widoku i pełna obsługa telefonu.
- Mapa na żywo rysowana z danych serwera: lista map, tła i proporcje pochodzą z jednej tabeli w Pythonie, więc panel nie ma już drugiej, rozjeżdżającej się listy w JavaScripcie.
- Pulpit pokazuje wszystkie mapy z podkładem graficznym, licznik zawieszonych botów oraz datę ostatniego restartu odczytywaną z kolejki.
- Ochrona CSRF na każdym formularzu zmieniającym stan serwera, nie tylko na aktualizatorze.
- Zlecenia do kontenera gry są podpisywane prefiksem `tuike-`, a blokada nadań ma własną nazwę, więc oba panele nie blokują się nawzajem.
- Kolektor i worker nadań uruchamiane jako `python collector.py` i `python grants_worker.py`, z własnym heartbeatem widocznym w panelu.
