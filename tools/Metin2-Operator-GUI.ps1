<#
.SYNOPSIS
    Konsola operatora: publikacja zmian na GitHub i sterowanie serwerem na VPS.

.DESCRIPTION
    Okno dla osoby, która rozwija ten serwer — nie dla gracza. Gracz dostaje
    Metin2-Launcher-GUI.bat (Docker, GRAJ, klient, kopie bazy); ten plik go nie
    zastępuje i niczego mu nie zabiera.

    Przepływ, który to obsługuje, jest jeden:

        edycja na PC  →  [WYŚLIJ NA GITHUB]  →  [Sprawdź]  →  [Zastosuj na VPS]

    Sterowanie zdalne idzie przez HTTP API panelu na porcie 9797, a nie przez
    własne polecenia po SSH. To celowe: aktualizacja ma jedną implementację,
    z jednym kompletem zabezpieczeń (opt-in na repozytorium, .env nietykany,
    nieudany krok zatrzymuje resztę, log akcji). Drugi zestaw poleceń robiący
    to samo rozjechałby się z tamtym przy pierwszej poprawce.

    Hasło do panelu jest zapisywane przez DPAPI (ConvertFrom-SecureString) —
    zaszyfrowane kluczem Twojego konta Windows, nieczytelne dla innego
    użytkownika i na innej maszynie.

.NOTES
    Wymaga: PowerShell 5.1 (jest w Windows), gita w PATH.
    Uruchamiaj przez Metin2-Operator.bat obok tego pliku.
#>
[CmdletBinding()]
param(
    # Sprawdza logikę, która nie potrzebuje okna (szyfrowanie hasła, wyciąganie
    # tokenu CSRF, budowanie adresów), wypisuje wynik i kończy. Bez tego jedyną
    # metodą sprawdzenia tego pliku byłoby klikanie.
    [switch]$SelfTest
)

$ErrorActionPreference = 'Stop'
Add-Type -AssemblyName System.Windows.Forms
Add-Type -AssemblyName System.Drawing
[Windows.Forms.Application]::EnableVisualStyles()

$script:serverRoot = Split-Path -Parent $PSScriptRoot
$script:configPath = Join-Path $script:serverRoot '.m2operator.json'
$script:deployConfigPath = Join-Path $script:serverRoot '.m2vps-deploy.json'
$script:publishScript = Join-Path $PSScriptRoot 'Publish-ToGit.ps1'
$script:panelSession = $null
$script:panelCsrf = $null
$script:job = $null

# --- paleta, ta sama co w panelu WWW -----------------------------------------
$colBg      = [Drawing.Color]::FromArgb(7, 11, 19)
$colPanel   = [Drawing.Color]::FromArgb(15, 24, 40)
$colRaised  = [Drawing.Color]::FromArgb(22, 35, 58)
$colBorder  = [Drawing.Color]::FromArgb(30, 49, 80)
$colText    = [Drawing.Color]::FromArgb(234, 241, 253)
$colMuted   = [Drawing.Color]::FromArgb(113, 137, 168)
$colOk      = [Drawing.Color]::FromArgb(63, 211, 160)
$colWarn    = [Drawing.Color]::FromArgb(245, 196, 81)
$colBad     = [Drawing.Color]::FromArgb(251, 113, 133)
$colAccent  = [Drawing.Color]::FromArgb(37, 99, 235)

# =============================================================================
#  Konfiguracja
# =============================================================================
function Get-OperatorConfig {
    $deploy = $null
    if (Test-Path -LiteralPath $script:deployConfigPath) {
        try { $deploy = Get-Content -LiteralPath $script:deployConfigPath -Raw -Encoding UTF8 | ConvertFrom-Json } catch { }
    }
    $config = [pscustomobject]@{
        host          = if ($deploy) { [string]$deploy.host } else { '' }
        panelPort     = 9797
        panelPassword = ''       # DPAPI, nigdy jawny tekst
        remoteUrl     = ''
        branch        = 'main'
    }
    if (Test-Path -LiteralPath $script:configPath) {
        try {
            $saved = Get-Content -LiteralPath $script:configPath -Raw -Encoding UTF8 | ConvertFrom-Json
            foreach ($name in 'host', 'panelPort', 'panelPassword', 'remoteUrl', 'branch') {
                if ($null -ne $saved.$name -and "$($saved.$name)" -ne '') { $config.$name = $saved.$name }
            }
        }
        catch { }
    }
    return $config
}

