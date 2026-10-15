$ErrorActionPreference = "Stop"
$ProjectDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Push-Location $ProjectDir
try {
    & gcc -std=c11 -Wall -Wextra -Werror `
        -I tests/host -I Bsp/Inc -I App/Inc -I Algorithm/Inc `
        tests/host/test_protocol.c Bsp/Src/serial_dma.c App/Src/command.c `
        App/Src/telemetry.c App/Src/speed_control.c App/Src/chassis_test.c `
        App/Src/attitude_control.c Algorithm/Src/attitude_filter.c `
        Algorithm/Src/pid.c Algorithm/Src/mecanum.c `
        -lm -o tests/host/test_protocol.exe
    if ($LASTEXITCODE -ne 0) { throw "Host compilation failed." }
    & "tests/host/test_protocol.exe"
    if ($LASTEXITCODE -ne 0) { throw "Protocol regression failed." }
    & gcc -std=c11 -Wall -Wextra -Werror `
        -I tests/host -I Bsp/Inc -I App/Inc -I Algorithm/Inc `
        tests/host/test_gyro.c App/Src/gyro.c Algorithm/Src/attitude_filter.c `
        -lm -o tests/host/test_gyro.exe
    if ($LASTEXITCODE -ne 0) { throw "Gyro host compilation failed." }
    & "tests/host/test_gyro.exe"
    if ($LASTEXITCODE -ne 0) { throw "Gyro regression failed." }
} finally {
    Pop-Location
}
