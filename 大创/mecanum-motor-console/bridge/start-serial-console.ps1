param([ValidateRange(1024,65535)][int]$Port = 8088)
$ErrorActionPreference = 'Stop'
Write-Host '[1/4] Preparing direct Web Serial console. Please wait for the browser URL.'
$WebRoot = Split-Path -Parent $PSScriptRoot
$PythonExe = Join-Path $PSScriptRoot '.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $PythonExe)) {
    $PythonExe = (Get-Command python -ErrorAction Stop).Source
}

# Release only this project's bridge. The direct browser must own COM exclusively.
$BridgeScript = (Resolve-Path (Join-Path $PSScriptRoot 'motor_vofa_bridge.py')).Path
$BridgePattern = '(?:^|\s|")' + [regex]::Escape($BridgeScript.Replace('/', '\')) + '(?:"|\s|$)'
$Listeners = @(Get-NetTCPConnection -State Listen -ErrorAction SilentlyContinue | Where-Object { $_.LocalPort -in @(8766,1347,$Port) })
foreach ($ListenPort in @(8766,1347)) {
    foreach ($Listener in @($Listeners | Where-Object { $_.LocalPort -eq $ListenPort })) {
        $Owner = Get-CimInstance Win32_Process -Filter "ProcessId=$($Listener.OwningProcess)" -ErrorAction SilentlyContinue
        if ($Owner.Name -match '^python(w)?\.exe$' -and ($Owner.CommandLine -replace '/', '\') -match $BridgePattern) {
            Write-Host "Closing this project's serial bridge (PID $($Owner.ProcessId))."
            Stop-Process -Id $Owner.ProcessId -Force -ErrorAction Stop
        }
    }
}

Write-Host '[2/4] Checking the local webpage service.'
$Existing = @($Listeners | Where-Object { $_.LocalPort -eq $Port })
$WebRootPattern = '(?:^|\s|")' + [regex]::Escape($WebRoot.Replace('/', '\')) + '(?:"|\s|$)'
if ($Existing.Count) {
    foreach ($Listener in $Existing) {
        $Owner = Get-CimInstance Win32_Process -Filter "ProcessId=$($Listener.OwningProcess)" -ErrorAction SilentlyContinue
        if ($Owner.Name -notmatch '^python(w)?\.exe$' -or $Owner.CommandLine -notmatch 'http\.server' -or
            ($Owner.CommandLine -replace '/', '\') -notmatch $WebRootPattern) {
            throw "HTTP port $Port belongs to another program. Use -Port with a free port."
        }
    }
} else {
    $LogBase = Join-Path $env:TEMP ("bm50-static-web-" + $Port)
    $WebProcess = Start-Process -FilePath $PythonExe -ArgumentList @('-m','http.server',"$Port",'--bind','127.0.0.1','--directory',('"'+$WebRoot+'"')) `
        -WorkingDirectory $WebRoot -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput ($LogBase+'.out.log') -RedirectStandardError ($LogBase+'.err.log')
    $Ready = $false
    for ($Attempt=0;$Attempt -lt 20;$Attempt++) {
        if ($WebProcess.HasExited) { throw "Static HTTP server exited; see $LogBase.err.log" }
        try {
            $null = Invoke-WebRequest "http://127.0.0.1:$Port/serial-transport.js" -UseBasicParsing -TimeoutSec 1
            $Ready=$true;break
        } catch { Start-Sleep -Milliseconds 100 }
    }
    if (-not $Ready) { throw "Static HTTP server did not become ready; see $LogBase.err.log" }
}

$BrowserCandidates = @(
    (Join-Path ${env:ProgramFiles(x86)} 'Microsoft\Edge\Application\msedge.exe'),
    (Join-Path $env:ProgramFiles 'Google\Chrome\Application\chrome.exe'),
    (Join-Path $env:LOCALAPPDATA 'Google\Chrome\Application\chrome.exe')
)
$BrowserExe = $BrowserCandidates | Where-Object { Test-Path -LiteralPath $_ } | Select-Object -First 1
$Url = "http://127.0.0.1:$Port/?transport=serial"
Write-Host '[3/4] Webpage ready. Opening Chrome / Edge.'
Write-Host "Direct Web Serial: $Url"
Write-Host 'This launcher does not use a serial bridge. The HTTP process only serves page files.'
Write-Host 'Click Connect and choose the LQ208 CH340 port. Do not choose the DAP probe.'
if ($BrowserExe) { Start-Process -FilePath $BrowserExe -ArgumentList @($Url) }
else { Write-Host 'Open the URL above in desktop Chrome or Edge.' }
Write-Host '[4/4] Startup complete. Click Connect in the webpage; no bridge is required.'
