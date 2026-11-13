#!/usr/bin/env python3
"""Run the simulator publisher through a local MQTT broker into the Qt receiver.

Requires sim's Python environment and the built Qt receiver executable.
Use --hud to also validate the actual QML text and save rendered HUD screenshots.
The receiver acknowledges each frame before the next one is sent.
"""
from __future__ import annotations

import argparse
import copy
from contextlib import redirect_stdout
from datetime import datetime, timedelta, timezone
import json
import os
from pathlib import Path
import shutil
import socket
import subprocess
import sys
import time

ROOT = Path(__file__).resolve().parents[1]
sys.path.insert(0, str(ROOT / "sim"))
from server.mqtt_publisher import MQTTPublisher
from server.state_manager import StateManager


def wait_for(predicate, process=None, timeout=10):
    deadline = time.monotonic() + timeout
    while time.monotonic() < deadline:
        if process is not None and process.poll() is not None:
            raise RuntimeError(f"Child process exited early: {process.returncode}")
        if predicate():
            return
        time.sleep(0.02)
    raise TimeoutError("Timed out waiting for broker/receiver acknowledgement")


def port_open(port):
    try:
        with socket.create_connection(("127.0.0.1", port), timeout=0.1):
            return True
    except OSError:
        return False


def scenario(robot_id):
    manager = StateManager()
    health = [500, 400, 0, 300, 450, 350, 0, 250]
    for index, hp in enumerate(health):
        manager.set_robot_hp(index, hp)
    frames = []
    known = {"red": None, "blue": None}

    def snapshot(sparse=False):
        state = {"robotIds": list(manager.robot_ids),
                 "globalUnitStatus": copy.deepcopy(manager.global_unit_status)}
        for team in known:
            value = state["globalUnitStatus"][f"{team}_control_points"]
            if value is not None:
                known[team] = value
        expected_health = list(state["globalUnitStatus"]["robot_health"])
        if sparse:
            state["globalUnitStatus"].pop("robot_health")
            state["globalUnitStatus"].pop("robot_bullets")
        frames.append({"state": state, "health": expected_health,
                       "red_cp": known["red"], "blue_cp": known["blue"]})

    snapshot()  # 首包控制度未知，血量槽位含4号占位0。
    manager.set_control_points("red", 600)
    manager.set_control_points("blue", 590)
    snapshot()
    manager.set_robot_hp(manager.robot_ids.index(3), 123)
    manager.set_robot_hp(manager.robot_ids.index(107), 11)
    manager.set_control_points("red", 321)
    manager.set_control_points("blue", 222)
    snapshot()
    manager.set_robot_hp(manager.robot_ids.index(3), 0)
    manager.set_robot_hp(manager.robot_ids.index(107), 0)
    manager.set_control_points("red", 0)
    manager.set_control_points("blue", 0)
    snapshot()
    manager.set_control_points("red", None)
    manager.set_control_points("blue", None)
    snapshot()  # 后续缺失不覆盖已经收到的0。
    manager.set_control_points("red", 17)
    snapshot(sparse=True)  # 单侧控制度，缺失血量保持旧快照。
    manager.set_control_points("red", None)
    manager.set_control_points("blue", 29)
    snapshot(sparse=True)
    return manager, {"robot_id": robot_id, "frames": frames}


