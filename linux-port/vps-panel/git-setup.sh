#!/bin/sh
# =============================================================================
#  git-setup.sh -- zamienia istniejące drzewo serwera na VPS w checkout gita,
#  z którego panel (port 9797) potrafi się aktualizować.
#
#  Uruchamiasz to RAZ, na serwerze:
#
#      sudo sh git-setup.sh https://github.com/<user>/<repo>.git [gałąź]
#
#  Co robi:
#    1. sprawdza, że wskazany katalog to naprawdę drzewo serwera 2.x,
#    2. dogrywa gita, jeśli go nie ma,
#    3. `git init` W MIEJSCU -- nie klonuje obok i nic nie przenosi, więc .env,
#       backups/ i cała reszta stanu serwera zostają dokładnie tam, gdzie są,
#    4. pobiera gałąź i pokazuje, czym drzewo na serwerze różni się od repo,
#    5. dopiero po potwierdzeniu robi `git reset --hard` (albo z --yes),
#    6. dopisuje M2_GIT_* do /etc/m2-vps-panel.env i restartuje panel.
#
#  Nie dotyka bazy. Nie robi `docker compose down`. Nie kasuje plików
#  nieśledzonych przez gita -- .env jest jednym z nich i ma taki zostać.
# =============================================================================
set -eu

REMOTE_URL="${1:-}"
BRANCH="${2:-main}"
ENV_FILE="/etc/m2-vps-panel.env"
STATE_DIR="/var/lib/m2-vps-panel"
ASSUME_YES=0

# --yes gdziekolwiek w argumentach: tryb nieinteraktywny.
for arg in "$@"; do
    [ "$arg" = "--yes" ] && ASSUME_YES=1
done

die() { echo "BŁĄD: $*" >&2; exit 1; }

[ "$(id -u)" = "0" ] || die "uruchom przez sudo: sudo sh git-setup.sh <url-repo> [gałąź]"
[ -n "$REMOTE_URL" ] || die "podaj adres repozytorium, np. https://github.com/user/repo.git"

# --- gdzie jest drzewo serwera ------------------------------------------------
# Ta sama heurystyka co w install.sh, tylko szukamy katalogu o dwa poziomy
# wyżej niż docker-compose.yml -- to jest korzeń repozytorium (VERSION obok
# linux-port/), a nie sam katalog compose.
GIT_DIR=""
if [ -n "${M2_GIT_DIR:-}" ]; then
    GIT_DIR="$M2_GIT_DIR"
elif [ -f "$ENV_FILE" ] && grep -q '^M2_COMPOSE_DIR=' "$ENV_FILE"; then
    COMPOSE_DIR="$(grep '^M2_COMPOSE_DIR=' "$ENV_FILE" | cut -d= -f2-)"
    GIT_DIR="$(cd "$COMPOSE_DIR/../.." && pwd)"
