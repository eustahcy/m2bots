#!/bin/sh
# Instalator panelu Tuike po stronie serwera (Linux).
#
# Uruchamia go skrypt Zainstaluj-Tuike.ps1 z Windowsa, ale da się go odpalić
# także ręcznie na serwerze:
#
#   sudo sh /opt/tuike-panel/install/install.sh --stack /home/debian/metin2-playerbots/linux-port/docker
#
# Co robi: znajduje stos gry, czyta z jego .env hasło do bazy i nazwę projektu,
# zapisuje własny plik tuike.env, buduje obraz panelu i podnosi trzy kontenery
# w osobnym projekcie Compose. Nie dotyka plików ani wolumenów stosu gry.
set -eu

DIR=$(CDPATH= cd -- "$(dirname -- "$0")/.." && pwd)
COMPOSE_FILE="$DIR/install/docker-compose.tuike.yml"
ENV_FILE="$DIR/tuike.env"
PROJECT="tuike-panel"
PORT=""
STACK=""
SERVICE_FILE="/etc/systemd/system/tuike-panel.service"

say() { printf '%s\n' "$*"; }
die() { printf 'BLAD: %s\n' "$*" >&2; exit 1; }

while [ $# -gt 0 ]; do
  case "$1" in
    --stack) STACK=${2:-}; shift 2 ;;
    --port) PORT=${2:-}; shift 2 ;;
    --project) PROJECT=${2:-}; shift 2 ;;
    --help|-h)
      say "Uzycie: install.sh [--stack KATALOG_DOCKER] [--port 7799] [--project tuike-panel]"
      exit 0 ;;
    *) die "nieznany argument: $1" ;;
  esac
done

# --- czego potrzebujemy ------------------------------------------------------
command -v docker >/dev/null 2>&1 || die "nie ma Dockera na tej maszynie."
if docker compose version >/dev/null 2>&1; then
  DC="docker compose"
elif command -v docker-compose >/dev/null 2>&1; then
  DC="docker-compose"
else
  die "nie ma docker compose ani docker-compose."
fi
[ -f "$COMPOSE_FILE" ] || die "brak pliku $COMPOSE_FILE - paczka jest niekompletna."
[ -f "$DIR/panel/Dockerfile" ] || die "brak zrodel panelu w $DIR/panel - paczka jest niekompletna."

# --- gdzie stoi stos gry -----------------------------------------------------
# Szukamy katalogu z docker-compose.yml i .env, czyli tego, z ktorego wlasciciel
# serwera uruchamia gre. Zgadujemy tylko typowe miejsca; reszta to --stack.
if [ -z "$STACK" ]; then
  for candidate in \
    "$HOME/metin2-playerbots/linux-port/docker" \
    "/home/debian/metin2-playerbots/linux-port/docker" \
    "/root/metin2-playerbots/linux-port/docker" \
    "/opt/metin2/linux-port/docker" \
    "$DIR/../linux-port/docker"
  do
    if [ -f "$candidate/.env" ] && [ -f "$candidate/docker-compose.yml" ]; then
      STACK=$(CDPATH= cd -- "$candidate" && pwd)
      break
    fi
  done
fi
[ -n "$STACK" ] || die "nie znalazlem katalogu stosu gry. Podaj go: --stack /sciezka/do/linux-port/docker"
[ -f "$STACK/.env" ] || die "w $STACK nie ma pliku .env - to nie jest katalog stosu gry."
say "Stos gry: $STACK"

# Czytamy tylko te klucze, ktore sa nam potrzebne, i nie wypisujemy hasel.
read_env() {
  sed -n "s/^$1=//p" "$STACK/.env" | tail -n 1 | tr -d '\r'
}

STACK_PROJECT=$(read_env M2_COMPOSE_PROJECT_NAME)
[ -n "$STACK_PROJECT" ] || STACK_PROJECT=$(read_env M2_CONTAINER_PREFIX)
[ -n "$STACK_PROJECT" ] || STACK_PROJECT="metin2"
DB_USER=$(read_env M2_DB_USER); [ -n "$DB_USER" ] || DB_USER="metin2"
DB_PASSWORD=$(read_env M2_DB_PASSWORD)
[ -n "$DB_PASSWORD" ] || die "w $STACK/.env nie ma M2_DB_PASSWORD - bez hasla panel nie polaczy sie z baza."
TZONE=$(read_env M2_TZ); [ -n "$TZONE" ] || TZONE="UTC"
PUBLIC_ADDRESS=$(read_env M2_PUBLIC_ADDRESS); [ -n "$PUBLIC_ADDRESS" ] || PUBLIC_ADDRESS="127.0.0.1"
BIND_ADDRESS=$(read_env M2_PANEL_BIND_ADDRESS)
[ -n "$BIND_ADDRESS" ] || BIND_ADDRESS=$(read_env M2_HOST_BIND_ADDRESS)
[ -n "$BIND_ADDRESS" ] || BIND_ADDRESS="0.0.0.0"
COLLECTOR=$(read_env M2_TUIKE_COLLECTOR_INTERVAL); [ -n "$COLLECTOR" ] || COLLECTOR="300"
ENGINE="mt2009"
[ -f "$STACK/ENGINE" ] && ENGINE=$(tr -d '\r\n' < "$STACK/ENGINE")

