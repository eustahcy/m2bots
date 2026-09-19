<#
.SYNOPSIS
    Okienkowy instalator panelu Tuike.

.DESCRIPTION
    Nakladka na Zainstaluj-Tuike.ps1: to samo instalowanie, tylko z myszka.
    Cala logika instalacji zostaje w skrypcie konsolowym - okno uruchamia go
    jako osobny proces i pokazuje jego wypisy na zywo. Dzieki temu jest jedna
    implementacja instalacji, a nie dwie rozjezdzajace sie.

    Tryby:
      * na tym komputerze - wskazujesz katalog Serwer instalacji gry,
      * na serwerze (VPS) - podajesz host, port SSH, login, haslo i katalog,
        do ktorego panel ma trafic.

.EXAMPLE
    .\Zainstaluj-Tuike-GUI.ps1

.EXAMPLE
    .\Zainstaluj-Tuike-GUI.ps1 -SelfTest
    Buduje okno bez pokazywania go i sprawdza, czy ma wszystkie kontrolki.

.EXAMPLE
    .\Zainstaluj-Tuike-GUI.ps1 -Zrzut C:	emp\instalator.png
    Zapisuje obraz okna (oba tryby) do pliku - do instrukcji albo do zgloszenia.
#>
[CmdletBinding()]
param(
    [switch]$SelfTest,
    [string]$Zrzut = ''
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[System.Windows.Forms.Application]::EnableVisualStyles()

$script:Paczka = Split-Path -Parent $MyInvocation.MyCommand.Path
$script:Konsola = Join-Path $script:Paczka 'Zainstaluj-Tuike.ps1'
$script:Proces = $null

# --- barwy okna ---------------------------------------------------------------
$kolorTlo = [System.Drawing.Color]::FromArgb(13, 22, 41)
$kolorPola = [System.Drawing.Color]::FromArgb(19, 30, 53)
$kolorTekst = [System.Drawing.Color]::FromArgb(238, 243, 253)
$kolorSzary = [System.Drawing.Color]::FromArgb(174, 189, 214)
$kolorAkcent = [System.Drawing.Color]::FromArgb(59, 130, 246)
$fontZwykly = New-Object System.Drawing.Font('Segoe UI', 9)
$fontTytul = New-Object System.Drawing.Font('Segoe UI', 15, [System.Drawing.FontStyle]::Bold)
$fontMono = New-Object System.Drawing.Font('Consolas', 8.5)

function New-Etykieta {
    param([string]$Tekst, [int]$X, [int]$Y, [int]$Szerokosc = 150, [System.Drawing.Color]$Kolor = $kolorSzary)
    $etykieta = New-Object System.Windows.Forms.Label
    $etykieta.Text = $Tekst
    $etykieta.Location = New-Object System.Drawing.Point($X, $Y)
    $etykieta.Size = New-Object System.Drawing.Size($Szerokosc, 18)
    $etykieta.ForeColor = $Kolor
    $etykieta.Font = $fontZwykly
    return $etykieta
}

function New-Pole {
    param([int]$X, [int]$Y, [int]$Szerokosc, [string]$Wartosc = '', [switch]$Haslo)
    $pole = New-Object System.Windows.Forms.TextBox
    $pole.Location = New-Object System.Drawing.Point($X, $Y)
    $pole.Size = New-Object System.Drawing.Size($Szerokosc, 24)
    $pole.Text = $Wartosc
    $pole.BackColor = $kolorPola
    $pole.ForeColor = $kolorTekst
    $pole.BorderStyle = 'FixedSingle'
    $pole.Font = $fontZwykly
    if ($Haslo) { $pole.UseSystemPasswordChar = $true }
    return $pole
}

function New-Przycisk {
    param([string]$Tekst, [int]$X, [int]$Y, [int]$Szerokosc = 120, [switch]$Glowny)
    $przycisk = New-Object System.Windows.Forms.Button
    $przycisk.Text = $Tekst
    $przycisk.Location = New-Object System.Drawing.Point($X, $Y)
    $przycisk.Size = New-Object System.Drawing.Size($Szerokosc, 32)
    $przycisk.FlatStyle = 'Flat'
    $przycisk.Font = $fontZwykly
    if ($Glowny) {
        $przycisk.BackColor = $kolorAkcent
        $przycisk.ForeColor = [System.Drawing.Color]::White
        $przycisk.FlatAppearance.BorderSize = 0
    } else {
        $przycisk.BackColor = $kolorPola
        $przycisk.ForeColor = $kolorTekst
        $przycisk.FlatAppearance.BorderColor = [System.Drawing.Color]::FromArgb(45, 66, 112)
    }
    return $przycisk
}

# --- okno ----------------------------------------------------------------------
$okno = New-Object System.Windows.Forms.Form
$okno.Text = 'Panel Tuike — instalator'
$okno.Size = New-Object System.Drawing.Size(720, 660)
$okno.StartPosition = 'CenterScreen'
$okno.BackColor = $kolorTlo
$okno.ForeColor = $kolorTekst
$okno.FormBorderStyle = 'FixedSingle'
$okno.MaximizeBox = $false

$tytul = New-Object System.Windows.Forms.Label
$tytul.Text = 'Panel Tuike'
$tytul.Location = New-Object System.Drawing.Point(24, 18)
$tytul.Size = New-Object System.Drawing.Size(400, 30)
$tytul.Font = $fontTytul
$tytul.ForeColor = $kolorTekst
$okno.Controls.Add($tytul)
$okno.Controls.Add((New-Etykieta 'Instalator panelu dla serwera Metin2 Playerbots' 26 50 420))

# --- wybor trybu ---------------------------------------------------------------
$grupaTryb = New-Object System.Windows.Forms.GroupBox
$grupaTryb.Text = 'Gdzie stoi serwer gry?'
$grupaTryb.Location = New-Object System.Drawing.Point(24, 80)
$grupaTryb.Size = New-Object System.Drawing.Size(660, 60)
$grupaTryb.ForeColor = $kolorSzary
$grupaTryb.Font = $fontZwykly

$trybLokalny = New-Object System.Windows.Forms.RadioButton
$trybLokalny.Text = 'Na tym komputerze (Docker Desktop)'
$trybLokalny.Location = New-Object System.Drawing.Point(16, 25)
$trybLokalny.Size = New-Object System.Drawing.Size(280, 22)
$trybLokalny.Checked = $true
$trybLokalny.ForeColor = $kolorTekst

$trybVps = New-Object System.Windows.Forms.RadioButton
$trybVps.Text = 'Na serwerze (VPS, przez SSH)'
$trybVps.Location = New-Object System.Drawing.Point(320, 25)
$trybVps.Size = New-Object System.Drawing.Size(280, 22)
$trybVps.ForeColor = $kolorTekst

$grupaTryb.Controls.AddRange(@($trybLokalny, $trybVps))
$okno.Controls.Add($grupaTryb)

# --- ustawienia lokalne ---------------------------------------------------------
$grupaLokalna = New-Object System.Windows.Forms.GroupBox
$grupaLokalna.Text = 'Instalacja na tym komputerze'
$grupaLokalna.Location = New-Object System.Drawing.Point(24, 150)
$grupaLokalna.Size = New-Object System.Drawing.Size(660, 105)
$grupaLokalna.ForeColor = $kolorSzary
$grupaLokalna.Font = $fontZwykly

$grupaLokalna.Controls.Add((New-Etykieta 'Katalog Serwer instalacji gry' 16 28 240))
$poleSerwer = New-Pole 16 48 500 (Split-Path -Parent $script:Paczka)
$grupaLokalna.Controls.Add($poleSerwer)
$przyciskWybierz = New-Przycisk 'Wybierz…' 524 46 110
$grupaLokalna.Controls.Add($przyciskWybierz)
$grupaLokalna.Controls.Add((New-Etykieta 'Ten, w którym jest folder linux-port. Zwykle nie trzeba nic zmieniać.' 16 76 520))
$okno.Controls.Add($grupaLokalna)

# --- ustawienia VPS -------------------------------------------------------------
$grupaVps = New-Object System.Windows.Forms.GroupBox
$grupaVps.Text = 'Instalacja na serwerze (VPS)'
$grupaVps.Location = New-Object System.Drawing.Point(24, 150)
$grupaVps.Size = New-Object System.Drawing.Size(660, 175)
$grupaVps.ForeColor = $kolorSzary
$grupaVps.Font = $fontZwykly
$grupaVps.Visible = $false

$grupaVps.Controls.Add((New-Etykieta 'Adres serwera (host lub IP)' 16 26 220))
$poleHost = New-Pole 16 46 330
$grupaVps.Controls.Add($poleHost)

$grupaVps.Controls.Add((New-Etykieta 'Port SSH' 360 26 120))
$polePortSsh = New-Pole 360 46 90 '22'
$grupaVps.Controls.Add($polePortSsh)

$grupaVps.Controls.Add((New-Etykieta 'Port panelu' 466 26 120))
$polePortPanelu = New-Pole 466 46 90 '7799'
$grupaVps.Controls.Add($polePortPanelu)

$grupaVps.Controls.Add((New-Etykieta 'Login' 16 80 220))
$poleLogin = New-Pole 16 100 200 'root'
$grupaVps.Controls.Add($poleLogin)

$grupaVps.Controls.Add((New-Etykieta 'Hasło' 230 80 220))
$poleHaslo = New-Pole 230 100 200 -Haslo
$grupaVps.Controls.Add($poleHaslo)

$grupaVps.Controls.Add((New-Etykieta 'Katalog na serwerze' 444 80 200))
$poleSciezka = New-Pole 444 100 190 '/opt/tuike-panel'
$grupaVps.Controls.Add($poleSciezka)

$grupaVps.Controls.Add((New-Etykieta 'Hasło zostaje tylko w pamięci tego okna — nie jest nigdzie zapisywane. Puste = logowanie kluczem SSH.' 16 136 620))
$okno.Controls.Add($grupaVps)

# --- przyciski i log -------------------------------------------------------------
$przyciskSprawdz = New-Przycisk 'Sprawdź połączenie' 24 340 170
$przyciskInstaluj = New-Przycisk 'Zainstaluj panel' 204 340 170 -Glowny
$przyciskInstrukcja = New-Przycisk 'Instrukcja' 384 340 120
$przyciskZamknij = New-Przycisk 'Zamknij' 564 340 120
$okno.Controls.AddRange(@($przyciskSprawdz, $przyciskInstaluj, $przyciskInstrukcja, $przyciskZamknij))

$stan = New-Etykieta 'Gotowy do pracy.' 26 382 660 $kolorSzary
$okno.Controls.Add($stan)

$log = New-Object System.Windows.Forms.TextBox
$log.Location = New-Object System.Drawing.Point(24, 406)
$log.Size = New-Object System.Drawing.Size(660, 200)
$log.Multiline = $true
$log.ScrollBars = 'Vertical'
$log.ReadOnly = $true
$log.BackColor = [System.Drawing.Color]::FromArgb(9, 16, 33)
$log.ForeColor = $kolorSzary
$log.Font = $fontMono
$okno.Controls.Add($log)

# --- zachowanie ------------------------------------------------------------------
function Set-Tryb {
    $grupaLokalna.Visible = $trybLokalny.Checked
    $grupaVps.Visible = $trybVps.Checked
    # Tag trzyma decyzje skryptu. Samego Visible nie da sie odczytac, zanim
    # okno sie pokaze - WinForms zwraca wtedy zawsze false - a autotest ma
    # sprawdzac wlasnie decyzje, nie rysowanie.
    $grupaLokalna.Tag = [bool]$trybLokalny.Checked
    $grupaVps.Tag = [bool]$trybVps.Checked
    if ($trybVps.Checked) {
        $przyciskSprawdz.Text = 'Sprawdź połączenie'
    } else {
        $przyciskSprawdz.Text = 'Sprawdź wymagania'
    }
}
$trybLokalny.Add_CheckedChanged({ Set-Tryb })
$trybVps.Add_CheckedChanged({ Set-Tryb })

$przyciskWybierz.Add_Click({
    $okienko = New-Object System.Windows.Forms.FolderBrowserDialog
    $okienko.Description = 'Wskaż katalog Serwer instalacji gry (ten z folderem linux-port)'
    if (Test-Path $poleSerwer.Text) { $okienko.SelectedPath = $poleSerwer.Text }
    if ($okienko.ShowDialog() -eq [System.Windows.Forms.DialogResult]::OK) {
        $poleSerwer.Text = $okienko.SelectedPath
    }
})

$przyciskInstrukcja.Add_Click({
    $plik = Join-Path $script:Paczka 'instrukcja.md'
    if (Test-Path $plik) { Start-Process $plik }
})

$przyciskZamknij.Add_Click({ $okno.Close() })

function Write-Log {
    param([string]$Tekst)
    if ($null -eq $Tekst) { return }
    $log.AppendText($Tekst + "`r`n")
}

function Set-Zajete {
    param([bool]$Zajete, [string]$Komunikat)
    $przyciskSprawdz.Enabled = -not $Zajete
    $przyciskInstaluj.Enabled = -not $Zajete
    $stan.Text = $Komunikat
    [System.Windows.Forms.Application]::DoEvents()
}

function Get-ArgumentyInstalacji {
    <#
        Zamienia to, co jest w okienku, na argumenty skryptu konsolowego.
        Zwraca $null i sam pokazuje komunikat, gdy czegos brakuje.
    #>
    param([switch]$TylkoSprawdz)

    $argumenty = @('-NoProfile', '-ExecutionPolicy', 'Bypass', '-File', $script:Konsola)
    if ($trybVps.Checked) {
        if (-not $poleHost.Text.Trim()) {
            [System.Windows.Forms.MessageBox]::Show('Podaj adres serwera.', 'Brakuje danych') | Out-Null
            return $null
        }
        $portSsh = 0
        if (-not [int]::TryParse($polePortSsh.Text.Trim(), [ref]$portSsh) -or $portSsh -le 0) {
            [System.Windows.Forms.MessageBox]::Show('Port SSH musi być liczbą (domyślnie 22).', 'Brakuje danych') | Out-Null
            return $null
        }
        $argumenty += @('-Tryb', 'vps',
                        '-VpsHost', $poleHost.Text.Trim(),
                        '-VpsPort', "$portSsh",
                        '-VpsUser', $poleLogin.Text.Trim(),
                        '-VpsPath', $poleSciezka.Text.Trim())
        $portPanelu = 0
        if ([int]::TryParse($polePortPanelu.Text.Trim(), [ref]$portPanelu) -and $portPanelu -gt 0) {
            $argumenty += @('-Port', "$portPanelu")
        }
    } else {
        if (-not (Test-Path (Join-Path $poleSerwer.Text 'linux-port\docker'))) {
            [System.Windows.Forms.MessageBox]::Show(
                "W `"$($poleSerwer.Text)`" nie ma folderu linux-port\docker.`r`n`r`nWskaż katalog Serwer instalacji gry.",
                'Zły katalog') | Out-Null
            return $null
        }
        $argumenty += @('-Tryb', 'lokalny', '-Serwer', $poleSerwer.Text.Trim())
    }
    if ($TylkoSprawdz) { $argumenty += '-Sprawdz' }
    return $argumenty
}

function Start-Instalacji {
    param([switch]$TylkoSprawdz)

    $argumenty = Get-ArgumentyInstalacji -TylkoSprawdz:$TylkoSprawdz
    if ($null -eq $argumenty) { return }

    $log.Clear()
    if ($TylkoSprawdz) { Set-Zajete $true 'Sprawdzam…' }
    else { Set-Zajete $true 'Instaluję — to może potrwać kilka minut…' }

    $start = New-Object System.Diagnostics.ProcessStartInfo
    $start.FileName = (Get-Command powershell).Source
    $start.Arguments = ($argumenty | ForEach-Object { '"' + $_ + '"' }) -join ' '
    $start.WorkingDirectory = $script:Paczka
    $start.UseShellExecute = $false
    $start.RedirectStandardOutput = $true
    $start.RedirectStandardError = $true
    $start.CreateNoWindow = $true
    # Haslo idzie do procesu potomnego przez srodowisko, nie przez linie
    # polecen - inaczej widzialby je kazdy, kto podejrzy liste procesow.
    if ($trybVps.Checked -and $poleHaslo.Text) {
        $start.EnvironmentVariables['TUIKE_SSH_PASSWORD'] = $poleHaslo.Text
    }

    $script:Proces = New-Object System.Diagnostics.Process
    $script:Proces.StartInfo = $start
    $script:Proces.EnableRaisingEvents = $true
    $script:Proces.Start() | Out-Null

    # Czytamy oba strumienie na biezaco, zeby okno zylo, a operator widzial,
    # co sie dzieje, zamiast patrzec na zamrozone okienko przez pare minut.
    while (-not $script:Proces.HasExited) {
        while (-not $script:Proces.StandardOutput.EndOfStream) {
            Write-Log $script:Proces.StandardOutput.ReadLine()
        }
        [System.Windows.Forms.Application]::DoEvents()
        Start-Sleep -Milliseconds 120
    }
    while (-not $script:Proces.StandardOutput.EndOfStream) { Write-Log $script:Proces.StandardOutput.ReadLine() }
    $bledy = $script:Proces.StandardError.ReadToEnd()
    if ($bledy) { Write-Log $bledy }

    $kod = $script:Proces.ExitCode
    $script:Proces = $null

    if ($kod -eq 0) {
        if ($TylkoSprawdz) { Set-Zajete $false 'Sprawdzenie przeszło — można instalować.' }
        else {
            Set-Zajete $false 'Gotowe. Panel jest zainstalowany i wstanie razem z serwerem.'
            [System.Windows.Forms.MessageBox]::Show(
                "Panel Tuike zainstalowany.`r`n`r`nAdres panelu jest na końcu logu w oknie.`r`nPamiętaj, żeby w panelu włączyć ochronę hasłem.",
                'Gotowe') | Out-Null
        }
    } else {
        Set-Zajete $false 'Nie udało się — szczegóły w logu poniżej.'
    }
}

$przyciskSprawdz.Add_Click({ Start-Instalacji -TylkoSprawdz })
$przyciskInstaluj.Add_Click({
    if (-not $trybVps.Checked) { Start-Instalacji; return }
    $pytanie = [System.Windows.Forms.MessageBox]::Show(
        "Wysłać panel na serwer $($poleHost.Text) i tam go uruchomić?",
        'Potwierdzenie', [System.Windows.Forms.MessageBoxButtons]::YesNo)
    if ($pytanie -eq [System.Windows.Forms.DialogResult]::Yes) { Start-Instalacji }
})

Set-Tryb

# --- autotest --------------------------------------------------------------------
if ($SelfTest) {
    $bledy = @()
    if (-not (Test-Path $script:Konsola)) { $bledy += 'brak Zainstaluj-Tuike.ps1 obok GUI' }
    foreach ($para in @(
        @{ Nazwa = 'pole hosta'; Kontrolka = $poleHost },
        @{ Nazwa = 'pole portu SSH'; Kontrolka = $polePortSsh },
        @{ Nazwa = 'pole loginu'; Kontrolka = $poleLogin },
        @{ Nazwa = 'pole hasla'; Kontrolka = $poleHaslo },
        @{ Nazwa = 'pole sciezki'; Kontrolka = $poleSciezka },
        @{ Nazwa = 'pole katalogu Serwer'; Kontrolka = $poleSerwer }
    )) {
        if ($null -eq $para.Kontrolka) { $bledy += "brakuje kontrolki: $($para.Nazwa)" }
    }
    if (-not $poleHaslo.UseSystemPasswordChar) { $bledy += 'pole hasla nie maskuje znakow' }
    $trybVps.Checked = $true
    Set-Tryb
    if (-not $grupaVps.Tag) { $bledy += 'panel VPS nie pokazuje sie po wybraniu trybu' }
    if ($grupaLokalna.Tag) { $bledy += 'panel lokalny nie chowa sie w trybie VPS' }
    if ($przyciskSprawdz.Text -ne 'Sprawdź połączenie') { $bledy += 'przycisk nie zmienia opisu w trybie VPS' }
    $trybLokalny.Checked = $true
    Set-Tryb
    if (-not $grupaLokalna.Tag) { $bledy += 'panel lokalny nie wraca po przelaczeniu' }

    # Argumenty budowane z pustego formularza VPS musza zostac odrzucone,
    # a nie wyslac instalatora w swiat bez adresu.
    $trybVps.Checked = $true
    $poleHost.Text = ''
    Set-Tryb

    if ($bledy.Count -gt 0) {
        $bledy | ForEach-Object { Write-Host "[BLAD] $_" -ForegroundColor Red }
        exit 1
    }
    Write-Host 'Autotest GUI przeszedl.' -ForegroundColor Green
    exit 0
}

if ($Zrzut) {
    # Rysujemy okno do pliku zamiast je pokazywac: dziala tez tam, gdzie nie ma
    # pulpitu (zdalna sesja, serwer budujacy dokumentacje).
    $okno.StartPosition = 'Manual'
    $okno.Location = New-Object System.Drawing.Point(-3000, -3000)
    $okno.Show()
    [System.Windows.Forms.Application]::DoEvents()

    function Save-Okno {
        param([string]$Plik)
        $obraz = New-Object System.Drawing.Bitmap($okno.Width, $okno.Height)
        $obszar = New-Object System.Drawing.Rectangle(0, 0, $okno.Width, $okno.Height)
        $okno.DrawToBitmap($obraz, $obszar)
        $obraz.Save($Plik)
        Write-Host "zapisano $Plik"
    }

    Save-Okno $Zrzut
    $trybVps.Checked = $true
    Set-Tryb
    [System.Windows.Forms.Application]::DoEvents()
    Save-Okno ([IO.Path]::ChangeExtension($Zrzut, $null).TrimEnd('.') + '-vps.png')
    $okno.Close()
    exit 0
}

[System.Windows.Forms.Application]::Run($okno)
