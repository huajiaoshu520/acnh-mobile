#!/usr/bin/env python3
"""Private branch-local build; requires locally prepared resources under local/."""
import json,subprocess,sys,shutil,os
from pathlib import Path
root=Path(__file__).resolve().parents[1]
local=root/'local'
v=json.loads((root/'config/version.json').read_text())
output=local/'outputs'/f"ACNHMobile-iOS18-25适配测试版-v{v['version']}-build{v['build']}-unsigned.ipa"
args=[sys.executable,str(root/'tools/public_setup.py'),'build',
 '--patched',str(local/'data/patched'),'--romfs',str(local/'data/romfs'),
 '--shaders',str(local/'data/shaders-ios'),'--font',str(local/'data/standard-font.ttf'),
 '--work',str(local/'build/current'),'--output',str(output)]
if '--show-paths' in sys.argv:
 print(json.dumps({'workspace':str(root),'private_data':str(local/'data'),'build':str(local/'build/current'),'output':str(output)},ensure_ascii=False,indent=2))
else:
 stage=local/'build/source'
 def ignored(directory,names):
  return {n for n in names if n in {'.git','local','.venv','__pycache__','Gen','gen','bin','obj'} or n.endswith(('.xcodeproj','.xcworkspace','.dSYM'))}
 shutil.copytree(root,stage,ignore=ignored,dirs_exist_ok=True)
 args[1]=str(stage/'tools/public_setup.py')
 e=dict(os.environ,TMPDIR=str(local/'temp'),CLANG_MODULE_CACHE_PATH=str(local/'build/module-cache'))
 (local/'temp').mkdir(parents=True,exist_ok=True)
 subprocess.run(args,cwd=stage,env=e,check=True)
