@echo off
setlocal
title BUCT Clash Fix

powershell -NoProfile -ExecutionPolicy Bypass -Command "$p='%~f0'; $raw=[System.IO.File]::ReadAllText($p); $marker='### POWERSHELL ###'; $idx=$raw.LastIndexOf($marker); if($idx -lt 0){ throw 'PowerShell payload not found.' }; Invoke-Expression $raw.Substring($idx + $marker.Length)"

echo.
echo Done. If Edge is still open, fully close Edge and reopen it.
echo You may also click Restart Core / Apply Config in Clash Verge.
pause
exit /b

### POWERSHELL ###
$ErrorActionPreference = 'Stop'

$base = Join-Path $env:APPDATA 'io.github.clash-verge-rev.clash-verge-rev'
$profilesYaml = Join-Path $base 'profiles.yaml'
$profilesDir = Join-Path $base 'profiles'
$runtimePath = Join-Path $base 'clash-verge.yaml'
$vergePath = Join-Path $base 'verge.yaml'

$fallbackRules = Join-Path $profilesDir 'rxNMsAeYHHOd.yaml'
$fallbackMerge = Join-Path $profilesDir 'mE6Q9IpEqsGO.yaml'

$neededRules = @(
  'DOMAIN,portal.buct.edu.cn,DIRECT',
  'DOMAIN,course.buct.edu.cn,DIRECT',
  'DOMAIN-SUFFIX,buct.edu.cn,DIRECT',
  'IP-CIDR,121.195.153.209/32,DIRECT,no-resolve',
  'IP-CIDR,121.195.154.229/32,DIRECT,no-resolve'
)

$neededHosts = [ordered]@{
  'portal.buct.edu.cn' = '121.195.153.209'
  'course.buct.edu.cn' = '121.195.154.229'
}

$bypassItems = @(
  '*.buct.edu.cn',
  'portal.buct.edu.cn',
  'course.buct.edu.cn',
  '121.195.153.209',
  '121.195.154.229'
)

function Write-Step($message) {
  Write-Host "[BUCT fix] $message"
}

function Backup-ExistingFiles($paths) {
  $stamp = Get-Date -Format 'yyyyMMdd-HHmmss'
  $backupDir = Join-Path $base "codex-backups\bat-$stamp"
  New-Item -ItemType Directory -Force -Path $backupDir | Out-Null

  foreach ($path in $paths) {
    if (Test-Path -LiteralPath $path) {
      $name = ($path -replace '[:\\\/]', '_')
      Copy-Item -LiteralPath $path -Destination (Join-Path $backupDir $name) -Force
    }
  }

  $reg = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'
  if (Test-Path -LiteralPath $reg) {
    (Get-ItemProperty -LiteralPath $reg).ProxyOverride |
      Set-Content -LiteralPath (Join-Path $backupDir 'Windows_ProxyOverride.txt') -Encoding UTF8
  }

  Write-Step "Backup saved: $backupDir"
}

function Get-CurrentEnhancementFile($kind, $fallbackPath) {
  if (!(Test-Path -LiteralPath $profilesYaml)) {
    return $fallbackPath
  }

  $text = Get-Content -LiteralPath $profilesYaml -Raw -Encoding UTF8
  $currentMatch = [regex]::Match($text, '(?m)^current:\s*(\S+)\s*$')
  if (!$currentMatch.Success) {
    return $fallbackPath
  }

  $currentUid = [regex]::Escape($currentMatch.Groups[1].Value)
  $currentBlockMatch = [regex]::Match($text, "(?ms)^-\s+uid:\s*$currentUid\s*\r?\n.*?(?=^-\s+uid:|\z)")
  if (!$currentBlockMatch.Success) {
    return $fallbackPath
  }

  $enhancementUidMatch = [regex]::Match($currentBlockMatch.Value, "(?m)^\s+${kind}:\s*(\S+)\s*$")
  if (!$enhancementUidMatch.Success -or $enhancementUidMatch.Groups[1].Value -eq 'null') {
    return $fallbackPath
  }

  $enhancementUid = [regex]::Escape($enhancementUidMatch.Groups[1].Value)
  $enhancementBlockMatch = [regex]::Match($text, "(?ms)^-\s+uid:\s*$enhancementUid\s*\r?\n.*?(?=^-\s+uid:|\z)")
  if (!$enhancementBlockMatch.Success) {
    return $fallbackPath
  }

  $fileMatch = [regex]::Match($enhancementBlockMatch.Value, '(?m)^\s+file:\s*(\S+)\s*$')
  if (!$fileMatch.Success) {
    return $fallbackPath
  }

  return (Join-Path $profilesDir $fileMatch.Groups[1].Value)
}

