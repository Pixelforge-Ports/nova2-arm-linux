# N.O.V.A. 2 for PortMaster

This ARM Linux port of the Android **v1.0.3** release is playable on tested
H700 handhelds. Gameplay controls, graphics, music, and sound effects work.
It does not include any game files.

## Install

1. Place `nova2.zip` in PortMaster's `autoinstall` folder and run it.
2. Copy your owned v1.0.3 APK and the complete `gameloft/games/GloftN2HP`
   data folder into `/ports/nova2/`, preserving the nested folder structure.
3. Launch **N.O.V.A. 2** from Ports. First launch validates and imports the
   files and expands the game audio. PortMaster shows extraction progress;
   keep the device powered on until it completes.

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

## Controls

The game starts in normal controller mode with its on-screen fire control
hidden. Press Select at any time to switch between normal gameplay controls
and mouse mode. In mouse mode, use the D-pad or right stick to move the
cursor, A to click, and B to go back. Press Select again to return to normal
controls. Start opens or closes the pause menu without changing the current
input mode.

| Control (default Nintendo layout) | Gameplay action |
| --- | --- |
| Left stick | Up = forward, Down = back, Left = left, Right = right |
| D-pad (gameplay) | Up = forward, Down = back, Left = left, Right = right |
| Right stick | Continuous camera / aim; Up = look up, Down = look down |
| R1 or R2 | Fire |
| Hold L1 or L2 | Activate the selected special power (when available) |
| A | Throw grenade / selected item |
| B | Jump / interact (context dependent) |
| X | Reload weapon |
| Y | Throw grenade / selected item (observed on H700) |

Most gameplay buttons use the original Xperia Play scancodes and control
scheme; X calls the game's native weapon-reload function. Y retains its
existing grenade input.
Face labels default to Nintendo layout; set `NOVA2_FACE_LAYOUT=xbox` to swap
A/B and X/Y. Flight and other context actions depend on the original game;
this does not add N.O.V.A. 3 abilities to N.O.V.A. 2. The new X reload
function still needs a device retest.

Resolution defaults to the detected display. To override it, create
`ports/nova2/resolution.txt` with a supported value such as `640x480`,
`720x480`, `720x720`, `1024x768`, or `1280x720`.

## Test status

The port is playable on tested H700 devices. RG34XXSP testing on muOS at
720x480 confirms gameplay controls, rendering, music, and sound effects. A
ROCKNIX test also confirmed music and game audio. The new X reload action
has not yet been retested on device.

The game sends music and effects through its Vox AudioTrack service. On muOS,
the launcher declares `PORT_32BIT="Y"` in the form its port launcher recognizes
and points PipeWire, SPA, and ALSA at the firmware's 32-bit plugins. This is
needed for the system default output to reach the speaker: the previous build
generated audio but failed to load PipeWire's `support.system` plugin and fell
back to HDMI. Weston changes the display session, not the game's audio route.
The corrected route is confirmed working on muOS. If sound is missing, check
`log.txt` for the `Audio: 32-bit modules` line, `AudioTrack: opened system
default output`, and `AudioTrack: first nonzero PCM buffer`. `NOVA2_AUDIODEV`
can select an output manually, and a failed selection falls back to the
system default.

| CFW and graphics mode | Device / resolution | Status |
| --- | --- | --- |
| muOS libMali | RG34XXSP, 720x480 | Playable; controls, rendering, music, and sound effects work. |
| muOS fbdev | Any supported device | Not tested |
| muOS libMali | Any supported device | Not tested |
| ROCKNIX Wayland/Sway (GPU driver not recorded) | RG34XXSP, 720x480 | Game runs; music and game audio reported working. |
| ROCKNIX Wayland/Sway, Panfrost | Any supported device | Not tested separately |
| ROCKNIX Wayland/Sway, libMali | Any supported device | Not tested |
| dArkOS KMSDRM, current libMali | Any supported device | Not tested |
| Knulli KMSDRM, libMali | Any supported device | Not tested |
| AmberELEC, legacy libMali | Any supported device | Not tested |
| ArkOS | Any supported device | Not tested |

| Resolution | Status |
| --- | --- |
| 640x480 | Not tested |
| 720x480 | Playable on RG34XXSP with muOS; X reload awaits retest |
| 720x720 | Not tested |
| 1024x768 | Not tested |
| 1280x720 | Not tested |

| CPU target | Status |
| --- | --- |
| H700 | Playable on tested handhelds |
| rk3326, rk3566, A133P | Not tested |
| Snapdragon 865, Snapdragon 662, Amlogic S922X | Not tested |

## Notes

Thanks to Gameloft for creating N.O.V.A. 2. The original game and its assets
remain the property of their respective rights holders.

Porter: Pixelforge Ports (Ronax). The original game is © Gameloft.
