#!/usr/bin/env python3
"""Official alpha-library tests and profile patch isolation."""
import pathlib, sys, unittest, tempfile, importlib.util
sys.path.insert(0,str(pathlib.Path(__file__).resolve().parents[2]/'games'))
import game
spec=importlib.util.spec_from_file_location('official_alpha_tests', pathlib.Path(__file__).resolve().parents[1]/'tests/test_dmsh_alpha.py')
official=importlib.util.module_from_spec(spec);spec.loader.exec_module(official)
BASE=official.BASE
class Isolation(unittest.TestCase):
 def test_unconfigured_game_keeps_source(self):
  with tempfile.TemporaryDirectory() as td:
   p=pathlib.Path(td);(p/'game.tsv').write_text('name\tUnconfigured\n')
   self.assertEqual(game.shader_patch(game.Game(p))(BASE),BASE)
 def test_patch_pair_required(self):
  with tempfile.TemporaryDirectory() as td:
   p=pathlib.Path(td);(p/'game.tsv').write_text('shader_patch\tpatch.py\n');(p/'patch.py').write_text('')
   with self.assertRaises(ValueError):game.shader_patch(game.Game(p))
 def test_no_path_escape(self):
  with tempfile.TemporaryDirectory() as td:
   p=pathlib.Path(td);(p/'game.tsv').write_text('shader_patch\t../patch.py\n')
   with self.assertRaises(ValueError):game.shader_patch(game.Game(p))
 def test_patch_source_invalidates_profile_digest(self):
  import dmsh_chain
  with tempfile.TemporaryDirectory() as td:
   p=pathlib.Path(td);(p/'game.tsv').write_text('shader_patch\tpatch.py\n');f=p/'patch.py';f.write_text('one')
   g=game.Game(p);before=dmsh_chain.profile_rows(g);f.write_text('two')
   self.assertNotEqual(before,dmsh_chain.profile_rows(g))


def load_tests(loader, tests, pattern):
 tests.addTests(loader.loadTestsFromModule(official))
 return tests
if __name__=='__main__':unittest.main()
