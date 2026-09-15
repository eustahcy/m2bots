# Changelog

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
