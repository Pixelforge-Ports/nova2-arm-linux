"""Refresh the website-facing package files without copying commercial data."""

from pathlib import Path
import shutil


ROOT = Path(__file__).resolve().parents[1]
PORT = ROOT / "ports" / "nova2"
PACKAGE = ROOT / "package"


def sync() -> None:
    PACKAGE.mkdir(exist_ok=True)
    (PACKAGE / "nova2").mkdir(exist_ok=True)

    for name in ("port.json", "gameinfo.xml", "cover.png", "screenshot.png"):
        shutil.copyfile(PORT / name, PACKAGE / name)

    shutil.copyfile(PACKAGE / "nova2" / "README.md", PACKAGE / "README.md")
    shutil.copyfile(PORT / "port.json", PACKAGE / "nova2" / "port.json")
    shutil.copyfile(PORT / "gameinfo.xml", PACKAGE / "nova2" / "gameinfo.xml")
    shutil.copyfile(ROOT / "ports" / "N.O.V.A. 2.sh", PACKAGE / "N.O.V.A. 2.sh")


if __name__ == "__main__":
    sync()
    print("Updated package/ website metadata and artwork.")
