# Refresh an existing portable SDK and the simulator environment.
# The initial portable archive must be extracted to .deps/msys64 first.
$ErrorActionPreference = 'Stop'
$TaskRepo = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$TaskBash = Join-Path $TaskRepo '.deps\msys64\usr\bin\bash.exe'
if (-not (Test-Path -LiteralPath $TaskBash)) {
    throw 'Portable MSYS2 is missing. See docs/development/windows-setup.md.'
}
$NativePackages = @(
    'gcc', 'cmake', 'ninja', 'pkgconf', 'ccache',
    'qt6-base', 'qt6-declarative', 'qt6-multimedia', 'qt6-serialport',
    'qt6-svg', 'qt6-shadertools', 'qt6-httpserver',
    'protobuf', 'abseil-cpp', 'paho.mqtt.c', 'ffmpeg', 'mosquitto'
) | ForEach-Object { 'mingw-w64-ucrt-x86_64-' + $_ }
$env:MSYSTEM = 'UCRT64'
$env:CHERE_INVOKING = '1'
Push-Location $TaskRepo
try {
    # A core runtime upgrade can terminate its shell. Reopen before continuing.
    & $TaskBash --login -c 'pacman -Syu --noconfirm --disable-download-timeout'
    if ($LASTEXITCODE -ne 0) { throw 'MSYS2 update failed. Rerun after checking its diagnostics.' }
    & $TaskBash --login -c 'pacman -Syu --noconfirm --disable-download-timeout'
    if ($LASTEXITCODE -ne 0) { throw 'MSYS2 full update failed.' }
    & $TaskBash --login -c ('pacman -S --needed --noconfirm --disable-download-timeout ' + ($NativePackages -join ' '))
    if ($LASTEXITCODE -ne 0) { throw 'Native dependency installation failed.' }
    & "$PSScriptRoot\prepare-onnx.ps1"
    $TaskPython = Join-Path $TaskRepo 'sim\.venv\Scripts\python.exe'
    if (-not (Test-Path -LiteralPath $TaskPython)) {
        & py -3.13 -m venv sim/.venv
        if ($LASTEXITCODE -ne 0) { throw 'Creating Python virtual environment failed.' }
    }
    $env:PIP_CACHE_DIR = Join-Path $TaskRepo '.deps\pip-cache'
    & $TaskPython -m pip install --index-url https://pypi.org/simple --timeout 120 -e ./sim
    if ($LASTEXITCODE -ne 0) { throw 'Simulator dependency installation failed.' }
    & $TaskPython -m pip check
    if ($LASTEXITCODE -ne 0) { throw 'Python dependency validation failed.' }
} finally { Pop-Location }