function Save-OperatorConfig {
    param([Parameter(Mandatory = $true)]$Config)
    $Config | ConvertTo-Json | Set-Content -LiteralPath $script:configPath -Encoding UTF8
}

function Unprotect-PanelPassword {
    param([string]$Protected)
    if (-not $Protected) { return '' }
    try {
        $secure = ConvertTo-SecureString -String $Protected
        $pointer = [Runtime.InteropServices.Marshal]::SecureStringToBSTR($secure)
        try { return [Runtime.InteropServices.Marshal]::PtrToStringBSTR($pointer) }
        finally { [Runtime.InteropServices.Marshal]::ZeroFreeBSTR($pointer) }
    }
    catch { return '' }   # zapisane na innym koncie albo innej maszynie
}

function Protect-PanelPassword {
    param([string]$Plain)
    if (-not $Plain) { return '' }
    ConvertTo-SecureString -String $Plain -AsPlainText -Force | ConvertFrom-SecureString
}

# =============================================================================
#  Log w oknie
# =============================================================================
function Write-Log {
    param([string]$Text, [System.Drawing.Color]$Color = $null)
    if ($null -eq $script:logBox) { return }
    $stamp = (Get-Date -Format 'HH:mm:ss')
    $script:logBox.AppendText("[$stamp] $Text`r`n")
    $script:logBox.SelectionStart = $script:logBox.TextLength
    $script:logBox.ScrollToCaret()
}

function Set-Status {
    param([string]$Text, [System.Drawing.Color]$Color)
    $script:statusLabel.Text = $Text
    $script:statusLabel.ForeColor = $Color
}

# =============================================================================
#  Panel VPS — HTTP API
# =============================================================================
function Get-PanelBaseUrl {
    $config = Get-OperatorConfig
    if (-not $config.host) { throw 'Nie ustawiono adresu VPS. Kliknij „Ustawienia”.' }
    "http://$($config.host):$($config.panelPort)"
}

function Connect-Panel {
    <#
        Loguje się do panelu i zdobywa token CSRF.

        Panel wystawia CSRF dopiero przy renderowaniu pulpitu (csrf_token() jest
        wołane w widoku "/"), więc samo POST /login nie wystarczy — trzeba potem
        pobrać stronę główną i wyjąć token z osadzonego `const csrf = "..."`.
        Każda akcja panelu sprawdza go w nagłówku X-CSRF.
    #>
    if ($script:panelSession -and $script:panelCsrf) { return }
    $config = Get-OperatorConfig
    $password = Unprotect-PanelPassword $config.panelPassword
    if (-not $password) { throw 'Nie zapisano hasła do panelu. Kliknij „Ustawienia”.' }

    $base = Get-PanelBaseUrl
    $session = $null
    $response = Invoke-WebRequest -Uri "$base/login" -Method Post -Body @{ password = $password } `
        -SessionVariable session -TimeoutSec 25 -UseBasicParsing -MaximumRedirection 5
    if ($response.Content -match 'Błędne hasło' -or $response.Content -match 'name="password"') {
        throw 'Panel odrzucił hasło. Popraw je w „Ustawieniach”.'
    }
    $dashboard = Invoke-WebRequest -Uri "$base/" -WebSession $session -TimeoutSec 25 -UseBasicParsing
    if ($dashboard.Content -notmatch 'const csrf = "([^"]+)"') {
        throw 'Zalogowano, ale nie znalazłem tokenu CSRF — panel jest w innej wersji niż ten launcher.'
    }
    $script:panelCsrf = $Matches[1]
    $script:panelSession = $session
}

function Invoke-PanelApi {
    param(
        [Parameter(Mandatory = $true)][string]$Path,
        [ValidateSet('GET', 'POST')][string]$Method = 'GET',
        [int]$TimeoutSec = 60
    )
    Connect-Panel
    $base = Get-PanelBaseUrl
    $headers = @{ 'X-CSRF' = $script:panelCsrf }
    try {
        $response = Invoke-WebRequest -Uri "$base$Path" -Method $Method -Headers $headers `
            -WebSession $script:panelSession -TimeoutSec $TimeoutSec -UseBasicParsing
    }
    catch [Net.WebException] {
        # Panel zwraca sensowny komunikat w ciele odpowiedzi także przy 4xx/5xx
        # (np. "Trwa inna akcja" przy 409) — szkoda go zgubić na rzecz "(409)".
        $webResponse = $_.Exception.Response
        if ($webResponse) {
            $reader = [IO.StreamReader]::new($webResponse.GetResponseStream())
            $body = $reader.ReadToEnd(); $reader.Close()
            try { $parsed = $body | ConvertFrom-Json } catch { $parsed = $null }
            if ($parsed -and $parsed.error) { throw [string]$parsed.error }
        }
        # Sesja mogła wygasnąć po restarcie panelu — następne kliknięcie zaloguje od nowa.
        $script:panelSession = $null; $script:panelCsrf = $null
        throw
    }
    if ($response.Content) {
        try { return $response.Content | ConvertFrom-Json } catch { return $null }
    }
    return $null
}

