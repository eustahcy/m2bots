<#
.SYNOPSIS
    Instalator panelu Tuike dla serwera Metin2 Playerbots.

.DESCRIPTION
    Wgrywa panel na serwer i uruchamia go tak, zeby wstawal razem z nim.
    Dziala w dwoch trybach:

      lokalny - serwer chodzi na tym komputerze (Docker Desktop),
      vps     - serwer chodzi na zdalnej maszynie, laczymy sie przez SSH.

    Warunek: ten folder (tuike-panel) musi lezec w katalogu Serwer, obok
    linux-port. Stamtad skrypt wie, gdzie stoi stos gry i - jesli paczka nie
    ma wlasnej kopii - skad wziac zrodla panelu.

.EXAMPLE
    .\Zainstaluj-Tuike.ps1
    Pyta o tryb i przeprowadza instalacje.

.EXAMPLE
    .\Zainstaluj-Tuike.ps1 -Tryb vps -VpsHost 1.2.3.4 -VpsUser debian
    Instalacja na zdalnym serwerze bez pytan.

.EXAMPLE
    .\Zainstaluj-Tuike.ps1 -Sprawdz
    Nic nie instaluje: sprawdza tylko, czy wszystko jest na swoim miejscu.
#>
[CmdletBinding()]
param(
    [ValidateSet('pytaj', 'lokalny', 'vps')]
    [string]$Tryb = 'pytaj',

    [string]$VpsHost = '',
    [int]$VpsPort = 22,
    [string]$VpsUser = 'debian',
    [string]$VpsPath = '',
    [string]$KluczSsh = '',

    # Katalog Serwer instalacji gry. Domyslnie ten, w ktorym lezy ta paczka;
    # podaje sie go tylko wtedy, gdy na maszynie jest kilka instalacji.
    [string]$Serwer = '',

    [int]$Port = 0,

    # Tylko kontrola: zadnego kopiowania, budowania ani uruchamiania.
    [switch]$Sprawdz,

    # Autotest dla programisty - sprawdza sam skrypt, bez Dockera i bez sieci.
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'
$script:Paczka = Split-Path -Parent $MyInvocation.MyCommand.Path

# --- wypisywanie -------------------------------------------------------------
function Write-Naglowek {
    param([string]$Tekst)
    Write-Host ''
    Write-Host "== $Tekst" -ForegroundColor Cyan
}
function Write-Ok    { param([string]$T) Write-Host "  [OK]   $T" -ForegroundColor Green }
function Write-Info  { param([string]$T) Write-Host "  ...    $T" -ForegroundColor Gray }
function Write-Uwaga { param([string]$T) Write-Host "  [!]    $T" -ForegroundColor Yellow }
function Stop-Z-Bledem {
    <#
        Konczy instalacje jednym zdaniem po polsku. Swiadomie `exit`, a nie
        `throw`: odbiorca tej paczki ma zobaczyc, co zrobic, a nie slad stosu
        PowerShella pod spodem.
    #>
    param([string]$Tekst, [string]$Rada = '')
    Write-Host ''
    Write-Host "  [BLAD] $Tekst" -ForegroundColor Red
    if ($Rada) { Write-Host "         $Rada" -ForegroundColor Yellow }
    Write-Host ''
    exit 1
}

# --- warunek: folder musi lezec w katalogu Serwer ----------------------------
function Get-KatalogSerwer {
    <#
        Katalog Serwer instalacji gry, ktora obslugujemy. Domyslnie rodzic
        paczki (bo taki jest warunek jej polozenia), ale GUI pozwala wskazac
        inny - ktos moze miec dwie instalacje na jednym dysku.
    #>
    param([string]$Wskazany, [string]$Domyslny)

    if (-not $Wskazany) { return $Domyslny }
    $pelny = (Resolve-Path -LiteralPath $Wskazany -ErrorAction SilentlyContinue).Path
    if (-not $pelny) { Stop-Z-Bledem "Nie ma katalogu `"$Wskazany`"." }
    if (-not (Test-Path (Join-Path $pelny 'linux-port\docker'))) {
        Stop-Z-Bledem "W `"$pelny`" nie ma linux-port\docker." `
                      'Wskaz katalog Serwer instalacji gry - ten, w ktorym jest folder linux-port.'
    }
    return $pelny
}

function Test-MiejscePaczki {
    <#
        Zwraca sciezke do katalogu Serwer. Warunek jest twardy, bo z polozenia
        wynika wszystko inne: gdzie jest stos gry i skad wziac zrodla panelu.
    #>
    $rodzic = Split-Path -Parent $script:Paczka
    $nazwaRodzica = Split-Path -Leaf $rodzic
    $nazwaPaczki = Split-Path -Leaf $script:Paczka

    if ($nazwaPaczki -ne 'tuike-panel') {
        Stop-Z-Bledem "Ten folder nazywa sie `"$nazwaPaczki`", a musi nazywac sie `"tuike-panel`"." `
                      'Zmien nazwe folderu i uruchom instalator ponownie.'
    }
    if ($nazwaRodzica -ne 'Serwer') {
        Stop-Z-Bledem "Folder tuike-panel lezy w `"$nazwaRodzica`", a musi lezec w katalogu `"Serwer`"." `
                      'Przenies caly folder tuike-panel do katalogu Serwer (tam, gdzie jest linux-port) i sprobuj jeszcze raz.'
    }
    if (-not (Test-Path (Join-Path $rodzic 'linux-port\docker'))) {
        Stop-Z-Bledem 'W katalogu Serwer nie ma linux-port\docker.' `
                      'To nie wyglada na instalacje serwera Metin2 Playerbots.'
    }
    return $rodzic
}

# --- zrodla panelu -----------------------------------------------------------
function Get-ZrodlaPanelu {
    <#
        Paczka wyslana komus ma wlasny folder panel\. U nas w repozytorium go
        nie ma (te same 22 MB lezalyby dwa razy), wiec bierzemy zrodla z
        linux-port\docker\tuike obok. Dwa miejsca, jedna prawda.
    #>
    param([string]$Serwer)

    $wPaczce = Join-Path $script:Paczka 'panel'
    if (Test-Path (Join-Path $wPaczce 'Dockerfile')) { return $wPaczce }

    $wDrzewie = Join-Path $Serwer 'linux-port\docker\tuike'
    if (Test-Path (Join-Path $wDrzewie 'Dockerfile')) { return $wDrzewie }

    Stop-Z-Bledem 'Nie znalazlem zrodel panelu.' `
                  'Paczka powinna miec folder panel\ z plikiem Dockerfile.'
}

function Get-WersjaPanelu {
    param([string]$Zrodla)
    $plik = Join-Path $Zrodla 'VERSION'
    if (Test-Path $plik) { return (Get-Content $plik -Raw).Trim() }
    return 'nieznana'
}

# --- wspolne ------------------------------------------------------------------
function Read-EnvValue {
    param([string]$Plik, [string]$Klucz)
    if (-not (Test-Path $Plik)) { return '' }
    $linia = Select-String -Path $Plik -Pattern "^$Klucz=" -Encoding utf8 | Select-Object -Last 1
    if (-not $linia) { return '' }
    return $linia.Line.Substring($Klucz.Length + 1).Trim()
}

function Invoke-Cicho {
    <#
        Uruchamia zewnetrzny program i zwraca jego kod wyjscia, nie zamieniajac
        jego wypisow na stderr w wyjatek: przy $ErrorActionPreference = 'Stop'
        kazda linia z docker/ssh staje sie bledem i uzytkownik dostaje sciane
        czerwonego tekstu zamiast jednego zdania po polsku.
    #>
    param([string]$Program, [string[]]$Argumenty, [switch]$Pokazuj)

    $poprzednie = $ErrorActionPreference
    $ErrorActionPreference = 'Continue'
    try {
        # Out-Host, nie zwykle wypisanie: inaczej wyjscie programu wpadloby do
        # potoku razem z kodem wyjscia i wywolujacy dostalby tablice zamiast
        # liczby - udana instalacja wygladalaby wtedy jak blad.
        if ($Pokazuj) { & $Program @Argumenty 2>&1 | Out-Host }
        else { & $Program @Argumenty 2>&1 | Out-Null }
        return $LASTEXITCODE
    } finally {
        $ErrorActionPreference = $poprzednie
    }
}

function Set-HasloSsh {
    <#
        OpenSSH na Windowsie nie przyjmuje hasla z potoku - pyta o nie sam.
        SSH_ASKPASS to jedyna droga, zeby podac je z GUI. Skrypt pomocniczy
        czyta haslo ze zmiennej srodowiskowej, wiec samo haslo nigdy nie
        laduje na dysku; plik .cmd zawiera tylko odwolanie do zmiennej.
    #>
    if (-not $env:TUIKE_SSH_PASSWORD) { return '' }
    $plik = Join-Path $env:TEMP "tuike-askpass-$PID.cmd"
    Set-Content -LiteralPath $plik -Value @('@echo off', 'echo %TUIKE_SSH_PASSWORD%') -Encoding ASCII
    $env:SSH_ASKPASS = $plik
    $env:SSH_ASKPASS_REQUIRE = 'force'
    # Bez DISPLAY starsze OpenSSH nie siegaja po SSH_ASKPASS w ogole.
    if (-not $env:DISPLAY) { $env:DISPLAY = 'localhost:0' }
    return $plik
}

function Clear-HasloSsh {
    param([string]$Plik)
    if ($Plik -and (Test-Path $Plik)) { Remove-Item $Plik -Force -ErrorAction SilentlyContinue }
    Remove-Item Env:\SSH_ASKPASS -ErrorAction SilentlyContinue
    Remove-Item Env:\SSH_ASKPASS_REQUIRE -ErrorAction SilentlyContinue
}

function Test-Program {
    param([string]$Nazwa)
    $znaleziony = Get-Command $Nazwa -ErrorAction SilentlyContinue
    return [bool]$znaleziony
}

# --- instalacja lokalna -------------------------------------------------------
function Install-Lokalnie {
    param([string]$Serwer, [string]$Zrodla)

    $stos = Join-Path $Serwer 'linux-port\docker'
    Write-Naglowek 'Instalacja na tym komputerze'

    if (-not (Test-Program 'docker')) {
        Stop-Z-Bledem 'Nie ma polecenia docker.' 'Zainstaluj Docker Desktop i uruchom go przed instalacja panelu.'
    }
    if ((Invoke-Cicho 'docker' @('version', '--format', '{{.Server.Version}}')) -ne 0) {
        Stop-Z-Bledem 'Docker jest zainstalowany, ale nie odpowiada.' 'Uruchom Docker Desktop i poczekaj, az wstanie.'
    }
    Write-Ok 'Docker odpowiada'

    $projekt = Read-EnvValue (Join-Path $stos '.env') 'M2_COMPOSE_PROJECT_NAME'
    if (-not $projekt) { $projekt = Read-EnvValue (Join-Path $stos '.env') 'M2_CONTAINER_PREFIX' }
    if (-not $projekt) {
        Stop-Z-Bledem 'W linux-port\docker\.env nie ma nazwy projektu.' 'Uruchom raz serwer gry - plik .env powstaje przy instalacji.'
    }
    Write-Ok "Stos gry: $projekt"

    if ((Invoke-Cicho 'docker' @('network', 'inspect', "$($projekt)_backend")) -ne 0) {
        Stop-Z-Bledem "Nie ma sieci $($projekt)_backend." 'Uruchom najpierw serwer gry, potem ten instalator.'
    }
    Write-Ok 'Siec i wolumeny stosu gry istnieja'

    if ($Sprawdz) { Write-Uwaga 'Tryb -Sprawdz: nic nie zostalo zainstalowane.'; return }

    # Linux-owa czesc instalatora robi dokladnie to samo co tutaj, ale u siebie.
    # Na Windowsie nie ma powloki sh, wiec te same kroki sa powtorzone w PS.
    $praca = Join-Path $script:Paczka '.instalacja'
    New-Item -ItemType Directory -Force -Path $praca | Out-Null
    $envFile = Join-Path $praca 'tuike.env'

    $sekret = Read-EnvValue $envFile 'TUIKE_SESSION_SECRET'
    if (-not $sekret) {
        $bajty = New-Object byte[] 32
        [Security.Cryptography.RandomNumberGenerator]::Create().GetBytes($bajty)
        $sekret = -join ($bajty | ForEach-Object { $_.ToString('x2') })
    }

    $port = $Port
    if ($port -le 0) {
        $port = [int](Read-EnvValue (Join-Path $stos '.env') 'M2_TUIKE_PANEL_PORT')
        if ($port -le 0) { $port = 7799 }
    }

    $silnik = 'mt2009'
    $plikSilnika = Join-Path $stos 'ENGINE'
    if (Test-Path $plikSilnika) { $silnik = (Get-Content $plikSilnika -Raw).Trim() }

    $haslo = Read-EnvValue (Join-Path $stos '.env') 'M2_DB_PASSWORD'
    if (-not $haslo) {
        Stop-Z-Bledem 'W .env stosu gry nie ma M2_DB_PASSWORD.' 'Bez hasla panel nie polaczy sie z baza.'
    }
    $uzytkownik = Read-EnvValue (Join-Path $stos '.env') 'M2_DB_USER'
    if (-not $uzytkownik) { $uzytkownik = 'metin2' }
    $strefa = Read-EnvValue (Join-Path $stos '.env') 'M2_TZ'
    if (-not $strefa) { $strefa = 'UTC' }
    $adres = Read-EnvValue (Join-Path $stos '.env') 'M2_PUBLIC_ADDRESS'
    if (-not $adres) { $adres = '127.0.0.1' }

    $tresc = @(
        "TUIKE_PROJECT=tuike-panel",
        "TUIKE_PORT=$port",
        "TUIKE_BIND_ADDRESS=127.0.0.1",
        "TUIKE_SESSION_SECRET=$sekret",
        "TUIKE_IMAGE=tuike-panel:local",
        "M2_STACK_PROJECT=$projekt",
        "M2_DB_USER=$uzytkownik",
        "M2_DB_PASSWORD=$haslo",
        "M2_ENGINE=$silnik",
        "M2_TZ=$strefa",
        "TUIKE_TIERU_PANEL_URL=http://127.0.0.1:7788",
        "TUIKE_ITEMSHOP_URL=http://127.0.0.1:7791"
    ) -join "`n"
    Set-Content -Path $envFile -Value $tresc -Encoding UTF8 -NoNewline

    # Compose buduje z katalogu ..\panel wzgledem pliku compose, wiec zrodla
    # musza lezec obok. Gdy bierzemy je z drzewa, robimy tymczasowa kopie.
    $panelWPaczce = Join-Path $script:Paczka 'panel'
    $kopiaTymczasowa = $false
    if ($Zrodla -ne $panelWPaczce) {
        Write-Info 'Kopiuje zrodla panelu z linux-port\docker\tuike'
        if (Test-Path $panelWPaczce) { Remove-Item $panelWPaczce -Recurse -Force }
        Copy-Item $Zrodla $panelWPaczce -Recurse
        Get-ChildItem $panelWPaczce -Recurse -Directory -Filter '__pycache__' |
            Remove-Item -Recurse -Force
        $kopiaTymczasowa = $true
    }

    try {
        $compose = Join-Path $script:Paczka 'install\docker-compose.tuike.yml'
        Write-Info 'Buduje obraz panelu (pierwszy raz trwa to kilka minut)'
        $kod = Invoke-Cicho 'docker' @('compose', '--env-file', $envFile, '-f', $compose, 'build') -Pokazuj
        if ($kod -ne 0) { Stop-Z-Bledem 'Budowa obrazu panelu nie powiodla sie.' }
        Write-Info 'Uruchamiam panel'
        $kod = Invoke-Cicho 'docker' @('compose', '--env-file', $envFile, '-f', $compose, 'up', '-d') -Pokazuj
        if ($kod -ne 0) { Stop-Z-Bledem 'Nie udalo sie uruchomic kontenerow panelu.' }
    } finally {
        if ($kopiaTymczasowa -and (Test-Path $panelWPaczce)) { Remove-Item $panelWPaczce -Recurse -Force }
    }

    Write-Ok "Panel dziala: http://127.0.0.1:$port"
    Write-Info 'Kontenery maja restart: unless-stopped, wiec wstaja razem z Docker Desktop.'
    return "http://127.0.0.1:$port"
}

# --- instalacja na VPS --------------------------------------------------------
function Install-NaVps {
    param([string]$Serwer, [string]$Zrodla)

    Write-Naglowek 'Instalacja na zdalnym serwerze'

    if (-not (Test-Program 'ssh') -or -not (Test-Program 'scp')) {
        Stop-Z-Bledem 'Brakuje ssh lub scp.' 'Windows: Ustawienia > Aplikacje > Funkcje opcjonalne > Klient OpenSSH.'
    }

    $cel = $VpsHost
    if (-not $cel) {
        $zapisane = Join-Path $Serwer '.m2vps-deploy.json'
        if (Test-Path $zapisane) {
            $dane = Get-Content $zapisane -Raw | ConvertFrom-Json
            if ($dane.host) { $cel = $dane.host }
            if ($dane.user -and -not $PSBoundParameters.ContainsKey('VpsUser')) { $script:VpsUser = $dane.user }
        }
    }
    if (-not $cel) { $cel = Read-Host 'Adres serwera (IP albo domena)' }
    if (-not $cel) { Stop-Z-Bledem 'Bez adresu serwera nie ma gdzie instalowac.' }

    $sciezkaZdalna = $VpsPath
    if (-not $sciezkaZdalna) { $sciezkaZdalna = '/opt/tuike-panel' }

    # Wspolne opcje: port, ewentualny klucz i zgoda na nieznany jeszcze serwer
    # (bez tego pierwsze polaczenie zatrzymuje sie na pytaniu yes/no, ktorego
    # w okienku nikt nie zobaczy).
    $sshArgs = @('-o', 'StrictHostKeyChecking=accept-new', '-o', 'ConnectTimeout=20')
    if ($KluczSsh) { $sshArgs += @('-i', $KluczSsh) }
    elseif (-not $env:TUIKE_SSH_PASSWORD) {
        $domyslnyKlucz = Join-Path $env:USERPROFILE '.ssh\m2_vps_deploy_ed25519'
        if (Test-Path $domyslnyKlucz) { $sshArgs += @('-i', $domyslnyKlucz) }
    }
    $sshOnly = $sshArgs + @('-p', "$VpsPort")
    $scpOnly = $sshArgs + @('-P', "$VpsPort")
    $askpass = Set-HasloSsh
    $polaczenie = "$VpsUser@$cel"

    Write-Info "Sprawdzam polaczenie z $polaczenie"
    $kod = Invoke-Cicho 'ssh' ($sshOnly + @($polaczenie, 'echo TUIKE_SSH_OK'))
    if ($kod -ne 0) {
        Clear-HasloSsh $askpass
        Stop-Z-Bledem "Nie moge zalogowac sie na $polaczenie (port $VpsPort)." `
                      'Sprawdz adres, port, uzytkownika i haslo albo klucz SSH.'
    }
    Write-Ok 'Polaczenie dziala'

    if ($Sprawdz) {
        Clear-HasloSsh $askpass
        Write-Uwaga 'Tryb -Sprawdz: nic nie zostalo wyslane.'
        return
    }

    # Pakujemy tylko to, co potrzebne: zrodla panelu i pliki instalacyjne.
    $tymczasowy = Join-Path ([IO.Path]::GetTempPath()) ("tuike-panel-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
    New-Item -ItemType Directory -Force -Path (Join-Path $tymczasowy 'install') | Out-Null
    Copy-Item $Zrodla (Join-Path $tymczasowy 'panel') -Recurse
    Copy-Item (Join-Path $script:Paczka 'install\*') (Join-Path $tymczasowy 'install') -Recurse
    Get-ChildItem $tymczasowy -Recurse -Directory -Filter '__pycache__' | Remove-Item -Recurse -Force

    $archiwum = Join-Path ([IO.Path]::GetTempPath()) 'tuike-panel.tar.gz'
    if (Test-Path $archiwum) { Remove-Item $archiwum -Force }
    Write-Info 'Pakuje panel do wyslania'
    $kod = Invoke-Cicho 'tar' @('-czf', $archiwum, '-C', $tymczasowy, '.')
    if ($kod -ne 0) { Stop-Z-Bledem 'Nie udalo sie spakowac panelu (tar).' }

    $rozmiar = [math]::Round((Get-Item $archiwum).Length / 1MB, 1)
    Write-Info "Wysylam paczke ($rozmiar MB) na serwer"
    $kod = Invoke-Cicho 'scp' ($scpOnly + @('-q', $archiwum, "$($polaczenie):/tmp/tuike-panel.tar.gz"))
    if ($kod -ne 0) { Stop-Z-Bledem 'Nie udalo sie wyslac paczki na serwer.' }

    $portArg = ''
    if ($Port -gt 0) { $portArg = "--port $Port" }
    $komenda = @(
        "sudo rm -rf $sciezkaZdalna",
        "sudo mkdir -p $sciezkaZdalna",
        "sudo tar -xzf /tmp/tuike-panel.tar.gz -C $sciezkaZdalna",
        "rm -f /tmp/tuike-panel.tar.gz",
        "sudo sh $sciezkaZdalna/install/install.sh $portArg"
    ) -join ' && '

    Write-Info 'Instaluje panel na serwerze'
    $kod = Invoke-Cicho 'ssh' ($sshOnly + @($polaczenie, $komenda)) -Pokazuj
    Clear-HasloSsh $askpass
    if ($kod -ne 0) {
        Stop-Z-Bledem 'Instalator na serwerze zglosil blad.' 'Powyzej jest jego wlasny komunikat - on mowi, czego zabraklo.'
    }

    Remove-Item $tymczasowy -Recurse -Force -ErrorAction SilentlyContinue
    Remove-Item $archiwum -Force -ErrorAction SilentlyContinue
    Write-Ok 'Panel zainstalowany na serwerze'
    return ''
}

# --- autotest -----------------------------------------------------------------
function Invoke-SelfTest {
    Write-Naglowek 'Autotest instalatora'
    $bledy = @()

    $wymagane = @(
        'install\docker-compose.tuike.yml',
        'install\install.sh',
        'install\tuike-panel.service',
        'instrukcja.md',
        'Zainstaluj-Tuike.bat'
    )
    foreach ($plik in $wymagane) {
        if (Test-Path (Join-Path $script:Paczka $plik)) { Write-Ok "jest $plik" }
        else { $bledy += "brakuje pliku $plik" }
    }

    # install.sh musi miec konce linii LF - z CR nie uruchomi sie na Linuksie.
    $sh = Join-Path $script:Paczka 'install\install.sh'
    if (Test-Path $sh) {
        $bajty = [IO.File]::ReadAllBytes($sh)
        if ($bajty -contains 13) { $bledy += 'install.sh ma konce linii CRLF - Linux tego nie uruchomi' }
        else { Write-Ok 'install.sh ma konce linii LF' }
        if ($bajty.Length -gt 3 -and $bajty[0] -eq 239) { $bledy += 'install.sh ma BOM - sh tego nie znosi' }
    }

    $nazwaPaczki = Split-Path -Leaf $script:Paczka
    if ($nazwaPaczki -eq 'tuike-panel') { Write-Ok 'folder nazywa sie tuike-panel' }
    else { $bledy += "folder nazywa sie $nazwaPaczki" }

    Write-Host ''
    if ($bledy.Count -gt 0) {
        foreach ($blad in $bledy) { Write-Host "  [BLAD] $blad" -ForegroundColor Red }
        exit 1
    }
    Write-Host '  Autotest przeszedl.' -ForegroundColor Green
    exit 0
}

# --- przebieg -----------------------------------------------------------------
Write-Host ''
Write-Host '  ____  Panel Tuike' -ForegroundColor Cyan
Write-Host '  instalator dla serwera Metin2 Playerbots' -ForegroundColor DarkGray

if ($SelfTest) { Invoke-SelfTest }

$rodzicPaczki = Test-MiejscePaczki
$serwer = Get-KatalogSerwer -Wskazany $Serwer -Domyslny $rodzicPaczki
Write-Naglowek 'Sprawdzam paczke'
Write-Ok "Katalog Serwer: $serwer"
$zrodla = Get-ZrodlaPanelu -Serwer $serwer
Write-Ok "Zrodla panelu: $zrodla (wersja $(Get-WersjaPanelu -Zrodla $zrodla))"

$wybrany = $Tryb
if ($wybrany -eq 'pytaj') {
    Write-Host ''
    Write-Host '  Gdzie stoi serwer gry?' -ForegroundColor White
    Write-Host '    1) na tym komputerze (Docker Desktop)'
    Write-Host '    2) na zdalnym serwerze (VPS, przez SSH)'
    $odpowiedz = Read-Host '  Wybierz 1 albo 2'
    if ($odpowiedz -eq '2') { $wybrany = 'vps' } else { $wybrany = 'lokalny' }
}

$adresPanelu = ''
if ($wybrany -eq 'vps') { $adresPanelu = Install-NaVps -Serwer $serwer -Zrodla $zrodla }
else { $adresPanelu = Install-Lokalnie -Serwer $serwer -Zrodla $zrodla }

if (-not $Sprawdz) {
    Write-Naglowek 'Gotowe'
    Write-Host '  Panel Tuike jest zainstalowany i bedzie wstawal razem z serwerem.' -ForegroundColor Green
    if ($adresPanelu) {
        Write-Host "  Adres: $adresPanelu" -ForegroundColor Green
        $otworz = Read-Host '  Otworzyc panel w przegladarce? [t/n]'
        if ($otworz -eq 't') { Start-Process $adresPanelu }
    }
    Write-Host '  Reszta - w pliku instrukcja.md obok tego skryptu.' -ForegroundColor Gray
}
Write-Host ''
