. "$PSScriptRoot\environment.ps1"
$ArchivePath = Join-Path $RepoRoot '.deps\downloads\protoc-33.2-win64.zip'
$ProtocRoot = Join-Path $RepoRoot '.deps\protoc-33.2'
$ArchiveUrl = 'https://github.com/protocolbuffers/protobuf/releases/download/v33.2/protoc-33.2-win64.zip'
$ExpectedHash = '376770cd4073beb63db56fdd339260edb9957b3c4472e05a75f5f9ec8f98d8f5'
# This compiler emits Python 6.33.2 code, compatible with sim/pyproject.toml.
# C++ continues to use the compiler shipped with its matching native runtime.
if (-not (Test-Path -LiteralPath $ArchivePath)) {
    New-Item -ItemType Directory -Force -Path (Split-Path $ArchivePath) | Out-Null
    Invoke-WebRequest -Uri $ArchiveUrl -OutFile $ArchivePath -UseBasicParsing
}
if ((Get-FileHash -LiteralPath $ArchivePath -Algorithm SHA256).Hash.ToLowerInvariant() -ne $ExpectedHash) {
    throw 'protoc archive SHA-256 mismatch; remove the invalid archive and retry.'
}
Expand-Archive -LiteralPath $ArchivePath -DestinationPath $ProtocRoot -Force
& "$RepoRoot\sim\.venv\Scripts\python.exe" "$RepoRoot\tools\release\generate_sim_protobuf.py" --protoc "$ProtocRoot\bin\protoc.exe"
if ($LASTEXITCODE -ne 0) { throw 'Simulator Protobuf generation failed.' }
