$ErrorActionPreference = 'Stop'
$Source = Get-Content -LiteralPath (Join-Path $PSScriptRoot 'start-serial-console.ps1') -Raw
$Errors = $null
$null = [System.Management.Automation.Language.Parser]::ParseInput($Source,[ref]$null,[ref]$Errors)
if ($Errors.Count) { throw 'Direct launcher syntax errors.' }
$Begin = $Source.IndexOf('# Release only')
$End = $Source.IndexOf('$BrowserCandidates =')
if ($Begin -lt 0 -or $End -le $Begin) { throw 'Launcher ownership section missing.' }
$SectionText=$Source.Substring($Begin,$End-$Begin).Replace('$PSScriptRoot',("'"+$PSScriptRoot.Replace("'","''")+"'"))
$Section = [scriptblock]::Create($SectionText)
$WebRoot = Split-Path -Parent $PSScriptRoot
$Port = 8088
$ExactBridge = (Resolve-Path (Join-Path $PSScriptRoot 'motor_vofa_bridge.py')).Path

# No real processes, listeners, web server or browser can be altered by this test.
function Get-NetTCPConnection {
    param($State,$ErrorAction)
    $script:Enumerations++
    [pscustomobject]@{LocalPort=8766;OwningProcess=42424}
    [pscustomobject]@{LocalPort=1347;OwningProcess=42424}
    [pscustomobject]@{LocalPort=8088;OwningProcess=43434}
}
function Get-CimInstance {
    param($ClassName,$Filter,$ErrorAction)
    if ($Filter -eq 'ProcessId=42424') {
        if ($script:Alive) { [pscustomobject]@{ProcessId=42424;Name='python.exe';CommandLine=$script:BridgeCommand} }
    } else { [pscustomobject]@{ProcessId=43434;Name='python.exe';CommandLine=$script:WebCommand} }
}
function Stop-Process {
    param($Id,[switch]$Force,$ErrorAction)
    if ($Id -ne 42424) { throw 'Unexpected process mutation.' }
    $script:Stops++;$script:Alive=$false
}
function Start-Process { throw 'The existing web server must be reused.' }
foreach ($Case in @('project','forward-slash','other-bridge','suffix-bridge','other-http')) {
    $script:Alive=$true;$script:Stops=0;$script:Enumerations=0
    $script:BridgeCommand='python.exe "'+$ExactBridge+'" --transport serial'
    $script:WebCommand='python.exe -m http.server 8088 --directory "'+$WebRoot+'"'
    if ($Case -eq 'forward-slash') { $script:BridgeCommand=$script:BridgeCommand.Replace('\','/') }
    if ($Case -eq 'other-bridge') { $script:BridgeCommand='python.exe unrelated.py' }
    if ($Case -eq 'suffix-bridge') { $script:BridgeCommand='python.exe "'+$ExactBridge+'.other"' }
    if ($Case -eq 'other-http') { $script:WebCommand='python.exe -m http.server --directory "C:\unrelated"' }
    $Rejected=$false
    try { . $Section } catch {
        if ($Case -ne 'other-http' -or $_.Exception.Message -notmatch 'belongs to another program') { throw }
        $Rejected=$true
    }
    if ($Rejected -ne ($Case -eq 'other-http')) { throw 'Wrong HTTP ownership decision.' }
    $ExpectedStops=if ($Case -in @('other-bridge','suffix-bridge')) {0} else {1}
    if ($script:Stops -ne $ExpectedStops -or $script:Enumerations -ne 1) { throw "Wrong process/listener handling: $Case" }
}
Write-Host 'PASS: direct launcher syntax; five ownership/reuse cases; one listener enumeration; no real process changes.'
