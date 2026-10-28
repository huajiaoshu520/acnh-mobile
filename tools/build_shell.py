#!/usr/bin/env python3
"""Build a game-free iOS shell from public sources; requires Xcode 26+ on Mac."""
import argparse, hashlib, json, os, plistlib, re, shutil, subprocess, tempfile, zipfile
from pathlib import Path

ROOT = Path(__file__).resolve().parents[1]

def source_id(root):
    """Fingerprint compatibility code, not locally regenerated shader metadata."""
    excluded = {'shader_map.tsv', 'shader_facts.tsv', 'shader_pairs.tsv', 'c1_full.tsv', 'dmsh_chain.h', 'dmsh_chain.lock', 'cb_extent.tsv'}
    h = hashlib.sha256()
    for base in ['dm/host', 'dm/gfx', 'dm/ios-acnh/Sources', 'dm/ios/Sources', 'dm/games/01006F8002326000']:
        for p in sorted((root/base).rglob('*')):
            if p.is_file() and p.suffix in {'.c', '.h', '.m', '.S', '.inc', '.py', '.tsv', '.cs', '.csproj', '.swift'} and p.name not in excluded and not any(x in {'gen','bin','obj','__pycache__'} for x in p.relative_to(root).parts):
                h.update(str(p.relative_to(root)).encode()+b'\0'+p.read_bytes()+b'\0')
    return h.hexdigest()

