"""DLC cache invalidation and transactional external delivery regressions."""
import sys,tempfile,unittest
from pathlib import Path
from unittest.mock import patch
sys.path.insert(0,str(Path(__file__).parent))
import one_click as c,prepare_game as p

class DlcTests(unittest.TestCase):
    def test_dlc_subdirectory_is_scanned_without_build_cache_directories(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);(root/'prod.keys').write_text('fixture')
            for name in ('[01006F8002326000].nsp','[01006F8002326800][v2228224].nsp'):
                (root/name).write_bytes(b'PFS0'+bytes(12))
            dlc=root/'DLC'/'nested';dlc.mkdir(parents=True)
            file=dlc/'[01006F800232712C].nsp';file.write_bytes(b'PFS0'+bytes(12))
            cache=root/'unrelated-build';cache.mkdir();(cache/'[0100000000000000].nsp').write_bytes(b'PFS0'+bytes(12))
            self.assertEqual(p.raw_plan(root,root)[3]['dlc'][0][2],file.resolve())
            (dlc/'[01006F8002326800].nsp').write_bytes(b'PFS0'+bytes(12))
            with self.assertRaisesRegex(ValueError,'DLC folder'):p.raw_plan(root,root)

    def test_added_changed_removed_dlc_invalidates_preparation(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t)
            base=root/'base';update=root/'update';dlc=root/'dlc'
            for f in (base,update,dlc):f.write_bytes(b'fixture')
            fam=dict(base=base,updates=[(1,update)],dlc=[])
            with patch('build_shell.source_id',return_value='fixture'),patch.object(p,'ROOT',root):
                initial=p.cache_key(fam)
                fam['dlc']=[(123,1,dlc)];added=p.cache_key(fam)
                dlc.write_bytes(b'changed fixture');changed=p.cache_key(fam)
                fam['dlc']=[]
                self.assertEqual(initial,p.cache_key(fam))
                self.assertEqual(len({initial,added,changed}),3)

    def test_complete_replacement_removes_stale_dlc_and_preserves_data(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);source=root/'source';out=root/'out';source.mkdir();out.mkdir()
            (source/'300.img').write_bytes(b'new DLC')
            old=out/'aoc';old.mkdir();(old/'999.img').write_bytes(b'old DLC')
            data=out/'ACNHMobile';data.mkdir();(data/'ACNH-001.data').write_bytes(b'keep')
            def retain(path):path.rename(root/'previous-aoc')
            with patch.object(c,'trash_file',side_effect=retain):c.publish_aoc(source,out)
            self.assertEqual([x.name for x in (out/'aoc').iterdir()],['300.img'])
            self.assertEqual((data/'ACNH-001.data').read_bytes(),b'keep')
            with patch.object(c,'trash_file',side_effect=AssertionError('unchanged set replaced')):c.publish_aoc(source,out)

    def test_empty_set_clears_old_dlc_and_copy_failure_preserves_it(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);source=root/'source';out=root/'out';source.mkdir();out.mkdir()
            old=out/'aoc';old.mkdir();(old/'300.img').write_bytes(b'old')
            with patch.object(c.shutil,'copytree',side_effect=OSError('copy failed')):
                with self.assertRaises(OSError):c.publish_aoc(source,out)
            self.assertEqual((old/'300.img').read_bytes(),b'old')
            with patch.object(c,'trash_file',side_effect=lambda x:x.rename(root/'backup')):c.publish_aoc(source,out)
            self.assertEqual(list(old.iterdir()),[])

    def test_rejects_raw_nsp_and_links(self):
        with tempfile.TemporaryDirectory() as t:
            root=Path(t);source=root/'source';out=root/'out';source.mkdir();out.mkdir()
            f=source/'DLC.nsp';f.write_bytes(b'PFS0')
            with self.assertRaises(ValueError):c.publish_aoc(source,out)
            f.unlink();(source/'300.img').symlink_to(root/'missing')
            with self.assertRaises(ValueError):c.publish_aoc(source,out)

if __name__=='__main__':unittest.main()
