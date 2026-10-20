import unittest

from pid_ws_logger import (
    BRIDGE_GREETING, BridgeClient, distance_from_encoder, encoder_row,
    hlog_row, speed_drive_row, telemetry_row,
)


class TelemetryRowTests(unittest.TestCase):
    def test_bridge_greeting_accepts_line_ending(self):
        self.assertEqual(BRIDGE_GREETING, "BRIDGE,BM50_ACK_R3")
        self.assertEqual("BRIDGE,BM50_ACK_R3\n".strip(), BRIDGE_GREETING)

    def test_extracts_selected_motor_and_normalizes_tick(self):
        line = "TEL,1234,0,1,30,-40,50,60,11,-12,13,14,215,-225,235,245,30,-40,50,60,12540,0,0,1,0,0,1"
        row, first_tick = telemetry_row(line, 2, None)
        self.assertEqual(first_tick, 1234)
        self.assertEqual(row, (0.0, 50, 23.5, 13))

        next_row, next_tick = telemetry_row(line.replace("1234", "1534"), 2, first_tick)
        self.assertEqual(next_tick, 1234)
        self.assertEqual(next_row, (0.3, 50, 23.5, 13))

    def test_ignores_disabled_or_malformed_telemetry(self):
        line = "TEL,1234,0,0,30,-40,50,60,11,-12,13,14,215,-225,235,245,30,-40,50,60,12540,0,0,1,0,0,0"
        self.assertIsNone(telemetry_row(line, 0, None))
        self.assertIsNone(telemetry_row("TEL,1,2", 0, None))

    def test_prepare_can_capture_non_fixed_future_loop_gains_when_explicitly_allowed(self):
        class FakeWebSocket:
            pass
        client = BridgeClient(FakeWebSocket(), require_fixed_gains=False)
        self.assertFalse(client.require_fixed_gains)

    def test_angle_packet_units_modes_wrap_and_invalid_values(self):
        packet="ATEL,1234,2,1,15000,14250,5400,170000,-175000,15000,15000,0,1,12000"
        self.assertEqual(telemetry_row(packet,None,None,"direction")[0],
                         (0.0,170.0,-175.0,15.0))
        self.assertIsNone(telemetry_row(packet,None,None,"angular_velocity"))
        rate=packet.replace(",2,1,",",1,1,")
        self.assertEqual(telemetry_row(rate,None,None,"angular_velocity")[0],
                         (0.0,15.0,14.25,5.4))
        self.assertIsNone(telemetry_row(rate.replace("14250","nan"),None,None,"angular_velocity"))
        self.assertIsNone(telemetry_row("TEL,1,2",None,None,"direction"))
        self.assertAlmostEqual(telemetry_row(rate,None,0xFFFFFF00,"angular_velocity")[0][0],1.49)

    def test_encoder_distance_uses_380_counts_and_127mm_wheel(self):
        packet = "ENC,2000,1,380,380,380,380,380,10,10,10,10,300,300,300,300"
        parsed = encoder_row(packet)
        self.assertEqual(parsed[3], (380, 380, 380, 380))
        self.assertAlmostEqual(
            distance_from_encoder(parsed[3], (0, 0, 0, 0)), 0.398982,
            places=5,
        )

    def test_speed_drive_row_averages_four_wheels(self):
        packet = "TEL,1234,0,1,30,30,30,30,10,10,10,10,300,300,300,300,30,30,30,30,12540,0,0,1,0,0,1"
        row, tick = speed_drive_row(packet)
        self.assertEqual(tick, 1234)
        self.assertEqual(row, (0.0, 30.0, 30.0, 10.0))

    def test_hlog_row_decodes_scaled_direction_loop_record(self):
        packet = (
            "HLOG,DATA,12,3456,15000,12000,3000,8000,7600,40000,12000,0,20,0"
        )
        row = hlog_row(packet)
        self.assertEqual(row["sequence"], 12)
        self.assertAlmostEqual(row["target_heading_deg"], 15.0)
        self.assertAlmostEqual(row["actual_rate_dps"], 7.6)
        self.assertEqual(row["forward_rpm"], 20)


if __name__ == "__main__":
    unittest.main()
