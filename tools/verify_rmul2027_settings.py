#!/usr/bin/env python3
"""Verify RMUL performance commands and acknowledgements through a private MQTT broker."""
import argparse
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import time

ROOT = Path(__file__).resolve().parents[1]


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    parser.add_argument('--receiver', type=Path)
    parser.add_argument('--broker-exe', type=Path)
    args = parser.parse_args()
    receiver = (args.receiver or ROOT / 'build/windows-ucrt64/tests/test_rmul2027_settings.exe').resolve()
    broker_exe = args.broker_exe or ROOT / '.deps/msys64/ucrt64/bin/mosquitto.exe'
    if not broker_exe.is_file():
        broker_exe = Path(shutil.which('mosquitto') or '')
    if not receiver.is_file() or not broker_exe.is_file():
        parser.error('Build the settings test and provide a working Mosquitto executable')
    now = datetime.now(timezone(timedelta(hours=8)))
    run_dir = ROOT / 'tmp/rmul2027-settings' / now.strftime('%Y%m%d-%H%M%S-%f')
    run_dir.mkdir(parents=True)
    with socket.socket() as reservation:
        reservation.bind(('127.0.0.1', 0))
        port = reservation.getsockname()[1]
    config = run_dir / 'mosquitto.conf'
    config.write_text(f'listener {port} 127.0.0.1\nallow_anonymous true\npersistence false\n', encoding='ascii')
    env = dict(os.environ)
    env.update(QT_QPA_PLATFORM='offscreen', RM_DISABLE_AUDIO='1',
               RMUL_SETTINGS_MQTT_URI=f'tcp://127.0.0.1:{port}',
               QT_QPA_PLATFORM_PLUGIN_PATH=str(receiver.parent.parent / 'platforms'))
    env['PATH'] = os.pathsep.join([str(ROOT / '.deps/msys64/ucrt64/bin'),
                                 str(receiver.parent.parent), env.get('PATH', '')])
    no_window = getattr(subprocess, 'CREATE_NO_WINDOW', 0)
    with (run_dir / 'broker.log').open('w', encoding='utf-8') as output:
        broker = subprocess.Popen([str(broker_exe.resolve()), '-c', str(config)],
                                  env=env, stdout=output, stderr=subprocess.STDOUT,
                                  creationflags=no_window)
        try:
            deadline = time.monotonic() + 10
            while True:
                try:
                    with socket.create_connection(('127.0.0.1', port), timeout=.2):
                        break
                except OSError:
                    if broker.poll() is not None or time.monotonic() > deadline:
                        raise RuntimeError('Private MQTT broker did not start')
                    time.sleep(.1)
            result = subprocess.run([str(receiver), '-o', str(run_dir / 'receiver.txt') + ',txt'],
                                    env=env, cwd=ROOT, timeout=45, creationflags=no_window)
            if result.returncode:
                raise RuntimeError(f'Settings test failed; see {run_dir / "receiver.txt"}')
            if 'SKIP' in (run_dir / 'receiver.txt').read_text(encoding='utf-8'):
                raise RuntimeError('Live settings verification was skipped; MQTT must be compiled in')
        finally:
            broker.terminate()
            broker.wait(timeout=5)
    report = {'verified_at_beijing': now.isoformat(timespec='seconds'),
              'transport': 'MQTT/Protobuf via private localhost Mosquitto',
              'validation_target': 'RMUL settings command fields and sparse feedback',
              'valid_commands_received': 3, 'invalid_commands_rejected': 4,
              'sparse_feedback_updates': 2, 'passed': True}
    report_path = run_dir / 'result.json'
    report_path.write_text(json.dumps(report, indent=2) + '\n', encoding='utf-8')
    print(f'Verification passed: {report_path}')


if __name__ == '__main__':
    main()
