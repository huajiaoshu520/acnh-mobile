import importlib.util,json,subprocess,sys,tempfile,unittest
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
class ShaderSidecars(unittest.TestCase):
 def test_table_tools_ignore_binary_appledouble(self):
  with tempfile.TemporaryDirectory() as tmp:
   lib=Path(tmp)/'b-0123456789ab';lib.mkdir();key='00112233aabbccdd'
   (lib/(key+'_b.metal')).write_text('fragment float4 main0(constant cb& fp_c1) { int _5 = in.v; return (fp_c1)._m0[_5]; }\n')
   (lib/(key+'_b.json')).write_text(json.dumps({'bindings':[{'kind':'cbuf','reg':0,'msl_name':'fp_c1'}]}))
   for name in [key+'_b.metal',key+'_b.json','inputs.tsv']:(lib/('._'+name)).write_bytes(b'\0\x05\x16\x07'+b'\xa0'*4092)
   for tool,table in [('c1_guard.py','c1_full.tsv'),('cb_extent.py','cb_extent.tsv')]:
    r=subprocess.run([sys.executable,str(ROOT/'dm/gfx/tools'/tool),str(lib),str(Path(tmp)/table)],capture_output=True,text=True)
    self.assertEqual(r.returncode,0,r.stderr)
   self.assertEqual((Path(tmp)/'c1_full.tsv').read_text().splitlines()[1:],[key])
 def test_cache_accepts_only_real_shader_names(self):
  spec=importlib.util.spec_from_file_location('mtlc',ROOT/'dm/gfx/tools/dmsh_mtlcache.py');m=importlib.util.module_from_spec(spec);spec.loader.exec_module(m)
  self.assertIsNotNone(m.OUTPUT.fullmatch('00112233aabbccdd_b.metal'))
  for name in ['._00112233aabbccdd_b.metal','input_b.metal','00112233aabbccdd_b.metal.bak']:self.assertIsNone(m.OUTPUT.fullmatch(name))
if __name__=='__main__':unittest.main()