function Test-PortOpen {
    param([string]$TargetHost, [int]$Port, [int]$TimeoutMs = 2500)
    $client = [Net.Sockets.TcpClient]::new()
    try {
        $async = $client.BeginConnect($TargetHost, $Port, $null, $null)
        if (-not $async.AsyncWaitHandle.WaitOne($TimeoutMs, $false)) { return $false }
        $client.EndConnect($async)
        return $true
    }
    catch { return $false }
    finally { $client.Close() }
}

# =============================================================================
#  Zadania w tle (żeby okno nie zamarzało)
# =============================================================================
function Start-BackgroundJob {
    param(
        [Parameter(Mandatory = $true)][scriptblock]$Script,
        [object[]]$Arguments = @(),
        [Parameter(Mandatory = $true)][string]$Label
    )
    if ($script:job) {
        [Windows.Forms.MessageBox]::Show('Poczekaj — poprzednia operacja jeszcze trwa.',
            'Zajęte', 'OK', 'Information') | Out-Null
        return
    }
    Write-Log "$Label…"
    Set-Status "⏳ $Label…" $colWarn
    Set-ButtonsEnabled $false
    $script:job = Start-Job -ScriptBlock $Script -ArgumentList $Arguments
    $script:jobLabel = $Label
    $script:jobTimer.Start()
}

function Complete-BackgroundJob {
    $job = $script:job
    foreach ($line in (Receive-Job -Job $job -ErrorAction SilentlyContinue)) {
        if ("$line".Trim()) { Write-Log "  $line" }
    }
    $failed = $job.State -eq 'Failed'
    foreach ($reason in $job.ChildJobs.JobStateInfo.Reason) {
        if ($reason) { Write-Log "  BŁĄD: $($reason.Message)"; $failed = $true }
    }
    Remove-Job -Job $job -Force -ErrorAction SilentlyContinue
    $script:job = $null
    $script:jobTimer.Stop()
    Set-ButtonsEnabled $true
    if ($failed) { Set-Status "✖ $($script:jobLabel) — nie powiodło się" $colBad }
    else { Set-Status "✔ $($script:jobLabel) — gotowe" $colOk }
}

function Set-ButtonsEnabled {
    param([bool]$Enabled)
    foreach ($button in $script:actionButtons) { $button.Enabled = $Enabled }
}

# =============================================================================
#  Autotest (-SelfTest): wszystko, co da się sprawdzić bez okna
# =============================================================================
if ($SelfTest) {
    $failures = @()
    function Assert-That {
        param([string]$Label, [bool]$Condition, [string]$Detail = '')
        Write-Host ("  {0}  {1}{2}" -f $(if ($Condition) { 'OK  ' } else { 'FAIL' }), $Label,
            $(if ($Detail) { " — $Detail" } else { '' }))
        if (-not $Condition) { $script:selfTestFailures += $Label }
    }
    $script:selfTestFailures = @()

    Write-Host '1. Szyfrowanie hasła panelu (DPAPI)'
    $secret = 'tajne-haslo-panelu-123'
    $protected = Protect-PanelPassword $secret
    Assert-That 'zaszyfrowane nie jest jawnym tekstem' ($protected -ne $secret -and $protected.Length -gt 40)
    Assert-That 'odszyfrowanie zwraca oryginał' ((Unprotect-PanelPassword $protected) -eq $secret)
    Assert-That 'pusty wejściowy daje pusty wyjściowy' ((Protect-PanelPassword '') -eq '')
    Assert-That 'śmieci nie wywalają, tylko dają pustkę' ((Unprotect-PanelPassword 'nie-dpapi') -eq '')

    Write-Host '2. Wyciąganie tokenu CSRF ze strony panelu'
    # Dokładnie to, co renderuje szablon panelu: const csrf = {{ csrf|tojson }};
    $page = 'blah <script>' + "`n" + 'const csrf = "a1b2c3d4e5f6";' + "`n" + 'let x = 1;</script>'
    Assert-That 'regex znajduje token' ($page -match 'const csrf = "([^"]+)"')
    Assert-That 'token jest poprawny' ($Matches[1] -eq 'a1b2c3d4e5f6') $Matches[1]

    Write-Host '3. Konfiguracja'
    $config = Get-OperatorConfig
    Assert-That 'domyślny port panelu to 9797' ([int]$config.panelPort -eq 9797) $config.panelPort
    Assert-That 'adres VPS podciągnięty z .m2vps-deploy.json' ([bool]$config.host) $config.host
    Assert-That 'domyślna gałąź to main' ($config.branch -eq 'main') $config.branch

    Write-Host '4. Ścieżki'
    Assert-That 'Publish-ToGit.ps1 jest obok' (Test-Path -LiteralPath $script:publishScript)
    Assert-That 'korzeń repo to folder Serwer' ((Split-Path -Leaf $script:serverRoot) -eq 'Serwer') $script:serverRoot

    Write-Host ''
    if ($script:selfTestFailures.Count) {
        Write-Host "NIEPOWODZENIA ($($script:selfTestFailures.Count)): $($script:selfTestFailures -join ', ')"
        exit 1
    }
    Write-Host 'OK: konsola operatora przeszła autotest.'
    exit 0
}

