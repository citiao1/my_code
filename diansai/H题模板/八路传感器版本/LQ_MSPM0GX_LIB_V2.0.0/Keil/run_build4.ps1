$ErrorActionPreference = "Continue"
$baseDir = "d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\Keil"
$projectPath = Join-Path $baseDir "LQ_MSPM0GX_LIB.uvprojx"
$logPath = "d:\桌面\My_ele_contest2\LQ_MSPM0GX_LIB_V2.0.0\build_new.log"

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "D:\keil5\UV4\UV4.exe"
$psi.Arguments = "-r `"$projectPath`" -j0"
$psi.WorkingDirectory = $baseDir
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true
$p = [System.Diagnostics.Process]::Start($psi)
$out = $p.StandardOutput.ReadToEnd()
$err = $p.StandardError.ReadToEnd()
$p.WaitForExit()
$combined = $out + $err
[System.IO.File]::WriteAllText($logPath, $combined, [System.Text.Encoding]::UTF8)
Write-Host "ExitCode: $($p.ExitCode)"
