# Builds Release and packs a zip that Definitive Mod Manager (DMM) can import.
#
#   powershell -ExecutionPolicy Bypass -File .\package.ps1
#
# Layout: the archive root holds a bin64\ folder. DMM sends archives that have
# a bin64 folder to the game folder, so the files land in
# <game>\bin64\CombatAwareStamina.asi / .ini.
$ErrorActionPreference = "Stop"
$root = $PSScriptRoot

cmake -S $root -B "$root\build" -A x64
cmake --build "$root\build" --config Release

$stage = Join-Path $root "build\package"
$bin64 = Join-Path $stage "bin64"
if (Test-Path $stage) { Remove-Item $stage -Recurse -Force }
New-Item $bin64 -ItemType Directory -Force | Out-Null

Copy-Item "$root\build\Release\CombatAwareStamina.asi" $bin64
Copy-Item "$root\CombatAwareStamina.ini" $bin64

$zip = Join-Path $root "build\CombatAwareStamina-DMM.zip"
if (Test-Path $zip) { Remove-Item $zip }
Compress-Archive -Path "$stage\*" -DestinationPath $zip
Write-Host "-> $zip" -ForegroundColor Green
