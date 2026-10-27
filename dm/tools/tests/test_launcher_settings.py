"""The launcher offers frame buffering only for the graphics API selected by the packer."""
import importlib.util
import os
from pathlib import Path
import tempfile
import unittest
from unittest.mock import patch

ROOT = Path(__file__).resolve().parents[3]
spec = importlib.util.spec_from_file_location('launcher_settings', ROOT / 'dm/games/settings.py')
settings = importlib.util.module_from_spec(spec)
spec.loader.exec_module(settings)


class LauncherSettingsTests(unittest.TestCase):
    def test_mac_and_ios_settings_are_separate(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(settings.nacp, 'languages', return_value=['en-US']):
            for platform in ('ios','mac'):
                with patch.dict(os.environ, {'SETTINGS_PLATFORM':platform,'GFXAPI':'nvn'},clear=False):
                    rows={r['env']:r for r in settings.build('01006F8002326000',tmp,'unused')['settings']}
                    self.assertEqual('DM_FULLSCREEN' in rows,platform=='mac')
                    self.assertEqual('ACNH_CONTROLLER_LAYOUT' in rows,platform=='ios')
                    self.assertEqual('ACNH_AUDIO_IGNORE_SILENT' in rows,platform=='ios')
                    self.assertEqual('DM_TOUCH_HAPTICS' in rows,platform=='ios')
                    if platform=='mac':self.assertEqual(rows['DM_FULLSCREEN']['level'],'basic')

    def test_frame_buffering_by_graphics_api(self):
        with tempfile.TemporaryDirectory() as tmp, patch.object(settings.nacp, 'languages', return_value=['en-US']):
            for api, gid in [('nvn', '01006F8002326000'), ('vulkan', '01006F8002326000'), ('gl', '01006F8002326000'), ('', '01006F8002326000')]:
                with self.subTest(api=api), patch.dict(os.environ, {'GFXAPI': api}):
                    rows = settings.build(gid, tmp, Path(tmp) / 'control.nacp')['settings']
                    found = [s for s in rows if s['env'] == 'DM_PRESENT_SNAP']
                    self.assertEqual(len(found), int(api == 'nvn'))
                    if found:
                        self.assertEqual(found[0]['kind'], 'toggle')
                        self.assertEqual(found[0]['default'], '0')
                        self.assertIn('latency', found[0]['note'])


if __name__ == '__main__':
    unittest.main()
