param(
    [string]$Port = 'COM7',
    [int]$Baud = 76800,
    [string]$Image = (Join-Path $PSScriptRoot '../mecanum_motor_test/MDK-ARM/mecanum_motor_test/mecanum_motor_test.hex'),
    [switch]$ProbeOnly
)
$ErrorActionPreference = 'Stop'
$flashPython = Join-Path $PSScriptRoot '.venv-flash/Scripts/python.exe'
if (-not (Test-Path -LiteralPath $flashPython)) {
    python -m venv (Join-Path $PSScriptRoot '.venv-flash')
    if ($LASTEXITCODE -ne 0) { throw 'Cannot create flash Python environment' }
    & $flashPython -m pip install -r (Join-Path $PSScriptRoot 'tools/flash-requirements.txt')
    if ($LASTEXITCODE -ne 0) { throw 'Cannot install flash dependencies' }
}
$flashArgs = @((Join-Path $PSScriptRoot 'tools/flash_stm32_uart.py'), '--port', $Port, '--baud', "$Baud")
if (-not $ProbeOnly) { $flashArgs += @('--flash', '--image', $Image) }
& $flashPython @flashArgs
exit $LASTEXITCODE
