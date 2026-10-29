"""Apply optional profile-owned network permissions without changing app identity."""
from pathlib import Path
import sys, plistlib
import game

def platform_info(g):
    """Optional profile-owned network permission metadata; identity stays with the packer."""
    import json
    path = g.table('platform_info.json') if g else None
    if path is None or not path.is_file(): return {}
    data = json.loads(path.read_text())
    allowed = {'NSLocalNetworkUsageDescription', 'NSBonjourServices'}
    if not isinstance(data, dict) or data.keys() - allowed:
        raise ValueError('Unsupported profile platform metadata')
    if 'NSLocalNetworkUsageDescription' in data and not isinstance(data['NSLocalNetworkUsageDescription'], str):
        raise ValueError('Local network description must be text')
    if 'NSBonjourServices' in data and (not isinstance(data['NSBonjourServices'], list) or any(not isinstance(v, str) for v in data['NSBonjourServices'])):
        raise ValueError('Bonjour services must be a string list')
    return data


if __name__ == '__main__':
    profile, target = sys.argv[1:]
    out = Path(target)
    info = plistlib.loads(out.read_bytes())
    info.update(platform_info(game.load(profile)))
    out.write_bytes(plistlib.dumps(info))
