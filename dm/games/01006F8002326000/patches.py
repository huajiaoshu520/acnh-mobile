# ACNH 3.0.3 only; rewriter verifies build identity and original instructions.
OBS = {'ff1d1c05670db6021c85b624a710b963': [(0x37e5968, 32, 'bd409800', 'ACNH expanded-view perspective'), (0x27d4c0, 33, 'fc1b0fe8', 'ACNH screen-layout aspect and transition masks'), (0x1296c4, 34 | (1 << 8), 'd10143ff', 'ACNH LAN result copy observer; read-only'), (0x129780, 34 | (2 << 8), 'd10143ff', 'ACNH LAN result propagation observer; read-only')]}

BINDING = [
    {'what': 'expanded-view camera and UI observation sites', 'now': 'build', 'goal': 'unreviewed', 'purpose': 'rendering', 'why': 'Verified instructions and fixed offsets in ACNH 3.0.3 only'},
    {'what': 'read-only local-network result observers', 'now': 'build', 'goal': 'unreviewed', 'purpose': 'runtime', 'why': 'Fixed caller ranges and result-copy sites in ACNH 3.0.3 only'},
]