# Port: domyslnie 7799, a jesli stoi tam juz czyjs panel - 7800 i dalej.
if [ -z "$PORT" ]; then
  PORT=$(read_env M2_TUIKE_PANEL_PORT)
  [ -n "$PORT" ] || PORT=7799
fi
port_taken() {
  if command -v ss >/dev/null 2>&1; then
    ss -ltn 2>/dev/null | awk '{print $4}' | grep -qE "[:.]$1\$"
  else
    docker ps --format '{{.Ports}}' | grep -q ":$1->"
  fi
}
# Port zajety przez NASZ wlasny kontener to nie konflikt, tylko ponowna instalacja.
if port_taken "$PORT" && ! docker ps --format '{{.Names}} {{.Ports}}' | grep "^$PROJECT " | grep -q ":$PORT->"; then
  ORIGINAL=$PORT
  while port_taken "$PORT" && [ "$PORT" -lt 7820 ]; do
    PORT=$((PORT + 1))
  done
  say "Port $ORIGINAL jest zajety - panel stanie na $PORT."
fi

# --- czy stos w ogole istnieje ----------------------------------------------
# Sieci i wolumeny tworzy stos gry przy pierwszym starcie. Jesli ich nie ma,
# lepiej powiedziec to teraz niz zostawic kontener, ktory nie wstaje.
for resource in "_backend"; do
  docker network inspect "$STACK_PROJECT$resource" >/dev/null 2>&1 \
    || die "nie ma sieci $STACK_PROJECT$resource. Uruchom najpierw serwer gry, potem ten instalator."
done
for volume in "_game-var" "_rates-spool" "_update-spool"; do
  docker volume inspect "$STACK_PROJECT$volume" >/dev/null 2>&1 \
    || die "nie ma wolumenu $STACK_PROJECT$volume. Uruchom najpierw serwer gry, potem ten instalator."
done

# --- konfiguracja panelu ------------------------------------------------------
# Klucz sesji zostaje ten sam przy kolejnej instalacji, zeby aktualizacja panelu
# nie wylogowywala operatora.
SECRET=""
if [ -f "$ENV_FILE" ]; then
  SECRET=$(sed -n 's/^TUIKE_SESSION_SECRET=//p' "$ENV_FILE" | tail -n 1)
fi
if [ -z "$SECRET" ]; then
  if [ -r /dev/urandom ]; then
    SECRET=$(head -c 32 /dev/urandom | od -An -tx1 | tr -d ' \n')
  else
    SECRET=$(date +%s%N)$$
  fi
fi

umask 077
cat > "$ENV_FILE" <<ENVEOF
# Plik pisany przez install.sh. Zawiera haslo do bazy - nie wysylaj go nikomu.
TUIKE_PROJECT=$PROJECT
TUIKE_PORT=$PORT
TUIKE_BIND_ADDRESS=$BIND_ADDRESS
TUIKE_SESSION_SECRET=$SECRET
TUIKE_IMAGE=tuike-panel:local
M2_STACK_PROJECT=$STACK_PROJECT
M2_DB_USER=$DB_USER
M2_DB_PASSWORD=$DB_PASSWORD
M2_ENGINE=$ENGINE
M2_TZ=$TZONE
M2_TUIKE_COLLECTOR_INTERVAL=$COLLECTOR
TUIKE_TIERU_PANEL_URL=http://$PUBLIC_ADDRESS:7788
TUIKE_ITEMSHOP_URL=http://$PUBLIC_ADDRESS:7791
TUIKE_SEBAN_PANEL_URL=
TUIKE_VPS_PANEL_URL=
ENVEOF
chmod 600 "$ENV_FILE"

# --- budowa i start ----------------------------------------------------------
say "Buduje obraz panelu (pierwszy raz trwa to kilka minut)..."
$DC --env-file "$ENV_FILE" -f "$COMPOSE_FILE" build
say "Uruchamiam panel..."
$DC --env-file "$ENV_FILE" -f "$COMPOSE_FILE" up -d

# --- start razem z maszyna ----------------------------------------------------
# Sam `restart: unless-stopped` wystarcza, zeby panel wracal po restarcie hosta.
# Usluga systemd dokladana jest po to, zeby dalo sie go tez swiadomie zatrzymac
# i wlaczyc jedna komenda, tak jak reszte uslug serwera.
if command -v systemctl >/dev/null 2>&1 && [ -d /etc/systemd/system ]; then
  sed -e "s#@DIR@#$DIR#g" -e "s#@ENV@#$ENV_FILE#g" -e "s#@COMPOSE@#$COMPOSE_FILE#g" \
      -e "s#@DC@#$DC#g" "$DIR/install/tuike-panel.service" > "$SERVICE_FILE"
  systemctl daemon-reload
  systemctl enable tuike-panel.service >/dev/null 2>&1 || true
  say "Usluga systemd tuike-panel wlaczona (panel wstaje razem z maszyna)."
else
  say "Brak systemd - panel wstaje przy starcie Dockera dzieki restart: unless-stopped."
fi

say ""
say "Gotowe. Panel Tuike dziala pod adresem:"
say "  http://$PUBLIC_ADDRESS:$PORT"
say ""
say "Konfiguracja: $ENV_FILE"
say "Logi:         $DC --env-file $ENV_FILE -f $COMPOSE_FILE logs -f tuike"
say "Zatrzymanie:  $DC --env-file $ENV_FILE -f $COMPOSE_FILE stop"
