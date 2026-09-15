# M2 VPS Panel

Osobny, jednoplikowy panel do sterowania całym stosem Docker Compose z zewnątrz:
start, stop, restart (gry albo całego stosu), kompilacja `game`, **aktualizacja
z GitHuba**, status portów, liczba aktywnych postaci, stan kontenerów,
obciążenie hosta.

Działa jako usługa systemd **poza** Dockerem, więc potrafi pokazać "serwer stoi"
i zaoferować przycisk Start, nawet gdy cały stos jest zatrzymany. Startuje
automatycznie po restarcie VPS (`systemctl enable`).

## Wygląd

Układ launchera, nie tablicy wskaźników: nagłówek z marką i wersją, hero
z dużym stanem serwera i **jednym** przyciskiem, który przełącza się między
`▶ URUCHOM SERWER` a `■ ZATRZYMAJ SERWER`, kafelki ze statystykami, a niżej
karty — Sterowanie, Aktualizacja z GitHuba, Panele serwera, Kontenery, Log.

Jeden przycisk zamiast pary Uruchom/Zatrzymaj dlatego, że w danej chwili
sensowna jest dokładnie jedna z tych akcji, a launcher ma pokazywać tę jedną.
Zatrzymanie pyta o potwierdzenie i mówi wprost, że postęp zostaje.

Marka w nagłówku, w tej kolejności: `M2_VPS_PANEL_BRAND` z
`/etc/m2-vps-panel.env`, potem `M2_BRAND` z `.env` stosu, w ostateczności
„Metin2 Playerbots". Ustawia się ją raz:

```sh
echo 'M2_VPS_PANEL_BRAND=NazwaSerwera' | sudo tee -a /etc/m2-vps-panel.env
sudo systemctl restart m2-vps-panel
```

## Instalacja

```sh
scp -r vps-panel debian@<vps>:~/vps-panel
ssh debian@<vps>
sudo sh ~/vps-panel/install.sh
```

Skrypt sam znajdzie katalog z `docker-compose.yml` (albo podaj go jako argument),
wygeneruje losowe hasło (pokazane raz, przy pierwszej instalacji) i uruchomi
usługę na porcie 9797.

## Aktualizacja

Po zmianie `app.py` wystarczy ponownie skopiować katalog i uruchomić
`sudo sh install.sh` — hasło i sekret sesji zostają bez zmian.

---

## Aktualizacja serwera z GitHuba

Zamiast pakować `linux-port/docker` w ZIP i wysyłać go z Windows, drzewo serwera
na VPS jest checkoutem gita. Na PC wypychasz zmiany, na panelu je pobierasz.

> **Aktualizator jest opt-in.** Dopóki `M2_GIT_DIR` nie stoi w
> `/etc/m2-vps-panel.env`, sekcja aktualizacji pokazuje „nieskonfigurowane"
> i żaden przycisk nic nie robi. Ustawia to wyłącznie `git-setup.sh`.
>
> Ta ostrożność ma powód. Pierwsza wersja brała po prostu katalog serwera,
> „jeśli akurat jest checkoutem". Na prawdziwym VPS-ie ten katalog okazał się
> checkoutem **upstreamu** z 77 lokalnie zmienionymi plikami — całą pracą
> dowiezioną wcześniej ZIP-ami. Uczynny przycisk „Zastosuj aktualizację"
> zrobiłby `reset --hard` na cudzą gałąź i wyrzucił to wszystko. Panel nie ma
> prawa znaleźć sobie celu przypadkiem.

### Konfiguracja (raz)

Na PC — założenie repozytorium i pierwszy import:

```powershell
cd "…\Serwer"
.\tools\Publish-ToGit.ps1 -RemoteUrl https://github.com/<user>/<repo>.git -Message "Pierwszy import"
```

Na VPS — zamiana istniejącego drzewa w checkout:

```sh
sudo sh ~/vps-panel/git-setup.sh https://github.com/<user>/<repo>.git main
```

`git-setup.sh` robi `git init` **w miejscu**: nic nie przenosi, nie klonuje obok
i nie dotyka `.env` ani `backups/`. Pokazuje, czym drzewo na serwerze różni się
od repozytorium, i czeka na potwierdzenie, zanim cokolwiek nadpisze (`--yes`
pomija pytanie). Skrypt odmówi, jeśli w repozytorium znajdzie
`linux-port/docker/.env` — hasła do bazy nie mają prawa przyjść z gita.

