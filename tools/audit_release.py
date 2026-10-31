#!/usr/bin/env python3
"""Audit source-only release contents. Does not accept local/generated runtime artifacts."""
import argparse,hashlib,json,re,sys
from pathlib import Path
FORBIDDEN_DIRS={'Gen','gen','bin','obj','build','__pycache__','node_modules','RomFS','romfs','ExeFS','save','save.work','Payload'}
FORBIDDEN_EXT={'.zs','.szs','.bwav','.bars','.bfres','.msbt','.metal','.bin','.metallib','.air','.spv','.nsp','.xci','.nca','.nso','.nro','.data','.keys','.key','.p8','.p12','.pem','.mobileprovision','.ipa','.o','.a','.dylib','.ttf','.otf','.png','.jpg','.jpeg','.webp','.mov','.mp4','.pyc'}
APP_ICON=Path("dm/ios/Resources/AppIcon.png")
APP_ICON_SHA256="2277057516772ef6059f77476cfed5ea0a1743333458c1f0241c331594665cf5"
MAGICS=(b'NSO0',b'PFS0',b'HFS0',b'ACNHDAT1',b'\xcf\xfa\xed\xfe',b'\xfe\xed\xfa\xcf',b'\x7fELF')
SECRET=re.compile(r'-----BEGIN (?:RSA |EC |OPENSSH )?PRIVATE KEY-----|(?:master_key_[0-9a-f]+|titlekek_[0-9a-f]+|header_key)\s*=\s*[0-9a-fA-F]{32,}|AKIA[0-9A-Z]{16}')
def audit(root):
 errors=[];manifest=[]
 for p in sorted(root.rglob('*')):
  rel=p.relative_to(root);parts=rel.parts
  if '.git'in parts:continue
  if p.is_symlink():errors.append(str(rel)+': symlink');continue
  if not p.is_file():continue
  if any(x in FORBIDDEN_DIRS or x.endswith(('.dSYM','.app','.xcodeproj','.xcworkspace'))for x in parts):errors.append(str(rel)+': generated/private directory')
  # The single, pinned app-identification icon is the only binary asset allowed.
  if rel==APP_ICON:
   data=p.read_bytes()
   if not data.startswith(b'\x89PNG\r\n\x1a\n') or hashlib.sha256(data).hexdigest()!=APP_ICON_SHA256:
    errors.append(str(rel)+': unexpected project icon content')
   manifest.append({'path':str(rel),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
   continue
  if p.suffix.lower()in FORBIDDEN_EXT and rel != Path('dm/gfx/tools/dmsh_pull.metal'):errors.append(str(rel)+': runtime/binary/resource format')
  data=p.read_bytes()
  if data.startswith(MAGICS)or b'\0'in data:errors.append(str(rel)+': binary/module content');continue
  try:s=data.decode('utf-8')
  except UnicodeError:errors.append(str(rel)+': non-text content');continue
  if SECRET.search(s):errors.append(str(rel)+': possible key material')
  # Inspect content, not tool references to this audit's own deny list.
  private_prefixes=('/'+'Users/','/'+'Volumes/2T/')
  if any(x in s for x in private_prefixes):errors.append(str(rel)+': local personal path')
  manifest.append({'path':str(rel),'bytes':len(data),'sha256':hashlib.sha256(data).hexdigest()})
 return errors,manifest
if __name__=='__main__':
 ap=argparse.ArgumentParser(description=__doc__);ap.add_argument('root',type=Path);ap.add_argument('--report',type=Path);a=ap.parse_args();errors,files=audit(a.root.resolve());report={'source_only':not errors,'files':len(files),'bytes':sum(x['bytes']for x in files),'errors':errors,'manifest':files}
 if a.report:a.report.write_text(json.dumps(report,indent=2))
 print(json.dumps({k:v for k,v in report.items()if k!='manifest'},indent=2));sys.exit(bool(errors))
