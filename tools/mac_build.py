"""Build the native Mac development host using matching prepared ACNH inputs."""
import os
from pathlib import Path
import shutil
import subprocess
import sys
import json
from prepare_game import load_pipeline
from shader_tables import chain_header, validate_cache, ensure_tables, TITLE


def build(root, patched, romfs, shaders, font, work, launch=False, aoc=None):
    root, patched, romfs, shaders, font, work = map(lambda p: Path(p).resolve(),
                                                   (root, patched, romfs, shaders, font, work))
    chain = chain_header(root)
    validate_cache(root, shaders, chain)
    ensure_tables(root, shaders, chain, shaders)
    if not font.is_file():
        raise ValueError('Missing standard font')
    for module in ('main', 'subsdk0', 'sdk'):
        if not (patched / (module + '.patched.bin')).is_file():
            raise ValueError('Missing rewritten module: ' + module)
    work.mkdir(parents=True, exist_ok=True)
    logs = work / 'logs'; logs.mkdir(exist_ok=True)
    sys.path.insert(0, str(root / 'dm/games'))
    import game
    g = game.load(TITLE)
    pipeline = load_pipeline()
    env = dict(os.environ, DM_GAME=TITLE)
    for key in ('DM_GAMES_DIR', 'DM_GAMES_FIRST', 'IOSOUT'):
        env.pop(key, None)
    command = pipeline.mac_run(sys.executable, root, work, romfs.parent, patched, shaders,
                               aoc, 'nvn', 'zh-Hans', env, logs, g)
    shutil.copyfile(font, work / 'font/standard.ttf')
    script = work / 'run-mac.command'
    script.write_text('#!/bin/bash\nset -e\n' + command + '\n')
    script.chmod(0o755)
    print('Mac development host: ' + str(work / 'tools/dmhost'))
    print('Run: ' + str(script))
    if launch:
        subprocess.run(['bash', str(script)], check=True)
    return script


def pack(root, patched, romfs, shaders, font, work, output, sign='-', env_file=None, aoc=None):
    """Use the upstream standalone launcher/packer with this project's prepared inputs."""
    root, patched, romfs, shaders, font, work, output = map(lambda p: Path(p).expanduser().resolve(),
                                                        (root, patched, romfs, shaders, font, work, output))
    if output.suffix != '.app':
        raise ValueError('--output must end with .app')
    # The upstream packer replaces an existing app atomically. Refuse unrelated folders.
    if output.exists():
        import plistlib
        info = output / 'Contents/Info.plist'
        if not info.is_file() or plistlib.loads(info.read_bytes()).get('CFBundleIdentifier') != 'org.acnhmobile.experimental':
            raise ValueError('Refusing to replace an unrelated existing output: ' + str(output))
    if env_file is not None:
        env_file = Path(env_file).expanduser().resolve()
        if not env_file.is_file():
            raise ValueError('Missing --env-file')
    build(root, patched, romfs, shaders, font, work, aoc=aoc)
    sys.path.insert(0, str(root / 'dm/games'))
    import game
    g = game.load(TITLE)
    version = json.loads((root / 'config/version.json').read_text())
    pipeline = load_pipeline()
    pipeline.mac_app(sys.executable, root, output, int(TITLE, 16), work, romfs.parent,
                     patched, work / 'tools/dmhost', work / 'font', shaders, aoc, 'nvn',
                     'zh-Hans', g, 'ACNH Mobile', version['version'],
                     'org.acnhmobile.experimental', root / 'dm/ios/Resources/AppIcon.png',
                     sign, dict(os.environ, DM_GAME=TITLE), work / 'logs', str(env_file or ''))
    print('Mac app: ' + str(output))
    return output
