#!/usr/bin/env python3
"""Install build dependencies in a project venv and standard user tool locations."""
import importlib.util,os,shutil,subprocess,sys
from pathlib import Path
ROOT=Path(__file__).resolve().parents[1]

def call(args):
    subprocess.run([str(x) for x in args],check=True)

def available(module):
    try:return importlib.util.find_spec(module) is not None
    except (ImportError,ModuleNotFoundError):return False

def ensure(raw=False,plan=False,mac=False):
    os.environ['PATH']=str(Path.home()/'.dotnet')+os.pathsep+os.environ.get('PATH','')
    tools=([] if mac else ['xcodegen'])+(['cmake','zstd'] if raw else [])
    missing=[name for name in tools if not shutil.which(name)]
    if sys.version_info<(3,12):missing.append('python@3.14')
    dotnet=raw and (not shutil.which('dotnet') or not any(x.startswith('10.') for x in subprocess.check_output(['dotnet','--list-sdks'],text=True).splitlines()))
    packages=[package for module,package in [('lz4.block','lz4'),('capstone','capstone'),('fontTools','fonttools'),('nsz','nsz')] if not available(module)]
    if raw and sys.version_info<(3,14) and not available('backports.zstd'):packages.append('backports.zstd')
    if plan:return dict(homebrew=missing,dotnet10=dotnet,python=packages)
    if subprocess.run(['xcodebuild','-version'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
        raise ValueError('请先安装完整 Xcode，打开并完成许可确认，然后重新运行。')
    if missing:
        brew=shutil.which('brew')
        if not brew:
            import tempfile,urllib.request
            print('自动安装 Homebrew；若系统权限不足，会提示安装方式。',flush=True)
            with tempfile.TemporaryDirectory() as tmp:
                script=Path(tmp)/'brew-install.sh'
                with urllib.request.urlopen('https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh',timeout=60) as response:script.write_bytes(response.read())
                subprocess.run(['bash',str(script)],env=dict(os.environ,NONINTERACTIVE='1'),check=True)
            brew='/opt/homebrew/bin/brew'
            if not Path(brew).is_file():raise ValueError('Homebrew 未安装成功，请按 https://brew.sh 完成安装后重试。')
        print('自动安装构建工具：'+', '.join(missing),flush=True);call([brew,'install',*missing])
    if sys.version_info<(3,12):
        python=Path('/opt/homebrew/opt/python@3.14/bin/python3.14')
        os.environ['ACNH_DEPENDENCIES_READY']='1'
        os.execv(str(python),[str(python),*sys.argv])
    if dotnet:
        # Microsoft official installer; user-local SDK, no administrator privileges.
        import tempfile,urllib.request
        with tempfile.TemporaryDirectory() as tmp:
            script=Path(tmp)/'dotnet-install.sh'
            with urllib.request.urlopen('https://dot.net/v1/dotnet-install.sh',timeout=60) as response:script.write_bytes(response.read())
            call(['bash',script,'--channel','10.0','--install-dir',Path.home()/'.dotnet'])
        os.environ['PATH']=str(Path.home()/'.dotnet')+os.pathsep+os.environ['PATH']
    if raw and shutil.which('dotnet'):
        runtime=next(line for line in subprocess.check_output(['dotnet','--list-runtimes'],text=True).splitlines() if line.startswith('Microsoft.NETCore.App 10.'))
        os.environ['DOTNET_ROOT']=str(Path(runtime.split('[',1)[1].rstrip(']')).parents[1])
    if packages:
        venv=ROOT/'.venv';python=venv/'bin/python'
        if not python.exists():call([sys.executable,'-m','venv',venv])
        elif subprocess.check_output([python,'-c','import sys; print(sys.version_info >= (3,12))'],text=True).strip()!='True':
            call([sys.executable,'-m','venv','--upgrade',venv])
        print('自动安装 Python 构建依赖：'+', '.join(packages),flush=True)
        call([python,'-m','pip','install','-r',ROOT/'requirements.txt',*(['backports.zstd'] if sys.version_info<(3,14) else [])])
        # Keep the private interpreter setting from causing a re-exec loop.
        os.environ['ACNH_DEPENDENCIES_READY']='1'
        os.execv(str(python),[str(python),*sys.argv])
    if raw:
        for sdk in (('macosx',) if mac else ('macosx','iphoneos')):
            if subprocess.run(['xcrun','-sdk',sdk,'metal','--version'],stdout=subprocess.DEVNULL,stderr=subprocess.DEVNULL).returncode:
                print('自动下载 Xcode Metal 编译工具…',flush=True)
                call(['xcodebuild','-downloadComponent','MetalToolchain']);break
