#!/usr/bin/env python3
"""Add locally prepared owned runtime files to an ACNH shell. Never upload the resulting IPA."""
import sys
sys.dont_write_bytecode = True
import argparse, importlib.util, json, os, plistlib, re, shutil, tempfile, zipfile
from pathlib import Path, PurePosixPath
from build_shell import source_id, ROOT

def extract_shell(archive, dest):
    with zipfile.ZipFile(archive) as z:
        seen=set()
        for i in z.infolist():
            p=PurePosixPath(i.filename)
            if p.is_absolute() or '..' in p.parts or '\\' in i.filename or i.filename in seen or (i.external_attr>>16)&0o170000==0o120000:
                raise ValueError('Unsafe ZIP entry: '+i.filename)
            if not i.filename.startswith('Payload/ACNHMobile.app/'):
                raise ValueError('Not an ACNH shell archive')
            seen.add(i.filename)
        z.extractall(dest)

def main():
    ap=argparse.ArgumentParser(description=__doc__)
    for name in ['shell','patched','shaders','font','output']:
        ap.add_argument('--'+name,type=Path,required=True)
    ap.add_argument('--mac-shaders',type=Path,help='Matching Mac shader output parent')
    ap.add_argument("--control",type=Path,help="Optional owned control.nacp for supported game languages")
    a=ap.parse_args(); a.output=a.output.resolve()
    if a.output.exists(): raise ValueError('Output already exists')
    from rewritten_display import validate_display_observations
    validate_display_observations(a.patched)
    profile=ROOT/'dm/games/01006F8002326000'
    # Check everything before creating a large temporary install package.
    from shader_tables import chain_header,ensure_tables,validate_cache
    chain=chain_header(ROOT)
    validate_cache(ROOT,a.shaders,chain)
    ensure_tables(ROOT,a.shaders,chain,a.mac_shaders)
    cache=a.shaders/('b-'+chain)
    libs=list(cache.glob('*_b.metallib'))
    if not libs: raise ValueError('Missing prepared iOS shader libraries: '+str(cache))
    for lib in libs:
        reflection=json.loads(lib.with_suffix('.json').read_text())
        if reflection.get('chain')!=chain: raise ValueError('Shader chain mismatch: '+lib.name)
    from fontTools.ttLib import TTFont
    with TTFont(a.font): pass
    for name in ['main','subsdk0','sdk']:
        for suffix in ['patched.bin','fixups.tsv','gates.tsv','cuts.tsv','obs.tsv']:
            if not (a.patched/(name+'.'+suffix)).is_file(): raise ValueError('Missing rewritten module table: '+name+'.'+suffix)
    with tempfile.TemporaryDirectory(prefix='acnh-pack-') as tmp:
        work=Path(tmp); extract_shell(a.shell,work)
        app=work/'Payload/ACNHMobile.app'; dm=app/'dm'
        metadata=json.loads((dm/'shell_info.json').read_text())
        if metadata.get('format')!=1 or metadata.get('source_id')!=source_id(ROOT):
            raise ValueError('Shell and source version differ; use the same release source')
        if (app/'Frameworks').exists() or (app/'msl').exists(): raise ValueError('Input is not an empty shell')
        spec=importlib.util.spec_from_file_location('guest',ROOT/'dm/tools/guest_dylib.py'); guest=importlib.util.module_from_spec(spec); spec.loader.exec_module(guest)
        guest.write(str(a.patched),str(app/'Frameworks'),metadata['modules'],'org.acnhmobile.experimental.guest')
        for name in metadata['modules']:
            for suffix in ['fixups.tsv','gates.tsv','cuts.tsv','obs.tsv']:
                shutil.copyfile(a.patched/(name+'.'+suffix),dm/(name+'.'+suffix))
        for name in ['shader_map.tsv','pso_manifest.tsv','c1_full.tsv','cb_extent.tsv']: shutil.copyfile(profile/name,dm/name)
        for name in ('shader_pairs.tsv','pso_pairs.tsv'):
            if (profile/name).is_file():shutil.copyfile(profile/name,dm/name)
        target=app/'msl'/('b-'+chain); target.mkdir(parents=True)
        for p in cache.iterdir():
            if p.is_file() and (p.suffix in {'.metallib','.json'} or p.name=='inputs.tsv'): shutil.copyfile(p,target/p.name)
        (dm/'font').mkdir(); shutil.copyfile(a.font,dm/'font/standard.ttf')
        import subprocess,sys
        info=plistlib.loads((app/'Info.plist').read_bytes())
        env=dict(os.environ,DM_LANG=str(info.get('DMLanguage','zh-Hans')),DM_SHOW_FPS='1' if info.get('DMShowFPS') else '0')
        subprocess.run([sys.executable,ROOT/'dm/games/settings.py','01006F8002326000',a.patched,a.control or Path('/nonexistent/control.nacp'),dm/'launcher.json'],env=env,check=True)
        subprocess.run([sys.executable,ROOT/'dm/tools/make_ext_font.py',dm/'font/nintendo_ext.ttf'],check=True)
        with (dm/'env.txt').open('a') as f: f.write('\nDM_SHELL_CHAIN='+chain+'\n')
        a.output.parent.mkdir(parents=True,exist_ok=True)
        with zipfile.ZipFile(a.output,'x',zipfile.ZIP_DEFLATED) as z:
            for p in sorted((work/'Payload').rglob('*')):
                if p.is_file():z.write(p,p.relative_to(work))
    print('Private unsigned IPA:',a.output)
    print('Sign the entire IPA, including Guest.framework. Keep your .data files in Documents/ACNHMobile. Do not publish this IPA.')

if __name__=='__main__': main()