def settlement_scenario(robot_id):
    """Exercise the production adjudicator, wire encoder and result widget."""
    manager = StateManager()
    frames = []
    health = [500, 123, 600, 300, 450, 350, 0, 250]

    def snapshot(winner, reason, red_cp, blue_cp, red_hp="923", blue_hp="1050", elapsed="05:00"):
        state = manager.get_state()
        frames.append({"state": state, "match_status": state,
                       "expected": {"winner": winner, "reason": reason,
                                    "title": {255: "等待裁判结果", 0: "本局平局", 1: "红方获胜", 2: "蓝方获胜"}[winner],
                                    "red_cp": str(red_cp), "blue_cp": str(blue_cp),
                                    "red_hp": red_hp, "blue_hp": blue_hp, "elapsed": elapsed}})

    def begin(round_number, red=600, blue=590):
        manager.game_status["current_round"] = round_number
        manager.set_match_stage(4, 300)
        manager.global_unit_status.update(robot_health=list(health), red_control_points=red,
                                         blue_control_points=blue, total_damage_red=0, total_damage_blue=0)
        manager.set_stage_countdown(0)

    begin(1)
    snapshot(255, 255, 600, 590)  # Fresh battle: an old score cannot supply a result.
    manager.end_match()
    saved = dict(manager.last_game_result)
    manager.last_game_result.update(winner=255, end_reason=255)
    snapshot(255, 255, 600, 590)  # Stage arrives before the authoritative result.
    manager.last_game_result = saved
    snapshot(1, 2, 600, 590)      # The user's exact regression: blue has more HP.
    begin(2, 590, 600)
    manager.end_match()
    snapshot(2, 2, 590, 600)
    begin(3, 600, 600)
    manager.set_logistics_protocol_state("blue", total_damage=99)
    manager.end_match()
    snapshot(2, 3, 600, 600)
    begin(4, 600, 600)
    manager.end_match()
    snapshot(2, 4, 600, 600)
    begin(5, 600, 600)
    manager.global_unit_status["robot_health"] = [500, 123, 600, 300, 500, 123, 0, 300]
    manager.end_match()
    snapshot(0, 5, 600, 600, blue_hp="923")
    begin(6)
    manager.set_stage_countdown(177)
    manager.set_control_points("red", 0)
    snapshot(2, 2, 0, 590, elapsed="02:03")
    begin(7)
    manager.set_control_points("blue", 0)
    snapshot(1, 2, 600, 0)
    begin(8)
    manager.set_control_points("blue", None)
    manager.end_match()
    snapshot(255, 255, 600, "—")
    manager.set_control_points("blue", 590)
    snapshot(1, 2, 600, 590)
    begin(9)
    manager.end_match()
    # A referee may send abnormal termination independently of normal ranking.
    manager.last_game_result.update(winner=0, end_reason=6)
    snapshot(0, 6, 600, 590)
    frames[-1]["expected"]["title"] = "比赛异常终止"
    begin(10)
    snapshot(255, 255, 600, 590)
    return manager, {"robot_id": robot_id, "frames": frames}


def run_receiver(robot_id, port, receiver, run_dir, env, hud=False, settlement=False):
    manager, fixture = settlement_scenario(robot_id) if settlement else scenario(robot_id)
    if hud:
        for index, frame in enumerate(fixture["frames"]):
            frame["match_status"] = {
                "current_stage": 4, "current_round": 1,
                "stage_countdown_sec": [300, 299, 298, 0, 0, 0, 0][index],
                "is_paused": index == 2,
            }
    fixture_path = run_dir / f"fixture-{robot_id}.json"
    fixture_path.write_text(json.dumps(fixture, ensure_ascii=False, indent=2), encoding="utf-8")
    progress_path = run_dir / f"progress-{robot_id}.json"
    test_log = run_dir / f"receiver-{robot_id}.txt"
    env = {**env, "RMUL2027_FIXTURE_PATH": str(fixture_path),
           "RMUL2027_PROGRESS_PATH": str(progress_path),
           "RMUL2027_MQTT_URI": f"tcp://127.0.0.1:{port}"}
    publisher = MQTTPublisher(manager, broker_host="127.0.0.1", broker_port=port,
                              client_id=f"rmul2027-validation-{robot_id}", current_robot_id=robot_id)
    child = None
    no_window = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    try:
        if not publisher.connect():
            raise RuntimeError("Simulator publisher could not connect")
        wait_for(lambda: publisher.connected)
        with (run_dir / f"process-{robot_id}.log").open("w", encoding="utf-8") as output:
            child = subprocess.Popen([str(receiver), "testSimulatorMqttFrames", "-o", f"{test_log},txt"],
                                     cwd=run_dir, env=env, stdout=output, stderr=subprocess.STDOUT,
                                     creationflags=no_window)

            def is_ready(index):
                acknowledgement = Path(f"{progress_path}.{index}")
                if not acknowledgement.exists():
                    return False
                return json.loads(acknowledgement.read_text(encoding="utf-8")).get("ready") == index

            for index, frame in enumerate(fixture["frames"]):
                wait_for(lambda: is_ready(index), child)
                # 调用真实模拟器编码和发布入口，不在测试里手写wire数据。
                if settlement:
                    publisher._publish_game_status(frame["match_status"])
                publisher._publish_global_unit_status(frame["state"])
                if hud:
                    publisher._publish_game_status(frame["match_status"])
            code = child.wait(timeout=10)
            if code != 0:
                raise RuntimeError(f"Qt receiver failed ({code}); see {test_log}")
    finally:
        if child is not None and child.poll() is None:
            child.terminate()
            child.wait(timeout=5)
        publisher.disconnect()
    print(f"PASS perspective={robot_id}, frames={len(fixture['frames'])}", flush=True)
    return {"robot_id": robot_id, "frames": len(fixture["frames"]), "receiver_test_log": test_log.name}


