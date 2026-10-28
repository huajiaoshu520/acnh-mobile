#!/usr/bin/env python3
"""Build a private official-core IPA with external direct-read ACNH .data resources."""
import argparse,json,os,re,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
p=argparse.ArgumentParser(description=__doc__);cmd=p.add_subparsers(dest='cmd',required=True)
b=cmd.add_parser('build')
for key in ['patched','romfs','shaders','font','work','output']:b.add_argument('--'+key,type=Path,required=True)
b.add_argument('--mac-shaders',type=Path,help='Matching Mac shader output parent, used to regenerate local tables')
b.add_argument('--shell',type=Path,help='Optional matching game-free IPA or shell ZIP from Releases')
m=cmd.add_parser('mac',help='Build the Mac development host without an IPA')
for key in ['patched','romfs','shaders','font','work']:m.add_argument('--'+key,type=Path,required=True)
m.add_argument('--run',action='store_true',help='Open the game after building')
ma=cmd.add_parser('mac-app',help='Build a self-contained Apple silicon Mac .app')
for key in ['patched','romfs','shaders','game','keys']:ma.add_argument('--'+key,type=Path)
for key in ['font','work','output']:ma.add_argument('--'+key,type=Path,required=True)
ma.add_argument('--sign',default='-',help='codesign identity; default: local ad-hoc signature')
ma.add_argument('--env-file',type=Path,help='Optional runtime settings to include in the app')
d=cmd.add_parser('data');d.add_argument('--romfs',type=Path,required=True);d.add_argument('--output',type=Path,required=True)
cmd.add_parser('check');a=p.parse_args()
if a.cmd=='check':
 import shutil
 for name in ['xcodebuild','xcodegen']:assert shutil.which(name),'Missing '+name
 print('Build tools found. Use your own rewritten modules, RomFS and matching official shader cache.');sys.exit(0)
if a.cmd=='data':
 subprocess.run([sys.executable,str(ROOT/'tools/pack_data.py'),str(a.romfs.resolve()),str(a.output.resolve())],check=True);sys.exit(0)
if a.cmd=='mac':
 from mac_build import build
 build(ROOT,a.patched,a.romfs,a.shaders,a.font,a.work,a.run);sys.exit(0)
if a.cmd=='mac-app':
 from mac_build import pack
 if a.game or a.keys:
  if not (a.game and a.keys) or any((a.patched,a.romfs,a.shaders)):p.error('Use --game and --keys together, or --patched --romfs --shaders, not both')
  from prepare_game import prepare
  from build_dependencies import ensure
  ensure(raw=True,mac=True)
  prepared=prepare(a.game,a.keys,a.work.resolve(),None,None,platform='mac')
  pack(prepared['build_root'],prepared['patched'],prepared['romfs'],prepared['shaders'],a.font,a.work/'mac-app-build',a.output,a.sign,a.env_file)
 else:
  if not all((a.patched,a.romfs,a.shaders)):p.error('Provide --game --keys, or --patched --romfs --shaders')
  pack(ROOT,a.patched,a.romfs,a.shaders,a.font,a.work,a.output,a.sign,a.env_file)
 sys.exit(0)
a.patched=a.patched.resolve();a.romfs=a.romfs.resolve();a.shaders=a.shaders.resolve();a.font=a.font.resolve();a.work=a.work.resolve();a.output=a.output.resolve()
from rewritten_display import validate_display_observations
validate_display_observations(a.patched)
from shader_tables import chain_header,ensure_tables,validate_cache
chain=chain_header(ROOT)
assert (a.shaders/('b-'+chain)).is_dir(),'Expected the matching complete official b-'+chain+' directory under --shaders'
validate_cache(ROOT,a.shaders,chain)
ensure_tables(ROOT,a.shaders,chain,a.mac_shaders)
assert a.font.is_file();a.work.mkdir(parents=True,exist_ok=True);a.output.parent.mkdir(parents=True,exist_ok=True)
if a.shell:
 subprocess.run([sys.executable,str(ROOT/'tools/pack_shell.py'),'--shell',str(a.shell.resolve()),'--patched',str(a.patched),'--shaders',str(a.shaders),'--font',str(a.font),'--output',str(a.output),'--control',str(a.romfs.parent/'control.nacp')],check=True);sys.exit(0)
v=json.loads((ROOT/'config/version.json').read_text())
e=dict(os.environ,GAME='01006F8002326000',W=str(a.work),PATCHED=str(a.patched),ROMFS=str(a.romfs),MSL_IOS=str(a.shaders),STD_FONT=str(a.font),FONT_PY=sys.executable,DM_EXTERNAL_ROMFS='1',DM_BUNDLE_ID='org.acnhmobile.experimental',DM_DISPLAY_NAME='ACNH Mobile',DM_APP_VERSION=v['version'],DM_BUILD_NUMBER=str(v['build']),DM_LANG='zh-Hans',DM_ICON=str(ROOT/'dm/ios/Resources/AppIcon.png'))
# Never carry legacy diagnostics or runtime overrides into the official baseline.
for key in ['DM_ENV_FILE','ACNH_RENDER_FLAGS']:e.pop(key,None)
subprocess.run(['bash',str(ROOT/'dm/ios/tools/build_ipa.sh'),str(a.output)],env=e,check=True)
