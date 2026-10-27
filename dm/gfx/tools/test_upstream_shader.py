"""Generic upstream precision/capture fixes and alpha + tile-fetch integration."""
import ast
from pathlib import Path
import re
import unittest
import dmsh_fetch, dmsh_geometry
import sys, types
sys.path.insert(0, str(Path(__file__).resolve().parents[2] / "games"))
import game
dmsh_alpha=types.SimpleNamespace(fragment=game.shader_patch(game.load("01006F8002326000")))

HERE=Path(__file__).parent
node=next(n for n in ast.parse((HERE/'dmsh_build.py').read_text()).body if isinstance(n,ast.FunctionDef) and n.name=='precise')
ns={'re':re};exec(compile(ast.Module(body=[node],type_ignores=[]),'precise','exec'),ns)

class Shaders(unittest.TestCase):
 def test_precise_nan_intrinsics(self):
  self.assertEqual(ns['precise']('fast::max(a,b); fast::min(a,b); fast::clamp(a,b,c); fast::normalize(a);'),
                   'precise::max(a,b); precise::min(a,b); precise::clamp(a,b,c); fast::normalize(a);')
 def test_fetch_integer_entry_and_empty_parameters(self):
  src="""#include <metal_stdlib>
using namespace metal;
struct main0_out
{
 float4 m_0 [[color(0)]];
};
fragment main0_out main0()
{
    main0_out out = {};
    out.m_0 = float4(1);
    return out;
}
struct main0_out_u
{
 uint4 m_0 [[color(0)]];
};
fragment main0_out_u main0_u() { return main0_out_u{uint4(1)}; }
"""
  variant,meta=dmsh_fetch.variant(src,[],om_only_ok=True)
  self.assertTrue(variant,meta)
  self.assertNotIn('main0_u',variant)
  self.assertNotIn('main0(,',variant)
  self.assertIn('main0(float4 fb0',variant)
 def test_capture_clip_distance(self):
  for decl in ('float clip[8] [[clip_distance]];', 'float clip [[clip_distance]] [8];'):
   src='struct main0_out {\nfloat4 p [[position]];\n'+decl+'\nfloat clip0 [[user(clip0)]];\n};\nvertex main0_out main0(uint v [[vertex_id]]) {main0_out out; return out;}'
   cap=dmsh_geometry.capture(src)
   self.assertIn('dm_p[12] = as_type<uint>(out.clip[7]);',cap)
   self.assertIn('dm_p[5] = as_type<uint>(out.clip0);',cap)
 def test_alpha_fetch_scoped_exits(self):
  src='''#include <metal_stdlib>
using namespace metal;
struct main0_out
{
 float4 m_0 [[color(0)]];
};
fragment main0_out main0(float4 p [[position]])
{
    main0_out out = {};
    out.m_0 = float4(1,0,0,0.5);
    if (p.x < 0) return out;
    return out;
}
'''
  alpha=dmsh_alpha.fragment(src)
  variant,meta=dmsh_fetch.variant(alpha,[],om_only_ok=True)
  self.assertTrue(variant,meta)
  self.assertEqual(variant.count('dm_alpha_test(float(spv_o_m_0.w), dm_alpha);'),2)
  self.assertEqual(variant.count('out.m_0_at0 = spv_o_m_0;'),2)
  self.assertIn('constant dm_alpha_state& dm_alpha [[buffer(30)]]',variant)
  # Conditional early return stays scoped; output mapping follows its alpha test.
  self.assertRegex(variant,r'if \(p.x < 0\) \{ dm_alpha_test[\s\S]*?return out; \}')

if __name__=='__main__':unittest.main()
