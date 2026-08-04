param(
    [Parameter(Mandatory = $true)]
    [string]$OutputDirectory,

    [Parameter(Mandatory = $true)]
    [string[]]$InputFiles,

    [switch]$SkipExport
)

$ErrorActionPreference = 'Stop'

New-Item -ItemType Directory -Path $OutputDirectory -Force | Out-Null

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

    foreach ($inputPath in $InputFiles) {
        $file = [IO.Path]::GetFileName($inputPath)
        if (-not (Test-Path -LiteralPath $inputPath)) {
            Write-Warning "Missing: $inputPath"
            continue
        }

        Write-Output "OPEN=$file"
        $document = $word.Documents.Open($inputPath, $false, $true, $false)
        $document.Repaginate()

        $pageCount = $document.ComputeStatistics(2)
        $section = $document.Sections.Item(1).PageSetup
        Write-Output ("INFO={0}|pages={1}|page={2:F1}x{3:F1}|margins={4:F1},{5:F1},{6:F1},{7:F1}" -f `
            $file, $pageCount, $section.PageWidth, $section.PageHeight, `
            $section.TopMargin, $section.BottomMargin, $section.LeftMargin, $section.RightMargin)

        $paragraphLimit = [Math]::Min(30, $document.Paragraphs.Count)
        for ($i = 1; $i -le $paragraphLimit; $i++) {
            $paragraphText = $document.Paragraphs.Item($i).Range.Text -replace "[\r\a]", ''
            if (-not [string]::IsNullOrWhiteSpace($paragraphText)) {
                Write-Output ("TEXT={0}|{1:D2}|{2}" -f $file, $i, $paragraphText)
            }
        }

        if (-not $SkipExport) {
            $safeName = [IO.Path]::GetFileNameWithoutExtension($file)
            $pdfPath = Join-Path $OutputDirectory ($safeName + '.pdf')
            $document.ExportAsFixedFormat($pdfPath, 17, $false, 0, 3, 1, 1)
            Write-Output "PDF=$pdfPath"
        }

        $document.Close(0)
        [Runtime.InteropServices.Marshal]::FinalReleaseComObject($document) | Out-Null
        $document = $null
    }
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
