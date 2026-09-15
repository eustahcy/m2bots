<#
.SYNOPSIS
    Wypycha zmiany z tego PC do repozytorium GitHub, z którego aktualizuje się VPS.

.DESCRIPTION
    Jedna komenda zamiast pięciu: sprawdza, czy nie wypychasz sekretu albo pliku
    większego niż GitHub przyjmuje, commituje i pushuje. Repozytorium ma korzeń
    w folderze Serwer\ — to samo drzewo, które na VPS leży w
    /home/debian/metin2-playerbots, więc ścieżki po obu stronach są identyczne.

    Pierwsze uruchomienie (zakłada repo lokalnie i wypycha całość):
        .\tools\Publish-ToGit.ps1 -RemoteUrl https://github.com/user/repo.git -Message "Pierwszy import"

    Każde kolejne:
        .\tools\Publish-ToGit.ps1 -Message "Boty wchodza glebiej w loch malp"

    Potem na panelu VPS (port 9797): Sprawdź aktualizacje → Zastosuj aktualizację.

.NOTES
    Skrypt nie robi `git pull`. Jeśli ktoś commitował bezpośrednio na GitHubie,
    push zostanie odrzucony i trzeba to rozwiązać ręcznie — celowo, bo
    automatyczny merge w drzewie z 40 tysiącami plików to zły pomysł.
#>
[CmdletBinding()]
param(
    [Parameter(Mandatory = $true)]
    [string]$Message,

    # Potrzebny tylko przy pierwszym uruchomieniu, kiedy nie ma jeszcze .git.
    [string]$RemoteUrl = '',

    [string]$Branch = 'main',

    # Pokaż, co poszłoby na GitHuba, i zakończ bez commita i pusha.
    [switch]$WhatIfOnly
)

$ErrorActionPreference = 'Stop'

# Korzeń repozytorium to folder Serwer\ — o jeden wyżej niż tools\.
$repoRoot = Split-Path -Parent $PSScriptRoot

function Find-Git {
    # PATH w bieżącej sesji bywa starszy niż instalacja gita (np. tuż po
    # winget install), więc zaglądamy też w standardowe lokalizacje.
    $command = Get-Command git -ErrorAction SilentlyContinue
    if ($command) { return $command.Source }
    foreach ($candidate in @(
            "$env:ProgramFiles\Git\cmd\git.exe",
            "${env:ProgramFiles(x86)}\Git\cmd\git.exe",
            "$env:LOCALAPPDATA\Programs\Git\cmd\git.exe")) {
        if (Test-Path -LiteralPath $candidate) { return $candidate }
    }
    throw 'Nie znaleziono gita. Zainstaluj: winget install --id Git.Git -e'
}

$git = Find-Git

