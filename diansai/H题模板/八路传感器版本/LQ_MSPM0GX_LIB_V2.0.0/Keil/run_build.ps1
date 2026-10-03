$baseDir = $PSScriptRoot
$projectPath = Join-Path $baseDir 'LQ_MSPM0GX_LIB.uvprojx'
$logPath = Join-Path $baseDir 'build_sflp_bank.txt'

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "D:\keil5\UV4\UV4.exe"
$psi.Arguments = "-r `"$projectPath`" -j0"
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true
$p = [System.Diagnostics.Process]::Start($psi)
$out = $p.StandardOutput.ReadToEnd()
$err = $p.StandardError.ReadToEnd()
$p.WaitForExit()
$combined = $out + $err
[System.IO.File]::WriteAllText($logPath, $combined, [System.Text.Encoding]::ASCII)
Write-Host "ExitCode: $($p.ExitCode)"
