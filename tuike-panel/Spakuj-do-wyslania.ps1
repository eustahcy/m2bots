<#
.SYNOPSIS
    Robi ZIP z panelem Tuike, gotowy do wyslania innemu operatorowi.

.DESCRIPTION
    W repozytorium folder tuike-panel nie ma wlasnej kopii zrodel panelu -
    te same 22 MB lezalyby wtedy dwa razy i po tygodniu rozjechalyby sie
    miedzy soba. Zrodla dokladane sa dopiero tutaj, przy pakowaniu, prosto
    z linux-port\docker\tuike.

    Wynik: tuike-panel-<wersja>.zip. Odbiorca rozpakowuje go do swojego
    katalogu Serwer i uruchamia Zainstaluj-Tuike.bat.

.EXAMPLE
    .\Spakuj-do-wyslania.ps1
#>
[CmdletBinding()]
param(
    [string]$Wynik = ''
)

$ErrorActionPreference = 'Stop'
$paczka = Split-Path -Parent $MyInvocation.MyCommand.Path
$serwer = Split-Path -Parent $paczka
$zrodla = Join-Path $serwer 'linux-port\docker\tuike'

if (-not (Test-Path (Join-Path $zrodla 'Dockerfile'))) {
    throw "Nie ma zrodel panelu w $zrodla - uruchom ten skrypt z drzewa serwera."
}

$wersja = (Get-Content (Join-Path $zrodla 'VERSION') -Raw).Trim()
if (-not $Wynik) { $Wynik = Join-Path $serwer "tuike-panel-$wersja.zip" }

$budowa = Join-Path ([IO.Path]::GetTempPath()) ("tuike-pack-" + [Guid]::NewGuid().ToString('N').Substring(0, 8))
$docelowy = Join-Path $budowa 'tuike-panel'
New-Item -ItemType Directory -Force -Path $docelowy | Out-Null

Write-Host "Pakuje panel Tuike $wersja" -ForegroundColor Cyan

# Pliki instalatora - bez tego skryptu, odbiorcy nie jest do niczego potrzebny.
foreach ($plik in @('Zainstaluj-Tuike.ps1', 'Zainstaluj-Tuike.bat',
                    'Zainstaluj-Tuike-GUI.ps1', 'Zainstaluj-Tuike-GUI.bat',
                    'instrukcja.md')) {
    Copy-Item (Join-Path $paczka $plik) (Join-Path $docelowy $plik)
}
Copy-Item (Join-Path $paczka 'install') (Join-Path $docelowy 'install') -Recurse
Copy-Item (Join-Path $paczka 'obrazy') (Join-Path $docelowy 'obrazy') -Recurse

# Zrodla panelu, bez smieci Pythona.
Copy-Item $zrodla (Join-Path $docelowy 'panel') -Recurse
Get-ChildItem (Join-Path $docelowy 'panel') -Recurse -Directory -Filter '__pycache__' |
    Remove-Item -Recurse -Force
Get-ChildItem (Join-Path $docelowy 'panel') -Recurse -File -Include '*.pyc', '.env' |
    Remove-Item -Force

if (Test-Path $Wynik) { Remove-Item $Wynik -Force }
Compress-Archive -Path $docelowy -DestinationPath $Wynik
Remove-Item $budowa -Recurse -Force

$rozmiar = [math]::Round((Get-Item $Wynik).Length / 1MB, 1)
Write-Host "Gotowe: $Wynik ($rozmiar MB)" -ForegroundColor Green
Write-Host 'Odbiorca rozpakowuje ten plik do swojego katalogu Serwer i uruchamia Zainstaluj-Tuike-GUI.bat.' -ForegroundColor Gray