function Read-LinesOrDefault($path, $defaultLines) {
  if (Test-Path -LiteralPath $path) {
    $list = [System.Collections.Generic.List[string]]::new([string[]](Get-Content -LiteralPath $path -Encoding UTF8))
    return ,$list
  }
  $list = [System.Collections.Generic.List[string]]::new([string[]]$defaultLines)
  return ,$list
}

function Save-Lines($path, $lines) {
  $dir = Split-Path -Parent $path
  New-Item -ItemType Directory -Force -Path $dir | Out-Null
  Set-Content -LiteralPath $path -Value ([string[]]$lines) -Encoding UTF8
}

function Ensure-EnhancementRules($path) {
  $default = @(
    '# Profile Enhancement Rules Template for Clash Verge',
    '',
    'prepend: []',
    '',
    'append: []',
    '',
    'delete: []'
  )

  $lines = Read-LinesOrDefault $path $default
  $prependIndex = -1
  for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^\s*prepend:\s*(\[\])?\s*$') {
      $prependIndex = $i
      break
    }
  }

  if ($prependIndex -lt 0) {
    $insert = [string[]](@('prepend:') + ($neededRules | ForEach-Object { "  - $_" }) + @(''))
    $lines.InsertRange(0, $insert)
  } else {
    if ($lines[$prependIndex] -match '\[\]') {
      $lines[$prependIndex] = 'prepend:'
    }

    $missing = @()
    $wholeText = ($lines -join "`n")
    foreach ($rule in $neededRules) {
      if ($wholeText -notmatch [regex]::Escape($rule)) {
        $missing += "  - $rule"
      }
    }

    if ($missing.Count -gt 0) {
      $lines.InsertRange($prependIndex + 1, [string[]]$missing)
    }
  }

  if (($lines -join "`n") -notmatch '(?m)^\s*append:') {
    $lines.Add('')
    $lines.Add('append: []')
  }
  if (($lines -join "`n") -notmatch '(?m)^\s*delete:') {
    $lines.Add('')
    $lines.Add('delete: []')
  }

  Save-Lines $path $lines
  Write-Step "Rules enhanced: $path"
}

function Ensure-HostsBlock($path) {
  $default = @(
    '# Profile Enhancement Merge Template for Clash Verge',
    ''
  )

  $lines = Read-LinesOrDefault $path $default

  for ($i = $lines.Count - 1; $i -ge 0; $i--) {
    if ($lines[$i] -match '^\s*(portal\.buct\.edu\.cn|course\.buct\.edu\.cn):\s*') {
      $lines.RemoveAt($i)
    }
  }

  $hostsIndex = -1
  for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^hosts:\s*(\{\})?\s*$') {
      $hostsIndex = $i
      break
    }
  }

  $hostLines = [string[]]($neededHosts.GetEnumerator() | ForEach-Object { "  $($_.Key): $($_.Value)" })

  if ($hostsIndex -lt 0) {
    if ($lines.Count -gt 0 -and $lines[$lines.Count - 1] -ne '') {
      $lines.Add('')
    }
    $lines.Add('hosts:')
    $lines.AddRange($hostLines)
  } else {
    if ($lines[$hostsIndex] -match '\{\}') {
      $lines[$hostsIndex] = 'hosts:'
    }
    $lines.InsertRange($hostsIndex + 1, $hostLines)
  }

  Save-Lines $path $lines
  Write-Step "Hosts enhanced: $path"
}

function Ensure-RuntimeConfig($path) {
  if (!(Test-Path -LiteralPath $path)) {
    Write-Step "Runtime config not found, skipped: $path"
    return
  }

  $lines = Read-LinesOrDefault $path @()

  for ($i = $lines.Count - 1; $i -ge 0; $i--) {
    if ($lines[$i] -match '^\s*(portal\.buct\.edu\.cn|course\.buct\.edu\.cn):\s*') {
      $lines.RemoveAt($i)
    }
  }

  $hostsIndex = -1
  for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^hosts:\s*(\{\})?\s*$') {
      $hostsIndex = $i
      break
    }
  }

  $hostLines = [string[]]($neededHosts.GetEnumerator() | ForEach-Object { "  $($_.Key): $($_.Value)" })
  if ($hostsIndex -lt 0) {
    $profileIndex = -1
    for ($i = 0; $i -lt $lines.Count; $i++) {
      if ($lines[$i] -match '^profile:\s*$') {
        $profileIndex = $i
        break
      }
    }

    if ($profileIndex -ge 0) {
      $insertAt = $profileIndex + 1
      while ($insertAt -lt $lines.Count -and $lines[$insertAt] -match '^\s{2,}\S') {
        $insertAt++
      }
      $lines.InsertRange($insertAt, [string[]](@('hosts:') + $hostLines))
    } else {
      $lines.InsertRange(0, [string[]](@('hosts:') + $hostLines + @('')))
    }
  } else {
    if ($lines[$hostsIndex] -match '\{\}') {
      $lines[$hostsIndex] = 'hosts:'
    }
    $lines.InsertRange($hostsIndex + 1, $hostLines)
  }

  $rulesIndex = -1
  for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^rules:\s*$') {
      $rulesIndex = $i
      break
    }
  }

  if ($rulesIndex -ge 0) {
    $wholeText = ($lines -join "`n")
    $missing = @()
    foreach ($rule in $neededRules) {
      if ($wholeText -notmatch [regex]::Escape($rule)) {
        $missing += "- $rule"
      }
    }
    if ($missing.Count -gt 0) {
      $lines.InsertRange($rulesIndex + 1, [string[]]$missing)
    }
  }

  Save-Lines $path $lines
  Write-Step "Runtime config patched: $path"
}

