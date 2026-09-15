# Testy Tuike

Trzy skrypty, które nie potrzebują ani bazy, ani serwera gry: `pymysql.connect`
jest podmieniane na atrapę odpowiadającą z małej tablicy fragmentów SQL, a
kolejki plikowe wskazują na katalogi tymczasowe.

```bash
cd linux-port/docker/tuike
pip install -r requirements.txt
python tests/test_pages.py     # renderuje wszystkie strony i API
python tests/test_writes.py    # kolejki plikowe, formularze, oba workery
python tests/test_auth.py      # kreator i bramka hasła
```

Każdy kończy się kodem różnym od zera, jeśli coś nie przeszło, więc nadają się
prosto do CI.

| Skrypt | Co sprawdza |
| --- | --- |
| `test_pages.py` | każda strona i endpoint API zwraca oczekiwany kod; POST bez tokenu CSRF dostaje 403 |
| `test_writes.py` | zlecenia w kolejce plikowej, przycinanie wag zachowania, aktualizator, walidacja formularzy, przebieg kolektora i workera nadań |
| `test_auth.py` | kreator pierwszego uruchomienia, logowanie, wylogowanie i to, że aktualizacja jest niedostępna na panelu bez hasła |

`test_writes.py` importuje `test_pages.py` wyłącznie po to, żeby zainstalować
atrapę bazy — stąd wspólna tablica `ROUTES` w tym pierwszym pliku. Gdy dodasz
zapytanie o nowym kształcie, dopisz jego fragment do `ROUTES`; najbardziej
szczegółowe wpisy muszą iść pierwsze, bo wygrywa pierwsze dopasowanie.
