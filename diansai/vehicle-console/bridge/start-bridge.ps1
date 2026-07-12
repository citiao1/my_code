$ErrorActionPreference = "Stop"

$BridgeDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$VenvDir = Join-Path $BridgeDir ".venv"
$VenvPython = Join-Path $VenvDir "Scripts\python.exe"

$BundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if (Test-Path -LiteralPath $BundledPython) {
    $BasePython = $BundledPython
} else {
    $PythonCommand = Get-Command python -ErrorAction Stop
    $BasePython = $PythonCommand.Source
}

if (-not (Test-Path -LiteralPath $VenvPython)) {
    Write-Host "Creating bridge environment..."
    & $BasePython -m venv $VenvDir
}

Write-Host "Installing bridge dependencies..."
& $VenvPython -m pip install --disable-pip-version-check -r (Join-Path $BridgeDir "requirements.txt")

$WebRoot = Split-Path -Parent $BridgeDir
$WebProcess = $null
$WebPortInUse = Get-NetTCPConnection -LocalPort 8765 -State Listen -ErrorAction SilentlyContinue
if (-not $WebPortInUse) {
    $WebProcess = Start-Process -FilePath $VenvPython `
        -ArgumentList @("-m", "http.server", "8765", "--bind", "127.0.0.1", "--directory", $WebRoot) `
        -WorkingDirectory $WebRoot -WindowStyle Hidden -PassThru
}

Write-Host "Web console: http://127.0.0.1:8765"
Write-Host "VOFA+ TCP:  127.0.0.1:1347"
Write-Host "Starting BLE bridge..."
try {
    & $VenvPython (Join-Path $BridgeDir "ble_vofa_bridge.py") @args
} finally {
    if ($WebProcess) {
        Stop-Process -Id $WebProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
