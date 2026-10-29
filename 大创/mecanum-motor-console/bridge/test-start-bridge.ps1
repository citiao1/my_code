$ErrorActionPreference = "Stop"
$BridgeDir = $PSScriptRoot
$Source = Get-Content -LiteralPath (Join-Path $BridgeDir "start-bridge.ps1") -Raw
$ParseTokens = $null
$ParseErrors = $null
$null = [System.Management.Automation.Language.Parser]::ParseInput(
    $Source, [ref]$ParseTokens, [ref]$ParseErrors)
if ($ParseErrors.Count) { throw "Startup script has syntax errors." }

# Execute the real ownership/restart section, without starting BLE, a browser,
# pip, or any process. Mock the OS boundary so no real process can be stopped.
$Begin = $Source.IndexOf("# Restart only")
$End = $Source.IndexOf('if (Test-Path -LiteralPath $BundledPython)')
if ($Begin -lt 0 -or $End -le $Begin) { throw "Restart section was not found." }
$Restart = [scriptblock]::Create($Source.Substring($Begin, $End - $Begin))
$ExactScript = (Resolve-Path (Join-Path $BridgeDir "motor_vofa_bridge.py")).Path

function Get-NetTCPConnection {
    param($LocalPort, $State, $ErrorAction)
    if ($script:ListenerAlive -and $LocalPort -eq 8766) {
        [pscustomobject]@{ OwningProcess = 42424 }
    }
}
function Get-CimInstance {
    param($ClassName, $Filter, $ErrorAction)
    [pscustomobject]@{
        Name = $script:ProcessName
        CommandLine = $script:ProcessCommand
    }
}
function Stop-Process {
    param($Id, [switch]$Force, $ErrorAction)
    if ($Id -ne 42424) { throw "Unexpected process target." }
    $script:StopCalls++
    $script:ListenerAlive = $false
}

foreach ($Case in @("project", "project-forward-slash", "project-mixed-slash", "other-python", "suffix-path", "forward-slash-suffix", "other-executable")) {
    $script:ListenerAlive = $true
    $script:StopCalls = 0
    $script:ProcessName = "python.exe"
    $script:ProcessCommand = 'python.exe "' + $ExactScript + '" --transport ble'
    if ($Case -eq "project-forward-slash") {
        $script:ProcessCommand = 'python.exe -u "' + $ExactScript.Replace('\', '/') + '" --transport serial'
    } elseif ($Case -eq "project-mixed-slash") {
        $script:ProcessCommand = 'python.exe "' + $ExactScript.Replace('\bridge\', '/bridge/') + '"'
    } elseif ($Case -eq "other-python") {
        $script:ProcessCommand = "python.exe another-bridge.py"
    } elseif ($Case -eq "suffix-path") {
        $script:ProcessCommand = 'python.exe "' + $ExactScript + '.other"'
    } elseif ($Case -eq "forward-slash-suffix") {
        $script:ProcessCommand = 'python.exe "' + $ExactScript.Replace('\', '/') + '.other"'
    } elseif ($Case -eq "other-executable") {
        $script:ProcessName = "unrelated.exe"
    }
    $Rejected = $false
    try { . $Restart } catch {
        if ($_.Exception.Message -notmatch "used by another program") { throw }
        $Rejected = $true
    }
    if ($Case -like "project*") {
        if ($Rejected -or $script:StopCalls -ne 1 -or $script:ListenerAlive) {
            throw "The exact project bridge was not restarted once."
        }
    } elseif (-not $Rejected -or $script:StopCalls -ne 0 -or -not $script:ListenerAlive) {
        throw "An unrelated listener was not preserved: $Case"
    }
}
Write-Host "PASS: startup syntax, exact bridge restart, unrelated process preservation."
