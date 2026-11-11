param([switch]$Test, [int]$Jobs = 4)
. "$PSScriptRoot\environment.ps1"
if (-not (Test-Path -LiteralPath "$OnnxRoot\include\onnxruntime\onnxruntime_cxx_api.h")) {
    & "$PSScriptRoot\prepare-onnx.ps1"
}
Push-Location $RepoRoot
try {
    & "$NativeBin\cmake.exe" --preset windows-ucrt64
    if ($LASTEXITCODE -ne 0) { throw 'CMake configuration failed.' }
    & "$NativeBin\cmake.exe" --build --preset windows-ucrt64 --parallel $Jobs
    if ($LASTEXITCODE -ne 0) { throw 'Build failed.' }
    # Windows searches System32 before PATH. Keep the matching SDK DLL beside
    # every executable to avoid loading the Windows ONNX placeholder DLL.
    $OutputRoot = Join-Path $RepoRoot 'build\windows-ucrt64'
    Copy-Item -LiteralPath "$OnnxRoot\lib\onnxruntime.dll" -Destination $OutputRoot -Force
    if (Test-Path -LiteralPath "$OutputRoot\tests") {
        Copy-Item -LiteralPath "$OnnxRoot\lib\onnxruntime.dll" -Destination "$OutputRoot\tests" -Force
    }
    if ($Test) {
        & "$NativeBin\ctest.exe" --preset windows-ucrt64
        if ($LASTEXITCODE -ne 0) { throw 'CTest failed.' }
    }
} finally { Pop-Location }
