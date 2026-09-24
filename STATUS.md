# N.O.V.A. 2: development status

Experimental native ARM adapter for the Android v1.0.3 build. It boots on an RG34XXSP running muOS at 720x480 and reaches a level. The Xperia Play gameplay controller mapping has been added and needs device testing.

## Latest source update

- Corrected reversed gameplay D-pad Up/Down and right-stick vertical aim. Y now activates the selected special power through the native power manager, with the original availability/cooldown checks and one activation per press. ARM compilation passed; device verification remains required. Menu pointer directions are unchanged.

- Added a checked hook for the verified N.O.V.A. 2 ARM library's pixel converter. It decodes the ATC RGB (format 21) and explicit-alpha (format 22) textures present in the supplied game data into RGBA pixels, then uses the game's regular texture upload path.
- Added overflow and allocation-length handling, plus direct row copies when the destination is already RGBA.
- Kept D-pad/right-stick cursor movement, A click/drag and B Back in menus. Corrected gameplay input to use Xperia hardware scancodes, as passed by the original APK. Both right shoulders fire and both left shoulders aim, with shared-button release handling. Virtual pads now use fixed hardware coordinates and integrate right-stick movement into continuous relative aiming. Gameplay selects native control scheme 8, required by the touchpad handlers. Revised controls need device testing.

## Validation

The ARM runtime compiles, all three importer tests pass, and bounded 120- and 300-frame QEMU startup tests exit cleanly. With the current `GloftN2HP` archives, ATC texture conversions return success, including menu font textures and `splash.atc`. A framebuffer capture showed the N.O.V.A. 2 title/anti-piracy splash and main menu rendering. A synthetic center-screen touch advances the splash to the main menu and a touch on New Game opens difficulty selection. A synthetic Android A key is received by the game, but did not select New Game in the headless test.

The supplied archives were scanned for the ATC variants used in startup: formats 21 and 22 were observed; format 23 was not. This does not establish texture coverage for every level. The smoke tests use QEMU and dummy audio; they do not establish performance, audible sound, working handheld controls, or gameplay on MUOS/RG34XXSP.

The native library still prints `DATA FOLDER NOT FOUND`, but this warning is non-fatal in the tested path: archives load and the game reaches a level on the RG34XXSP. The controller change, gameplay behavior, performance and audio need further device testing. No PortMaster ZIP was produced.
