import importlib.util
from pathlib import Path
import tempfile
from types import SimpleNamespace
import unittest
import zipfile

ROOT = Path(__file__).resolve().parents[1]
spec = importlib.util.spec_from_file_location('eapx', ROOT / 'tools/eapx.py')
eapx = importlib.util.module_from_spec(spec)
spec.loader.exec_module(eapx)


class DiscoveryTests(unittest.TestCase):
    def test_launcher_discovers_apk_and_loose_data(self):
        for location in ('.', 'gamedata'):
            with self.subTest(location=location), tempfile.TemporaryDirectory() as temp:
                root = Path(temp)
                inputs = root / location
                data = inputs / 'gameloft/games/GloftN2HP'
                data.mkdir(parents=True)
                (data / 'actors.gla').write_bytes(b'test fixture')
                with zipfile.ZipFile(inputs / 'game.apk', 'w') as apk:
                    apk.writestr('lib/armeabi/libnova2.so', b'test fixture')
                recipe = SimpleNamespace(search_dirs=['gamedata', '.'],
                    marker='.eapx-nova2-data.json', log='eapx.log')
                logger = SimpleNamespace(log=lambda *args: None)
                candidates = eapx.discover(recipe, str(root), [], logger)
                try:
                    self.assertTrue(any(c.name == 'game.apk' for c in candidates))
                    self.assertTrue(any(c.archive and
                        'games/GloftN2HP/actors.gla' in c.archive.entries
                        for c in candidates))
                finally:
                    for c in candidates:
                        if c.archive and hasattr(c.archive, 'close'):
                            c.archive.close()
        for launcher in ('ports/N.O.V.A. 2.sh', 'package/N.O.V.A. 2.sh'):
            self.assertNotIn('--input "$GAMEDIR"', (ROOT / launcher).read_text())


if __name__ == '__main__':
    unittest.main()
