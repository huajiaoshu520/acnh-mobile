"""NVN fixed-function alpha test, applied after fragment colour computation.

Metal has no fixed alpha test. Buffer 30 supplies the replay-time comparison and
reference, independently of the descriptor argument buffer and shader identity.
The compiler chain hashes this file so old libraries cannot silently omit it.
"""
import re

HELPER = '''
struct dm_alpha_state { uint function; float reference; };
inline void dm_alpha_test(float a, constant dm_alpha_state& s) {
    bool pass = true;
    switch (s.function) {
        case 1u: pass = false; break;
        case 2u: pass = a < s.reference; break;
        case 3u: pass = a == s.reference; break;
        case 4u: pass = a <= s.reference; break;
        case 5u: pass = a > s.reference; break;
        case 6u: pass = a != s.reference; break;
        case 7u: pass = a >= s.reference; break;
        case 8u: break;
    }
    if (!pass) discard_fragment();
}
'''


def fragment(src):
    """Instrument float colour-0 outputs; depth-only/integer outputs stay intact.

    A missing alpha component is 1. Check each exit, before blending/depth writes.
    The extra argument also propagates to the optional integer entry point.
    """
    st = re.search(r'struct main0_out\s*\{(.*?)\};', src, re.S)
    if not st:
        return src
    col = re.search(r'\b(float[1-4]?|half[1-4]?)\s+(\w+)\s*\[\[color\(0\)\]\]', st[1])
    if not col:
        return src
    entry = re.search(r'\bfragment\s+main0_out\s+main0\s*\(', src)
    if not entry:
        raise ValueError('colour output without the expected fragment entry')
    if '[[buffer(30)]]' in src:
        raise ValueError('fragment buffer 30 is reserved for NVN alpha test')
    # Match balanced parentheses: MSL parameter attributes contain nested (...).
    start = entry.end()
    pos, depth = start, 1
    while depth and pos < len(src):
        depth += (src[pos] == '(') - (src[pos] == ')')
        pos += 1
    if depth:
        raise ValueError('unclosed fragment parameter list')
    extra = (', ' if src[start:pos-1].strip() else '') + 'constant dm_alpha_state& dm_alpha [[buffer(30)]]'
    src = src[:pos-1] + extra + src[pos-1:]
    alpha = 'float(out.%s.w)' % col[2] if col[1].endswith('4') else '1.0f'
    head, body = src[:entry.start()], src[entry.start():]
    if 'return out;' not in body:
        raise ValueError('fragment has no recognised output exit')
    body = body.replace('return out;', '{ dm_alpha_test(%s, dm_alpha); return out; }' % alpha)
    return head + HELPER + body