# =============================================================================
#  Okno
# =============================================================================
$form = [Windows.Forms.Form]::new()
$form.Text = 'Konsola operatora — Metin2 Playerbots'
$form.Size = [Drawing.Size]::new(780, 800)
$form.MinimumSize = [Drawing.Size]::new(700, 640)
$form.StartPosition = 'CenterScreen'
$form.BackColor = $colBg
$form.ForeColor = $colText
$form.Font = [Drawing.Font]::new('Segoe UI', 9)

function New-Card {
    param([int]$X, [int]$Y, [int]$W, [int]$H, [string]$Title)
    $card = [Windows.Forms.Panel]::new()
    $card.Location = [Drawing.Point]::new($X, $Y)
    $card.Size = [Drawing.Size]::new($W, $H)
    $card.BackColor = $colPanel
    $header = [Windows.Forms.Label]::new()
    $header.Text = $Title
    $header.Location = [Drawing.Point]::new(14, 10)
    $header.Size = [Drawing.Size]::new($W - 28, 18)
    $header.ForeColor = $colMuted
    $header.Font = [Drawing.Font]::new('Segoe UI Semibold', 8)
    $card.Controls.Add($header)
    $form.Controls.Add($card)
    return $card
}

function New-Button {
    param([string]$Text, [int]$X, [int]$Y, [int]$W, [int]$H,
          [System.Drawing.Color]$Back = $colRaised, [switch]$Primary)
    $button = [Windows.Forms.Button]::new()
    $button.Text = $Text
    $button.Location = [Drawing.Point]::new($X, $Y)
    $button.Size = [Drawing.Size]::new($W, $H)
    $button.BackColor = $Back
    $button.ForeColor = $colText
    $button.FlatStyle = 'Flat'
    $button.FlatAppearance.BorderColor = $colBorder
    $button.FlatAppearance.BorderSize = 1
    $button.Font = if ($Primary) { [Drawing.Font]::new('Segoe UI Semibold', 10) }
                   else { [Drawing.Font]::new('Segoe UI', 9) }
    $button.Cursor = 'Hand'
    return $button
}

# --- nagłówek ---------------------------------------------------------------
$title = [Windows.Forms.Label]::new()
$title.Text = '⬢  Konsola operatora'
$title.Location = [Drawing.Point]::new(24, 18)
$title.Size = [Drawing.Size]::new(430, 30)
$title.Font = [Drawing.Font]::new('Segoe UI Semibold', 14)
$form.Controls.Add($title)

$subtitle = [Windows.Forms.Label]::new()
$subtitle.Text = $script:serverRoot
$subtitle.Location = [Drawing.Point]::new(26, 48)
$subtitle.Size = [Drawing.Size]::new(560, 18)
$subtitle.ForeColor = $colMuted
$form.Controls.Add($subtitle)

$settingsButton = New-Button '⚙ Ustawienia' 610 20 130 30
$form.Controls.Add($settingsButton)

# --- karta: publikacja ------------------------------------------------------
$publishCard = New-Card 24 82 716 168 'PUBLIKACJA NA GITHUB'

$messageLabel = [Windows.Forms.Label]::new()
$messageLabel.Text = 'Opis zmiany (trafi do commita i do listy w panelu VPS):'
$messageLabel.Location = [Drawing.Point]::new(14, 38)
$messageLabel.Size = [Drawing.Size]::new(480, 18)
$messageLabel.ForeColor = $colMuted
$publishCard.Controls.Add($messageLabel)