function Ensure-VergeBypass($path) {
  if (!(Test-Path -LiteralPath $path)) {
    Write-Step "Verge settings not found, skipped: $path"
    return
  }

  $lines = Read-LinesOrDefault $path @()
  $defaultBypass = 'localhost;127.*;192.168.*;10.*;172.16.*;172.17.*;172.18.*;172.19.*;172.20.*;172.21.*;172.22.*;172.23.*;172.24.*;172.25.*;172.26.*;172.27.*;172.28.*;172.29.*;172.30.*;172.31.*;<local>'
  $bypass = ($defaultBypass.Split(';') + $bypassItems | Where-Object { $_ } | Select-Object -Unique) -join ';'

  $foundDefault = $false
  $foundBypass = $false
  for ($i = 0; $i -lt $lines.Count; $i++) {
    if ($lines[$i] -match '^use_default_bypass:') {
      $lines[$i] = 'use_default_bypass: true'
      $foundDefault = $true
    }
    if ($lines[$i] -match '^system_proxy_bypass:') {
      $lines[$i] = "system_proxy_bypass: $bypass"
      $foundBypass = $true
    }
  }

  if (!$foundDefault) {
    $lines.Add('use_default_bypass: true')
  }
  if (!$foundBypass) {
    $lines.Add("system_proxy_bypass: $bypass")
  }

  Save-Lines $path $lines
  Write-Step "Verge bypass saved: $path"
}

function Ensure-WindowsProxyBypass {
  $reg = 'HKCU:\Software\Microsoft\Windows\CurrentVersion\Internet Settings'
  $current = (Get-ItemProperty -LiteralPath $reg).ProxyOverride
  $items = @()
  if ($current) {
    $items += $current -split ';' | Where-Object { $_ }
  }
  foreach ($item in $bypassItems) {
    if ($items -notcontains $item) {
      $items += $item
    }
  }

  $newValue = ($items | Select-Object -Unique) -join ';'
  Set-ItemProperty -LiteralPath $reg -Name ProxyOverride -Value $newValue

  Add-Type -Namespace WinInet -Name NativeMethods -MemberDefinition @'
[System.Runtime.InteropServices.DllImport("wininet.dll", SetLastError=true)]
public static extern bool InternetSetOption(System.IntPtr hInternet, int dwOption, System.IntPtr lpBuffer, int dwBufferLength);
'@
  [WinInet.NativeMethods]::InternetSetOption([IntPtr]::Zero, 39, [IntPtr]::Zero, 0) | Out-Null
  [WinInet.NativeMethods]::InternetSetOption([IntPtr]::Zero, 37, [IntPtr]::Zero, 0) | Out-Null

  Write-Step 'Windows proxy bypass updated.'
}

function Test-MihomoConfig {
  $mihomo = 'D:\clash\verge-mihomo.exe'
  if ((Test-Path -LiteralPath $mihomo) -and (Test-Path -LiteralPath $runtimePath)) {
    Write-Step 'Testing mihomo config...'
    & $mihomo -t -f $runtimePath
    if ($LASTEXITCODE -ne 0) {
      throw "Config test failed with exit code $LASTEXITCODE."
    }
  } else {
    Write-Step 'mihomo config test skipped.'
  }
}

if (!(Test-Path -LiteralPath $base)) {
  throw "Clash Verge config directory not found: $base"
}

$rulesPath = Get-CurrentEnhancementFile 'rules' $fallbackRules
$mergePath = Get-CurrentEnhancementFile 'merge' $fallbackMerge

Backup-ExistingFiles @($profilesYaml, $rulesPath, $mergePath, $runtimePath, $vergePath)

Ensure-EnhancementRules $rulesPath
Ensure-HostsBlock $mergePath
Ensure-RuntimeConfig $runtimePath
Ensure-VergeBypass $vergePath
Ensure-WindowsProxyBypass

ipconfig /flushdns | Out-Host
Test-MihomoConfig

Write-Step 'All patches applied.'
