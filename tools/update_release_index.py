#!/usr/bin/env python3
"""Advance the CDN index only to GitHub's latest published formal release."""
import base64
import json
import os
import re
import urllib.request


def main():
    repo = os.environ['GITHUB_REPOSITORY']
    if repo != 'huajiaoshu520/acnh-mobile':
        raise SystemExit('Unexpected release repository')
    token = os.environ['GH_TOKEN']
    def api(path, data=None):
        request = urllib.request.Request('https://api.github.com/repos/' + repo + '/' + path,
            data=None if data is None else json.dumps(data).encode(),
            headers={'Authorization': 'Bearer ' + token, 'Accept': 'application/vnd.github+json'},
            method='GET' if data is None else 'PUT')
        with urllib.request.urlopen(request, timeout=30) as response:
            return json.load(response)
    release = api('releases/latest')
    tag = release['tag_name']
    if release['draft'] or release['prerelease'] or tag != os.environ['RELEASE_TAG'] or not re.fullmatch(r'v\d+\.\d+\.\d+', tag):
        raise SystemExit('Latest formal release does not match this publication')
    content = (json.dumps({'version': tag[1:], 'url': release['html_url']}, indent=2) + '\n').encode()
    path = 'config/latest-release.json'
    current = api('contents/' + path + '?ref=main')
    if base64.b64decode(current['content']) != content:
        api('contents/' + path, {'branch': 'main', 'sha': current['sha'],
            'message': 'chore: update published release index to ' + tag,
            'content': base64.b64encode(content).decode()})
    # jsDelivr caches branch files. Request refresh after updating the release marker.
    with urllib.request.urlopen('https://purge.jsdelivr.net/gh/' + repo + '@main/' + path, timeout=30) as response:
        if response.status != 200:
            raise SystemExit('CDN refresh failed')
    print('Published release index:', tag)


if __name__ == '__main__':
    main()
