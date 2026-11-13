import unittest

import robomaster_pb2 as pb
from server.mqtt_publisher import MQTTPublisher
from server.state_manager import StateManager


class RmulSettlementTest(unittest.TestCase):
    def setUp(self):
        self.manager = StateManager()
        self.manager.set_match_stage(4, 300)
        self.manager.global_unit_status.update(
            red_control_points=600, blue_control_points=590,
            robot_health=[500, 123, 600, 300, 450, 350, 0, 250],
            total_damage_red=0, total_damage_blue=0)

    def finish(self):
        self.manager.end_match()
        return self.manager.get_state()['lastGameResult']

    def test_control_points_override_legacy_buildings_and_blue_hp_lead(self):
        self.manager.global_unit_status.update(red_base_health=0, blue_base_health=5000)
        result = self.finish()
        self.assertEqual((result['winner'], result['end_reason']), (1, 2))

    def test_blue_control_points_win(self):
        self.manager.global_unit_status['blue_control_points'] = 601
        self.assertEqual(self.finish()['winner'], 2)

    def test_damage_tie_break(self):
        self.manager.global_unit_status.update(blue_control_points=600, total_damage_blue=99)
        result = self.finish()
        self.assertEqual((result['winner'], result['end_reason']), (2, 3))

    def test_hp_tie_break_counts_sentry_and_ignores_four_slot(self):
        self.manager.global_unit_status.update(blue_control_points=600)
        result = self.finish()
        self.assertEqual((result['winner'], result['end_reason']), (2, 4))

    def test_draw(self):
        self.manager.global_unit_status.update(blue_control_points=600,
            robot_health=[500, 123, 600, 300, 500, 123, 0, 300])
        result = self.finish()
        self.assertEqual((result['winner'], result['end_reason']), (0, 5))

    def test_zero_ends_battle_immediately_once(self):
        self.manager.set_control_points('blue', 0)
        self.assertEqual(self.manager.game_status['current_stage'], 5)
        self.assertEqual(self.manager.last_game_result['winner'], 1)
        self.manager.end_match()
        self.assertEqual(self.manager.game_status['red_score'], 1)
        self.assertEqual(len(self.manager.game_result_queue), 1)

    def test_red_zero_overrides_damage(self):
        self.manager.global_unit_status['total_damage_red'] = 999999
        self.manager.set_control_points('red', 0)
        self.assertEqual(self.manager.last_game_result['winner'], 2)
        self.assertEqual(self.manager.last_game_result['end_reason'], 2)

    def test_missing_control_points_wait_then_complete(self):
        self.manager.set_control_points('blue', None)
        result = self.finish()
        self.assertEqual(result['winner'], 255)
        self.assertEqual(self.manager.game_status['red_score'], 0)
        self.manager.set_control_points('blue', 590)
        self.assertEqual(self.manager.last_game_result['winner'], 1)

    def test_both_zero_is_invalid_not_a_fabricated_draw(self):
        self.manager.global_unit_status.update(red_control_points=0, blue_control_points=0)
        self.assertEqual(self.finish()['winner'], 255)

    def test_elapsed_preserved_and_new_battle_clears_result(self):
        self.manager.game_status['stage_elapsed_sec'] = 299
        self.manager.global_unit_status['total_damage_blue'] = 123
        self.finish()
        self.assertEqual(self.manager.game_status['stage_elapsed_sec'], 299)
        self.manager.set_match_stage(4)
        self.assertEqual(self.manager.game_status['stage_countdown_sec'], 300)
        self.assertEqual(self.manager.last_game_result['winner'], 255)
        self.assertEqual(self.manager.global_unit_status['blue_control_points'], 600)
        self.assertEqual(self.manager.global_unit_status['total_damage_blue'], 0)
        self.manager.set_control_points('red', 0)
        self.assertEqual(self.manager.game_status['blue_score'], 1)

    def test_skip_to_battle_uses_300_seconds_and_initial_control_points(self):
        self.manager.skip_to_battle()
        self.assertEqual(self.manager.game_status['stage_countdown_sec'], 300)
        for team in ('red', 'blue'):
            self.assertEqual(self.manager.global_unit_status[team + '_control_points'], 600)

    def test_no_result_is_published_as_unknown(self):
        self.manager.set_control_points('red', None)
        self.finish()
        publisher = object.__new__(MQTTPublisher)
        publisher.state_manager = self.manager
        published = []
        publisher._publish = lambda topic, payload: published.append((topic, payload))
        publisher._publish_game_status(self.manager.get_state())
        msg = pb.GameStatus.FromString(published[-1][1])
        self.assertEqual((msg.game_result, msg.end_reason), (255, 255))

    def test_pending_damage_can_complete_but_never_award_twice(self):
        self.manager.global_unit_status.update(blue_control_points=600, total_damage_blue=None)
        self.assertEqual(self.finish()['winner'], 255)
        self.manager.set_logistics_protocol_state('blue', total_damage=99)
        self.assertEqual(self.manager.last_game_result['winner'], 2)
        self.manager.set_logistics_protocol_state('red', total_damage=100)
        self.assertEqual(self.manager.last_game_result['winner'], 2)
        self.assertEqual(self.manager.game_status['blue_score'], 1)

    def test_cp_zero_result_survives_subsequent_demo_frames(self):
        self.manager.set_control_points('blue', 0)
        self.manager.apply_match_simulation_frame(
            positions=None, robot_health=None, current_stage=4,
            stage_countdown_sec=123, stage_elapsed_sec=177,
            simulation_status={'status': 'running'})
        self.assertEqual(self.manager.game_status['current_stage'], 5)
        self.assertEqual(self.manager.last_game_result['winner'], 1)


if __name__ == '__main__':
    unittest.main()
