import unittest
from types import SimpleNamespace
from unittest.mock import Mock, patch

import robomaster_pb2 as pb
from server.mqtt_publisher import MQTTPublisher
from server.state_manager import StateManager


class RmulSettingsTests(unittest.TestCase):
    def test_sparse_performance_keeps_untouched_field_and_static_matches(self):
        state = StateManager()
        state.set_robot_performance_selection_sync(shooter=2, chassis=1)
        state.set_robot_performance_selection_sync(chassis=2)
        self.assertEqual(state.get_robot_performance_selection_sync()['shooter'], 2)
        self.assertEqual(state.get_robot_static_status()['performance_system_shooter'], 2)
        self.assertEqual(state.get_robot_static_status()['performance_system_chassis'], 2)

    def test_old_hero_and_semi_auto_values_rejected(self):
        state = StateManager()
        before = state.get_robot_performance_selection_sync()
        for change in ({'shooter': 3}, {'chassis': 4}, {'sentry_control': 1}):
            self.assertFalse(state.set_robot_performance_selection_sync(**change))
            self.assertEqual(state.get_robot_performance_selection_sync(), before)

    def test_mqtt_partial_command_updates_feedback_without_resetting_other_field(self):
        for robot_id in (3, 103):
            for stage in (1, 2, 3):
                with self.subTest(robot_id=robot_id, stage=stage):
                    state = StateManager()
                    state.set_match_stage(stage, 30)
                    state.set_robot_performance_selection_sync(shooter=2, chassis=1)
                    with patch.object(MQTTPublisher, '_init_client'):
                        publisher = MQTTPublisher(state, current_robot_id=robot_id)
                    publisher.publish_robot_performance_selection_sync = Mock()
                    command = pb.RobotPerformanceSelectionCommand(chassis=2)
                    publisher._on_message(None, None, SimpleNamespace(
                        topic='RobotPerformanceSelectionCommand', payload=command.SerializeToString()))
                    self.assertEqual(state.get_robot_performance_selection_sync()['shooter'], 2)
                    self.assertEqual(state.get_robot_static_status()['performance_system_chassis'], 2)
                    publisher.publish_robot_performance_selection_sync.assert_called_once()

    def test_mqtt_rejects_wrong_role_phase_hero_enum_and_semi_auto(self):
        cases = [(robot_id, 1, {'shooter': 1}) for robot_id in (1, 7, 101, 107)]
        cases += [(3, stage, {'chassis': 2}) for stage in (0, 4, 5)]
        cases += [(3, 1, fields) for fields in (
            {'shooter': 3}, {'chassis': 4}, {'shooter': 0}, {},
            {'shooter': 1, 'sentry_control': 1})]
        for robot_id, stage, fields in cases:
            with self.subTest(robot_id=robot_id, stage=stage, fields=fields):
                state = StateManager()
                state.set_match_stage(stage, 30)
                before = state.get_robot_performance_selection_sync()
                with patch.object(MQTTPublisher, '_init_client'):
                    publisher = MQTTPublisher(state, current_robot_id=robot_id)
                publisher.publish_robot_performance_selection_sync = Mock()
                command = pb.RobotPerformanceSelectionCommand(**fields)
                publisher._on_message(None, None, SimpleNamespace(
                    topic='RobotPerformanceSelectionCommand', payload=command.SerializeToString()))
                self.assertEqual(state.get_robot_performance_selection_sync(), before)
                publisher.publish_robot_performance_selection_sync.assert_not_called()