$messageBox = [Windows.Forms.TextBox]::new()
$messageBox.Location = [Drawing.Point]::new(14, 58)
$messageBox.Size = [Drawing.Size]::new(688, 26)
$messageBox.BackColor = [Drawing.Color]::FromArgb(5, 10, 18)
$messageBox.ForeColor = $colText
$messageBox.BorderStyle = 'FixedSingle'
$publishCard.Controls.Add($messageBox)

$previewButton = New-Button '🔍 Pokaż, co pójdzie' 14 96 200 38
$publishButton = New-Button '↑  WYŚLIJ NA GITHUB' 224 96 260 38 ([Drawing.Color]::FromArgb(23, 70, 168)) -Primary
$openRepoButton = New-Button '🌐 Otwórz repo' 494 96 208 38
foreach ($b in @($previewButton, $publishButton, $openRepoButton)) { $publishCard.Controls.Add($b) }

# --- karta: serwer ----------------------------------------------------------
$serverCard = New-Card 24 262 716 208 'SERWER NA VPS'

$script:serverStateLabel = [Windows.Forms.Label]::new()
$script:serverStateLabel.Text = 'Stan: — (kliknij „Odśwież stan”)'
$script:serverStateLabel.Location = [Drawing.Point]::new(14, 36)
$script:serverStateLabel.Size = [Drawing.Size]::new(688, 20)
$script:serverStateLabel.ForeColor = $colMuted
$serverCard.Controls.Add($script:serverStateLabel)

$script:gitStateLabel = [Windows.Forms.Label]::new()
$script:gitStateLabel.Text = 'Aktualizacje: —'
$script:gitStateLabel.Location = [Drawing.Point]::new(14, 58)
$script:gitStateLabel.Size = [Drawing.Size]::new(688, 20)
$script:gitStateLabel.ForeColor = $colMuted
$serverCard.Controls.Add($script:gitStateLabel)

$refreshButton   = New-Button '⟲ Odśwież stan'        14  88 168 36
$checkButton     = New-Button '🔄 Sprawdź aktualizacje' 190 88 200 36
$applyButton     = New-Button '⬇  ZASTOSUJ NA VPS'    398  88 304 36 ([Drawing.Color]::FromArgb(29, 58, 44)) -Primary
$restartButton   = New-Button '⟳ Restart gry'          14 132 168 36
$compileButton   = New-Button '🔨 Kompiluj silnik'     190 132 200 36
$panelButton     = New-Button '🖥 Otwórz panel WWW'    398 132 304 36
foreach ($b in @($refreshButton, $checkButton, $applyButton, $restartButton, $compileButton, $panelButton)) {
    $serverCard.Controls.Add($b)
}

# --- log --------------------------------------------------------------------
$logLabel = [Windows.Forms.Label]::new()
$logLabel.Text = 'LOG'
$logLabel.Location = [Drawing.Point]::new(26, 484)
$logLabel.Size = [Drawing.Size]::new(200, 16)
$logLabel.ForeColor = $colMuted
$logLabel.Font = [Drawing.Font]::new('Segoe UI Semibold', 8)
$form.Controls.Add($logLabel)

$script:logBox = [Windows.Forms.TextBox]::new()
$script:logBox.Location = [Drawing.Point]::new(24, 504)
$script:logBox.Size = [Drawing.Size]::new(716, 208)
$script:logBox.Multiline = $true
$script:logBox.ReadOnly = $true
$script:logBox.ScrollBars = 'Vertical'
$script:logBox.BackColor = [Drawing.Color]::FromArgb(5, 10, 18)
$script:logBox.ForeColor = [Drawing.Color]::FromArgb(205, 220, 240)
$script:logBox.Font = [Drawing.Font]::new('Consolas', 8.5)
$script:logBox.Anchor = 'Top, Left, Right, Bottom'
$form.Controls.Add($script:logBox)

$script:statusLabel = [Windows.Forms.Label]::new()
$script:statusLabel.Text = 'Gotowe.'
$script:statusLabel.Location = [Drawing.Point]::new(24, 722)
$script:statusLabel.Size = [Drawing.Size]::new(716, 22)
$script:statusLabel.Font = [Drawing.Font]::new('Segoe UI Semibold', 9)
$script:statusLabel.Anchor = 'Left, Right, Bottom'
$form.Controls.Add($script:statusLabel)

