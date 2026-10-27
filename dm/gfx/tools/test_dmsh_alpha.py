#!/usr/bin/env python3
import math, pathlib, sys, unittest, tempfile
sys.path.insert(0, str(pathlib.Path(__file__).resolve().parents[2] / 'games'))
import game, types
# Test the game-owned implementation, through the same opt-in loader as builds.
dmsh_alpha = types.SimpleNamespace(fragment=game.shader_patch(game.load('01006F8002326000')))

BASE = '''#include <metal_stdlib>
using namespace metal;
struct main0_out { float4 colour [[color(0)]]; };
fragment main0_out main0(float4 p [[position]])
{
    main0_out out = {};
    out.colour = float4(1, 0, 0, p.x / 8.0f);
    return out;
}
'''
class Alpha(unittest.TestCase):
 def test_output(self):
  s=dmsh_alpha.fragment(BASE)
  self.assertIn('constant dm_alpha_state& dm_alpha [[buffer(30)]]',s)
  self.assertIn('dm_alpha_test(float(out.colour.w), dm_alpha); return out;',s)
 def test_early_exit(self):
  s=BASE.replace('return out;', 'if (p.y < 0) return out; return out;')
  self.assertEqual(dmsh_alpha.fragment(s).count('dm_alpha_test(float(out.'),2)
 def test_rgb(self):
  self.assertIn('dm_alpha_test(1.0f, dm_alpha)',dmsh_alpha.fragment(BASE.replace('float4 colour','float3 colour')))
 def test_no_colour_zero(self):
  for s in (BASE.replace('color(0)','color(1)'),BASE.replace('float4 colour','uint4 colour')):
   self.assertEqual(dmsh_alpha.fragment(s),s)
 def test_reserved_slot(self):
  with self.assertRaises(ValueError):dmsh_alpha.fragment(BASE.replace('float4 p [[position]]','constant float& p [[buffer(30)]]'))

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

if __name__=='__main__':
 if len(sys.argv)>1 and sys.argv[1]=='--fixture':
  source=BASE
  vertex='\nvertex float4 vertex_main(uint i [[vertex_id]]) { float2 v[3]={float2(-1,-1),float2(3,-1),float2(-1,3)};return float4(v[i],0,1);}\n'
  pathlib.Path(sys.argv[2]).write_text(dmsh_alpha.fragment(source)+vertex)
  source=BASE.replace('fragment main0_out','struct Args {constant float* width [[id(0)]];};\nfragment main0_out').replace('float4 p [[position]]','float4 p [[position]], constant Args& args [[buffer(0)]]').replace('p.x / 8.0f','p.x / *args.width')
  pathlib.Path(sys.argv[3]).write_text(dmsh_alpha.fragment(source)+vertex)
  if len(sys.argv)>4:
   import dmsh_fetch
   variant,meta=dmsh_fetch.variant(dmsh_alpha.fragment(BASE.replace('colour','m_0')),[],om_only_ok=True)
   assert variant,meta
   pathlib.Path(sys.argv[4]).write_text(variant+vertex)
 else:unittest.main()
