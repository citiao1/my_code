param(
    [string]$Device = "WHEELTEC-IOS",
    [int]$WebPort = 8770,
    [int]$WebSocketPort = 8766,
    [int]$TcpPort = 1347
)

$ErrorActionPreference = "Stop"
$BridgeDir = Split-Path -Parent $MyInvocation.MyCommand.Path
$WebRoot = Split-Path -Parent $BridgeDir
$VenvDir = Join-Path $BridgeDir ".venv"
$VenvPython = Join-Path $VenvDir "Scripts\python.exe"

$BundledPython = Join-Path $env:USERPROFILE ".cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
if (Test-Path -LiteralPath $BundledPython) {
    $BasePython = $BundledPython
} else {
    $BasePython = (Get-Command python -ErrorAction Stop).Source
}

if (-not (Test-Path -LiteralPath $VenvPython)) {
    Write-Host "Creating WHEELTEC bridge environment..."
    & $BasePython -m venv $VenvDir
}

$ErrorActionPreference = "Continue"
& $VenvPython -c "import bleak, websockets" 2>$null
$DependencyCheckExitCode = $LASTEXITCODE
$ErrorActionPreference = "Stop"
if ($DependencyCheckExitCode -ne 0) {
    Write-Host "Installing bridge dependencies..."
    & $VenvPython -m pip install --disable-pip-version-check -r (Join-Path $BridgeDir "requirements.txt")
}

if (Get-NetTCPConnection -LocalPort $WebSocketPort -State Listen -ErrorAction SilentlyContinue) {
    throw "WebSocket port $WebSocketPort is already in use. Close the current bridge and run this script again."
}

$WebProcess = $null
if (-not (Get-NetTCPConnection -LocalPort $WebPort -State Listen -ErrorAction SilentlyContinue)) {
    $WebProcess = Start-Process -FilePath $VenvPython `
        -ArgumentList @("-m", "http.server", $WebPort, "--bind", "127.0.0.1", "--directory", $WebRoot) `
        -WorkingDirectory $WebRoot -WindowStyle Hidden -PassThru
}

Write-Host ""
Write-Host "WHEELTEC device: $Device"
Write-Host "Web console:     http://127.0.0.1:$WebPort"
Write-Host "WebSocket:       ws://127.0.0.1:$WebSocketPort"
Write-Host "VOFA+ TCP:       127.0.0.1:$TcpPort"
Write-Host "Keep this window open. Press Ctrl+C to stop."
Write-Host ""

try {
    & $VenvPython -u (Join-Path $BridgeDir "wheeltec_bridge.py") `
        --device $Device --ws-port $WebSocketPort --tcp-port $TcpPort
} finally {
    if ($WebProcess) {
        Stop-Process -Id $WebProcess.Id -Force -ErrorAction SilentlyContinue
    }
}