### Codzienna praca

```powershell
.\tools\Publish-ToGit.ps1 -Message "Boty wchodza glebiej w loch malp"
```

Potem w panelu na porcie 9797, w sekcji **Aktualizacja z GitHuba**:

1. **Sprawdź aktualizacje** — `git fetch`, a potem lista nowych commitów
   i zmienionych plików. Nic jeszcze nie zostało nadpisane.
2. **Zastosuj aktualizację** — `git reset --hard`, `docker compose build game`,
   `docker compose up -d`. Przycisk jest nieaktywny, dopóki „Sprawdź” nie
   pokaże, że jest co brać.

Postęp leci do wspólnego **Logu ostatniej akcji** pod spodem.

`-WhatIfOnly` na skrypcie pokazuje, co poszłoby na GitHuba, i kończy bez pusha.

### Co ten aktualizator gwarantuje

- **`.env` nigdy nie jest ruszany.** Jest w `.gitignore`, więc git go nie śledzi,
  a `reset --hard` nie dotyka plików nieśledzonych. Tak samo `backups/`,
  `launcher-logs/` i stan launchera.
- **Baza jest nietykalna.** Nigdzie w tym kodzie nie pada `down`, tym bardziej
  `down -v`. Konta, postacie i postęp botów siedzą w wolumenach.
- **Nieudany krok zatrzymuje resztę.** Gdy `fetch` padnie, nic się nie zmienia;
  gdy padnie kompilacja, pliki są już nowe, ale stary kontener gry chodzi dalej
  — log mówi wprost, na czym stanęło.
- **`reset --hard`, nie `pull`.** Na serwerze nikt nie commituje, a konflikt
  merge'a o trzeciej w nocy na maszynie bez edytora to najgorszy możliwy
  scenariusz. To, co mówi gałąź zdalna, jest tym, co serwer uruchamia.
  Konsekwencja: ręczne poprawki wklepane na VPS zostaną nadpisane — panel
  ostrzega o tym liczbą „plików zmienionych na serwerze”.
- **Panel nie podmienia sam siebie.** Chodzi z `/opt/m2-vps-panel`, a git
  aktualizuje kopię w checkoucie. Jeśli aktualizacja zmienia
  `linux-port/vps-panel/`, panel mówi o tym przed i po — wtedy `sudo sh
  …/vps-panel/install.sh`.

### Testy

```sh
python3 smoke_test.py   # panel wstaje i serwuje strony
python3 git_test.py     # aktualizator na prawdziwym repo, bez Dockera
```

`git_test.py` buduje sobie własne repozytorium w katalogu tymczasowym, więc nie
potrzebuje ani GitHuba, ani Dockera, ani działającego serwera.

---

## Bezpieczeństwo

Ten panel ma pełną kontrolę nad kontenerami stosu (start/stop/restart/build)
**oraz nad treścią plików serwera** (aktualizacja z gita) — to poziom zaufania
równy dostępowi do gniazda Dockera na hoście. Trzyma się za hasłem (sesja +
CSRF na każdej akcji); jeśli VPS ma jakikolwiek firewall providera, otwórz na
nim port 9797 świadomie, najlepiej tylko dla swojego IP.

Panel chodzi jako root nad katalogiem należącym do użytkownika logowania, więc
`git-setup.sh` dopisuje ten katalog do `safe.directory` roota (bez tego git
2.35.2+ odmawia i w panelu wygląda to jak „brak aktualizacji”). Skutek uboczny:
pliki zaktualizowane przez panel należą do roota. Dla Dockera i buildów bez
znaczenia; do ręcznej edycji przez `debian` potrzebne będzie `sudo`.

**Repozytorium jest publiczne** — świadoma decyzja właściciela projektu. Zanim
dodasz cokolwiek do `Serwer/`, sprawdź, czy nie jest to sekret ani plik, którego
nie wolno publikować. `Serwer/.gitignore` broni znanych przypadków, a
`Publish-ToGit.ps1` sprawdza je jeszcze raz przed pushem — ale żadne z nich nie
zna pliku, który dopiero wymyślisz.
