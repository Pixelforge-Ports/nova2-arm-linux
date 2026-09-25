"""Exercise graphics selection without launching the game or requiring a GPU."""
import os
from pathlib import Path
import subprocess
import tempfile
import unittest

ROOT = Path(__file__).resolve().parents[1]


class GraphicsSelection(unittest.TestCase):
    def select(self, compositor, elf_class=1):
        with tempfile.TemporaryDirectory() as directory:
            root = Path(directory)
            for name in ('libEGL.so.1', 'libGLESv2.so.2', 'libmali-unused.so'):
                (root / name).write_bytes(b'\x7fELF' + bytes([elf_class]) + b'\0' * 16)
            source = (ROOT / 'ports/N.O.V.A. 2.sh').read_text()
            block = source[source.index('COMPOSITOR=0'):source.index('export NOVA2_RESOLUTION=')]
            block = '\n'.join(
                f'GL_DIRS="{root}"' if line.startswith('GL_DIRS=') else
                f'GL_SHIM="{root}/shim"' if line.startswith('GL_SHIM=') else line
                for line in block.splitlines())
            env = dict(os.environ, SDL_VIDEODRIVER='wayland' if compositor else '',
                       WAYLAND_DISPLAY='', DISPLAY='', SDL_VIDEO_EGL_DRIVER='',
                       SDL_VIDEO_GL_DRIVER='', LD_LIBRARY_PATH='',
                       SDL_INFO='sdl: video driver: mali')
            result = subprocess.run(['bash', '-c', block +
                '\nprintf "SELECTED=%s,%s\\n" "$SDL_VIDEODRIVER" "$GL_PROVIDER_FOUND"'],
                env=env, text=True, capture_output=True, check=True)
            return result.stdout, (root / 'shim/libEGL.so.1').is_symlink()

    def test_wayland_preserves_backend_and_uses_firmware_pair(self):
        output, shim = self.select(True)
        self.assertIn('compositor firmware EGL=', output)
        self.assertIn('SELECTED=wayland,1', output)
        self.assertFalse(shim)

    def test_wayland_rejects_64_bit_pair_without_raw_mali_fallback(self):
        output, shim = self.select(True, 2)
        self.assertIn('SELECTED=wayland,0', output)
        self.assertFalse(shim)

    def test_direct_mali_path_preserved(self):
        output, shim = self.select(False)
        self.assertIn('SELECTED=mali,1', output)
        self.assertTrue(shim)


if __name__ == '__main__':
    unittest.main()
