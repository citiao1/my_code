$ErrorActionPreference = "Stop"

# Usage:
#   1. Set your key in this PowerShell window:
#      $env:OPENAI_API_KEY="your_real_key_here"
#   2. Run from ai_imagegen:
#      .\scripts\generate_anime_image.ps1

if (-not $env:OPENAI_API_KEY) {
    Write-Error 'OPENAI_API_KEY is not set. Run: $env:OPENAI_API_KEY="your_real_key_here"'
}

$env:OPENAI_BASE_URL = "https://api2.68886868.xyz/v1"

$python = "C:\Users\28097\.cache\codex-runtimes\codex-primary-runtime\dependencies\python\python.exe"
$imageGen = "C:\Users\28097\.codex\skills\.system\imagegen\scripts\image_gen.py"

$scriptDir = Split-Path -Parent $PSCommandPath
$projectDir = Split-Path -Parent $scriptDir
$imageDir = Join-Path $projectDir "images"
$promptDir = Join-Path $projectDir "prompts"
$stamp = Get-Date -Format "yyyyMMdd_HHmmss"
$outFile = Join-Path $imageDir "moonlight_anime_character_$stamp.png"
$promptFile = Join-Path $promptDir "moonlight_anime_character_$stamp.txt"

New-Item -ItemType Directory -Force -Path $imageDir | Out-Null
New-Item -ItemType Directory -Force -Path $promptDir | Out-Null

$promptLines = @(
    "Create a high-quality anime fantasy character illustration.",
    "Subject: an adult woman, clearly over 20 years old, elegant and ethereal.",
    "Full-body vertical character art, game character design sheet style.",
    "Outfit: a moonlight-white chiffon fantasy dress, pure white, silver, and pale lavender color palette.",
    "The dress should feel like woven moonlight: graceful, airy, layered, and magical.",
    "Details: refined amethyst crystal ornament on the chest, ice-crystal flower decorations, translucent flowing ribbons, irregular lotus-like skirt hem, white silk ribbon ornaments on one thigh and around the ankles.",
    "Pose and mood: floating barefoot in silver moonlight, calm gentle expression, long hair flowing in the wind.",
    "Scene: night sky, full moon, stars, soft mist and subtle glowing particles.",
    "Style: polished 2D anime illustration, clean line art, delicate shading, soft lighting, highly detailed clothing materials, dreamy sacred atmosphere.",
    "Constraints: tasteful and non-explicit, no erotic pose, no excessive nudity, no childlike appearance, no text, no watermark.",
    "Avoid: low quality, bad anatomy, deformed hands, extra fingers, broken face, blurry image, explicit nudity, sexualized pose, minor-looking character."
)

$prompt = $promptLines -join "`n"
$prompt | Set-Content -LiteralPath $promptFile -Encoding UTF8

& $python $imageGen generate `
  --model "gpt-image-2" `
  --prompt-file $promptFile `
  --size "1024x1536" `
  --quality "high" `
  --out $outFile `
  --force

Write-Host "Done: $outFile"
