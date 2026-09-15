param([ValidateSet('Check','Install','Rollback','Uninstall')][string]$Mode='Check',
 [Parameter(Mandatory=$true)][string]$ServerRoot,[switch]$SourceOnly)
$ErrorActionPreference='Stop'
$pack=Split-Path $PSScriptRoot -Parent
$ServerRoot=[IO.Path]::GetFullPath($ServerRoot).TrimEnd('\','/')
$stateRel='.companion-pack-2x'
$lock=$null
function Hash($p){(Get-FileHash -LiteralPath $p -Algorithm SHA256).Hash.ToLowerInvariant()}
function Safe($root,$rel){
 if([IO.Path]::IsPathRooted($rel)){throw "Absolute path refused: $rel"}
 $p=[IO.Path]::GetFullPath((Join-Path $root $rel))
 if(!$p.StartsWith($root.TrimEnd('\','/')+[IO.Path]::DirectorySeparatorChar,[StringComparison]::OrdinalIgnoreCase)){throw "Path escapes root: $rel"}
 $q=$p
 while($q){if((Test-Path -LiteralPath $q) -and ((Get-Item -LiteralPath $q -Force).Attributes -band [IO.FileAttributes]::ReparsePoint)){throw "Linked path refused: $q"};$q=Split-Path $q -Parent}
 return $p
}
function StateWrite {
 $state|ConvertTo-Json -Depth 10|Set-Content -LiteralPath "$stateFile.tmp" -Encoding UTF8
 Move-Item -LiteralPath "$stateFile.tmp" -Destination $stateFile -Force
}
function Put($from,$to,$expected){
 if((Hash $from) -ne $expected){throw "Input checksum changed: $from"}
 $temp=$to+'.companion-'+[guid]::NewGuid().ToString('N')+'.tmp'
 try{[IO.File]::WriteAllBytes($temp,[IO.File]::ReadAllBytes($from));if((Hash $temp) -ne $expected){throw 'Staged checksum failed'};Move-Item -LiteralPath $temp -Destination $to -Force}finally{if(Test-Path -LiteralPath $temp){Remove-Item -LiteralPath $temp}}
}
function InvokeDocker([string[]]$Arguments){
 & $script:dockerExe @Arguments
 if($LASTEXITCODE -ne 0){throw "Docker failed: $($Arguments -join ' ')"}
}
function RuntimeCheck {
 $script:dockerExe=(Get-Command docker -ErrorAction SilentlyContinue).Source
 if(!$script:dockerExe){foreach($candidate in @("$env:LOCALAPPDATA/Programs/DockerDesktop/resources/bin/docker.exe","$env:ProgramFiles/Docker/Docker/resources/bin/docker.exe")){if(Test-Path -LiteralPath $candidate){$script:dockerExe=$candidate;break}}}
 if(!$script:dockerExe){throw 'Docker executable not found. Start Docker Desktop.'}
 InvokeDocker @('info','--format','{{.ServerVersion}}')
 InvokeDocker @('compose','config','--quiet')
}
function Build {
 InvokeDocker @('compose','build','--build-arg','MAKE_JOBS=2','game')
 InvokeDocker @('compose','up','-d','--no-deps','--force-recreate','--wait','--wait-timeout','120','game')
}
function SchemaCheck {
 $schema=(Get-Content -LiteralPath "$PSScriptRoot/schema.sh" -Raw).Replace("`r`n","`n")
 $schema | & $script:dockerExe compose exec -T mariadb sh -s
 if($LASTEXITCODE -ne 0){throw 'Companion schema check/setup failed'}
}
try{
 $manifest=Get-Content -LiteralPath "$pack/manifest.json" -Raw|ConvertFrom-Json
 $manifestHash=Hash "$pack/manifest.json"
 if($manifest.schemaVersion -ne 1 -or $manifest.buildLine -ne '2.x'){throw 'Unsupported manifest'}
 if((Get-Content -LiteralPath (Safe $ServerRoot 'VERSION') -Raw).Trim() -ne $manifest.serverVersion){throw "Requires server $($manifest.serverVersion). No cross-version overwrite is supported."}
 if((Get-Content -LiteralPath (Safe $ServerRoot 'linux-port/docker/game/src/server/__REVISION__') -Raw).Trim() -ne $manifest.revision){throw 'Wrong source revision'}
 $stateFile=Safe $ServerRoot "$stateRel/state.json"
 if($Mode -ne 'Check'){
  $null=New-Item -ItemType Directory -Path (Split-Path $stateFile -Parent) -Force
  $lock=[IO.File]::Open((Safe $ServerRoot "$stateRel/operation.lock"),'OpenOrCreate','ReadWrite','None')
 }
 foreach($g in $manifest.guards){if((Hash (Safe $ServerRoot $g.path)) -ne $g.hash){throw "Build structure/dependency changed: $($g.path). Review this update before applying the pack."}}
 $plan=@();$seen=@{}
 foreach($f in $manifest.files){
  if($seen.ContainsKey($f.path)){throw 'Duplicate manifest path'};$seen[$f.path]=$true
  $target=Safe $ServerRoot $f.path;$payload=Safe $pack ('payload/'+$f.path);$base=Safe $pack ('baseline/'+$f.path)
  if((Hash $payload) -ne $f.afterHash -or (Hash $base) -ne $f.beforeHash){throw "Package checksum failed: $($f.path)"}
  $current=Hash $target
  $plan+=@{file=$f;target=$target;payload=$payload;base=$base;current=$current}
 }
 $state=$null
 if(Test-Path -LiteralPath $stateFile){
  $state=Get-Content -LiteralPath $stateFile -Raw|ConvertFrom-Json
  if($state.manifestHash -ne $manifestHash -or $state.root -ne $ServerRoot){throw 'Backup belongs to another package or server root'}
  if(@($state.files).Count -ne $plan.Count){throw 'Incomplete backup journal'}
  foreach($p in $plan){
   $e=@($state.files|Where-Object path -eq $p.file.path)
   if($e.Count -ne 1){throw 'Invalid backup journal'}
   if($e[0].beforeHash -notin @($p.file.beforeHash,$p.file.afterHash)){throw 'Unrecognized journal checksum'}
   $bp=Safe $ServerRoot ($stateRel+'/backups/'+$p.file.path)
   if((Hash $bp) -ne $e[0].beforeHash){throw "Backup damaged: $($p.file.path)"}
   $p.backup=$bp;$p.backupHash=$e[0].beforeHash
  }
 }
 foreach($p in $plan){if($p.current -notin @($p.file.beforeHash,$p.file.afterHash)){throw "Later edits or unsupported source: $($p.file.path). Nothing overwritten."}}
 if($Mode -eq 'Check'){
  $changed=@($plan|Where-Object {$_.current -ne $_.file.afterHash}).Count
  Write-Host "Verified 2.0.14 / r41023: $changed file(s) need installation. Package and dependency hashes passed."
  if($state){Write-Host "Journal status: $($state.status)"}
  exit 0
 }
 if($Mode -in @('Rollback','Uninstall') -and !$state){throw 'Install journal required; no files changed'}
 if($Mode -eq 'Install' -and $state -and $state.status -eq 'install-complete' -and $state.runtime -eq 'game-rebuilt-and-started' -and @($plan|Where-Object {$_.current -ne $_.file.afterHash}).Count -eq 0){Write-Host 'Already installed and deployed; no rebuild needed.';exit 0}
 Push-Location (Safe $ServerRoot 'linux-port/docker')
 try{
  if(!$SourceOnly){RuntimeCheck;if($Mode -eq 'Install'){SchemaCheck}}
  if(!$state){
   $state=@{version=$manifest.version;manifestHash=$manifestHash;root=$ServerRoot;status='prepared';files=@();runtime='not-run'}
   foreach($p in $plan){
    $bp=Safe $ServerRoot ($stateRel+'/backups/'+$p.file.path)
    if(Test-Path -LiteralPath $bp){if((Hash $bp) -ne $p.current){throw 'Unjournaled backup differs from source; preserve it and review before retrying'}}
    else{$null=New-Item -ItemType Directory -Path (Split-Path $bp -Parent) -Force;[IO.File]::WriteAllBytes($bp,[IO.File]::ReadAllBytes($p.target))}
    if((Hash $bp) -ne $p.current){throw 'Source changed while backing up'}
    $state.files+=@{path=$p.file.path;beforeHash=$p.current}
   }
   StateWrite
  }
  foreach($p in $plan){if((Hash $p.target) -ne $p.current){throw 'Source changed during preflight'}}
  $state.status=$Mode.ToLowerInvariant()+'-applying';StateWrite
  foreach($p in $plan){
   if($Mode -eq 'Install'){$from=$p.payload;$expected=$p.file.afterHash}
   elseif($Mode -eq 'Uninstall'){$from=$p.base;$expected=$p.file.beforeHash}
   else{$from=$p.backup;$expected=$p.backupHash}
   if($p.current -ne $expected){Put $from $p.target $expected}
  }
  $state.status=$Mode.ToLowerInvariant()+'-source-applied';StateWrite
  if(!$SourceOnly){
   $state.runtime='build-or-deploy-pending';StateWrite
   Build
   $state.runtime='game-rebuilt-and-started'
  }else{$state.runtime='source-only; running image unchanged'}
  $state.status=$Mode.ToLowerInvariant()+'-complete';StateWrite
  Write-Host "$Mode complete. $($state.runtime). Backups retained in $stateRel."
 }finally{Pop-Location}
}catch{
 Write-Host "STOPPED: $($_.Exception.Message)" -ForegroundColor Red
 Write-Host 'If application began, retain the journal/backups. Rerun the same operation to resume, or run Rollback to restore the pre-install files and rebuild game. A deployment failure can leave game stopped until recovery succeeds.'
 exit 1
}finally{if($lock){$lock.Dispose()}}