$script:actionButtons = @($previewButton, $publishButton, $refreshButton, $checkButton,
                          $applyButton, $restartButton, $compileButton)

# --- zegar dla zadań w tle ---------------------------------------------------
$script:jobTimer = [Windows.Forms.Timer]::new()
$script:jobTimer.Interval = 700
$script:jobTimer.Add_Tick({
        if (-not $script:job) { $script:jobTimer.Stop(); return }
        foreach ($line in (Receive-Job -Job $script:job -Keep:$false -ErrorAction SilentlyContinue)) {
            if ("$line".Trim()) { Write-Log "  $line" }
        }
        if ($script:job.State -in 'Completed', 'Failed', 'Stopped') { Complete-BackgroundJob }
    })

# =============================================================================
#  Akcje
# =============================================================================
$settingsButton.Add_Click({
        $config = Get-OperatorConfig
        $dialog = [Windows.Forms.Form]::new()
        $dialog.Text = 'Ustawienia'
        $dialog.Size = [Drawing.Size]::new(470, 330)
        $dialog.StartPosition = 'CenterParent'
        $dialog.FormBorderStyle = 'FixedDialog'
        $dialog.MaximizeBox = $false; $dialog.MinimizeBox = $false
        $dialog.BackColor = $colBg; $dialog.ForeColor = $colText

        $fields = @{}
        $y = 16
        foreach ($item in @(
                @{ Key = 'host';      Label = 'Adres VPS' },
                @{ Key = 'panelPort'; Label = 'Port panelu' },
                @{ Key = 'remoteUrl'; Label = 'Repozytorium GitHub (https://…/repo.git)' },
                @{ Key = 'branch';    Label = 'Gałąź' })) {
            $label = [Windows.Forms.Label]::new()
            $label.Text = $item.Label
            $label.Location = [Drawing.Point]::new(16, $y)
            $label.Size = [Drawing.Size]::new(420, 16)
            $label.ForeColor = $colMuted
            $dialog.Controls.Add($label)
            $box = [Windows.Forms.TextBox]::new()
            $box.Location = [Drawing.Point]::new(16, $y + 18)
            $box.Size = [Drawing.Size]::new(420, 24)
            $box.Text = [string]$config.($item.Key)
            $box.BackColor = [Drawing.Color]::FromArgb(5, 10, 18)
            $box.ForeColor = $colText; $box.BorderStyle = 'FixedSingle'
            $dialog.Controls.Add($box)
            $fields[$item.Key] = $box
            $y += 48
        }

        $passLabel = [Windows.Forms.Label]::new()
        $passLabel.Text = 'Hasło panelu (zapisane szyfrowane, tylko na tym koncie Windows)'
        $passLabel.Location = [Drawing.Point]::new(16, $y)
        $passLabel.Size = [Drawing.Size]::new(420, 16)
        $passLabel.ForeColor = $colMuted
        $dialog.Controls.Add($passLabel)
        $passBox = [Windows.Forms.TextBox]::new()
        $passBox.Location = [Drawing.Point]::new(16, $y + 18)
        $passBox.Size = [Drawing.Size]::new(420, 24)
        $passBox.UseSystemPasswordChar = $true
        $passBox.Text = (Unprotect-PanelPassword $config.panelPassword)
        $passBox.BackColor = [Drawing.Color]::FromArgb(5, 10, 18)
        $passBox.ForeColor = $colText; $passBox.BorderStyle = 'FixedSingle'
        $dialog.Controls.Add($passBox)

        $save = New-Button 'Zapisz' 236 ($y + 56) 100 32 ([Drawing.Color]::FromArgb(23, 70, 168))
        $save.DialogResult = 'OK'
        $cancel = New-Button 'Anuluj' 344 ($y + 56) 92 32
        $cancel.DialogResult = 'Cancel'
        $dialog.Controls.AddRange(@($save, $cancel))
        $dialog.AcceptButton = $save; $dialog.CancelButton = $cancel

        if ($dialog.ShowDialog() -eq 'OK') {
            $config.host = $fields['host'].Text.Trim()
            $config.panelPort = [int]($fields['panelPort'].Text.Trim())
            $config.remoteUrl = $fields['remoteUrl'].Text.Trim()
            $config.branch = $fields['branch'].Text.Trim()
            $config.panelPassword = Protect-PanelPassword $passBox.Text
            Save-OperatorConfig $config
            $script:panelSession = $null; $script:panelCsrf = $null
            Write-Log 'Zapisano ustawienia.'
        }
        $dialog.Dispose()
    })

