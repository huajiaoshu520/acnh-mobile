"""The real SwiftUI model consumes synthetic module outputs without any game files."""
from pathlib import Path
import shutil
import subprocess
import tempfile
import unittest

REPO = Path(__file__).resolve().parents[3]

@unittest.skipUnless(shutil.which('xcrun'), 'Apple Swift toolchain required')
class CompanionTests(unittest.TestCase):
    def test_image_cache_lifecycle(self):
        with tempfile.TemporaryDirectory() as folder:
            obj, exe = Path(folder) / 'stub.o', Path(folder) / 'check'
            renderer = Path(folder) / 'ImageCacheContract.swift'
            renderer.write_text((REPO / 'dm/ios/Sources/EdenCompanion.swift').read_text() + '\n' + (REPO / 'dm/ios/tests/eden_images_check.swift').read_text())
            subprocess.run(['xcrun', 'clang', '-fobjc-arc', '-c', str(REPO / 'dm/ios/tests/companion_stub.m'), '-o', str(obj)], check=True, capture_output=True)
            result = subprocess.run(['xcrun', 'swiftc', '-swift-version', '5', '-parse-as-library', '-import-objc-header', str(REPO / 'dm/host/companion_apple.h'), str(REPO / 'dm/ios/Sources/ACNHCompanion.swift'), str(renderer), str(obj), '-o', str(exe), '-framework', 'SwiftUI', '-framework', 'AppKit'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('companion images:', result.stdout)

    def test_model_contract(self):
        with tempfile.TemporaryDirectory() as folder:
            obj, exe = Path(folder) / 'stub.o', Path(folder) / 'check'
            subprocess.run(['xcrun', 'clang', '-fobjc-arc', '-c', str(REPO / 'dm/ios/tests/companion_stub.m'), '-o', str(obj)], check=True, capture_output=True)
            result = subprocess.run(['xcrun', 'swiftc', '-swift-version', '5', '-parse-as-library', '-import-objc-header', str(REPO / 'dm/host/companion_apple.h'), str(REPO / 'dm/ios/Sources/ACNHCompanion.swift'), str(REPO / 'dm/ios/Sources/EdenCompanion.swift'), str(REPO / 'dm/ios/tests/companion_model_check.swift'), str(obj), '-o', str(exe), '-framework', 'SwiftUI', '-framework', 'AppKit'], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            result = subprocess.run([str(exe)], capture_output=True, text=True)
            self.assertEqual(result.returncode, 0, result.stdout + result.stderr)
            self.assertIn('companion model:', result.stdout)

if __name__ == '__main__':
    unittest.main()
