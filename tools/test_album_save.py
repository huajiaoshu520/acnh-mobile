"""Exercise the actual SDK save overload bodies with a recording local writer."""
from pathlib import Path
import re,subprocess,tempfile,unittest
ROOT=Path(__file__).resolve().parents[1]
class AlbumSave(unittest.TestCase):
 def test_sdk_overloads(self):
  source=(ROOT/'dm/games/01006F8002326000/network/hle_offline.c').read_text()
  functions=[]
  for name in ['h_album_Save','h_album_Save7']:
   match=re.search(r'HLE_FORCE\([^\n]+, Result, '+name+r',\s*([^}]+})',source)
   self.assertIsNotNone(match)
   functions.append('static unsigned '+name+'('+match[1])
  c='''#include <assert.h>
#include <stdint.h>
#include <stddef.h>
#include "album_save.h"
#define LOG(...) ((void)0)
#define RES_OK 0u
#define RES_FAIL 674u
static int result,calls;static const void *seen;static size_t count;
int dm_caps_save_rgba(const void*p,size_t n,uint8_t e[32]){assert(e);calls++;seen=p;count=n;return result;}
'''+ '\n'.join(functions)+'''
int main(void){char p[4];result=1;assert(h_album_Save(p,4,0,0,0)==0);assert(calls==1&&seen==p&&count==4);assert(h_album_Save7(p,4,0,0)==0);assert(calls==2);result=0;assert(h_album_Save(p,4,0,0,0)==674);assert(h_album_Save7(p,4,0,0)==674);assert(calls==4);}
'''
  with tempfile.TemporaryDirectory() as d:
   p=Path(d)/'test.c';p.write_text(c);exe=Path(d)/'test'
   subprocess.run(['clang','-I',str(ROOT/'dm/host'),str(p),'-o',str(exe)],check=True)
   subprocess.run([str(exe)],check=True)
if __name__=='__main__':unittest.main()
