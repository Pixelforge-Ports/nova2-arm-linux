# N.O.V.A. 2 for ARM Linux

**Experimental adapter; startup reaches the rendered front end in the headless test, but handheld controls and gameplay are not yet verified.**
See [STATUS.md](STATUS.md).

Target donor: Android 1.0.3, identified by the APK and library hashes in `donor-contract.json`. A version label alone does not guarantee matching native interfaces.

## Data preparation

Use your own APK and complete data tree. Preserve the original `gameloft/games/` or `Gameloft/games/` directory spelling where applicable. Place the donor files under `gamedata/` in a separate development game directory. The importer never supplies commercial files.

On Linux, check the import recipe with:

```sh
python3 tools/eapx.py check --recipe package/nova2/nova2.eapx.json
python3 tools/eapx.py plan --recipe package/nova2/nova2.eapx.json --game-dir build/data --input /path/to/NOVA2.apk --input /path/to/owned-game-folder
```

After reviewing the plan, replace `plan` with `install` to stage and verify the data.

To compile the ARM runtime and create the game-data-free PortMaster ZIP on
Windows, install Docker Desktop with Linux containers enabled, open PowerShell
in this source folder, and run:

```powershell
./build.ps1
```

The script builds the ARM runtime and support libraries, assembles and validates
the PortMaster package, and writes `build\nova2.zip`. The ZIP contains no APK
or game data. Copy this ZIP to your handheld; do not copy the source directory.

After installing the ZIP through PortMaster, copy your exact owned Android
v1.0.3 APK and the complete `gameloft\games\GloftN2HP` folder to the device's
`/ports/nova2/` directory. Preserve this layout:

```text
/ports/nova2/
├── your-owned-nova2.apk
└── gameloft/
    └── games/
        └── GloftN2HP/
            ├── actors.gla
            ├── effects.gla
            ├── menus.gla
            ├── sprites.gla
            └── other original game data files...
```

The APK filename can vary, but it must be the supported v1.0.3 APK containing
`lib/armeabi/libnova2.so` with the SHA-256 listed in `package/nova2/README.md`.
The first launch validates and imports the files. No APK or commercial game data
is included in the PortMaster ZIP.

## Implementation

`game/native_bindings.h` records exact JNI argument types, return types, static/instance receivers and ARM soft-float calling conventions. `donor-contract.json` distinguishes exported entry points from methods needing dynamic registration or further investigation. Do not call every declaration indiscriminately.

The adapter supplies Android Java services, controller input, audio
extraction, resolution detection and a gptokeyb2 exit mapping. The ARM runtime
compiles, importer tests pass, and the headless startup test renders the title
screen and front end. A controller A press did not select the menu item in the
headless test. Handheld controls, level startup, gameplay and audible playback
still need device testing. See STATUS.md.

## Credits

Porter: Pixelforge Ports (Ronax).

Copyright (c) 2026 Pixelforge Ports contributors.

Uses EapRules' eapx import tool from the existing Android ports. The original game remains the property of its respective rights holders. Original component licences are retained.

The AudioTrack compatibility service is adapted from EapRules' Modern Combat 3 port.
