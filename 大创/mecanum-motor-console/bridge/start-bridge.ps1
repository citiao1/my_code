param(
    [Parameter(Mandatory = $true)]
    [ValidateSet("serial", "ble")]
    [string]$Transport,
    [string]$Port,
    [string]$Device
)

$ErrorActionPreference = "Stop"
$BridgeDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$WebRoot = Split-Path -Parent $BridgeDir
$VenvDir = Join-Path $BridgeDir ".venv"
$VenvPython = Join-Path $VenvDir "Scripts\python.exe"
$BundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"

# Python does not reload a running bridge when its source file changes.
# Do not start a second process against the same BLE module/ports.
foreach ($BridgeListenPort in @(8766, 1347)) {
    $ExistingListener = Get-NetTCPConnection -LocalPort $BridgeListenPort -State Listen -ErrorAction SilentlyContinue
    if ($ExistingListener) {
        $ExistingIds = ($ExistingListener.OwningProcess | Sort-Object -Unique) -join ", "
        throw "Port $BridgeListenPort is already in use (PID $ExistingIds). Close the old bridge window before starting this version."
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

$BridgeArgs = @("--transport", $Transport)
if ($Port) { $BridgeArgs += @("--port", $Port) }
if ($Device) { $BridgeArgs += @("--device", $Device) }

Write-Host ""
Write-Host "Web console: http://127.0.0.1:8088"
Write-Host "Transport:  $Transport"
Write-Host "Bridge: BM50_ACK_R3 (LF resync; commands wait for MCU replies)."
Write-Host "BLE scan accepts FFE0 advertisements and named UART modules."
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
