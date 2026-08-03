param(
    [Parameter(Mandatory = $true)]
    [string]$InputPath,

    [string[]]$TargetTexts = @()
)

$ErrorActionPreference = 'Stop'

$word = $null
$document = $null
try {
    $wordPidsBefore = @(Get-Process -Name WINWORD -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id)
    $word = New-Object -ComObject Word.Application
    $word.Visible = $false
    $word.DisplayAlerts = 0
    $wordPidsAfter = @(Get-Process -Name WINWORD -ErrorAction SilentlyContinue | Select-Object -ExpandProperty Id)
    $newWordPids = @($wordPidsAfter | Where-Object { $_ -notin $wordPidsBefore })
    Write-Output ("WORD_PID=" + ($newWordPids -join ','))

    $document = $word.Documents.Open($InputPath, $false, $false, $false)
    [void]$document.Fields.Update()
    foreach ($toc in $document.TablesOfContents) {
        $toc.Update()
    }
    foreach ($section in $document.Sections) {
        foreach ($header in $section.Headers) {
            if ($header.Exists) { [void]$header.Range.Fields.Update() }
        }
        foreach ($footer in $section.Footers) {
            if ($footer.Exists) { [void]$footer.Range.Fields.Update() }
        }
    }

    $document.Repaginate()
    $pageCount = $document.ComputeStatistics(2)
    Write-Output "PAGES=$pageCount"

    foreach ($target in $TargetTexts) {
        $range = $document.Content.Duplicate
        $find = $range.Find
        $find.ClearFormatting()
        $find.Text = $target
        $find.Forward = $true
        $find.Wrap = 0
        if ($find.Execute()) {
            $page = $range.Information(3)
            Write-Output ("POSITION={0}|page={1}" -f $target, $page)
        }
    }

    $document.Save()
    Write-Output "SAVED=$InputPath"
}
finally {
    if ($null -ne $document) {
        try { $document.Close(-1) } catch {}
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($document) | Out-Null
    }
    if ($null -ne $word) {
        try { $word.Quit(0) } catch {}
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($word) | Out-Null
    }
    [GC]::Collect()
    [GC]::WaitForPendingFinalizers()
}
