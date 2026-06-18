$ErrorActionPreference = "Stop"

function Invoke-Checked {
    param(
        [Parameter(Mandatory = $true)]
        [string] $FilePath,
        [Parameter(ValueFromRemainingArguments = $true)]
        [string[]] $Arguments
    )

    & $FilePath @Arguments
    if ($LASTEXITCODE -ne 0) {
        throw "$FilePath failed with exit code $LASTEXITCODE"
    }
}

Invoke-Checked python -m pip install -r requirements.txt
Invoke-Checked python -m pip install pyinstaller

if (Test-Path .\build) {
    Remove-Item .\build -Recurse -Force
}
if (Test-Path .\dist) {
    Remove-Item .\dist -Recurse -Force
}
if (Test-Path .\AStockSentimentSystem.spec) {
    Remove-Item .\AStockSentimentSystem.spec -Force
}

Invoke-Checked python -m PyInstaller `
    --name "AStockSentimentSystem" `
    --onedir `
    --console `
    --clean `
    --add-data "app.py;." `
    --add-data "src;src" `
    --add-data ".streamlit;.streamlit" `
    --collect-all streamlit `
    --exclude-module matplotlib `
    --exclude-module plotly.matplotlylib `
    --exclude-module IPython `
    --exclude-module ipywidgets `
    --exclude-module jupyterlab `
    --exclude-module notebook `
    --exclude-module openpyxl `
    --hidden-import streamlit.web.cli `
    launcher.py

Write-Host ""
Write-Host "Build finished: dist\AStockSentimentSystem\AStockSentimentSystem.exe"