def audit_app(app):
    allowed = {'DmIOS', 'Info.plist', 'PkgInfo', 'dm/env.txt', 'dm/shell_info.json', 'ExpandedView-LICENSE.txt'}
    actual = {str(p.relative_to(app)) for p in app.rglob('*') if p.is_file()}
    icons = {'Assets.car', 'AppIcon60x60@2x.png', 'AppIcon60x60@3x.png', 'AppIcon76x76@2x~ipad.png'}
    if not allowed <= actual or actual - allowed - icons:
        raise ValueError('Unexpected shell files: '+str((allowed - actual) | (actual - allowed - icons)))
    if any(p.is_symlink() for p in app.rglob('*')):
        raise ValueError('Shell contains symbolic links')
    info = plistlib.loads((app/'Info.plist').read_bytes())
    if info['CFBundleIdentifier'] != 'org.acnhmobile.experimental':
        raise ValueError('Wrong bundle ID')
    if not info.get('CFBundleIcons', {}).get('CFBundlePrimaryIcon', {}).get('CFBundleIconFiles') or 'Assets.car' not in actual:
        raise ValueError('Shell app icon is missing')
    commands = subprocess.check_output(['otool', '-l', app/'DmIOS'], text=True)
    if re.search(r'(?:sectname __dm\w+x[01]|segname __DM\w+W)', commands):
        raise ValueError('Embedded game sections remain')
    return {str(p.relative_to(app)): hashlib.sha256(p.read_bytes()).hexdigest() for p in sorted(app.rglob('*')) if p.is_file()}

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    ap.add_argument('--output', type=Path, required=True)
    ap.add_argument('--work', type=Path, required=True)
    a=ap.parse_args(); a.output=a.output.resolve(); a.work=a.work.resolve()
    if a.work.exists(): raise ValueError('Choose a new empty work path')
    if a.output.exists() and any(a.output.iterdir()): raise ValueError('Choose an empty output directory; stale release assets are not allowed')
    a.work.mkdir(parents=True)
    tree=a.work/'tree'; shutil.copytree(ROOT,tree,ignore=shutil.ignore_patterns('.git','.venv','local','outputs','__pycache__','Gen','gen','bin','obj','build','*.xcodeproj','*.xcworkspace','*.dSYM','*.local.json'))
    ios=tree/'dm/ios'; gen=ios/'Gen'; (gen/'dm').mkdir(parents=True); (gen/'msl').mkdir()
    e=dict(os.environ,GAME='01006F8002326000',DM_BUNDLE_ID='org.acnhmobile.experimental',DM_DISPLAY_NAME='ACNH Mobile',DM_TEAM_ID='0000000000',DM_APP_CATEGORY='public.app-category.games')
    version=json.loads((ROOT/'config/version.json').read_text())
    e.update(DM_APP_VERSION=version['version'],DM_BUILD_NUMBER=str(version['build']),DM_SHOW_FPS='NO',DM_LANG='zh-Hans')
    e.pop('ACNH_RENDER_FLAGS',None);e.pop('DM_ENV_FILE',None)
    config=subprocess.check_output(['python3',ios/'tools/gen_build.py',tree/'dm/host',gen],env=e,text=True)
    config=re.sub(r'(?m)^OTHER_CFLAGS = (.*)$',r'OTHER_CFLAGS = \1 -DDM_SHELL=1',config)
    config+='\nOTHER_LDFLAGS = $(inherited) $(DM_FRAMEWORKS)\n'
    config += '\nDM_APPICON = AppIcon\n'
    (gen/'dm.xcconfig').write_text(config)
    (gen/'embed.S').write_text('// Game-free shell: no embedded module images.\n')
    (gen/'Assets.xcassets').mkdir(); (gen/'Assets.xcassets/Contents.json').write_text('{"info":{"author":"xcode","version":1}}')
    iconset=gen/'Assets.xcassets/AppIcon.appiconset'; iconset.mkdir()
    (iconset/'Contents.json').write_text(json.dumps({'images':[{'filename':'icon.png','idiom':'universal','platform':'ios','size':'1024x1024'}],'info':{'author':'xcode','version':1}}))
    shutil.copyfile(ios/'Resources/AppIcon.png', iconset/'icon.png')
    (gen/'dm/env.txt').write_text('# Official Metal 4 defaults; no legacy overrides.\n')
    version=json.loads((ROOT/'config/version.json').read_text())
    metadata={'format':1,'source_id':source_id(ROOT),'build':version['build'],'version':version['version'],'modules':['main','subsdk0','sdk'],'minimum_ios':'26.0','game_content':False}
    (gen/'dm/shell_info.json').write_text(json.dumps(metadata,indent=2)+'\n')
    subprocess.run(['xcodegen','generate','-q'],cwd=ios,env=e,check=True)
    with (a.work/'xcodebuild.log').open('w') as log:
        subprocess.run(['xcodebuild','-project','DmIOS.xcodeproj','-scheme','DmIOS','-configuration','Release','-destination','generic/platform=iOS','-derivedDataPath',str(a.work/'dd'),'CODE_SIGNING_ALLOWED=NO','CODE_SIGNING_REQUIRED=NO','CODE_SIGN_IDENTITY=','build'],cwd=ios,env=e,stdout=log,stderr=subprocess.STDOUT,check=True)
    original=a.work/'dd/Build/Products/Release-iphoneos/DmIOS.app'
    stage=a.work/'stage/Payload/ACNHMobile.app'; stage.parent.mkdir(parents=True); shutil.copytree(original,stage)
    for name in ['_CodeSignature','embedded.mobileprovision']:
        p=stage/name
        if p.is_dir(): shutil.rmtree(p)
        elif p.exists(): p.unlink()
    # Empty msl is intentional. Xcode can omit empty resource directories.
    files=audit_app(stage)
    a.output.mkdir(parents=True,exist_ok=True)
    ipa=a.output/('ACNHMobile-iOS26-v'+version['version']+'-build'+str(version['build'])+'-shell-unsigned.ipa')
    with zipfile.ZipFile(ipa,'w',zipfile.ZIP_DEFLATED) as z:
        for p in sorted(stage.rglob('*')):
            if p.is_file(): z.write(p,'Payload/ACNHMobile.app/'+str(p.relative_to(stage)))
    (a.output/'shell-audit.json').write_text(json.dumps(dict(metadata,files=files),indent=2)+'\n')
    (a.output/'SHA256SUMS.txt').write_text(hashlib.sha256(ipa.read_bytes()).hexdigest()+'  '+ipa.name+'\n')
    print(ipa)

if __name__=='__main__': main()
