# N.O.V.A. 2 for ARM Linux

**Playable on the tested H700 handhelds.** On RG34XXSP with muOS, gameplay
controls, graphics, music, and sound effects now work. ROCKNIX testing also
confirmed music and game audio. Other chipsets and firmware combinations have
not been verified; see [STATUS.md](STATUS.md).

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
extraction, resolution detection, and a gptokeyb2 exit mapping. The ARM runtime
compiles and the port is playable on the tested H700 devices. See
[STATUS.md](STATUS.md).

## Controls

The game starts in normal controller mode. Press Select to toggle mouse mode
for menus; use the D-pad or right stick to move the cursor, A to click, and B
to go back. Start opens or closes the pause menu without changing input mode.

| Control (default Nintendo layout) | Gameplay action |
| --- | --- |
| Left stick | Up = forward, Down = back, Left = left, Right = right |
| D-pad (gameplay) | Up = forward, Down = back, Left = left, Right = right |
| Right stick | Continuous camera / aim |
| R1 or R2 | Fire |
| Hold L1 or L2 | Activate the selected special power (when available) |
| A | Change weapon |
| B | Jump / interact (context dependent) |
| X | Reload weapon |
| Y | Throw grenade / selected item |

Face labels default to Nintendo layout; set `NOVA2_FACE_LAYOUT=xbox` to swap
A/B and X/Y. Flight and other context actions depend on the original game.

## Credits

Porter: Pixelforge Ports (Ronax).

Copyright (c) 2026 Pixelforge Ports contributors.

Uses EapRules' eapx import tool from the existing Android ports. The original game remains the property of its respective rights holders. Original component licences are retained.

The AudioTrack compatibility service is adapted from EapRules' Modern Combat 3 port.

## Website package metadata

The Pixelforge Ports website reads `package/port.json`, `package/README.md`,
and `package/screenshot.png` from the public source repository. The matching
`package/cover.png` and `package/gameinfo.xml` are kept beside them for
consistency with the other ports. Run `python tools/sync_package.py` after
changing the port metadata, guide, launcher, or artwork; packaging runs this
step automatically. The website reads these files from GitHub after they are
pushed, and shows a download when a published release has an uploaded
`nova2.zip` asset. No purchased APK or game data belongs in the repository.
