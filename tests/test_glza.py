from pathlib import Path
import importlib.util, tempfile, unittest, zipfile
spec=importlib.util.spec_from_file_location('extract_sounds',Path(__file__).resolve().parents[1]/'tools/extract_sounds.py')
module=importlib.util.module_from_spec(spec);spec.loader.exec_module(module)
class TestGlza(unittest.TestCase):
    def archive(self,root,name='test.wav',compressed=False):
        path=root/'sounds.glza'
        with zipfile.ZipFile(path,'w',compression=zipfile.ZIP_DEFLATED if compressed else zipfile.ZIP_STORED) as z:
            z.writestr(name,b'RIFF'+b'audio data'*100)
        raw=bytearray(path.read_bytes());raw[:4]=b'QL\x04\x05';path.write_bytes(raw)
        return path
    def test_stored_and_deflated(self):
        for compressed in (False,True):
            with tempfile.TemporaryDirectory() as temp:
                root=Path(temp);archive=self.archive(root,compressed=compressed)
                self.assertEqual(module.extract(archive,root/'out'),1)
                self.assertEqual((root/'out/test.wav').read_bytes(),b'RIFF'+b'audio data'*100)
    def test_traversal(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);archive=self.archive(root,'../escape.wav')
            with self.assertRaises(ValueError):module.extract(archive,root/'out')
            self.assertFalse((root/'escape.wav').exists())
    def test_corrupt_crc(self):
        with tempfile.TemporaryDirectory() as temp:
            root=Path(temp);archive=self.archive(root)
            raw=bytearray(archive.read_bytes());raw[30+len('test.wav')+5]^=1;archive.write_bytes(raw)
            with self.assertRaises(ValueError):module.extract(archive,root/'out')
            self.assertFalse((root/'out/test.wav').exists())
if __name__=='__main__':unittest.main()
