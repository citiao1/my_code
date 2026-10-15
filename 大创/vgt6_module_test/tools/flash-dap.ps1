param(
    [string]$ProbeId = "puso_daplink_lab",
    [string]$PackDirectory = "D:\Keil5\ARM\PACK\Keil\STM32F4xx_DFP\3.1.1"
)
$ErrorActionPreference = "Stop"
$FirmwareRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$PythonExe = Join-Path (Split-Path $FirmwareRoot -Parent) "mecanum-motor-console\bridge\.venv\Scripts\python.exe"
$ImageFile = Join-Path $FirmwareRoot "MDK-ARM\vgt6_module_test\vgt6_module_test.hex"
$FlashLog = Join-Path $FirmwareRoot "MDK-ARM\flash-r7.log"
if (!(Test-Path -LiteralPath $ImageFile)) { throw "Build firmware in Keil first." }
& $PythonExe -m pyocd load --pack $PackDirectory -t stm32f407vgtx -u $ProbeId `
    -O cmsis_dap.deferred_transfers=False -O cmsis_dap.limit_packets=True `
    -f 100000 -e sector --format hex --no-reset $ImageFile 2>&1 | Tee-Object -FilePath $FlashLog
$LoadExitCode = $LASTEXITCODE
# Some PUSO firmware returns an empty response during CMSIS-DAP uninit after
# programming. The image must be accepted or rejected by readback below.
& $PythonExe (Join-Path $PSScriptRoot "verify-dap.py") --pack $PackDirectory `
    --probe $ProbeId --image $ImageFile 2>&1 | Tee-Object -FilePath $FlashLog -Append
if ($LASTEXITCODE -ne 0) { throw "DAP byte readback failed; see $FlashLog" }
if ($LoadExitCode -ne 0) {
    Write-Warning "pyOCD reported an uninit error after programming; critical readback passed."
}
