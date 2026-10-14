$ErrorActionPreference = "Continue"
Set-Location $PSScriptRoot
$baseDir = (Get-Location).Path
$projectPath = Join-Path $baseDir 'Keil\LQ_MSPM0GX_LIB.uvprojx'
$logPath = Join-Path $baseDir 'build_check.log'

Write-Host "BaseDir: $baseDir"
Write-Host "Project: $projectPath"
Write-Host "Log: $logPath"

$psi = New-Object System.Diagnostics.ProcessStartInfo
$psi.FileName = "D:\keil5\UV4\UV4.exe"
$psi.Arguments = "-r `"$projectPath`" -j0"
$psi.WorkingDirectory = $baseDir
$psi.RedirectStandardOutput = $true
$psi.RedirectStandardError = $true
$psi.UseShellExecute = $false
$psi.CreateNoWindow = $true

try {
    $p = [System.Diagnostics.Process]::Start($psi)
    $out = $p.StandardOutput.ReadToEnd()
    $err = $p.StandardError.ReadToEnd()
    $p.WaitForExit()
    $combined = $out + $err
    [System.IO.File]::WriteAllText($logPath, $combined, [System.Text.Encoding]::UTF8)
    Write-Host "ExitCode: $($p.ExitCode)"
} catch {
    Write-Host "Error: $_"
}
