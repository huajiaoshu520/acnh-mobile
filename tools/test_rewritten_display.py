import tempfile, unittest
from pathlib import Path
from rewritten_display import BUILD, validate_display_observations

class RewrittenDisplayTests(unittest.TestCase):
    def test_current_program_and_stale_prepared_program(self):
        with tempfile.TemporaryDirectory() as tmp:
            p = Path(tmp) / 'main.obs.tsv'
            p.write_text('# BUILD_ID\t'+BUILD+'\n')
            with self.assertRaisesRegex(ValueError, '版本过旧'):
                validate_display_observations(tmp)
            p.write_text(p.read_text()+'0x37e5968\t26\tbd409800\tprojection\n0x27d4c0\t28\tfc1b0fe8\tlayout\n')
            validate_display_observations(tmp)
            p.write_text(p.read_text().replace('fc1b0fe8','00000000'))
            with self.assertRaises(ValueError): validate_display_observations(tmp)
    def test_missing_table(self):
        with tempfile.TemporaryDirectory() as tmp:
            with self.assertRaises(ValueError): validate_display_observations(tmp)

if __name__ == '__main__': unittest.main()
