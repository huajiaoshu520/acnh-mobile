"""Official tool policy: an explicit override permits another Metal compiler only."""
import importlib.util,os,sys,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).resolve().parents[1]/'dm/gfx/tools'))
import own_compilers as own
class OwnCompilerTests(unittest.TestCase):
 def check(self,issues,allow=True):
  with patch.object(own,'strict',return_value=issues),patch.dict(os.environ,{'DM_ALLOW_OTHER_METAL':'1' if allow else '0','DMSH_TARGETS':'mac,ios'}):return own.check_tools()
 def test_explicit_metal_override(self):
  self.assertEqual(self.check(['metal_iphoneos: lock old / actual new']),[])
 def test_other_compilers_are_rejected(self):
  issues=['spirv-cross: lock old / actual new','dmsh_xlate: lock old / actual new'];self.assertEqual(self.check(issues),issues)
 def test_missing_compiler_rejected(self):
  issues=['metal_iphoneos: lock old / actual missing'];self.assertEqual(self.check(issues),issues)
 def test_unapproved_metal_rejected(self):
  with self.assertRaises(SystemExit):self.check(['metal_iphoneos: lock old / actual new'],False)