$previewButton.Add_Click({
        $root = $script:serverRoot
        $publish = $script:publishScript
        Start-BackgroundJob -Label 'Sprawdzanie, co pójdzie na GitHuba' -Arguments @($publish, $root) -Script {
            param($PublishScript, $Root)
            & $PublishScript -Message 'podglad' -WhatIfOnly
        }
    })

$publishButton.Add_Click({
        $message = $messageBox.Text.Trim()
        if (-not $message) {
            [Windows.Forms.MessageBox]::Show('Wpisz opis zmiany — bez niego nie ma commita.',
                'Brak opisu', 'OK', 'Warning') | Out-Null
            return
        }
        $config = Get-OperatorConfig
        $publish = $script:publishScript
        $remote = $config.remoteUrl
        $branch = $config.branch
        Start-BackgroundJob -Label 'Wysyłanie na GitHub' -Arguments @($publish, $message, $remote, $branch) -Script {
            param($PublishScript, $Message, $RemoteUrl, $Branch)
            $arguments = @{ Message = $Message }
            if ($RemoteUrl) { $arguments['RemoteUrl'] = $RemoteUrl }
            if ($Branch) { $arguments['Branch'] = $Branch }
            & $PublishScript @arguments
        }
        $messageBox.Clear()
    })

$openRepoButton.Add_Click({
        $config = Get-OperatorConfig
        if (-not $config.remoteUrl) {
            [Windows.Forms.MessageBox]::Show('Nie ustawiono adresu repozytorium. Kliknij „Ustawienia”.',
                'Brak repozytorium', 'OK', 'Information') | Out-Null
            return
        }
        Start-Process ($config.remoteUrl -replace '\.git$', '')
    })

$panelButton.Add_Click({
        try { Start-Process (Get-PanelBaseUrl) }
        catch { [Windows.Forms.MessageBox]::Show($_.Exception.Message, 'Błąd', 'OK', 'Warning') | Out-Null }
    })

$refreshButton.Add_Click({
        try {
            $config = Get-OperatorConfig
            if (-not $config.host) { throw 'Nie ustawiono adresu VPS. Kliknij „Ustawienia”.' }
            # Najpierw same porty: mówią "sieć działa" nawet wtedy, gdy panel
            # jeszcze nie wstał albo hasło jest złe, i oddzielają jedno od drugiego.
            $panelUp = Test-PortOpen $config.host ([int]$config.panelPort)
            $authUp  = Test-PortOpen $config.host 11000
            $gameUp  = Test-PortOpen $config.host 13000
            $parts = @(
                "panel $($config.panelPort): $(if ($panelUp) { 'działa' } else { 'brak' })",
                "logowanie 11000: $(if ($authUp) { 'działa' } else { 'brak' })",
                "świat 13000: $(if ($gameUp) { 'działa' } else { 'brak' })")
            $script:serverStateLabel.Text = 'Stan: ' + ($parts -join '   ·   ')
            $script:serverStateLabel.ForeColor = if ($panelUp -and $authUp -and $gameUp) { $colOk }
                                                 elseif ($panelUp) { $colWarn } else { $colBad }
            if (-not $panelUp) {
                $script:gitStateLabel.Text = 'Aktualizacje: panel nie odpowiada'
                $script:gitStateLabel.ForeColor = $colBad
                Set-Status '✖ Panel nie odpowiada' $colBad
                return
            }
            $status = Invoke-PanelApi -Path '/api/status'
            $git = $status.git
            if ($git.ok) {
                $behind = if ($null -ne $git.behind) { $git.behind } else { '?' }
                $script:gitStateLabel.Text = "Aktualizacje: $behind commitów w tyle · $($git.branch) $($git.commit) · $($git.subject)"
                $script:gitStateLabel.ForeColor = if ($git.behind -gt 0) { $colWarn } else { $colOk }
            }
            else {
                $script:gitStateLabel.Text = "Aktualizacje: $($git.error)"
                $script:gitStateLabel.ForeColor = $colWarn
            }
            Write-Log "Wersja na VPS: $($status.version); postacie online: $($status.live_characters); kontenery: $($status.containers.Count)"
            Set-Status '✔ Stan odświeżony' $colOk
        }
        catch {
            Write-Log "BŁĄD: $($_.Exception.Message)"
            Set-Status '✖ Nie udało się odświeżyć' $colBad
        }
    })