else
    for candidate in /home/*/metin2-playerbots /opt/metin2-playerbots; do
        [ -f "$candidate/linux-port/docker/docker-compose.yml" ] && { GIT_DIR="$candidate"; break; }
    done
fi

[ -n "$GIT_DIR" ] || die "nie znalazłem drzewa serwera. Podaj je: M2_GIT_DIR=/sciezka sudo sh git-setup.sh ..."
[ -f "$GIT_DIR/VERSION" ] || die "$GIT_DIR nie wygląda na drzewo serwera (brak pliku VERSION)"
[ -f "$GIT_DIR/linux-port/docker/docker-compose.yml" ] || die "$GIT_DIR nie ma linux-port/docker/docker-compose.yml"
if [ -f "$GIT_DIR/linux-port/docker/ENGINE" ] \
   && [ "$(tr -d ' \r\n' < "$GIT_DIR/linux-port/docker/ENGINE")" != "mt2009" ]; then
    die "to nie jest linia 2.x (mt2009) -- marker ENGINE mówi co innego"
fi

echo "Drzewo serwera: $GIT_DIR"
echo "Repozytorium:   $REMOTE_URL (gałąź $BRANCH)"
echo ""

# --- git --------------------------------------------------------------------
if ! command -v git >/dev/null 2>&1; then
    echo "Dogrywam gita..."
    apt-get update -qq && apt-get install -y -qq git
fi

# Panel chodzi jako root nad katalogiem należącym do użytkownika logowania;
# bez tego git 2.35.2+ odmawia ("detected dubious ownership"), a w panelu
# wygląda to identycznie jak "brak aktualizacji".
git config --global --add safe.directory "$GIT_DIR" 2>/dev/null || true

mkdir -p "$STATE_DIR"
DOCKER_ENV="$GIT_DIR/linux-port/docker/.env"
if [ -f "$DOCKER_ENV" ]; then
    BACKUP="$STATE_DIR/env-backup-$(date +%Y%m%d-%H%M%S)"
    cp "$DOCKER_ENV" "$BACKUP"
    chmod 600 "$BACKUP"
    echo "Kopia .env: $BACKUP"
fi

cd "$GIT_DIR"

if [ -d .git ]; then
    echo "To już jest repozytorium git -- ustawiam tylko adres zdalny."
    if git remote get-url origin >/dev/null 2>&1; then
        git remote set-url origin "$REMOTE_URL"
    else
        git remote add origin "$REMOTE_URL"
    fi
else
    git init -q
    git remote add origin "$REMOTE_URL"
fi

echo "Pobieram $BRANCH..."
GIT_TERMINAL_PROMPT=0 git fetch --depth=1 origin "$BRANCH" \
    || die "nie udało się pobrać. Sprawdź adres repo, gałąź i sieć na VPS."

# --- bezpiecznik: .env nie ma prawa być w repozytorium ------------------------
# Gdyby kiedyś wpadł tam przez pomyłkę, `reset --hard` nadpisałby hasła na
# serwerze cudzymi. Taniej sprawdzić raz tutaj niż tłumaczyć to potem.
if git ls-tree -r --name-only "origin/$BRANCH" | grep -qx 'linux-port/docker/.env'; then
    die "repozytorium zawiera linux-port/docker/.env -- usuń go z repo i z historii, zanim to podepniesz"
fi

# --- co się zmieni ------------------------------------------------------------
# reset mieszany: ustawia HEAD i indeks, nie rusza plików. Dzięki temu
# `git status` pokazuje różnicę serwer <-> repo, zanim cokolwiek nadpiszemy.
git reset -q "origin/$BRANCH"

CHANGED=$(git status --porcelain --untracked-files=no | wc -l)
echo ""
if [ "$CHANGED" -eq 0 ]; then
    echo "Drzewo na serwerze jest identyczne z repozytorium — nie ma co nadpisywać."
else
    echo "Plików różniących się od repozytorium: $CHANGED"
    echo "(pierwsze 40; M = inna treść, D = jest w repo, brak na serwerze)"
    git status --porcelain --untracked-files=no | head -n 40
    echo ""
    echo "Hard reset nadpisze te pliki wersją z repozytorium."
    echo "NIE ruszy plików nieśledzonych: .env, backups/, launcher-logs/ zostają."
fi

if [ "$CHANGED" -ne 0 ] && [ "$ASSUME_YES" -ne 1 ]; then
    if [ -t 0 ]; then
        printf 'Kontynuować? [t/N] '
        read -r answer
        case "$answer" in
            t|T|y|Y) ;;
            *) echo "Przerwane. Repozytorium jest podpięte, ale plików nie zmieniono."; exit 0 ;;
        esac
    else
        echo "Brak terminala do potwierdzenia. Powtórz z --yes, jeśli to jest to, czego chcesz."
        exit 0
    fi
fi

git checkout -q -B "$BRANCH" "origin/$BRANCH"
git reset -q --hard "origin/$BRANCH"
echo "Drzewo ustawione na $(git rev-parse --short HEAD) ($BRANCH)."

# --- konfiguracja panelu ------------------------------------------------------
if [ -f "$ENV_FILE" ]; then
    # Przepisujemy plik bez starych kluczy M2_GIT_*, potem dopisujemy nowe --
    # idempotentnie, żeby powtórne uruchomienie nie mnożyło linii.
    grep -v '^M2_GIT_' "$ENV_FILE" > "$ENV_FILE.new" || true
    {
        echo "M2_GIT_DIR=$GIT_DIR"
        echo "M2_GIT_REMOTE=origin"
        echo "M2_GIT_BRANCH=$BRANCH"
    } >> "$ENV_FILE.new"
    chmod 600 "$ENV_FILE.new"
    mv "$ENV_FILE.new" "$ENV_FILE"
    systemctl restart m2-vps-panel.service 2>/dev/null || true
    echo "Panel przeładowany — sekcja „Aktualizacja z GitHuba” jest już aktywna."
else
    echo ""
    echo "UWAGA: nie ma $ENV_FILE — panel nie jest jeszcze zainstalowany."
    echo "Uruchom najpierw: sudo sh $(dirname "$0")/install.sh"
fi

echo ""
echo "Gotowe. Od teraz: wypchnij zmiany z PC, a na panelu 9797 kliknij"
echo "„Sprawdź aktualizacje”, a potem „Zastosuj aktualizację”."
