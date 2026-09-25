# N.O.V.A. 2: development status

Experimental native ARM adapter for the Android v1.0.3 build. It boots on an RG34XXSP running muOS at 720x480 and reaches a level. The Xperia Play gameplay controller mapping has been added and needs device testing.

## PortMaster packaging update

- First-launch import now opens PortMaster's dialog before eapx starts, allowing the importer to report extraction progress through the PortMaster progress bar.
- Added the supplied in-game screenshot at 640x480 and a matching cover image. Both assets are included in the package with an artwork notice.
- Kept all component license files in one flat folder and made the package check reject device-provided system libraries. The library collector leaves SDL2, graphics drivers, glibc, libstdc++, and libgcc to the CFW.
- Hardware testing is still required for this source revision. The Docker daemon was inaccessible in this session, so no updated PortMaster ZIP was built.

## Latest source update

- RG DS startup: import succeeded but EGL display creation failed after the launcher forced a raw Mali blob under Wayland. Compositor sessions now retain the firmware backend and select a 32-bit firmware EGL/GLES pair ahead of bundled libraries. Both launcher copies are synchronized. Three graphics-selection tests pass; RG DS device verification remains pending.

- Corrected reversed gameplay D-pad Up/Down and right-stick vertical aim. Y now activates the selected special power through the native power manager, with the original availability/cooldown checks and one activation per press. ARM compilation passed; device verification remains required. Menu pointer directions are unchanged.

- Added a checked hook for the verified N.O.V.A. 2 ARM library's pixel converter. It decodes the ATC RGB (format 21) and explicit-alpha (format 22) textures present in the supplied game data into RGBA pixels, then uses the game's regular texture upload path.
- Added overflow and allocation-length handling, plus direct row copies when the destination is already RGBA.
- Kept D-pad/right-stick cursor movement, A click/drag and B Back in menus. Corrected gameplay input to use Xperia hardware scancodes, as passed by the original APK. Both right shoulders fire and both left shoulders aim, with shared-button release handling. Virtual pads now use fixed hardware coordinates and integrate right-stick movement into continuous relative aiming. Gameplay selects native control scheme 8, required by the touchpad handlers. Revised controls need device testing.

## Validation

The ARM runtime compiles, all three importer tests pass, and bounded 120- and 300-frame QEMU startup tests exit cleanly. With the current `GloftN2HP` archives, ATC texture conversions return success, including menu font textures and `splash.atc`. A framebuffer capture showed the N.O.V.A. 2 title/anti-piracy splash and main menu rendering. A synthetic center-screen touch advances the splash to the main menu and a touch on New Game opens difficulty selection. A synthetic Android A key is received by the game, but did not select New Game in the headless test.

The supplied archives were scanned for the ATC variants used in startup: formats 21 and 22 were observed; format 23 was not. This does not establish texture coverage for every level. The smoke tests use QEMU and dummy audio; they do not establish performance, audible sound, working handheld controls, or gameplay on MUOS/RG34XXSP.

The native library still prints `DATA FOLDER NOT FOUND`, but this warning is non-fatal in the tested path: archives load and the game reaches a level on the RG34XXSP. The controller change, gameplay behavior, performance and audio need further device testing. No PortMaster ZIP was produced.
