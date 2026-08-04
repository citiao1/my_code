param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [Parameter(Mandatory = $true)]
    [string]$OutputPath
)

$ErrorActionPreference = 'Stop'

$outputDirectory = Split-Path -Parent $OutputPath
New-Item -ItemType Directory -Path $outputDirectory -Force | Out-Null
if (Test-Path -LiteralPath $OutputPath) {
    Remove-Item -LiteralPath $OutputPath -Force
}

$word = $null
$document = $null
$missing = [Type]::Missing
try {
    $wordPidsBefore = @(Get-Process -Name WINWORD -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id)
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0
    $wordPidsAfter = @(Get-Process -Name WINWORD -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id)
    $newWordPids = @($wordPidsAfter | Where-Object { $_ -notin $wordPidsBefore })
    Write-Output ("WORD_PID=" + ($newWordPids -join ','))

    $document = $word.Documents.Open(
        $InputPath,
        $false,
        $false,
        $false,
        $missing,
        $missing,
        $false,
        $missing,
        $missing,
        $missing,
        $missing,
        $false,
        $true
    )
    $document.SaveAs2($OutputPath, 16)
    Write-Output "SAVED=$OutputPath"
}
finally {
    if ($null -ne $document) {
        try { $document.Close(0) } catch {}
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($document) | Out-Null
    }
    if ($null -ne $word) {
        try { $word.Quit(0) } catch {}
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($word) | Out-Null
    }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
