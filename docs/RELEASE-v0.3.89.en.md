# ACNH Mobile 0.3.89 · Build 98

- Adopt the official tail-call recognizer, requiring proof that the stack frame is restored before accepting a nonterminal indirect branch.
- Adopt Heaps PAK shader-cache extraction and regression tests.
- All 21 rewritten ACNH outputs are byte-identical to the previous build. Existing external data and aoc remain reusable.

Only the LiveContainer + SideStore LAN host IPA is attached. Allow local network access when using this host; the same patched host needs no reinstall. Direct sideloads need no host. Phone behavior still requires device verification.
