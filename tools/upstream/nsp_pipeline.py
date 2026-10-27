import shlex
# Selected unchanged helpers from seedds/nxnative scripts/build_ipa_from_nsp.py.
# Baseline e72a06510e6dfff4a77c49fd205a82e21cad0786; see repository LICENSE.
# Only extraction, rewriting and the official shader pipeline are retained.
import ast, collections, ctypes, getpass, hashlib, json, os, re, shlex, shutil, struct, subprocess, sys, tempfile, threading, time, types, zlib


from pathlib import Path


ALLOW_DAMAGED = False                   # a damaged NSP (files failing the NCA's hash check: an incomplete copy or a broken download) stops the build; True writes those files as read and goes on


BEAT = 30                               # seconds between progress lines in 'lines' mode


class BuildError(Exception):
    pass


def clock():
    return time.strftime('%H:%M:%S')


def say(msg):
    PROG.say(f'[{clock()}] {msg}')


def hms(s):   # seconds → m:ss, or h:mm:ss from an hour on
    s = int(s); h, m = divmod(s // 60, 60)
    return f'{h}:{m:02d}:{s % 60:02d}' if h else f'{m}:{s % 60:02d}'


def size_unit(v):   # (unit, divisor) for a byte count
    return next(((u, d) for u, d in (('GB', 2**30), ('MB', 2**20)) if v >= d), ('KB', 2**10))


def amount(v, unit=''):   # a count, or a byte size when unit is 'B'
    if unit == 'B':
        u, d = size_unit(v); return f'{v / d:.1f} {u}'
    return f'{v:,}' + (f' {unit}' if unit else '')


class Progress:
    """The console view of the build: one line per stage, "[k/n] stage · step", with a bar, done/total, percent, elapsed time and ETA when the work can be counted, else a spinner and the elapsed time.
    Modes: bar = the line is redrawn in place (a terminal, Spyder's console) and replaced by a ✓ line when the stage ends; lines = a plain line when a stage or step starts, every BEAT seconds and at the end
    (for output piped to a file); off = only each stage's start and end lines. Counters come from set(), or from a poll function that runs in a thread of its own (it may walk a big folder) and returns set()'s arguments.
    Messages go through say(), which prints them above the live line. json (for a program that shows the build) = one JSON object per stdout line instead: the events stage, step, progress, stage_end and say;
    a count change goes out at most every JSON_INTERVAL seconds (the ticker sends one held back), any other change at once"""
    SPIN, BLOCKS, POLL, JSON_INTERVAL = '⠋⠙⠹⠸⠼⠴⠦⠧⠇⠏', ' ▏▎▍▌▋▊▉', 1.0, 0.5

    def __init__(self, mode='off'):
        self.mode, self.lock, self.k, self.n, self.cur, self.shown, self.ticker, self.tstop = mode, threading.RLock(), 0, 0, None, 0, None, None

    def plan(self, n):   # the next n stages are numbered 1..n
        with self.lock: self.k, self.n = 0, n

    def add(self, n):   # n more stages than planned (one that turns out to be needed: a provisional profile's heap check)
        with self.lock: self.n += n

    def stage(self, name, poll=None, step='', **kw):
        """Start the next stage, ending the current one. kw: set()'s arguments"""
        with self.lock:
            self.end()
            self.k += 1; now = time.time()
            self.cur = types.SimpleNamespace(name=name, step=step, unit='', total=None, done=None, est=False, note='', t0=now, hist=collections.deque(), beat=now, pstop=None, jlast=None, jstate=None)
            if self.mode == 'json':
                self._json({'ev': 'stage', 'k': self.k, 'n': self.n, 'name': name, 'step': step, 't': round(now, 3)})
            elif self.mode != 'bar': self._out(f'[{clock()}] [{self.k}/{self.n}] {name}' + (f' · {step}' if step else '') + ' …')
            self.set(**kw)
            if self.mode != 'off' and not self.ticker:
                self.tstop = stop = threading.Event()
                self.ticker = threading.Thread(target=self._tick, args=(stop,), daemon=True); self.ticker.start()
        if poll: self.watch(poll)

    def watch(self, poll):
        """Call poll() over and over until the stage ends or watch() is called again (None: stop); poll returns set()'s arguments, or None to leave the line as it is"""
        with self.lock:
            st = self.cur
            if not st: return
            if st.pstop: st.pstop.set()
            st.pstop = stop = threading.Event()
        if poll: threading.Thread(target=self._poll, args=(st, poll, stop), daemon=True).start()

    def set(self, done=None, total=None, step=None, unit=None, est=None, note=None):
        """done / total: the count of the current step (unit 'B' = bytes); est: total is an estimate (shown with ~); note: shown after the count;
        step: the step within the stage (a new step starts with no count and restarts the ETA)"""
        with self.lock:
            st = self.cur
            if not st: return
            if step is not None and step != st.step:
                st.step, st.done, st.total, st.unit, st.est, st.note, st.beat = step, None, None, '', False, '', time.time(); st.hist.clear()
                if self.mode == 'lines': self._out(f'[{clock()}] [{self.k}/{self.n}] {st.name} · {step} …')
                if self.mode == 'json': self._json({'ev': 'step', 'k': self.k, 'name': st.name, 'step': step})
            if unit is not None: st.unit = unit
            if total is not None: st.total = total
            if est is not None: st.est = est
            if note is not None: st.note = note
            if done is not None:
                st.done = done; now = time.time(); st.hist.append((now, done))
                while len(st.hist) > 2 and st.hist[1][0] < now - 60: st.hist.popleft()   # the ETA follows the rate of the last minute
            if self.mode == 'json': self._json_progress(st)

    def _json_progress(self, st, force=False):   # a progress event when anything changed; a count change alone waits JSON_INTERVAL unless forced (the caller holds the lock)
        state = (st.step, st.total, st.unit, st.est, st.note, st.done)
        if state == st.jstate: return
        now = time.monotonic()
        if not force and st.jstate is not None and state[:-1] == st.jstate[:-1] and now - st.jlast < self.JSON_INTERVAL: return
        st.jlast, st.jstate = now, state
        self._json({'ev': 'progress', 'k': self.k, 'name': st.name, 'step': st.step, 'done': st.done, 'total': st.total,
                    'unit': st.unit, 'est': st.est, 'note': st.note,
                    'frac': min(st.done / st.total, 0.99) if st.total and st.done is not None else None,
                    'eta': self._eta(st), 'elapsed': round(time.time() - st.t0, 3)})

    def _json(self, obj):   # one JSON object per stdout line (a message's newlines are escaped)
        obj.setdefault('t', round(time.time(), 3))
        print(json.dumps(obj, ensure_ascii=False, allow_nan=False), flush=True)

    def end(self, summary='', ok=True):
        """End the current stage with a ✓ (or ✗) line: its elapsed time and a summary"""
        with self.lock:
            st, self.cur = self.cur, None
            if not st: return
            if st.pstop: st.pstop.set()
            if self.mode == 'json':
                self._json_progress(st, force=True)   # the last count, if it was held back
                self._json({'ev': 'stage_end', 'k': self.k, 'name': st.name, 'ok': ok, 'summary': summary, 'elapsed': round(time.time() - st.t0, 3)})
                return
            self._out(f'[{clock()}] {"✓" if ok else "✗"} [{self.k}/{self.n}] {st.name} ({hms(time.time() - st.t0)})' + (f': {summary}' if summary else ''))

    def say(self, msg):
        with self.lock:
            if self.mode == 'json': self._json({'ev': 'say', 'msg': msg})
            else: self._out(msg)

    def close(self, ok=True):   # end the current stage and stop the ticker (the mode stays: a message after it is still JSON in json mode)
        with self.lock:
            self.end(ok=ok)
            if self.tstop: self.tstop.set()
            self.ticker = self.tstop = None

    def _count(self, st):   # "1.9/~5.0 GB", "1,203/19,592 shaders", "2,301 files" (no total); '' without a count
        if st.done is None: return ''
        if not st.total: return amount(st.done, st.unit)
        t = '~' if st.est else ''
        if st.unit == 'B':
            u, d = size_unit(st.total); return f'{st.done / d:.1f}/{t}{st.total / d:.1f} {u}'
        return f'{st.done:,}/{t}{st.total:,}' + (f' {st.unit}' if st.unit else '')

    def _eta(self, st):   # seconds left at the rate of the last minute; None while unknown
        if not st.total or len(st.hist) < 2: return None
        (t1, d1), (t2, d2) = st.hist[0], st.hist[-1]
        if t2 - t1 < 3 or d2 <= d1: return None
        return max(st.total - st.done, 0) * (t2 - t1) / (d2 - d1)

    def _fields(self, st):   # (head, fraction done or None, count, note, times)
        head = f'[{self.k}/{self.n}] {st.name}' + (f' · {st.step}' if st.step else '')
        frac = min(st.done / st.total, 0.99) if st.total and st.done is not None else None   # 100 % only when the stage ends
        eta = self._eta(st)
        return head, frac, self._count(st), st.note, hms(time.time() - st.t0) + (f' · ETA {hms(eta)}' if eta is not None else '')

    def _line(self, st, width):
        head, frac, count, note, times = self._fields(st)
        head = f'{self.SPIN[int(time.time() * 8) % len(self.SPIN)]} {head}'
        pct = f'{frac * 100:.0f}%' if frac is not None else ''
        for note in (note, ''):   # a narrow console loses the note first, then the bar
            rest = '  '.join(x for x in (' · '.join(x for x in (count, note) if x), times) if x)
            barw = min(20, width - len(f'{head}  {pct:>4}  {rest}') - 3)   # the bar takes what the console's width leaves, up to 20 cells; none below 8
            if frac is None or barw >= 8: break
        if frac is not None and barw >= 8:
            cells = frac * barw; full = int(cells); part = int((cells - full) * 8)
            pct = ('█' * full + (self.BLOCKS[part] if part else '')).ljust(barw, '░') + f' {pct:>4}'
        return '  '.join(x for x in (head, pct, rest) if x)[:width - 1]

    def _out(self, text):   # a whole line above the live one (the caller holds the lock)
        if self.shown: sys.stdout.write('\r' + ' ' * self.shown + '\r'); self.shown = 0
        print(text, flush=True)
        if self.mode == 'bar' and self.cur: self._draw()

    def _draw(self):
        line = self._line(self.cur, shutil.get_terminal_size((80, 24)).columns)   # Spyder has no size to ask, and a line that wraps there cannot be redrawn: 80 columns
        sys.stdout.write('\r' + line + ' ' * max(self.shown - len(line), 0)); sys.stdout.flush()
        self.shown = len(line)

    def _tick(self, stop):
        while not stop.wait(0.25 if self.mode == 'bar' and not in_ipython() else 1):   # Spyder's console gets fewer redraws
            with self.lock:
                if stop.is_set(): return   # close() stopped this ticker while it waited for the lock
                st = self.cur
                if not st: continue
                try:
                    if self.mode == 'bar': self._draw()
                    elif self.mode == 'json': self._json_progress(st)   # a count that set() held back
                    elif self.mode == 'lines' and time.time() - st.beat >= BEAT:
                        st.beat = time.time(); head, frac, count, note, times = self._fields(st)
                        self._out(f'[{clock()}] {head}: ' + ', '.join(x for x in (f'{frac * 100:.0f}%' if frac is not None else '', count, note, times) if x))
                except Exception: pass   # progress never stops the build, and one bad line does not stop the next

    def _poll(self, st, poll, stop):
        while not stop.is_set():
            t, step = time.time(), st.step
            try: r = poll()
            except Exception: r = None   # progress never stops the build
            with self.lock:   # a count read for a step that ended meanwhile is dropped
                if r and self.cur is st and not stop.is_set() and ('step' in r or st.step == step): self.set(**r)
            stop.wait(max(self.POLL, (time.time() - t) * 5))   # a slow poll (a walk over tens of thousands of files) runs less often


PROG = Progress()


def tree_size(d):   # (files, bytes) under d; (0, 0) when it does not exist yet
    n = b = 0; todo = [str(d)]
    while todo:
        try: it = os.scandir(todo.pop())
        except OSError: continue
        with it:
            for e in it:
                try:
                    if e.is_dir(follow_symlinks=False): todo.append(e.path)
                    else: n += 1; b += e.stat(follow_symlinks=False).st_size
                except OSError: pass
    return n, b


def in_ipython():   # Spyder (and Jupyter) run this file inside IPython: input works there, and sys.exit must not be called (it piles an error onto the console)
    import builtins
    return hasattr(builtins, '__IPYTHON__')


def run(cmd, log, env=None, cwd=None, on_line=None):
    """Run a subprocess with all its output written to log (the console shows the stage line instead: the shader build alone emits tens of thousands of compiler warnings);
    stop on a non-zero exit, printing the log's last lines. on_line: called with every output line (build_ipa.sh's own step lines move its stages on)"""
    cmd = [str(c) for c in cmd]
    log.parent.mkdir(parents=True, exist_ok=True)
    tail = collections.deque(maxlen=40)
    p = subprocess.Popen(cmd, env=env, cwd=cwd, stdout=subprocess.PIPE, stderr=subprocess.STDOUT, text=True, errors='replace', bufsize=1)
    try:
        with open(log, 'w') as f:
            for line in p.stdout:
                f.write(line); tail.append(line)
                if on_line:
                    try: on_line(line.rstrip('\n'))
                    except Exception: pass   # progress never stops the build
        p.wait()
    except KeyboardInterrupt:
        p.terminate(); p.wait(); raise
    finally:
        p.stdout.close()
    if p.returncode:
        PROG.end(ok=False)
        PROG.say(f'---- last lines of {log} ----\n' + ''.join(tail).rstrip('\n'))
        tool = Path(cmd[1] if cmd[0] in (sys.executable, 'bash') else cmd[0]).name
        raise BuildError(f'{tool} exit code {p.returncode} (full output: {log})')


def build_id(nso):
    with open(nso, 'rb') as f:
        f.seek(0x40); return f.read(16).hex()


def rmtree(p):
    if p.is_symlink() or p.is_file(): p.unlink()
    elif p.exists(): shutil.rmtree(p)


def build_nx_extract(repo, work, logs, env):
    """Build nx_extract into <work>/tools/nx_extract/. It is built in a temporary folder and copied over: with an apostrophe in the output path (e.g. "Bowser's Fury") MSBuild's conditions break,
    LibHac.dll and the executable are not copied, and the build still reports success"""
    nx = work / 'tools/nx_extract/nx_extract'
    with tempfile.TemporaryDirectory(prefix='nx_extract_') as t:
        run(['dotnet', 'build', repo / 'dm/tools/nx_extract', '-c', 'Release', '-o', t, '-v', 'quiet', '-nologo'], logs / 'nx_extract_build.log', env=env)
        if not (Path(t) / 'nx_extract').exists(): raise BuildError(f'nx_extract built without an executable (see {logs / "nx_extract_build.log"})')
        rmtree(nx.parent); shutil.copytree(t, nx.parent)
    return nx


def swap_in(tmp, dst):   # swap the new output in as a whole (the old one is untouched on failure)
    rmtree(dst); tmp.rename(dst)


def pfs0_names(path):   # NSP = PFS0: list the file names inside
    with open(path, 'rb') as f:
        h = f.read(16)
        if h[:4] != b'PFS0': return []
        n, st = struct.unpack_from('<II', h, 4)
        ent, strs = f.read(n * 0x18), f.read(st)
    return [strs[o:strs.index(b'\0', o)].decode(errors='replace') for o in (struct.unpack_from('<I', ent, i * 0x18 + 0x10)[0] for i in range(n))]


def title_of(path):   # the ticket name (the first 16 digits of the rights ID = title ID), else [title ID] in the file name
    for n in pfs0_names(path):   # the ticket first: a file name's tag can be wrong (FFT's base NSP is tagged …0001, its ticket is …0000)
        if re.match(r'[0-9A-Fa-f]{32}\.tik$', n): return int(n[:16], 16)
    m = re.search(r'\[([0-9A-Fa-f]{16})\]', path.name)
    return int(m.group(1), 16) if m else None


def version_of(path):
    m = re.search(r'\[v(\d+)\]', path.name)
    return int(m.group(1)) if m else None


def scan_nsps(nsp_dir):
    """Group by game: {application title ID: {'base': path, 'updates': [(version, path)], 'dlc': [(title ID, version, path)]}}"""
    fams, unknown = {}, []
    for p in sorted([*nsp_dir.glob('*.nsp'), *nsp_dir.glob('*.xci')]):   # a base game may be a cartridge dump (.xci: Monster Hunter Rise); nx_extract reads its secure partition
        if p.stat().st_size == 0:   # an empty file left by an incomplete download (the Zelda BotW folder holds a 0-byte DLC NSP next to the complete (1).nsp)
            say(f'skipping an empty file: {p.name}'); continue
        tid = title_of(p)
        if tid is None: unknown.append(p); continue
        low = tid & 0xFFF   # base …000, update …800, DLC = base + 0x1000 + index
        app = tid if low == 0 else tid & ~0xFFF if low == 0x800 else (tid & ~0xFFF) - 0x1000
        f = fams.setdefault(app, {'base': None, 'updates': [], 'dlc': []})
        if low == 0: f['base'] = p
        elif low == 0x800: f['updates'].append((version_of(p), p))
        else: f['dlc'].append((tid, version_of(p), p))
    if len(fams) == 1 and len(unknown) == 1 and next(iter(fams.values()))['base'] is None:   # a base game converted from a cartridge may have neither tags nor a ticket; build IDs are checked afterwards
        next(iter(fams.values()))['base'] = unknown[0]
    if not fams: raise BuildError(f'no NSP with a readable title ID in {nsp_dir}')
    if len(fams) > 1: raise BuildError('more than one game in the folder, keep one game per folder: ' + ', '.join(f'{t:016X}' for t in fams))
    app, fam = next(iter(fams.items()))
    if fam['base'] is None: raise BuildError(f'no base game NSP found in {nsp_dir} (title ID {app:016X})')
    return app, fam


def files_under(d):   # (relative path, stat) of every file under d, sorted; [] when it does not exist
    out, todo = [], [str(d)]
    while todo:
        try: it = os.scandir(todo.pop())
        except OSError: continue
        with it:
            for e in it:
                if e.is_dir(follow_symlinks=False): todo.append(e.path)
                else: out.append((os.path.relpath(e.path, d), e.stat(follow_symlinks=False)))
    return sorted(out, key=lambda x: x[0])


def fingerprint(*items, env=None):
    """sha256 over a stage's inputs. Items: ('files', paths) = names and content (a missing file counts too); ('tree', folder) = relative path and size of every file under it
    (a RomFS: check_build pins its version, so names and sizes stand for its content); ('stat', folder) = relative path, size and mtime of every file under it (the translator's binaries);
    ('cmd', argv) = a tool's version output (run with env); anything else = its repr"""
    h = hashlib.sha256()
    for it in items:
        kind, v = it if isinstance(it, tuple) and len(it) == 2 and it[0] in ('files', 'tree', 'stat', 'cmd') else ('repr', it)
        h.update(f'\0{kind}\0'.encode())
        if kind == 'files':
            for p in map(Path, v):
                h.update(p.name.encode() + b'\0')
                try: h.update(hashlib.sha256(p.read_bytes()).digest())
                except OSError: h.update(b'missing')
        elif kind in ('tree', 'stat'):
            for rel, st in files_under(v): h.update(f'{rel}\0{st.st_size}\0{st.st_mtime_ns if kind == "stat" else ""}\n'.encode())
        elif kind == 'cmd':
            try: r = subprocess.run(v, env=env, capture_output=True, text=True, timeout=120); h.update((r.stdout + r.stderr).encode())
            except (OSError, subprocess.TimeoutExpired): h.update(b'missing')
        else: h.update(repr(v).encode())
    return h.hexdigest()


def reused(keyfile, key):   # the note the stage's last successful run wrote beside its key ('' when none), or None when the key differs or is missing
    try: k, _, note = keyfile.read_text().partition('\n')
    except OSError: return None
    return note.strip() if k == key else None


def tool_files(repo):   # what the shader stages run: dm/gfx/tools' scripts and the lock of the translator's fingerprints, the profile reader, the iOS compile convention, the NSO reader
    return sorted((repo / 'dm/gfx/tools').glob('*.py')) + [repo / p for p in ('dm/gfx/tools/dmsh_chain.lock', 'dm/games/game.py', 'dm/ios/tools/ios_metal.py')]


def chain_files(repo):   # what the iOS shaders run: dmsh_build.py and every repository module it imports, at any depth (dmsh_*.py, ios_metal.py, game.py), and the translator's lock; not the split's readers (shader_split.py, unityfs.py, uepak.py)
    dirs, todo, seen = [repo / d for d in ('dm/gfx/tools', 'dm/ios/tools', 'dm/games')], [repo / 'dm/gfx/tools/dmsh_build.py', repo / 'dm/gfx/tools/own_compilers.py'], set()
    while todo:
        p = todo.pop()
        if p in seen or not p.is_file(): continue
        seen.add(p)
        for n in ast.walk(ast.parse(p.read_bytes())):
            mods = [a.name for a in n.names] if isinstance(n, ast.Import) else [n.module] if isinstance(n, ast.ImportFrom) and n.module else []
            todo += [d / f'{m.split(".")[0]}.py' for m in mods for d in dirs]
    return sorted(seen) + [repo / 'dm/gfx/tools/dmsh_chain.lock']


def package_key(pkg):   # the shader package's content (file names and bytes): what the iOS shaders are built from
    fs = [rel for rel, _ in files_under(pkg)]
    return fingerprint(fs, ('files', [pkg / f for f in fs]))


def chain_of(repo):   # the chain version dm/gfx/nvn/dmsh_chain.h holds (None when it has none)
    try: m = re.search(r'#define DMSH_CHAIN "(.*)"', (repo / CHAIN_H).read_text())
    except OSError: return None
    return m.group(1) if m else None

def chain_header(py, repo, env, logs):
    """The chain version header, written again when it no longer holds the version the source tree gives (a chain change committed without dmsh_chain.py --header): dmsh_build.py stops
    on a stale one, and the host and build_ipa.sh read it. The version follows from the source alone (dmsh_chain.py), one for every game (GitHub issue #78: Breath of the Wild after bb585cd)"""
    r = subprocess.run([str(py), str(repo / 'dm/gfx/tools/dmsh_chain.py')], env=env, capture_output=True, text=True)
    if r.returncode or not r.stdout.split(): raise BuildError(f'dmsh_chain.py exit code {r.returncode}: {(r.stderr.strip() or "no version")[-500:]}')
    want, have = r.stdout.split()[-1], chain_of(repo)
    if have == want: return
    run([py, repo / 'dm/gfx/tools/dmsh_chain.py', '--header'], logs / 'dmsh_chain.log', env=env)
    say(f'chain version header rewritten: {repo / CHAIN_H} held {have or "no version"}, the source tree gives {want}')

CHAIN_TABLES = (('c1_full.tsv', 'c1_guard.py', 'c1 guard'), ('cb_extent.tsv', 'cb_extent.py', 'uniform read extents'))

def chain_tables(py, repo, sh, env, logs, g, chain):
    """c1_full.tsv (c1_guard.py: shaders whose c1 reads are not proven bounded) and cb_extent.tsv (cb_extent.py: how far each shader reads into its uniform blocks) from the Mac outputs of the
    shader build (sh/cache/b-<chain>/), into the profile folder. They are not in the repository (git ignores them): they follow from the shaders this Mac built, so every pack makes its own.
    A full dmsh_build.py run makes both; this makes those missing or not naming the library's stamp again, which the host would ignore (every c1 its own 64 KiB, whole uniform bindings
    copied; GitHub issue #78). True when both name it; False when the library has no stamp (no full build's Mac outputs to make them from)"""
    lib = sh / 'cache' / f'b-{chain}'
    want = lib_head(lib)
    if not want: return False
    for name, tool, step in CHAIN_TABLES:
        p = g.table(name)
        try: head = p.read_text(errors='replace').partition('\n')[0]
        except OSError: head = ''
        if head.startswith(want + ' '): continue
        PROG.set(step=step)
        run([py, repo / 'dm/gfx/tools' / tool, lib, p], logs / f'{tool[:-3]}.log', env=env)
        say(f'{name} ' + ('named ' + " ".join(head.split()[2:5]) if head.startswith('# chain ') else 'names no library' if head else 'missing') + f': made for {want[2:]} ({p})')
    return True


def profile_files(g):   # every file of the game profile (game.tsv and its tables) but the c1 / uniform extent tables, which the iOS shader step makes (chain_tables): no step reads them,
    out = {name for name, _, _ in CHAIN_TABLES} | {'patches.py'}   # and the rewriter's patches, which only the rewrite step reads (patch_files)
    return sorted(p for p in g.dir.iterdir() if p.is_file() and p.name not in out)


def extracted(log):   # (files, bytes) from the summary lines nx_extract wrote to log ("<dir>: N files X MB, ..." for ExeFS + RomFS; older logs have "| N files X MB"); None without them
    try: rows = re.findall(r'[:|] (\d+) files ([\d.]+) MB', log.read_text(errors='replace'))
    except OSError: return None
    return (sum(int(n) for n, _ in rows), int(sum(float(mb) for _, mb in rows) * 2**20)) if rows else None


def extract(nx, keys, ex, name, base, upd, cmp_dir, logs, g):   # the build IDs are checked by the caller (build_mismatch)
    dst, marker, log = ex / name, g.get('romfs_marker'), logs / f'extract_{name}.log'
    if (dst / 'ExeFS' / 'main').exists() and (dst / 'RomFS' / marker).exists():
        PROG.stage(f'extract {name}')
        say(f'reusing the extracted {name}: {dst}')
        PROG.end('reused')
        return dst
    tmp = ex / (name + '.partial'); rmtree(tmp)
    last = extracted(log)   # the last extraction of this version (its log is about to be overwritten) has the exact size; without one, the NSPs' size is the estimate
    def poll():
        n, b = tree_size(tmp); return {'done': b, 'note': f'{n:,} files'}
    PROG.stage(f'extract {name}', poll=poll, unit='B', total=last[1] if last else base.stat().st_size + (upd.stat().st_size if upd else 0), est=not last)
    run([nx, keys, base, upd or '-', tmp] + ([cmp_dir] if cmp_dir else []) + (['--allow-damaged'] if ALLOW_DAMAGED else []), log)
    swap_in(tmp, dst)
    n, b = extracted(log) or (0, 0)
    PROG.end(f'{n:,} files, {amount(b, "B")}')
    return dst


def build_mismatch(d, name, g):   # the modules of an extraction whose build ID is not the one the profile's build row of that version registers ([] = the registered build)
    want = g.builds().get(name)
    if want is None: return [f'{name} has no build row']
    return [f'{m} {build_id(d / "ExeFS" / m)} (registered {bid})' for m, bid in want.items() if m != 'display' and build_id(d / 'ExeFS' / m) != bid]


def rewrite(py, repo, exe_dir, out, env, logs, g):
    """patch_nso.py on every code module (each writes its own <module>.* files, all the same build_src.sha256 line), each with its log, as many at once as the Mac has cores.
    The modules are the profile's and the NROs the game registers (collect_nros; nro.tsv beside them lists those). Reused when the modules, patch_nso.py (with nso_code.py, nso_asm.py
    and the patch files of their builds), its switch and capstone are as at the last successful rewrite (Monster Hunter Rise's main alone takes 5.5 min)"""
    import capstone
    mods = g.words('modules')
    PROG.stage('rewrite', step='checking the inputs')
    nro_dir = out.with_name('nro_' + out.name.removeprefix('patched_'))
    nros = collect_nros(py, repo, exe_dir.parent / 'RomFS', nro_dir, env, logs)
    manifest = nro_dir / 'nro.tsv'
    src = {m: exe_dir / m for m in mods} | nros
    names = list(src)
    kf = out.with_name(out.name + '.key')
    key = fingerprint(('files', [repo / 'dm/tools' / f for f in ('patch_nso.py', 'nso_code.py', 'nso_asm.py', 'nso_syms.py')] + patch_files(repo, exe_dir, mods) + list(src.values()) + ([manifest] if manifest.is_file() else [])), names, capstone.__version__,
                      {k: env.get(k) for k in ('DM_PATCH_FRONT',)})
    if all((out / f'{m}.patched.bin').is_file() for m in names) and reused(kf, key) is not None:
        PROG.end(f'{", ".join(mods)}{f" and {len(nros)} NROs" if nros else ""}, reused (inputs unchanged)'); return
    tmp = out.with_name(out.name + '.partial'); rmtree(tmp); tmp.mkdir(parents=True); logs.mkdir(parents=True, exist_ok=True)
    if manifest.is_file(): shutil.copyfile(manifest, tmp / 'nro.tsv')   # refused-only manifests preserve the game's offline registration path too
    PROG.set(step=', '.join(mods) + (f' and {len(nros)} NROs' if nros else ''), done=0, total=len(names), unit='modules')
    procs, logf, todo, cores = {}, {}, list(names), max(2, os.cpu_count() or 4)
    try:
        while todo or procs:
            while todo and len(procs) < cores:
                m = todo.pop(0); logf[m] = open(logs / f'rewrite_{m}.log', 'w')
                procs[m] = subprocess.Popen([str(py), str(repo / 'dm/tools/patch_nso.py'), str(src[m]), str(tmp), m], env=env, stdout=logf[m], stderr=subprocess.STDOUT)
            for m in [m for m in procs if procs[m].poll() is not None]:
                p = procs.pop(m); logf[m].close(); PROG.set(done=len(names) - len(todo) - len(procs))
                if p.returncode:
                    log = logs / f'rewrite_{m}.log'
                    PROG.end(ok=False)
                    PROG.say(f'---- last lines of {log} ----\n' + ''.join(collections.deque(open(log, errors='replace'), maxlen=40)).rstrip('\n'))
                    raise BuildError(f'patch_nso.py exit code {p.returncode} on {m} (full output: {log})')
            time.sleep(0.2)
    finally:   # an error or an interrupt stops the other rewrites
        for p in procs.values():
            if p.poll() is None: p.terminate(); p.wait()
        for f in logf.values(): f.close()
    rmtree(kf); swap_in(tmp, out); kf.write_text(f'{key}\n')
    PROG.end(', '.join(mods) + (f' and {len(nros)} NROs' if nros else ''))


def data_rows(b):   # a table's data rows (without # comment lines: the header changes with the shader_split.py version, not with the game files)
    return [l for l in b.splitlines() if not l.startswith(b'#')]


def split_shaders(py, repo, roms, mains, pkg, env, logs, g, provisional=False, oodle=None):
    """The shader split into pkg, checked against the profile's tables. The package is reused when the tools, the profile and each version's main, RomFS (paths and sizes) and run-time capture
    are as at the last successful split. Returns the package's content key, the iOS shaders' input (None for a provisional profile or damaged files, which never reuse them): a split that runs
    again because a reader of another engine changed writes the same package, and the iOS shaders stay reused.
    oodle: for a game with Trinity packs (Pokémon Legends: Z-A; profile_guess.py shader_sources), a function that builds the Mac host and returns the split's DM_OODLE_HOST / DM_OODLE_MODULES; called only when the split runs"""
    names = ('shader_map.tsv', 'shader_facts.tsv', 'shader_pairs.tsv')
    PROG.stage('shader split', step='checking the inputs')   # then a spinner: shader_split.py writes the package and the tables only at the end
    kf = pkg.with_name(pkg.name + '.key')
    key = None if provisional or ALLOW_DAMAGED else fingerprint(('files', tool_files(repo)), ('files', profile_files(g)),
        *[x for r, m in zip(roms, mains) for x in (('files', [m]), ('tree', r), ('files', sorted((r.parent / 'glslc_capture').glob('*.glslc'))),
                                                   ('files', sorted((r.parent / 'ue_shaders').glob('*.ushaderbytecode'))))])
    if key and pkg.is_dir() and (note := reused(kf, key)) is not None:
        PROG.end(f'{note}, reused (inputs unchanged)'); return package_key(pkg)
    if oodle:   # the packs are decoded by the game's own Oodle in the Mac host (dm/host/hle_oodle.c)
        PROG.set(step='building the Mac host for the game\'s Oodle decoder'); env = dict(env, **oodle())
    PROG.set(step='splitting')
    tmp = pkg.with_name(pkg.name + '.partial'); rmtree(tmp); (tmp / 'tables').mkdir(parents=True)
    run([py, repo / 'dm/gfx/tools/shader_split.py', ','.join(map(str, roms)), ','.join(map(str, mains)), tmp / 'pkg'] + [tmp / 'tables' / n for n in names], logs / 'shader_split.log', env=env)
    m = re.search(r'shaders: (\d+) \(', (logs / 'shader_split.log').read_text(errors='replace'))
    if provisional:   # a provisional profile takes the tables as they come
        for n in names: shutil.copyfile(tmp / 'tables' / n, g.table(n))
    else:
        # Data rows byte-identical to the profile's = the same game files the tables were built from; if not, stop (the profile's tables are left alone). Only data rows are compared: the header of Engage's profile tables was written by an older shader_split
        diff = [n for n in names if data_rows((tmp / 'tables' / n).read_bytes()) != data_rows(g.table(n).read_bytes())]
        if diff: raise BuildError(f'the split shader tables differ from game profile {g.id}\'s: {", ".join(diff)} (game files not the versions the profile registers?)')
    rmtree(kf); swap_in(tmp / 'pkg', pkg); rmtree(tmp)
    note = f'{int(m.group(1)):,} shaders' if m else ''
    if key: kf.write_text(f'{key}\n{note}\n')
    PROG.end(note)
    return key and package_key(pkg)


class ShaderWatch:
    """Progress of dmsh_build.py, read from the files it writes (it prints nothing until it ends). It works in three phases:
    the cache check (a hit restores <key>_b.json into the library directory b-<chain>/: the iOS one, or the Mac's for a Mac-only build; a miss writes <key>.bin into its temporary folder dmsh_* under WORK),
    the translation of the misses (<key>.spv in the temporary folder), and the compile, which takes the keys in a fixed order (sorted, the vertex shaders with pixel pairs last)
    and writes <key>_b.json again for every miss and every pixel shader: the furthest key written so far tells how far it got"""
    def __init__(self, g, out, tmp):   # out: the library directory b-<chain>/ (the iOS one, or the Mac's for a Mac-only build)
        keys = set()
        for ln in open(g.table('shader_map.tsv'), errors='replace'):   # the keys dmsh_build.py builds: column 4 of the rows from a shader package
            c = ln.rstrip('\n').split('\t')
            if len(c) > 4 and c[0].endswith('.bin'): keys.add(c[3])
        pairs = {ln.split()[0] for ln in open(g.table('shader_pairs.tsv'), errors='replace') if ln.strip() and not ln.startswith('#')}
        order = sorted(keys - pairs) + sorted(keys & pairs)
        self.pos, self.n = {k: i for i, k in enumerate(order)}, len(order)
        self.out, self.tmp, self.t0, self.t1, self.w = out, tmp, time.time(), None, None
        self.hits, self.fin, self.top = set(), set(), -1

    def written(self, keys, since):   # those of keys whose <key>_b.json in the output was written after since
        found = set()
        try:
            with os.scandir(self.out) as it:
                for e in it:
                    if e.name.endswith('_b.json') and e.name[:-7] in keys and e.stat().st_mtime > since: found.add(e.name[:-7])
        except OSError: pass
        return found

    def __call__(self):
        if not self.w:   # dmsh_build.py's temporary folder: the dmsh_* under WORK made since it started
            self.w = max((d for d in self.tmp.glob('dmsh_*') if d.stat().st_birthtime >= self.t0 - 2), key=lambda d: d.stat().st_birthtime, default=None)
        try: ext = collections.Counter(os.path.splitext(x)[1] for x in os.listdir(self.w)) if self.w else collections.Counter()
        except OSError: ext = collections.Counter()
        if self.t1 is None:
            self.hits |= self.written(self.pos.keys() - self.hits, self.t0 - 2)
            if len(self.hits) + ext['.bin'] < self.n and not ext['.spv'] and not ext['.metal']:
                return {'step': 'cache check', 'done': len(self.hits) + ext['.bin'], 'total': self.n, 'unit': 'shaders', 'note': f'{len(self.hits):,} cached'}
            self.t1 = time.time()   # the cache check is over: from now on a <key>_b.json is written by the compile
        if ext['.bin'] and not ext['.metal'] and ext['.spv'] < ext['.bin']:
            return {'step': 'translating', 'done': ext['.spv'], 'total': ext['.bin'], 'unit': 'shaders'}
        new = self.written(self.pos.keys() - self.fin, self.t1)
        self.fin |= new; self.top = max([self.top] + [self.pos[k] for k in new])
        return {'step': 'compiling' if ext['.bin'] else 'finishing', 'done': max(len(self.fin), self.top + 1), 'total': self.n, 'unit': 'shaders',
                'note': f'{ext[".bin"]:,} not cached' if ext['.bin'] else ''}


def toolchain():   # the compilers' versions, part of the shader stages' keys
    return ('cmd', ['xcodebuild', '-version']), ('cmd', ['spirv-cross', '--revision'])


def build_shaders(py, repo, sh, xlate, env, logs, g, pkg_key=None, mac=False):
    """The iOS shader libraries (dmsh_build.py, which builds the Mac's into sh/cache in the same run), or with mac only the Mac's (a Mac-only build: no IOSOUT). Returns the folder the host takes
    them from. Reused when the shader package (pkg_key, its content; None never reuses) and the chain's tools, translator binaries, profile and compilers are as at the last successful build:
    dmsh_build.py would only restore every shader from its cache (63,000 for Breath of the Wild: about 100 s). The key of the iOS libraries is sh/ios/b-<chain>.key, that of the Mac's
    sh/mac-b-<chain>.key (not in sh/cache, whose b-* entries dmsh_build.py reads as library folders), which an iOS build writes too, so a Mac-only build after it is reused; either is taken only
    while the Mac libraries carry a full build's stamp (inputs.tsv) for c1_full.tsv and cb_extent.tsv to name (chain_tables).
    An iOS build after a Mac-only one translates the shaders again (dmsh_build.py's per-shader cache keys include the iOS compile convention), but takes the Mac libraries from its
    Metal compile cache (dmsh_mtlcache.py), so only the iPhone's are compiled"""
    ios, log = sh / 'ios', logs / 'dmsh_build.log'
    PROG.stage('Mac shaders' if mac else 'iOS shaders')
    chain = chain_of(repo)
    out = sh / 'cache' if mac else ios
    libs, kf, kmac = out / f'b-{chain}', ios / f'b-{chain}.key', sh / f'mac-b-{chain}.key'
    if pkg_key:
        PROG.set(step='checking the inputs')
        key = fingerprint(pkg_key, ('files', chain_files(repo)), ('stat', xlate.parent), ('files', profile_files(g)), *toolchain(), str(xlate),
                          {k: env.get(k) for k in ('XLATE_DEBUG', 'XLATE_REDUCED')}, env=env)
        note = next((n for k in ((kmac, kf) if mac else (kf,)) if (n := reused(k, key)) is not None), None)
        # the libraries are the last build's, so are tables that name their stamp; a missing or stale one is made from that build's Mac outputs, and without them the step runs again
        if next(libs.glob('*_b.metallib'), None) and note is not None and chain_tables(py, repo, sh, env, logs, g, chain):
            PROG.end(f'{note}, reused (inputs unchanged)'); return out
    else: key = None
    rmtree(kmac)
    if not mac: rmtree(kf)
    try: PROG.watch(ShaderWatch(g, libs, Path(env['WORK'])))
    except Exception: pass   # progress never stops the build (a missing table is dmsh_build.py's to report)
    benv = dict(env, XLATE=str(xlate), IOSOUT=str(ios))
    if mac: del benv['IOSOUT']   # dmsh_build.py builds the iOS libraries only with IOSOUT
    run([py, repo / 'dm/gfx/tools/own_compilers.py', sh / 'pkg', sh / 'cache'], log, env=benv)   # dmsh_build.py with this Mac's Metal compiler and spirv-cross
    PROG.watch(None)
    if not chain_tables(py, repo, sh, env, logs, g, chain): raise BuildError(f'no stamped Mac shader outputs in {sh / "cache" / f"b-{chain}"} (inputs.tsv): dmsh_build.py did not finish a full build')
    what = 'Mac' if mac else 'iOS'
    n = sum(1 for _ in libs.glob('*_b.metallib')) if libs.is_dir() else 0
    if n == 0: raise BuildError(f'no {what} shader libraries in {libs}')
    say(f'{n} {what} shader libraries ({libs})')
    t = log.read_text(errors='replace')
    if own := re.search(r'^note: (built with this Mac\'s compilers.*)$', t, re.M): say(own.group(1))
    m, c = re.search(r'shaders (\d+): ok (\d+)', t), re.search(r'shader cache: hit (\d+)', t)
    note = ''
    if m:
        total, ok = int(m.group(1)), int(m.group(2))
        failed = f'; {total - ok:,} failed, reasons at the end of {log}' if ok < total else ''
        note = f'{ok:,} of {total:,} shaders ready{failed}'
        PROG.end(f'{ok:,} of {total:,} shaders ready' + (f' ({int(c.group(1)):,} from the cache)' if c else '') + failed)
    else: PROG.end()
    if key:
        for k in (kmac,) if mac else (kf, kmac): k.write_text(f'{key}\n{note}\n')
    return out




CHAIN_H = 'dm/gfx/nvn/dmsh_chain.h'

HOST_DIRS = ('dm/host', 'dm/gfx', 'dm/gfx/nvn', 'dm/gfx/nvn/include', 'dm/gfx/tex', 'dm/gfx/vk', 'dm/gfx/gl')

def patch_files(repo, exe_dir, mods):
    """The patch files that name one of the modules' builds (dm/tools/sdk_patches.py, dm/games/*/patches.py): patch_nso.py loads them all but applies only those of a module's
    build ID, so another game's patches do not change this game's rewrite"""
    ids = {build_id(exe_dir / m) for m in mods}
    return [p for p in [repo / 'dm/tools/sdk_patches.py'] + sorted((repo / 'dm/games').glob('*/patches.py')) if any(i in p.read_text(errors='replace') for i in ids)]

def apple_silicon():   # hw.optional.arm64 is 1 on Apple silicon (for a Python under Rosetta too); an Intel Mac has no such key
    return subprocess.run(['sysctl', '-n', 'hw.optional.arm64'], capture_output=True, text=True).stdout.strip() == '1'

def build_mac_host(repo, out, env, logs, g):
    """The Mac host for the profile, built into out: dm/host's own compile line (make -n) with the output moved, since make takes no target path with spaces and dm/host/dmhost may be another build's.
    Reused when the compile line and its inputs (the source files of HOST_DIRS, the chain header dm/gfx/nvn/dmsh_chain.h among them, the profile header make generates, libvpx and the third-party headers, the compiler
    and the macOS SDK) are as at the last build into out. The host runs the game's own arm64 code natively, so an Intel Mac stops here (GitHub issue #54)"""
    if not apple_silicon(): raise BuildError('this step boots the game in the Mac host, which runs the game\'s own arm64 code natively: it needs a Mac with Apple silicon, and this Mac has an Intel processor')
    host = repo / 'dm/host'
    run(['make', '-s', f'GAME={g.id}', f'gen/{g.id}/game_profile.h', 'gen/vpx/libvpx.a'], logs / 'mac_host_make.log', env=env, cwd=host)
    dry = subprocess.run(['make', '-n', '-B', f'GAME={g.id}', 'dmhost'], env=env, cwd=host, capture_output=True, text=True)
    line = next((l for l in dry.stdout.splitlines() if ' -o dmhost ' in l), None)
    if not line: raise BuildError(f'make -n gave no compile line for the Mac host: {dry.stderr.strip()[-500:]}')
    cmd = shlex.split(line); cmd[cmd.index('-o') + 1] = out
    kf, tp = out.with_name(out.name + '.key'), repo / 'dm/third_party'
    srcs = [p for d in HOST_DIRS for p in sorted((repo / d).iterdir()) if p.is_file() and p.suffix in ('.c', '.m', '.h', '.S', '.inc')]
    key = fingerprint(cmd, ('files', srcs + [host / f'gen/{g.id}/game_profile.h', host / 'gen/vpx/libvpx.a', tp / 'stb_vorbis/stb_vorbis.c']),
                      ('tree', tp / 'vulkan-headers/include'), ('tree', tp / 'libvpx/vpx'), ('cmd', ['clang', '--version']), ('cmd', ['xcrun', '--show-sdk-build-version']), env=env)
    if out.is_file() and reused(kf, key) is not None: return out
    rmtree(kf); out.parent.mkdir(parents=True, exist_ok=True)
    run(cmd, logs / 'mac_host_build.log', env=env, cwd=host)
    kf.write_text(f'{key}\n')
    return out

def lib_head(lib):
    """What c1_full.tsv / cb_extent.tsv start with to be taken beside the library directory lib (dmsh_chain.py table_head): its chain version and the profile inputs its stamp (inputs.tsv,
    written by a full dmsh_build.py run) names. None when it has no stamp"""
    try: stamp = dict(l.split('\t', 1) for l in (lib / 'inputs.tsv').read_text().splitlines() if '\t' in l and not l.startswith('#'))
    except OSError: return None
    return f'# chain {lib.name} inputs {stamp["inputs"]}' if stamp.get('inputs') else None

def env_file_pairs(path):   # an environment file's switches (one KEY=VALUE per line, # starts a comment), read as the iOS app reads them (dm/ios/Sources/main.m)
    out = {}
    try: text = Path(os.path.expanduser(path)).read_text(errors='replace')
    except OSError as e: raise BuildError(f'cannot read the environment file (--env-file / DM_ENV_FILE): {e}') from e
    for ln in text.splitlines():
        k, eq, v = ln.strip().partition('=')
        if k.strip() and eq and not k.startswith('#'): out[k.strip()] = v
    return out

def mac_run(py, repo, work, game, patched, msl, aoc, gfx, lang, env, logs, g, env_file=''):
    """The last stage of a Mac-only build (PLATFORM 'mac'): the Mac host for the profile (build_mac_host, into <id>_build/tools/dmhost, where the run-time captures build it too; DM_GAMES_DIR /
    DM_GAMES_FIRST in env pick a provisional or derived profile) and the button-icon font the host gives a game for the Nintendo extension shared font (make_ext_font.py, into <id>_build/font,
    made again when the tool changed). Returns the command that runs the game in a window with sound (docs/development.md, Build by hand (Mac), step 5): its save and temporary storage in
    <id>_build/run (kept from run to run), the extracted RomFS, the Mac shader libraries (msl), the DLC, the profile's shader map (a host outside dm/host does not find it beside itself;
    c1_full.tsv and cb_extent.tsv are read beside it), the game language, and the switches of an environment file (DM_ENV_FILE / --env-file)"""
    host, font = mac_host(py, repo, work, env, logs, g)
    rd = work / 'run'
    for d in ('save', 'temp'): (rd / d).mkdir(parents=True, exist_ok=True)
    renv = {'DM_ROOT': work, 'DM_ROMFS': game / 'RomFS', 'DM_SAVE': rd / 'save', 'DM_TEMP': rd / 'temp', 'DM_MSL_DIR': msl, 'DM_FONT_DIR': font}
    if aoc: renv['DM_AOC_DIR'] = aoc
    if gfx == 'nvn': renv['DM_NVN_SHMAP'] = g.table('shader_map.tsv')
    renv.update(DM_LANG=lang, DM_WINDOW=1, DM_AUDIO=1)
    if env_file: renv.update(env_file_pairs(env_file))
    q = lambda v: shlex.quote(str(v))
    return f'cd {q(rd)} && ' + ' '.join(f'{k}={q(v)}' for k, v in renv.items()) + f' {q(host)} {q(patched)}'

def collect_nros(py, repo, romfs, dst, env, logs):
    """The NROs a game registers in NRR files (RomFS *.nrr) and loads at run time with nn::ro, found by their SHA-256 as loose files or inside the RomFS's archives (dm/tools/nro_collect.py):
    {module name: NRO path}, {} for a game without NRRs. The host maps their rewritten copies at start and loads one when the game asks (loader.c "NRO modules"; Super Smash Bros. Ultimate:
    98 in data.arc). Reused while the NRRs, RomFS tree and collection tools are unchanged"""
    nrrs = sorted(p for p in romfs.rglob('*.nrr') if p.is_file())   # Super Smash Bros. Ultimate keeps them in a folder named .nrr
    kf = dst.with_name(dst.name + '.key')
    if not nrrs:
        rmtree(dst); rmtree(kf)   # an update can remove its NRRs: do not carry the previous manifest into the rewrite
        return {}
    key = fingerprint(('files', nrrs + [repo / f for f in ('dm/tools/nro_collect.py', 'dm/tools/nso_syms.py', 'dm/gfx/tools/smash_arc.py')]), ('tree', romfs))
    if not ((dst / 'nro.tsv').is_file() and reused(kf, key) is not None):
        tmp = dst.with_name(dst.name + '.partial'); rmtree(tmp)
        PROG.set(step=f'finding the NROs {len(nrrs)} NRR files register')
        run([py, repo / 'dm/tools/nro_collect.py', romfs, tmp], logs / 'nro_collect.log', env=env)
        rmtree(kf); swap_in(tmp, dst); kf.write_text(f'{key}\n')
    # rows named 'refused' are online SDKs the host never loads (Epic Online Services): listed for the host's log, not rewritten
    return {l.split('\t')[0]: dst / (l.split('\t')[0] + '.nro') for l in (dst / 'nro.tsv').read_text().splitlines() if l and not l.startswith('#') and not l.startswith('refused\t')}


def mac_host(py, repo, work, env, logs, g):
    """The stage "Mac host" of a Mac-only build or a Mac app (PLATFORM 'mac', 'mac-app'): the Mac host for the profile (build_mac_host, into <id>_build/tools/dmhost, where the run-time captures
    build it too; DM_GAMES_DIR / DM_GAMES_FIRST in env pick a provisional or derived profile) and the button-icon font the host gives a game for the Nintendo extension shared font
    (make_ext_font.py, into <id>_build/font, made again when the tool changed). Returns (host, font folder)"""
    PROG.stage('Mac host', step='building')
    host = build_mac_host(repo, work / 'tools/dmhost', env, logs, g)
    PROG.set(step='button-icon font')
    font = work / 'font'; ttf, kf = font / 'nintendo_ext.ttf', font / 'nintendo_ext.key'
    key = fingerprint(('files', [repo / 'dm/tools/make_ext_font.py']))
    if not (ttf.is_file() and reused(kf, key) is not None):
        rmtree(kf); font.mkdir(parents=True, exist_ok=True)
        run([py, repo / 'dm/tools/make_ext_font.py', ttf], logs / 'make_ext_font.log', env=env)
        kf.write_text(f'{key}\n')
    PROG.end(str(host))
    return host, font



def mac_app_name(name):   # the app's file name: the game name with the characters Finder shows otherwise (':' and '/') as ' - '
    return re.sub(r'\s*[:/]\s*', ' - ', name).strip(' .-') or 'nxnative'



def mac_app(py, repo, out, app, work, game, patched, host, font, msl, aoc, gfx, lang, g, name, version, bundle_id, icon, sign, env, logs, env_file=''):
    """The last stage of PLATFORM 'mac-app': pack the Mac app (dm/mac/pack_app.py) from what a Mac-only build made: the Mac host and font (mac_host), the packed version's RomFS and rewritten
    modules, the Mac shader libraries (msl: <shader>/cache, whose b-<chain> goes in, or a Vulkan or GL game's <shader>, whose vk / gl goes in), the DLC, the profile's tables and an environment file.
    The save and the log go to the player's Library, keyed by the folder's title ID (app); a new app replaces the old one only when it is complete"""
    PROG.stage('Mac app', step='starting')
    settings=work / "mac-launcher.json"
    settings_env=dict(env,SETTINGS_PLATFORM="mac",GFXAPI=gfx,DM_LANG=lang)
    run([py,repo / "dm/games/settings.py",f"{app:016X}",patched,game / "control.nacp",settings],logs / "mac_launcher_settings.log",env=settings_env)
    lib = msl / (f'b-{chain_of(repo)}' if gfx == 'nvn' else 'vk' if gfx == 'vulkan' else 'gl')
    cmd = [py, repo / 'dm/mac/pack_app.py', out, '--host', host, '--patched', patched, '--romfs', game / 'RomFS', '--msl', lib, '--tables', g.table('shader_map.tsv').parent,
           '--font', font, '--settings', settings, '--title-id', f'{app:016X}', '--name', name, '--bundle-id', bundle_id, '--lang', lang, '--sign', sign or '-']
    for opt, v in (('--version', version), ('--category', g.get('app_category')), ('--marker', g.get('romfs_marker')), ('--aoc', aoc), ('--icon', icon),
                   ('--env-file', Path(os.path.expanduser(env_file)).resolve() if env_file else None)):
        if v: cmd += [opt, v]
    steps = []
    def line(ln):
        if ln.startswith('step: '): steps.append(ln[6:].strip()); PROG.set(step=steps[-1])
    run(cmd, logs / 'mac_app.log', env=env, on_line=line)
    if not (out / 'Contents/MacOS/dmhost').is_file(): raise BuildError(f'no app at {out}')
    PROG.end(f'{out.name}, {amount(tree_size(out)[1], "B")}')
    return any('(clones of the game files)' in s for s in steps)   # whether the game files are APFS clones of the build's (pack_app.py's first step says)
