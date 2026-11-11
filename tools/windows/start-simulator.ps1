. "$PSScriptRoot\environment.ps1"
$PythonExe = Join-Path $RepoRoot 'sim\.venv\Scripts\python.exe'
if (-not (Test-Path -LiteralPath $PythonExe)) { throw 'Simulator virtual environment is missing.' }
$RunRoot = Join-Path $RepoRoot 'tmp\windows-simulator'
New-Item -ItemType Directory -Force -Path $RunRoot | Out-Null
$BrokerConfig = Join-Path $RunRoot 'mosquitto.conf'
Set-Content -LiteralPath $BrokerConfig -Encoding ASCII -Value @(
    'listener 3333 127.0.0.1'
    'allow_anonymous true'
    'persistence false'
)
$BrokerProcess = $null
Push-Location (Join-Path $RepoRoot 'sim')
try {
    $Listener = Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue
    if ($Listener) { throw 'Port 3333 is occupied. Stop its owner before starting the simulator.' }
    $BrokerProcess = Start-Process -FilePath "$NativeBin\mosquitto.exe" `
        -ArgumentList @('-c', ('"' + $BrokerConfig + '"')) -WindowStyle Hidden -PassThru `
        -RedirectStandardOutput (Join-Path $RunRoot 'broker.stdout.log') `
        -RedirectStandardError (Join-Path $RunRoot 'broker.stderr.log')
    $BrokerDeadline = (Get-Date).AddSeconds(10)
    do {
        if ($BrokerProcess.HasExited) { throw 'Broker failed. See tmp/windows-simulator/broker.stderr.log.' }
        $Listener = Get-NetTCPConnection -LocalPort 3333 -State Listen -ErrorAction SilentlyContinue
        if ($Listener) { break }
        Start-Sleep -Milliseconds 200
    } while ((Get-Date) -lt $BrokerDeadline)
    if (-not $Listener) { throw 'Broker did not open port 3333.' }
    Write-Host 'Simulator: http://127.0.0.1:8000 ; local MQTT: 127.0.0.1:3333'
    & $PythonExe -m server.main --mqtt-host 127.0.0.1 --mqtt-port 3333 --web-host 127.0.0.1 --web-port 8000
    if ($LASTEXITCODE -ne 0) { throw 'Simulator exited with an error.' }
} finally {
    if ($BrokerProcess -and -not $BrokerProcess.HasExited) { $BrokerProcess.Kill() }
    Pop-Location
}
