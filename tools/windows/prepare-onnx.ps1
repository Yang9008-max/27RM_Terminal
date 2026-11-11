# Provide the include layout used by the upstream ONNX includes.
$ErrorActionPreference = 'Stop'
$OrtRepo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$OrtOfficial = Join-Path $OrtRepo '.deps\onnxruntime-win-x64-1.31.1'
if (-not (Test-Path -LiteralPath "$OrtOfficial\lib\onnxruntime.dll")) {
    throw 'Download the official ONNX Runtime 1.31.1 SDK first. See windows-setup.md.'
}
New-Item -ItemType Directory -Force -Path "$OrtOfficial\include\onnxruntime" | Out-Null
Get-ChildItem -LiteralPath "$OrtOfficial\include" -File |
    Copy-Item -Destination "$OrtOfficial\include\onnxruntime" -Force
Write-Host 'ONNX Runtime header layout prepared.'
