#!/usr/bin/env python3
"""Prepare ACNH 3.0.3 using upstream extraction, rewrite and Metal shader tools."""
import hashlib,importlib.util,json,os,re,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]
TITLE='01006F8002326000'

def picker(kind):
    # Swift uses macOS preferred languages for both the native top message and terminal step prompt.
    result=subprocess.run(['xcrun','swift',ROOT/'tools/choose_build_input.swift',kind],stdout=subprocess.PIPE,text=True)
    if result.returncode==130:raise ValueError('已取消选择，未生成或删除任何安装包。')
    if result.returncode:raise ValueError('无法打开文件选择窗口：'+(result.stderr or '请查看上方终端输出。'))
    p=Path(result.stdout.strip()).expanduser().resolve()
    if not p.exists():raise ValueError('选择的文件或目录不存在。')
    return p

def load_pipeline():
    spec=importlib.util.spec_from_file_location('upstream_nsp',ROOT/'scripts/build_ipa_from_nsp.py')
    module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
    module.BuildError=ValueError
    official_mac_run=module.mac_run
    def mac_run(py,repo,work,game,patched,msl,aoc,gfx,lang,env,logs,g,env_file=''):
        host,font=module.mac_host(py,repo,work,env,logs,g)
        return official_mac_run(work,game,patched,host,font,msl,aoc,gfx,lang,env,logs,g,env_file)
    module.mac_run=mac_run
    return module

def raw_plan(game,keys):
    game=Path(game).expanduser().resolve();keys=Path(keys).expanduser().resolve()
    if keys.is_dir():keys=keys/'prod.keys'
    if not keys.is_file():raise ValueError('请选择自己的 prod.keys 文件。')
    u=load_pipeline();app,family=u.scan_nsps(game)
    if app!=int(TITLE,16):raise ValueError('所选目录不是动森，请选择仅含动森的目录。')
    if len(family['updates'])!=1:raise ValueError('目录中请只保留一个 3.0.3 更新 NSP，避免选错版本。')
    version,update=family['updates'][0]
    if version is not None and version!=2228224:raise ValueError('当前仅支持动森 3.0.3 更新。')
    return u,game,keys,family

def cache_key(family):
    # Fingerprint input contents, never key values. A new input set gets a separate cache.
    from build_shell import source_id
    h=hashlib.sha256(source_id(ROOT).encode())
    for p in [family['base'],family['updates'][0][1]]:
        h.update(p.name.encode());h.update(str(p.stat().st_size).encode());h.update(str(p.stat().st_mtime_ns).encode())
    for base in ('tools','dm/tools','dm/gfx/tools','dm/games/'+TITLE,'dm/third_party/ryujinx'):
        for p in sorted((ROOT/base).rglob('*')):
            if p.is_file() and p.name not in ('c1_full.tsv','cb_extent.tsv') and p.suffix in ('.py','.cs','.csproj','.tsv','.h','.lock','.metal') and not any(x in ('obj','bin','__pycache__') for x in p.parts):h.update(p.read_bytes())
    return h.hexdigest()[:20]

def translator_preflight(xlate,env,log):
    """Exercise the exact translator before extraction; do not alter macOS security."""
    while True:
        try:
            result=subprocess.run([str(xlate),'--source-sha'],env=env,stdout=subprocess.PIPE,
                                  stderr=subprocess.STDOUT,text=True,timeout=30)
            output=result.stdout or ''
            status=str(result.returncode)
            ok=result.returncode==0 and re.fullmatch(r'[0-9a-f]{16}',output.strip())
        except (OSError,subprocess.TimeoutExpired) as error:
            output=str(error);status='launch failed';ok=False
        with Path(log).open('a') as f:f.write('translator preflight: '+status+'\n'+output+'\n')
        if ok:return
        message=('着色器工具启动检查未通过，尚未开始提取游戏。\n'
                 '如果 macOS 提示无法验证 dmsh_xlate，请先点“完成”；确认是本项目刚编译的工具后，'
                 '在系统设置 → 隐私与安全性中手动选择“仍要打开”。\n'
                 '工具：'+str(xlate)+'\n日志：'+str(log))
        if not sys.stdin.isatty():raise ValueError(message)
        print(message,flush=True)
        input('处理完成后按回车重试，或按 Ctrl+C 取消：')

