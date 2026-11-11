$ErrorActionPreference = 'Stop'
$RepoRoot = (Resolve-Path (Join-Path $PSScriptRoot '..\..')).Path
$SdkRoot = Join-Path $RepoRoot '.deps\msys64'
$NativeBin = Join-Path $SdkRoot 'ucrt64\bin'
$OnnxRoot = Join-Path $RepoRoot '.deps\onnxruntime-win-x64-1.31.1'
if (-not (Test-Path -LiteralPath (Join-Path $NativeBin 'qmake6.exe'))) {
    throw "Qt SDK missing: $NativeBin. See docs/development/windows-setup.md."
}
$env:PATH = "$NativeBin;$(Join-Path $SdkRoot 'usr\bin');$(Join-Path $OnnxRoot 'lib');$env:PATH"
$env:PKG_CONFIG_PATH = Join-Path $SdkRoot 'ucrt64\lib\pkgconfig'
$env:QT_PLUGIN_PATH = Join-Path $SdkRoot 'ucrt64\share\qt6\plugins'
$env:QML_IMPORT_PATH = Join-Path $SdkRoot 'ucrt64\share\qt6\qml'
$env:QML2_IMPORT_PATH = $env:QML_IMPORT_PATH
$env:PYTHONUTF8 = '1'
$env:PYTHONIOENCODING = 'utf-8'
$env:CCACHE_DIR = Join-Path $RepoRoot '.deps\ccache'
# Native GCC uses Windows TEMP/TMP. Keep compiler scratch files in the workspace.
$BuildTemp = Join-Path $RepoRoot 'tmp\native-build'
New-Item -ItemType Directory -Force -Path $BuildTemp | Out-Null
$env:TEMP = $BuildTemp
$env:TMP = $BuildTemp