function Invoke-Git {
    # Natywny exe pod $ErrorActionPreference='Stop': stderr gita (nawet zwykłe
    # "Switched to branch") potrafi zostać podniesione do wyjątku, więc na czas
    # wywołania rozluźniamy preferencję i patrzymy wyłącznie na kod wyjścia.
    param([string[]]$Arguments, [switch]$AllowFailure)
    $previous = $ErrorActionPreference
    try {
        $ErrorActionPreference = 'Continue'
        $output = & $git -C $repoRoot @Arguments 2>&1
    }
    finally { $ErrorActionPreference = $previous }
    if ($LASTEXITCODE -ne 0 -and -not $AllowFailure) {
        throw "git $($Arguments -join ' ') zakończyło się kodem $LASTEXITCODE`n$($output -join "`n")"
    }
    return @{ Code = $LASTEXITCODE; Output = ($output | ForEach-Object { [string]$_ }) }
}

Write-Host "Repozytorium: $repoRoot" -ForegroundColor Cyan

# --- inicjalizacja przy pierwszym uruchomieniu --------------------------------
$isRepo = Test-Path -LiteralPath (Join-Path $repoRoot '.git')
if (-not $isRepo) {
    if (-not $RemoteUrl) {
        throw 'To jeszcze nie jest repozytorium. Podaj -RemoteUrl https://github.com/user/repo.git'
    }
    Write-Host 'Zakładam repozytorium lokalne...' -ForegroundColor Cyan
    Invoke-Git @('init', '-q', '-b', $Branch) | Out-Null
}

# --- adres zdalny -------------------------------------------------------------
# Osobno od inicjalizacji, bo repozytorium może już istnieć BEZ origin -- tak
# wygląda drzewo, w którym `git init` zrobiono ręcznie albo wcześniejszym
# przebiegiem tego skryptu bez adresu. `remote set-url` na nieistniejącym
# remote kończy się błędem, więc trzeba rozróżnić "dodaj" od "zmień".
if ($RemoteUrl) {
    $origin = Invoke-Git @('remote', 'get-url', 'origin') -AllowFailure
    if ($origin.Code -ne 0) {
        Invoke-Git @('remote', 'add', 'origin', $RemoteUrl) | Out-Null
        Write-Host "Dodałem adres zdalny: $RemoteUrl" -ForegroundColor Cyan
    }
    elseif (($origin.Output -join '').Trim() -ne $RemoteUrl) {
        Write-Host "Zmieniam adres zdalny na $RemoteUrl" -ForegroundColor Yellow
        Invoke-Git @('remote', 'set-url', 'origin', $RemoteUrl) | Out-Null
    }
}
elseif ((Invoke-Git @('remote', 'get-url', 'origin') -AllowFailure).Code -ne 0) {
    throw 'To repozytorium nie ma adresu zdalnego. Podaj go raz: -RemoteUrl https://github.com/user/repo.git'
}

# Bez user.name/user.email commit się nie uda, a komunikat gita w tej sytuacji
# jest długi i mało pomocny. Ustawiamy je tylko dla tego repozytorium i tylko
# jeśli nie ma ich już w konfiguracji globalnej — czyjeś globalne ustawienia
# zostają nietknięte.
if ((Invoke-Git @('config', 'user.email') -AllowFailure).Code -ne 0) {
    Invoke-Git @('config', 'user.email', 'przylepadamian@gmail.com') | Out-Null
    Invoke-Git @('config', 'user.name', 'Damian') | Out-Null
    Write-Host 'Ustawiłem tożsamość commitów dla tego repozytorium.' -ForegroundColor DarkGray
}

# --- staging ------------------------------------------------------------------
Invoke-Git @('add', '-A') | Out-Null

$staged = (Invoke-Git @('diff', '--cached', '--name-only')).Output |
    Where-Object { $_ -and $_.Trim() }

if (-not $staged -or $staged.Count -eq 0) {
    Write-Host 'Nie ma żadnych zmian do wypchnięcia.' -ForegroundColor Yellow
    # Wciąż warto sprawdzić, czy lokalne commity nie czekają na push.
    $unpushed = (Invoke-Git @('log', '--oneline', "origin/$Branch..HEAD") -AllowFailure).Output |
        Where-Object { $_ -and $_.Trim() }
    if ($unpushed -and $unpushed.Count -gt 0) {
        Write-Host "Ale $($unpushed.Count) commitów czeka na wysłanie — wypycham." -ForegroundColor Cyan
    }
    else { return }
}
else {
    Write-Host "Plików do wysłania: $($staged.Count)" -ForegroundColor Cyan
}

# --- bezpieczniki -------------------------------------------------------------
# .gitignore powinien to złapać, ale ten skrypt jest ostatnim miejscem, w którym
# da się zatrzymać sekret przed publicznym repo. Taniej sprawdzić dwa razy.
$secretPatterns = @('linux-port/docker/.env', '.m2launcher.json', '.m2vps-deploy.json', 'm2panel.conf')
$leaked = $staged | Where-Object {
    $name = $_
    ($secretPatterns | Where-Object { $name -eq $_ -or $name -like "*/$_" }).Count -gt 0
}
if ($leaked) {
    throw ("Do commita trafiły pliki, które nigdy nie mogą wyjść na zewnątrz:`n  " +
           ($leaked -join "`n  ") +
           "`nSprawdź Serwer\.gitignore. Nic nie wysłano.")
}

# GitHub odrzuca pojedyncze pliki powyżej 100 MB i ostrzega powyżej 50 MB.
# Odrzucony push po przesłaniu kilkuset megabajtów to strata kwadransa.
$tooBig = foreach ($path in $staged) {
    $full = Join-Path $repoRoot $path
    if (Test-Path -LiteralPath $full -PathType Leaf) {
        $size = (Get-Item -LiteralPath $full).Length
        if ($size -gt 95MB) { "{0} ({1:N0} MB)" -f $path, ($size / 1MB) }
    }
}
if ($tooBig) {
    throw ("Te pliki są za duże dla GitHuba (limit 100 MB):`n  " + ($tooBig -join "`n  ") +
           "`nDopisz je do .gitignore. Nic nie wysłano.")
}

if ($WhatIfOnly) {
    Write-Host "`n--- poszłoby na GitHuba (pierwsze 50) ---" -ForegroundColor Yellow
    $staged | Select-Object -First 50 | ForEach-Object { "  $_" }
    if ($staged.Count -gt 50) { Write-Host "  … i $($staged.Count - 50) więcej" }
    Write-Host "`nNic nie zacommitowano ani nie wysłano (-WhatIfOnly)." -ForegroundColor Yellow
    return
}

# --- commit + push ------------------------------------------------------------
if ($staged.Count -gt 0) {
    $commit = Invoke-Git @('commit', '-m', $Message) -AllowFailure
    if ($commit.Code -ne 0) {
        # "nothing to commit" nie jest błędem, jeśli czekają wcześniejsze commity.
        $text = $commit.Output -join "`n"
        if ($text -notmatch 'nothing to commit') { throw "Commit nie przeszedł:`n$text" }
    }
}

Write-Host "Wysyłam na origin/$Branch..." -ForegroundColor Cyan
$push = Invoke-Git @('push', '-u', 'origin', $Branch) -AllowFailure
Write-Host ($push.Output -join [Environment]::NewLine)
if ($push.Code -ne 0) {
    throw ("Push nie przeszedł. Najczęstsze powody: brak uprawnień do repo, " +
           "albo ktoś commitował bezpośrednio na GitHubie (wtedy trzeba to " +
           "pogodzić ręcznie: git pull --rebase).")
}

$head = (Invoke-Git @('rev-parse', '--short', 'HEAD')).Output -join ''
Write-Host ""
Write-Host "Wysłano jako $head." -ForegroundColor Green
Write-Host 'Teraz na panelu VPS (port 9797): „Sprawdź aktualizacje" → „Zastosuj aktualizację".' -ForegroundColor Green
