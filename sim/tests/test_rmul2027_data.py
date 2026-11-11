import unittest

import robomaster_pb2 as pb
from server.mqtt_publisher import MQTTPublisher
from server.state_manager import StateManager


class Rmul2027DataTest(unittest.TestCase):
    def publisher(self, robot_id=1):
        publisher = object.__new__(MQTTPublisher)
        publisher.state_manager = StateManager()
        publisher.current_robot_id = robot_id
        published = []
        publisher._publish = lambda topic, payload, qos=1: published.append((topic, payload))
        return publisher, published

    def publish(self, publisher, published, state):
        publisher._publish_global_unit_status(state)
        self.assertEqual(published[-1][0], "GlobalUnitStatus")
        return pb.GlobalUnitStatus.FromString(published[-1][1])

    def test_control_point_descriptor_and_zero_wire(self):
        for name, number in (("total_CP_ally", 15), ("total_CP_enemy", 16)):
            field = pb.GlobalUnitStatus.DESCRIPTOR.fields_by_name.get(name)
            self.assertIsNotNone(field, name)
            self.assertEqual(field.number, number)
            self.assertEqual(field.type, field.TYPE_UINT32)
            self.assertTrue(field.has_presence)
        msg = pb.GlobalUnitStatus(total_CP_ally=0, total_CP_enemy=600)
        self.assertTrue(msg.HasField("total_CP_ally"))
        self.assertEqual(msg.SerializeToString(), bytes.fromhex("78008001d804"))
        self.assertFalse(pb.GlobalUnitStatus().HasField("total_CP_ally"))

    def test_fixed_slots_for_both_perspectives(self):
        ids = [1, 3, 4, 7, 101, 103, 104, 107]
        state = {"robotIds": ids, "globalUnitStatus": {
            "robot_health": [101, 103, 104, 107, 201, 203, 204, 207],
            "robot_bullets": [11, 13, 14, 17, 21, 23, 24, 27],
        }}
        for robot_id, hp, bullets in (
            (1, [101, 103, 104, 107, 201, 203, 204, 207], [11, 13, 14, 17]),
            (103, [201, 203, 204, 207, 101, 103, 104, 107], [21, 23, 24, 27]),
        ):
            with self.subTest(robot_id=robot_id):
                publisher, published = self.publisher(robot_id)
                msg = self.publish(publisher, published, state)
                self.assertEqual(list(msg.robot_health), hp)
                self.assertEqual(list(msg.robot_bullets), bullets)

    def test_rmul_missing_fourth_robot_keeps_zero_slot(self):
        publisher, published = self.publisher()
        msg = self.publish(publisher, published, {
            "robotIds": [1, 3, 7, 101, 103, 107],
            "globalUnitStatus": {"robot_health": [500, 0, 300, 400, 200, 0]},
        })
        self.assertEqual(list(msg.robot_health), [500, 0, 0, 300, 400, 200, 0, 0])

    def test_control_points_are_relative_and_zero_is_present(self):
        for robot_id, expected in ((1, (0, 321)), (103, (321, 0))):
            publisher, published = self.publisher(robot_id)
            msg = self.publish(publisher, published, {"globalUnitStatus": {
                "red_control_points": 0, "blue_control_points": 321,
            }})
            self.assertEqual((msg.total_CP_ally, msg.total_CP_enemy), expected)
            self.assertTrue(msg.HasField("total_CP_ally"))
            self.assertTrue(msg.HasField("total_CP_enemy"))

    def test_missing_control_points_are_not_invented(self):
        publisher, published = self.publisher()
        msg = self.publish(publisher, published, {"globalUnitStatus": {
            "red_control_points": None, "blue_control_points": 0,
        }})
        self.assertFalse(msg.HasField("total_CP_ally"))
        self.assertTrue(msg.HasField("total_CP_enemy"))
        msg = self.publish(publisher, published, {})
        self.assertFalse(msg.HasField("total_CP_ally"))
        self.assertFalse(msg.HasField("total_CP_enemy"))

    def test_logs_distinguish_missing_control_points_from_zero(self):
        message = pb.GlobalUnitStatus(total_CP_enemy=0)
        values = MQTTPublisher._message_to_log_dict(message)
        self.assertIsNone(values["total_CP_ally"])
        self.assertEqual(values["total_CP_enemy"], 0)

    def test_state_changes_reach_publisher(self):
        publisher, published = self.publisher(103)
        manager = publisher.state_manager
        self.assertEqual(manager.robot_ids, [1, 3, 4, 7, 101, 103, 104, 107])
        manager.set_control_points("red", 600)
        manager.set_control_points("blue", 599)
        manager.set_robot_hp(manager.robot_ids.index(3), 123)
        msg = self.publish(publisher, published, manager.get_state())
        self.assertEqual(msg.robot_health[5], 123)
        self.assertEqual((msg.total_CP_ally, msg.total_CP_enemy), (599, 600))
        manager.set_robot_hp(manager.robot_ids.index(3), 0)
        manager.set_control_points("red", 0)
        manager.set_control_points("blue", None)
        msg = self.publish(publisher, published, manager.get_state())
        self.assertEqual(msg.robot_health[5], 0)
        self.assertEqual(msg.total_CP_enemy, 0)
        self.assertTrue(msg.HasField("total_CP_enemy"))
        self.assertFalse(msg.HasField("total_CP_ally"))

    def test_control_points_reject_invalid_input(self):
        manager = StateManager()
        for team, value in (("green", 1), ("red", -1), ("blue", 2**32), ("red", 1.5)):
            with self.subTest(team=team, value=value):
                with self.assertRaises(ValueError):
                    manager.set_control_points(team, value)


if __name__ == "__main__":
    unittest.main()
