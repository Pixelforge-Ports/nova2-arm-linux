# N.O.V.A. 2 for PortMaster

This is an experimental ARM Linux adapter for the Android **v1.0.3** release.
It does not include any game files.

## Install

1. Place `nova2.zip` in PortMaster's `autoinstall` folder and run it.
2. Copy your owned v1.0.3 APK and the complete `gameloft/games/GloftN2HP`
   data folder into `/ports/nova2/`, preserving the nested folder structure.
3. Launch **N.O.V.A. 2** from Ports. First launch validates and imports the
   files and expands the game audio. Keep the device powered on until it
   completes.

The device layout should be:

```text
/ports/nova2/
├── your-owned-nova2.apk
└── gameloft/games/GloftN2HP/
    ├── actors.gla
    ├── effects.gla
    ├── menus.gla
    ├── sprites.gla
    └── other original game data files...
```

The APK is accepted only when it contains `lib/armeabi/libnova2.so` with
SHA-256 `476c275bcd942807d8c49068b4dc6c6e666b60190845716d71b1069d80d3caaa`.

To compile and package from the source folder on Windows, open PowerShell and
run `./build.ps1` with Docker Desktop using Linux containers. The validated
PortMaster ZIP is written to `build/nova2.zip`; it contains the port runtime
but does not include the APK or game data.

## Controls

In menus, use the D-pad or right stick to move the cursor, A to click, and B
to go back. After entering a level, move the left stick to enable gameplay
controls. Start opens the pause menu and restores cursor controls.

| Control | Gameplay action |
| --- | --- |
| Left stick | Move |
| D-pad (gameplay) | Move; Up = forward, Down = backward |
| Right stick | Continuous camera / aim; Up = look up, Down = look down |
| R1 or R2 | Fire |
| L1 or L2 | Aim down sights |
| A | Throw grenade / selected item |
| B | Jump / interact (context dependent) |
| X | Reload |
| Y | Use selected special power (must be unlocked and off cooldown) |

The adapter uses the original Xperia Play scancodes and control scheme.
Face labels default to Nintendo layout; set `NOVA2_FACE_LAYOUT=xbox` to swap
A/B and X/Y. Flight and other context actions depend on the original game;
this does not add N.O.V.A. 3 abilities to N.O.V.A. 2. These revised mappings
still need handheld verification.

Resolution defaults to the detected display. To override it, create
`ports/nova2/resolution.txt` with a supported value such as `640x480`,
`720x480`, `720x720`, `1024x768`, or `1280x720`.

## Test status

This is an experimental device-test build. It boots on an RG34XXSP running
muOS at 720x480 and reaches a level. The Xperia Play gameplay mapping has been
added and needs device verification. Performance and audio still need testing.

Porter: Pixelforge Ports (Ronax). The original game is © Gameloft.