def main():
    parser = argparse.ArgumentParser(description=__doc__)
    mode = parser.add_mutually_exclusive_group()
    mode.add_argument("--hud", action="store_true", help="Verify real RMUL QML HUD and save screenshots")
    mode.add_argument("--settlement", action="store_true", help="Verify RMUL adjudication and real result widget")
    parser.add_argument("--receiver", type=Path)
    parser.add_argument("--broker-exe", type=Path)
    args = parser.parse_args()
    target = "test_rmul2027_settlement.exe" if args.settlement else ("test_rmul2027_hud.exe" if args.hud else "test_rmul2027_data.exe")
    receiver = (args.receiver or ROOT / "build/windows-ucrt64/tests" / target).resolve()
    if not receiver.is_file():
        parser.error(f"Receiver not built: {receiver}")
    broker_exe = args.broker_exe or ROOT / ".deps/msys64/ucrt64/bin/mosquitto.exe"
    if not broker_exe.is_file():
        discovered = shutil.which("mosquitto")
        if not discovered:
            parser.error("Mosquitto not found; pass --broker-exe")
        broker_exe = Path(discovered)
    now = datetime.now(timezone(timedelta(hours=8)))
    run_dir = ROOT / "tmp" / "rmul2027-data" / now.strftime("%Y%m%d-%H%M%S-%f")
    run_dir.mkdir(parents=True)
    print(f"Evidence directory: {run_dir}", flush=True)
    with socket.socket() as reservation:
        reservation.bind(("127.0.0.1", 0))
        port = reservation.getsockname()[1]
    config = run_dir / "mosquitto.conf"
    config.write_text(f"listener {port} 127.0.0.1\nallow_anonymous true\npersistence false\n", encoding="ascii")
    env = dict(os.environ)
    native_bin = ROOT / ".deps/msys64/ucrt64/bin"
    env["PATH"] = os.pathsep.join([str(native_bin), str(receiver.parent.parent), env.get("PATH", "")])
    if args.hud or args.settlement:
        env.update(QT_QPA_PLATFORM="offscreen", QT_QUICK_BACKEND="software",
                   QT_QPA_PLATFORM_PLUGIN_PATH=str(receiver.parent.parent / "platforms"))
    no_window = getattr(subprocess, "CREATE_NO_WINDOW", 0)
    with (run_dir / "broker.log").open("w", encoding="utf-8") as output:
        broker = subprocess.Popen([str(broker_exe.resolve()), "-c", str(config)], env=env,
                                  stdout=output, stderr=subprocess.STDOUT, creationflags=no_window)
        try:
            wait_for(lambda: port_open(port), broker)
            with (run_dir / "simulator.log").open("w", encoding="utf-8") as simulator_log, redirect_stdout(simulator_log):
                results = [run_receiver(robot_id, port, receiver, run_dir, env, args.hud, args.settlement) for robot_id in (1, 103)]
        finally:
            broker.terminate()
            broker.wait(timeout=5)
    report = {"verified_at_beijing": now.isoformat(timespec="seconds"),
              "transport": "MQTT/Protobuf via localhost Mosquitto",
              "validation_target": "RMUL settlement" if args.settlement else ("QML HUD" if args.hud else "GameData"),
              "results": results, "total_frames": sum(r["frames"] for r in results)}
    report_path = run_dir / "result.json"
    report_path.write_text(json.dumps(report, ensure_ascii=False, indent=2) + "\n", encoding="utf-8")
    for result in results:
        print(f"PASS perspective={result['robot_id']}, frames={result['frames']}")
    print(f"Verification passed: {report_path}")
    return 0


if __name__ == "__main__":
    raise SystemExit(main())
