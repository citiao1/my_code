$ErrorActionPreference = "Stop"
$ProjectDir = (Resolve-Path (Join-Path $PSScriptRoot "..\..")).Path
Push-Location $ProjectDir
try {
    & gcc -std=c11 -Wall -Wextra -Werror `
        -I tests/host -I Bsp/Inc -I App/Inc -I Algorithm/Inc `
        tests/host/test_protocol.c Bsp/Src/serial_dma.c App/Src/command.c `
        App/Src/telemetry.c App/Src/speed_control.c App/Src/front_guard.c App/Src/chassis_test.c `
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
    & gcc -std=c11 -Wall -Wextra -Werror `
        -I tests/host/ps2 -I tests/host -I Bsp/Inc -I App/Inc -I Algorithm/Inc `
        tests/host/test_ps2.c Bsp/Src/ps2.c App/Src/ps2_control.c `
        App/Src/attitude_control.c App/Src/chassis_test.c `
        App/Src/speed_control.c App/Src/front_guard.c Algorithm/Src/pid.c Algorithm/Src/mecanum.c `
        -lm -o tests/host/test_ps2.exe
    if ($LASTEXITCODE -ne 0) { throw "PS2 host compilation failed." }
    & "tests/host/test_ps2.exe"
    if ($LASTEXITCODE -ne 0) { throw "PS2 regression failed." }
    & gcc -std=c11 -Wall -Wextra -Werror -I tests/host `
        tests/host/test_timebase.c -o tests/host/test_timebase.exe
    if ($LASTEXITCODE -ne 0) { throw "Timebase host compilation failed." }
    & "tests/host/test_timebase.exe"
    if ($LASTEXITCODE -ne 0) { throw "Timebase regression failed." }
    & gcc -std=c11 -Wall -Wextra -Werror -I tests/host -I Bsp/Inc -I App/Inc `
        tests/host/test_tof.c App/Src/tof.c App/Src/front_guard.c -o tests/host/test_tof.exe
    if ($LASTEXITCODE -ne 0) { throw "ToF host compilation failed." }
    & "tests/host/test_tof.exe"
    if ($LASTEXITCODE -ne 0) { throw "ToF regression failed." }
    & gcc -std=c11 -Wall -Wextra -Werror -I tests/host/tof_bus -I Bsp/Inc `
        tests/host/test_tof_bus.c Bsp/Src/tof_bus.c -o tests/host/test_tof_bus.exe
    if ($LASTEXITCODE -ne 0) { throw "ToF bus compilation failed." }
    & "tests/host/test_tof_bus.exe"
    if ($LASTEXITCODE -ne 0) { throw "ToF bus regression failed." }
} finally {
    Pop-Location
}
