import importlib.util, tempfile, unittest
from pathlib import Path
p=Path(__file__).resolve().parent/'upstream/nsp_pipeline.py'
spec=importlib.util.spec_from_file_location('external_drive_pipeline',p)
u=importlib.util.module_from_spec(spec);spec.loader.exec_module(u)
class ExternalDriveTests(unittest.TestCase):
 def test_appledouble_is_not_a_second_update(self):
  with tempfile.TemporaryDirectory() as t:
   root=Path(t)
   for name in ['game [01006F8002326000][v0].nsp','game [01006F8002326800][v2228224].nsp','._game [01006F8002326800][v2228224].nsp','._unknown.nsp']:(root/name).write_bytes(b'PFS0'+bytes(12))
   (root/'fake [01006F8002326800][v2228224].nsp').write_bytes(b'not a package')
   title,family=u.scan_nsps(root)
   self.assertEqual(title,0x01006F8002326000);self.assertEqual(len(family['updates']),1)
 def test_disappearing_sidecar_cleanup(self):
  u._gone(None,None,FileNotFoundError(2,'already removed'))
  with self.assertRaises(PermissionError):u._gone(None,None,PermissionError(13,'denied'))
if __name__=='__main__':unittest.main()