$checkButton.Add_Click({
        try {
            Set-Status '⏳ Sprawdzam aktualizacje na VPS…' $colWarn
            $result = Invoke-PanelApi -Path '/api/git/check' -Method Post -TimeoutSec 180
            if (-not $result.ok) { throw [string]$result.error }
            $count = @($result.incoming.commits).Count
            if ($count -eq 0) {
                Write-Log 'VPS ma już najnowszą wersję — nic do zastosowania.'
                Set-Status '✔ Serwer jest aktualny' $colOk
            }
            else {
                Write-Log "Do zastosowania: $count commitów, $($result.incoming.files_total) plików."
                foreach ($commit in $result.incoming.commits) { Write-Log "    $commit" }
                if ($result.panel_changed) {
                    Write-Log '    UWAGA: zmienia się kod panelu — po zastosowaniu uruchom install.sh na VPS.'
                }
                Set-Status "✔ $count commitów czeka na zastosowanie" $colWarn
            }
            $script:gitStateLabel.Text = "Aktualizacje: $count commitów w tyle"
            $script:gitStateLabel.ForeColor = if ($count -gt 0) { $colWarn } else { $colOk }
        }
        catch {
            Write-Log "BŁĄD: $($_.Exception.Message)"
            Set-Status '✖ Sprawdzenie nie powiodło się' $colBad
        }
    })

function Invoke-PanelAction {
    param([string]$Name, [string]$Label, [string]$Confirm)
    if ($Confirm) {
        $answer = [Windows.Forms.MessageBox]::Show($Confirm, $Label, 'YesNo', 'Warning')
        if ($answer -ne 'Yes') { return }
    }
    try {
        Invoke-PanelApi -Path "/api/action/$Name" -Method Post | Out-Null
        Write-Log "$Label — zlecone panelowi. Postęp poniżej."
        Set-Status "⏳ $Label…" $colWarn
        $script:logTimer.Start()
    }
    catch {
        Write-Log "BŁĄD: $($_.Exception.Message)"
        Set-Status "✖ $Label — nie ruszyło" $colBad
    }
}

$applyButton.Add_Click({
        Invoke-PanelAction -Name 'git-update' -Label 'Aktualizacja z GitHuba' -Confirm @'
Nadpisać pliki serwera wersją z GitHuba, przebudować obraz gry i wstać na nowo?

Baza, .env i kopie zapasowe zostają nietknięte.
'@
    })
$restartButton.Add_Click({ Invoke-PanelAction -Name 'restart-game' -Label 'Restart gry' })
$compileButton.Add_Click({ Invoke-PanelAction -Name 'compile' -Label 'Kompilacja silnika' })

# --- podciąganie logu akcji z panelu ----------------------------------------
# Panel prowadzi własny log akcji; zamiast go duplikować, dociągamy przyrosty
# i pokazujemy w tym samym oknie, aż akcja się skończy.
$script:lastLogLength = 0
$script:logTimer = [Windows.Forms.Timer]::new()
$script:logTimer.Interval = 2000
$script:logTimer.Add_Tick({
        try {
            $status = Invoke-PanelApi -Path '/api/status' -TimeoutSec 20
            $log = Invoke-PanelApi -Path '/api/action-log' -TimeoutSec 20
            $text = [string]$log.log
            if ($text.Length -gt $script:lastLogLength) {
                $fresh = $text.Substring($script:lastLogLength)
                foreach ($line in ($fresh -split "`n")) {
                    if ($line.Trim()) { Write-Log "  | $($line.TrimEnd())" }
                }
                $script:lastLogLength = $text.Length
            }
            if (-not $status.action.running) {
                $script:logTimer.Stop()
                $script:lastLogLength = 0
                Set-Status '✔ Akcja na VPS zakończona' $colOk
            }
        }
        catch {
            $script:logTimer.Stop()
            Write-Log "Przerwano podgląd logu: $($_.Exception.Message)"
        }
    })

$form.Add_Shown({
        Write-Log 'Konsola operatora gotowa.'
        $config = Get-OperatorConfig
        if (-not $config.host) { Write-Log 'Ustaw adres VPS i hasło panelu: przycisk „Ustawienia”.' }
        if (-not $config.remoteUrl) { Write-Log 'Ustaw adres repozytorium GitHub: przycisk „Ustawienia”.' }
    })
$form.Add_FormClosing({
        foreach ($timer in @($script:jobTimer, $script:logTimer)) { if ($timer) { $timer.Stop() } }
        if ($script:job) { Stop-Job -Job $script:job -ErrorAction SilentlyContinue }
    })

[void]$form.ShowDialog()
