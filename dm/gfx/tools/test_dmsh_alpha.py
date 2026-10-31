#!/usr/bin/env python3
"""Run the canonical NX alpha-library regression suite."""
import pathlib,runpy
if __name__=='__main__':
 runpy.run_path(str(pathlib.Path(__file__).resolve().parents[1]/'tests/test_dmsh_alpha.py'),run_name='__main__')
