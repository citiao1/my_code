param(
    [switch]$Force
)

$ErrorActionPreference = "Stop"

function Write-Step {
    param([string]$Message)
    Write-Host "==> $Message" -ForegroundColor Cyan
}

$projectRoot = (Get-Location).Path
Write-Step "Project: $projectRoot"

$codegraph = Get-Command codegraph -ErrorAction SilentlyContinue
if (-not $codegraph) {
    Write-Host "CodeGraph is not installed or not available in PATH." -ForegroundColor Yellow
    Write-Host ""
    Write-Host "Install it first, then reopen PowerShell:"
    Write-Host "  irm https://raw.githubusercontent.com/colbymchenry/codegraph/main/install.ps1 | iex"
    Write-Host ""
    Write-Host "Or with npm:"
    Write-Host "  npm i -g @colbymchenry/codegraph"
    exit 1
}

Write-Step "CodeGraph: $($codegraph.Source)"
codegraph version

$indexPath = Join-Path $projectRoot ".codegraph"
if ((Test-Path $indexPath) -and -not $Force) {
    Write-Step "Existing .codegraph index found"
    codegraph status
    exit 0
}

if ((Test-Path $indexPath) -and $Force) {
    Write-Step "Force enabled; removing existing .codegraph index"
    Remove-Item -LiteralPath $indexPath -Recurse -Force
}

Write-Step "Initializing CodeGraph index"
codegraph init

Write-Step "Status"
codegraph status

Write-Host ""
Write-Host "Done. Your AI coding agent can now use CodeGraph for this project." -ForegroundColor Green
