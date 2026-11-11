. "$PSScriptRoot\environment.ps1"
$BuildRoot = Join-Path $RepoRoot 'build\windows-ucrt64'
$ClientExe = Join-Path $BuildRoot 'RoboMasterClient2025.exe'
if (-not (Test-Path -LiteralPath $ClientExe)) {
    throw 'Build the client first: powershell -ExecutionPolicy Bypass -File tools/windows/build.ps1'
}
Push-Location $BuildRoot
try { & $ClientExe } finally { Pop-Location }
