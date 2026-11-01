import json
from pathlib import Path
import unittest

ROOT = Path(__file__).resolve().parents[1]


class ProfileTests(unittest.TestCase):
    def test_identity_resources_and_adapter_inputs(self):
        board = json.loads((ROOT / 'board.json').read_text(encoding='utf-8'))
        for path in board['profiles'].values():
            profile = json.loads((ROOT / path).read_text(encoding='utf-8'))
            self.assertEqual(profile['board'], board['id'])
            self.assertEqual(profile['kind'], 'platformio')
            resources = profile['resources']
            pins = [*resources['buttons'].values(), resources['power']['hold_gpio']]
            self.assertEqual(len(pins), len(set(pins)))
            self.assertTrue(all(type(pin) is int and 0 <= pin < 40 for pin in pins))
            self.assertFalse(resources['display']['touch'])
            for source in profile['adapter'].values():
                self.assertTrue((ROOT / source).exists())
            self.assertEqual(profile['resources']['usb']['baud'], int(profile['platformio']['monitor_speed']))

    def test_platform_has_no_application_dependency(self):
        source = (ROOT / 'arduino/src/Platform.cpp').read_text(encoding='utf-8')
        for forbidden in ('DeviceApplication', 'CompactUi', 'ArduinoJson', 'GarVibe', 'sessionId', 'uiAction'):
            self.assertNotIn(forbidden, source)
        self.assertIn('gar_board::power::hold_gpio', source)


if __name__ == '__main__':
    unittest.main()
