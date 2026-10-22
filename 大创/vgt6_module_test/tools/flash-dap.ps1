param(
    [string]$ProbeId = "puso_daplink_lab",
    [string]$PackDirectory = "D:\Keil5\ARM\PACK\Keil\STM32F4xx_DFP\3.1.1",
    [string]$KeilExe = "D:\Keil5\UV4\UV4.exe",
    [int]$SwdClockHz = 1000000,
    [switch]$UseKeil = $true
)
$ErrorActionPreference = "Stop"
$FirmwareRoot = (Resolve-Path (Join-Path $PSScriptRoot "..")).Path
$PythonExe = Join-Path (Split-Path $FirmwareRoot -Parent) "mecanum-motor-console\bridge\.venv\Scripts\python.exe"
$ImageFile = Join-Path $FirmwareRoot "MDK-ARM\vgt6_module_test\vgt6_module_test.hex"
$ProjectFile = Join-Path $FirmwareRoot "MDK-ARM\vgt6_module_test.uvprojx"
$FlashLog = Join-Path $FirmwareRoot "MDK-ARM\flash-r22.log"
$ResetLog = Join-Path $FirmwareRoot "MDK-ARM\flash-r22-reset.log"
$VerifyLog = Join-Path $FirmwareRoot "MDK-ARM\flash-r22-verify.log"
$PostVerifyLog = Join-Path $FirmwareRoot "MDK-ARM\flash-r22-postverify.log"
if (!(Test-Path -LiteralPath $ImageFile)) { throw "Build firmware in Keil first." }
$NeedFullReadback = $true
if ($UseKeil -and (Test-Path -LiteralPath $KeilExe) -and (Test-Path -LiteralPath $ProjectFile)) {
    # Keil's CMSIS-DAP backend is the verified path for this PUSO probe. It
    # can be selected explicitly for installations where its target backend
    # is known to write this probe correctly.
    # UV4 is a GUI executable: PowerShell's & returns before its batch
    # operation completes. Wait for this exact process before reading logs.
    $KeilProcess = Start-Process -FilePath $KeilExe -ArgumentList @(
        "-f", ('"{0}"' -f $ProjectFile), "-t", "vgt6_module_test",
        "-o", ('"{0}"' -f $FlashLog)
    ) -WindowStyle Hidden -PassThru
    if (-not $KeilProcess.WaitForExit(60000)) {
        throw "Keil flash timed out; see $FlashLog"
    }
    $LoadExitCode = $KeilProcess.ExitCode
    $KeilLogText = ""
    for ($LogAttempt = 1; $LogAttempt -le 5; $LogAttempt++) {
        Start-Sleep -Milliseconds 200
        if (Test-Path -LiteralPath $FlashLog) {
            $KeilLogText = Get-Content -Raw $FlashLog
            if ($KeilLogText -match "Flash Load finished") { break }
        }
    }
    if (($KeilLogText -match "Erase Done\.Programming Done\.Verify OK\.Application running") -and
        ($KeilLogText -match "Flash Load finished")) {
        # UV4 may return a non-zero process status even when its batch log
        # reports a complete erase/program/verify/run sequence.
        $LoadExitCode = 0
        # Keil has already verified every programmed byte. Keep an independent
        # device/vector/marker readback below, but do not read the whole image twice.
        $NeedFullReadback = $false
    } else {
        throw "Keil flash did not report erase/program/verify/run completion; see $FlashLog"
    }
} else {
    # pyOCD is the fallback PUSO path. Start at the configured SWD rate; lower
    # rates can be selected with -SwdClockHz if this specific probe is unstable.
    & $PythonExe -m pyocd load --pack $PackDirectory -t stm32f407vgtx -u $ProbeId `
        -O cmsis_dap.deferred_transfers=False -O cmsis_dap.limit_packets=True `
        -f $SwdClockHz -e sector --format hex --no-reset $ImageFile 2>&1 |
        Tee-Object -FilePath $FlashLog
    $LoadExitCode = $LASTEXITCODE
}
# Some PUSO firmware returns an empty response during CMSIS-DAP uninit after
# programming. The image must be accepted or rejected by readback below.
$VerifyScript = Join-Path $PSScriptRoot "verify-dap.py"
$VerifyPassed = $false
$VerifyArguments = @(
    $VerifyScript, "--pack", $PackDirectory,
    "--probe", $ProbeId, "--image", $ImageFile,
    "--frequency", "$SwdClockHz"
)
if (-not $NeedFullReadback) { $VerifyArguments += "--critical-only" }
for ($VerifyAttempt = 1; $VerifyAttempt -le 3; $VerifyAttempt++) {
    & $PythonExe @VerifyArguments 2>&1 |
        Tee-Object -FilePath $VerifyLog
    if ($LASTEXITCODE -eq 0) {
        $VerifyPassed = $true
        break
    }
    if ($VerifyAttempt -lt 3) {
        # PUSO can briefly expose the previous flash row while a prior
        # CMSIS-DAP session is releasing its cache; retry the independent
        # read before treating the write as failed.
        Start-Sleep -Milliseconds 500
    }
}
if (-not $VerifyPassed) { throw "DAP byte readback failed; see $FlashLog" }
if ($LoadExitCode -ne 0) {
    Write-Warning "pyOCD reported an uninit error after programming; critical readback passed."
}
# Programming with --no-reset can leave this probe halted. Reset and resume
# only after byte readback has accepted the image, then verify the run state.
@("reset", "exit") | & $PythonExe -m pyocd commander --pack $PackDirectory `
    -t stm32f407vgtx -u $ProbeId -O cmsis_dap.deferred_transfers=False `
    -O cmsis_dap.limit_packets=True -f $SwdClockHz 2>&1 |
    Tee-Object -FilePath $ResetLog
if ($LASTEXITCODE -ne 0) { throw "DAP reset/resume failed; see $FlashLog" }
$PostResetPassed = $false
for ($VerifyAttempt = 1; $VerifyAttempt -le 3; $VerifyAttempt++) {
    & $PythonExe $VerifyScript --pack $PackDirectory `
        --probe $ProbeId --image $ImageFile --frequency $SwdClockHz `
        --critical-only --require-running 2>&1 |
        Tee-Object -FilePath $PostVerifyLog
    if ($LASTEXITCODE -eq 0) {
        $PostResetPassed = $true
        break
    }
    if ($VerifyAttempt -lt 3) { Start-Sleep -Milliseconds 500 }
}
if (-not $PostResetPassed) { throw "DAP post-reset verification failed; see $FlashLog" }
