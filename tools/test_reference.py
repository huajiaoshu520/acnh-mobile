#!/usr/bin/env python3
"""Fetch pinned, source-only NX profiles into CI scratch space for generic regression fixtures."""
import io,os,tarfile,urllib.request
from pathlib import Path
REV='f11018a1f3125ec3046c30faae6147956a1a7c60'
def main():
 root=Path(os.environ['RUNNER_TEMP'])/'nxnative-test-reference'
 root.mkdir(parents=True,exist_ok=True)
 with urllib.request.urlopen('https://github.com/seedds/nxnative/archive/'+REV+'.tar.gz',timeout=60) as response:
  data=response.read()
 with tarfile.open(fileobj=io.BytesIO(data),mode='r:gz') as archive:
  for item in archive:
   parts=Path(item.name).parts
   if not item.isfile() or len(parts)<4 or parts[1:3]!=('dm','games'):continue
   if Path(item.name).suffix not in ('.py','.tsv','.md'):continue
   dest=root.joinpath(*parts[1:])
   if not dest.resolve().is_relative_to(root.resolve()):raise ValueError('invalid archive path')
   dest.parent.mkdir(parents=True,exist_ok=True);dest.write_bytes(archive.extractfile(item).read())
 if not (root/'dm/games/01007EF00011E000/game.tsv').is_file():raise ValueError('missing test profile')
 with open(os.environ['GITHUB_ENV'],'a') as env:env.write('DM_GAMES_DIR='+str(root/'dm/games')+'\n')
 print('Pinned NX source fixtures prepared: '+REV)
if __name__=='__main__':main()