def prepare(game,keys,work,run,log,platform='ios'):
    u,game,keys,family=raw_plan(game,keys)
    cache=Path(work)/'prepared'/cache_key(family);cache.mkdir(parents=True,exist_ok=True)
    repo=cache/'source';manifest=cache/('prepared-mac.json' if platform=='mac' else 'prepared.json')
    if manifest.is_file():
        data=json.loads(manifest.read_text())
        from shader_tables import chain_header,ensure_tables
        prepared_root=Path(data['build_root'])
        if chain_header(prepared_root)!=data['chain']:raise ValueError('Prepared shader chain changed; prepare the game again')
        ensure_tables(prepared_root,Path(data['shaders']),data['chain'])
        shutil.copyfile(ROOT/'config/version.json',prepared_root/'config/version.json')
        return {k:Path(v) if k!='chain' else v for k,v in data.items()}
    if not repo.exists():
        shutil.copytree(ROOT,repo,ignore=shutil.ignore_patterns('.git','.venv','work','local','outputs','__pycache__','Gen','gen','bin','obj','build','*.xcodeproj','*.xcworkspace','*.dSYM','*.local.json'))
    shutil.copyfile(ROOT/'config/version.json',repo/'config/version.json')
    print('私人准备缓存与日志：'+str(cache),flush=True)
    # All regenerated metadata stays in the private source copy, never the Git checkout.
    env=dict(os.environ,DM_GAME=TITLE,DM_ROOT=str(cache),WORK=str(cache/'tmp'),J=str(min(os.cpu_count() or 4,8)))
    for key in ('DM_GAMES_DIR','DM_GAMES_FIRST','DM_PATCH_FRONT','XLATE_DEBUG','XLATE_REDUCED','ACNH_RENDER_FLAGS','DM_ENV_FILE'):env.pop(key,None)
    (cache/'tmp').mkdir(exist_ok=True);logs=cache/'logs';logs.mkdir(exist_ok=True)
    sys.path.insert(0,str(repo/'dm/games'));import game as games
    g=games.load(TITLE);u.PROG.mode='lines';u.PROG.plan(6)
    xlate=repo/'dm/gfx/tools/dmsh_xlate_bin/dmsh_xlate'
    u.PROG.stage('build official shader translator')
    u.run(['dotnet','publish',repo/'dm/gfx/tools/dmsh_xlate','-c','Release','-o',xlate.parent],logs/'translator.log',env=env);u.PROG.end()
    dotnet=shutil.which('dotnet')
    if dotnet:env.setdefault('DOTNET_ROOT',str(Path(dotnet).resolve().parent))
    cross=repo/'dm/gfx/tools/spirv_cross_bin/spirv-cross'
    if not cross.is_file():u.run(['sh',repo/'dm/gfx/tools/spirv_cross_bin/build.sh',cache/'spirv-cross-build'],logs/'spirv-cross.log',env=env)
    env['XLATE']=str(xlate)
    translator_preflight(xlate,env,logs/'translator-preflight.log')
    u.PROG.stage('build extraction tool')
    nx=u.build_nx_extract(repo,cache,logs,env);u.PROG.end()
    family=u.decompress_nsz(game,int(TITLE,16),family,keys,env,work=cache)
    ex=u.extract(nx,keys,cache/'extract','3.0.3',family['base'],family['updates'][0][1],None,logs,g)
    bad=u.build_mismatch(ex,'v2228224',g)
    if bad:raise ValueError('游戏版本不匹配：'+', '.join(bad))
    patched=cache/'patched';u.rewrite(sys.executable,repo,ex/'ExeFS',patched,env,logs,g)
    u.run([sys.executable,repo/'dm/gfx/tools/tool_lock.py','--update-lock'],logs/'tool-lock.log',env=env)
    u.chain_header(sys.executable,repo,env,logs)
    sh=cache/'shader'
    key=u.split_shaders(sys.executable,repo,[ex/'RomFS'],[ex/'ExeFS/main'],sh/'pkg',env,logs,g)
    ios=u.build_shaders(sys.executable,repo,sh,xlate,env,logs,g,pkg_key=key,mac=platform=='mac')
    chain=u.chain_of(repo)   # build_shaders also generates/validates both local tables, even on reuse
    data=dict(patched=patched,romfs=ex/'RomFS',shaders=ios,build_root=repo,chain=chain)
    # Written only after every stage succeeds; later IPA construction validates the cache again.
    manifest.write_text(json.dumps({k:str(v) for k,v in data.items()}))
    return data
