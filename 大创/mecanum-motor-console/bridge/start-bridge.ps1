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

Write-Host "Checking bridge dependencies..."
& $VenvPython -m pip install --disable-pip-version-check -r (Join-Path $BridgeDir "requirements.txt")

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
Write-Host "VOFA+ TCP:  127.0.0.1:1347 (FireWater)"
Write-Host "Transport:  $Transport"
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
