param(
    [ValidateSet("serial", "ble")]
    [string]$Transport = "serial",
    [string]$Port,
    [ValidateRange(1200, 2000000)]
    [int]$Baud = 115200,
    [string]$Device
)

$ErrorActionPreference = "Stop"
$BridgeDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$WebRoot = Split-Path -Parent $BridgeDir
$VenvDir = Join-Path $BridgeDir ".venv"
$VenvPython = Join-Path $VenvDir "Scripts\python.exe"
$BundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"

# Restart only the bridge from this exact project; unrelated listeners
# must never be terminated simply because they use the same port.
$BridgeScript = (Resolve-Path (Join-Path $BridgeDir "motor_vofa_bridge.py")).Path
$BridgeScriptPattern = '(?:^|\s|")' + [regex]::Escape($BridgeScript.Replace('/', '\')) + '(?:"|\s|$)'
foreach ($BridgeListenPort in @(8766, 1347)) {
    $ExistingListener = Get-NetTCPConnection -LocalPort $BridgeListenPort -State Listen -ErrorAction SilentlyContinue
    if ($ExistingListener) {
        $ExistingIds = @($ExistingListener.OwningProcess | Sort-Object -Unique)
        $ExistingProcesses = @($ExistingIds | ForEach-Object {
            Get-CimInstance Win32_Process -Filter "ProcessId=$_" -ErrorAction SilentlyContinue
        })
        if ($ExistingProcesses.Count -ne $ExistingIds.Count -or
            @($ExistingProcesses | Where-Object {
                $_.Name -notmatch '^python(w)?\.exe$' -or
                ($_.CommandLine -replace '/', '\') -notmatch $BridgeScriptPattern
            }).Count -ne 0) {
            throw "Port $BridgeListenPort is used by another program (PID $($ExistingIds -join ', ')). Close that program first."
        }
        foreach ($ExistingId in $ExistingIds) {
            Write-Host "Restarting this project's old bridge (PID $ExistingId)."
            Stop-Process -Id $ExistingId -Force -ErrorAction SilentlyContinue
        }
        for ($PortAttempt = 0; $PortAttempt -lt 20; $PortAttempt++) {
            if (-not (Get-NetTCPConnection -LocalPort $BridgeListenPort -State Listen -ErrorAction SilentlyContinue)) { break }
            Start-Sleep -Milliseconds 100
        }
        if (Get-NetTCPConnection -LocalPort $BridgeListenPort -State Listen -ErrorAction SilentlyContinue) {
            throw "Port $BridgeListenPort did not close after restarting the project bridge."
        }
    }
}

if (Test-Path -LiteralPath $BundledPython) {
    $BasePython = $BundledPython
} else {
    $PythonCommand = Get-Command python -ErrorAction Stop
    $BasePython = $PythonCommand.Source
}

if (-not (Test-Path -LiteralPath $VenvPython)) {
    Write-Host "Creating bridge environment..."
    & $BasePython -m venv $VenvDir
    if ($LASTEXITCODE -ne 0) { throw "Could not create the bridge environment." }
}

Write-Host "Checking bridge dependencies..."
& $VenvPython -m pip install --disable-pip-version-check -r (Join-Path $BridgeDir "requirements.txt")
if ($LASTEXITCODE -ne 0) { throw "Could not install bridge dependencies." }

$WebProcess = $null
$WebPortInUse = Get-NetTCPConnection -LocalAddress 127.0.0.1 -LocalPort 8088 -State Listen -ErrorAction SilentlyContinue
if (-not $WebPortInUse) {
    $WebProcess = Start-Process -FilePath $VenvPython `
        -ArgumentList @("-m", "http.server", "8088", "--bind", "127.0.0.1", "--directory", $WebRoot) `
        -WorkingDirectory $WebRoot -WindowStyle Hidden -PassThru
}

$BridgeArgs = @("--transport", $Transport, "--baud", "$Baud")
if ($Port) { $BridgeArgs += @("--port", $Port) }
if ($Device) { $BridgeArgs += @("--device", $Device) }

Write-Host ""
Write-Host "Web console: http://127.0.0.1:8088"
Write-Host "Transport:  $Transport"
Write-Host "Bridge: BM50_ACK_R3 (LF resync; commands wait for MCU replies)."
Write-Host "Serial baud: $Baud (8N1); auto-selects a unique CH340/CH910 USB port."
Write-Host "Close this window to stop the bridge."
Write-Host ""

Start-Process "http://127.0.0.1:8088"
try {
    & $VenvPython (Join-Path $BridgeDir "motor_vofa_bridge.py") @BridgeArgs
} finally {
    if ($WebProcess) {
        Stop-Process -Id $WebProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
