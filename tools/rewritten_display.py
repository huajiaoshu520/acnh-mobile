"""Reject prepared modules that predate the ACNH display and read-only LAN observation sites."""
from pathlib import Path

BUILD = 'ff1d1c05670db6021c85b624a710b963'
SITES = {(0x37e5968, 32, 'bd409800'), (0x27d4c0, 33, 'fc1b0fe8'), (0x1296c4, 290, 'd10143ff'), (0x129780, 546, 'd10143ff')}

def validate_display_observations(patched):
    path = Path(patched) / 'main.obs.tsv'
    try:
        lines = path.read_text().splitlines()
        build = next(x.split()[2] for x in lines if x.startswith('# BUILD_ID'))
        found = {(int(c[0], 0), int(c[1]), c[2].lower())
                 for line in lines if line and not line.startswith('#')
                 if len(c := line.split('\t')) >= 3}
    except (OSError, ValueError, IndexError, StopIteration) as exc:
        raise ValueError('改写程序校验失败，请用当前项目重新准备 3.0.3 程序；外置 .data 不需重做。') from exc
    if build != BUILD or not SITES <= found:
        raise ValueError('改写程序版本过旧，请用当前项目重新准备 3.0.3 程序；外置 .data 不需重做。')
