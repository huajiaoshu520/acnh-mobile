#!/usr/bin/env python3
"""Check supported settings, safe defaults and language filtering without game files."""
import importlib.util,os,struct,sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
sys.dont_write_bytecode=True
ROOT=Path(__file__).resolve().parents[1]
spec=importlib.util.spec_from_file_location('launcher_settings',ROOT/'dm/games/settings.py');settings=importlib.util.module_from_spec(spec);spec.loader.exec_module(settings)
class LauncherTests(unittest.TestCase):
    def setUp(self):
        env = patch.dict(os.environ, {'GFXAPI':'nvn'}); env.start(); self.addCleanup(env.stop)
    def test_acnh_hides_unimplemented_fps_and_scale(self):
        with tempfile.TemporaryDirectory() as t:
            d=settings.build('01006F8002326000',t,Path(t)/'missing.nacp')
            self.assertEqual({s['env'] for s in d['settings']},{'ACNH_DISPLAY_MODE','DM_PSO_MTL4','DM_PSO_PREBUILD','DM_NVN_ENC_THREAD','DM_PRESENT_UPSCALE','DM_RUMBLE_MIN','DM_PRESENT_SNAP','DM_OPMODE','DM_SHOW_FPS','DM_METAL_HUD','ACNH_CONTROLLER_LAYOUT','ACNH_AUDIO_IGNORE_SILENT','ACNH_CUSTOM_BASE','DM_TOUCH_HAPTICS','DM_RUMBLE','DM_RUMBLE_GAIN'})
    def test_official_performance_defaults_and_typed_toggles(self):
        with tempfile.TemporaryDirectory() as t:
            rows={s['env']:s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings']}
            self.assertEqual({k:rows[k]['default'] for k in ('DM_PSO_MTL4','DM_PSO_PREBUILD','DM_NVN_ENC_THREAD')},{'DM_PSO_MTL4':'0','DM_PSO_PREBUILD':'1','DM_NVN_ENC_THREAD':'1'})
            self.assertNotIn('DM_FPS_HOLD',rows)  # ACNH has no official frame-step limiter
            self.assertEqual(rows['DM_PRESENT_UPSCALE']['default'],'off')
            self.assertEqual({s['section'] for s in rows.values()},{'Game','Controls','Performance'})
    def test_picture_modes_are_persistable_choices(self):
        with tempfile.TemporaryDirectory() as t:
            row=next(s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings'] if s['env']=='ACNH_DISPLAY_MODE')
            self.assertEqual(row['default'],'auto')
            self.assertEqual([v[0] for v in row['options']],['auto','original','stretch','crop'])
    def test_frame_buffering_is_opt_in(self):
        with tempfile.TemporaryDirectory() as t:
            row = next(s for s in settings.build('01006F8002326000', t, Path(t)/'missing.nacp')['settings'] if s['env']=='DM_PRESENT_SNAP')
            self.assertEqual((row['kind'],row['default']), ('toggle','0'))
    def test_touch_feedback_default(self):
        with tempfile.TemporaryDirectory() as t:
            row=next(s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings'] if s['env']=='DM_TOUCH_HAPTICS')
            self.assertEqual(row['kind'],'toggle')
            self.assertEqual(row['default'],'1')
    def test_game_rumble_defaults(self):
        with tempfile.TemporaryDirectory() as t:
            rows={s['env']:s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings']}
            self.assertEqual(rows['DM_RUMBLE']['default'],'1')
            self.assertEqual(rows['DM_RUMBLE_GAIN']['default'],'1.0')
            self.assertEqual((rows['DM_RUMBLE_GAIN']['min'],rows['DM_RUMBLE_GAIN']['max']),(0.0,3.0))
    def test_custom_template_choices(self):
        with tempfile.TemporaryDirectory() as t:
            row=next(s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings'] if s['env']=='ACNH_CUSTOM_BASE')
            self.assertEqual(row['default'],'acnh')
            self.assertEqual(row['options'],[['generic','Generic'],['acnh','Dedicated']])
            self.assertEqual(row['note'],'')
            self.assertEqual(row['needs'],'ACNH_CONTROLLER_LAYOUT=custom')
    def test_silent_mode_audio_is_opt_in(self):
        with tempfile.TemporaryDirectory() as t:
            row=next(s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings'] if s['env']=='ACNH_AUDIO_IGNORE_SILENT')
            self.assertEqual(row['kind'],'toggle')
            self.assertEqual(row['default'],'0')
    def test_controller_default_preserves_acnh_and_offers_official(self):
        with tempfile.TemporaryDirectory() as t:
            row=next(s for s in settings.build('01006F8002326000',t,Path(t)/'missing.nacp')['settings'] if s['env']=='ACNH_CONTROLLER_LAYOUT')
            self.assertEqual(row['default'],'acnh')
            self.assertEqual(row['options'],[['generic','Generic'],['acnh','Dedicated'],['custom','Custom']])
    def test_languages_only_from_control_and_build_default(self):
        with tempfile.TemporaryDirectory() as t:
            p=Path(t)/'control.nacp';raw=bytearray(0x3030)
            want={'en-US','zh-Hans'};flag=sum(1<<i for i,(code,_) in enumerate(settings.nacp.LANGUAGES) if code in want);struct.pack_into('<I',raw,0x302c,flag);p.write_bytes(raw)
            with patch.dict(os.environ,{'DM_LANG':'zh-Hans','DM_SHOW_FPS':'YES'}):d=settings.build('01006F8002326000',t,p)
            rows={s['env']:s for s in d['settings']};self.assertEqual(rows['DM_LANG']['default'],'zh-Hans');self.assertEqual({o[0] for o in rows['DM_LANG']['options']},want);self.assertEqual(rows['DM_SHOW_FPS']['default'],'1');self.assertEqual(rows['DM_METAL_HUD']['default'],'0')
            with patch.dict(os.environ,{'DM_LANG':'unsupported'}):d=settings.build('01006F8002326000',t,p)
            row=next(s for s in d['settings'] if s['env']=='DM_LANG');self.assertIn(row['default'],want)
if __name__=='__main__':unittest.main()
