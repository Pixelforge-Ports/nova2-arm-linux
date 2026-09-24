#!/bin/bash
# PORTMASTER: nova2.zip, N.O.V.A. 2.sh
XDG_DATA_HOME=${XDG_DATA_HOME:-$HOME/.local/share}
if [ -d /PortMaster ]; then controlfolder=/PortMaster
elif [ -d /opt/system/Tools/PortMaster ]; then controlfolder=/opt/system/Tools/PortMaster
elif [ -d /opt/tools/PortMaster ]; then controlfolder=/opt/tools/PortMaster
elif [ -d "$XDG_DATA_HOME/PortMaster" ]; then controlfolder="$XDG_DATA_HOME/PortMaster"
else controlfolder=/roms/ports/PortMaster; fi
source "$controlfolder/control.txt"
[ -f "$controlfolder/mod_${CFW_NAME}.txt" ] && source "$controlfolder/mod_${CFW_NAME}.txt"
get_controls
GAMEDIR="/${directory#/}/ports/nova2"
scriptdir="$(cd "$(dirname "$0")" && pwd)"
[ -d "$scriptdir/nova2" ] && GAMEDIR="$scriptdir/nova2"
cd "$GAMEDIR" || exit 1
mkdir -p saves
exec > "$GAMEDIR/log.txt" 2>&1
if [ ! -f .eapx-nova2-data.json ]; then
  command -v python3 >/dev/null || { pm_message 'N.O.V.A. 2 needs Python 3 for first-launch data import.'; pm_finish; exit 1; }
  python3 "$GAMEDIR/eapx.py" install --recipe "$GAMEDIR/nova2.eapx.json" --game-dir "$GAMEDIR" --abi arm || { pm_message 'N.O.V.A. 2 data import failed. See nova2/log.txt.'; pm_finish; exit 1; }
fi
[ -x "$GAMEDIR/nova2" ] || chmod +x "$GAMEDIR/nova2"
export PORT_32BIT=Y
export LD_LIBRARY_PATH="$GAMEDIR/libs.armhf${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
echo "SDL: configured video=${SDL_VIDEODRIVER:-default} EGL=${SDL_VIDEO_EGL_DRIVER:-default} GL=${SDL_VIDEO_GL_DRIVER:-default}"
SDL_INFO=$("$GAMEDIR/nova2" --sdl-info 2>&1)
printf '%s\n' "$SDL_INFO"
if printf '%s\n' "$SDL_INFO" | grep -qx 'sdl: video driver: mali'; then
  export SDL_VIDEODRIVER=mali
  echo 'SDL: selecting available mali video driver'
fi
GL_DIRS="/usr/local/lib/arm-linux-gnueabihf /usr/lib/arm-linux-gnueabihf /usr/lib/arm-linux-gnueabihf/mali /lib/arm-linux-gnueabihf /usr/lib32/mali /usr/lib32 /usr/lib /lib"
GL_SHIM=/tmp/nova2-gl
MALI_BLOB=""
GL_PROVIDER_FOUND=0
for GL_DIR in $GL_DIRS; do
  [ -d "$GL_DIR" ] || continue
  for GL_CANDIDATE in "$GL_DIR"/libmali-*.so "$GL_DIR"/libmali.so* "$GL_DIR"/libMali.so*; do
    [ -e "$GL_CANDIDATE" ] || continue
    MALI_BLOB="$GL_CANDIDATE"
    break 2
  done
done
if [ -n "$MALI_BLOB" ] && mkdir -p "$GL_SHIM"; then
  for GL_SONAME in libEGL.so libEGL.so.1 libGLESv1_CM.so.1 libGLESv2.so libGLESv2.so.2 libmali.so.1; do
    ln -sf "$MALI_BLOB" "$GL_SHIM/$GL_SONAME"
  done
  export LD_LIBRARY_PATH="$GL_SHIM:${MALI_BLOB%/*}:$LD_LIBRARY_PATH"
  echo "GL: using device Mali library $MALI_BLOB"
  GL_PROVIDER_FOUND=1
else
  for GL_DIR in $GL_DIRS; do
    [ -d "$GL_DIR" ] || continue
    GL_EGL=""
    GL_GLES=""
    for GL_CANDIDATE in "$GL_DIR"/libEGL.so "$GL_DIR"/libEGL.so.1; do
      [ -e "$GL_CANDIDATE" ] && { GL_EGL="$GL_CANDIDATE"; break; }
    done
    for GL_CANDIDATE in "$GL_DIR"/libGLESv2.so "$GL_DIR"/libGLESv2.so.2; do
      [ -e "$GL_CANDIDATE" ] && { GL_GLES="$GL_CANDIDATE"; break; }
    done
    if [ -n "$GL_EGL" ] && [ -n "$GL_GLES" ]; then
      export SDL_VIDEO_EGL_DRIVER="$GL_EGL"
      export SDL_VIDEO_GL_DRIVER="$GL_GLES"
      export LD_LIBRARY_PATH="$GL_DIR:$LD_LIBRARY_PATH"
      echo "GL: using device EGL/GLES libraries from $GL_DIR"
      GL_PROVIDER_FOUND=1
      break
    fi
  done
fi
[ "$GL_PROVIDER_FOUND" -eq 1 ] || echo "GL: no EGL/GLES provider found in: $GL_DIRS"
export NOVA2_RESOLUTION="${NOVA2_RESOLUTION:-auto}"
[ -f resolution.txt ] && NOVA2_RESOLUTION="$(tr -d '\r\n' < resolution.txt)"
export NOVA2_RESOLUTION
export HOME="$GAMEDIR/saves"
mapper_pid=""
if [ -n "$GPTOKEYB2" ]; then
  $GPTOKEYB2 nova2 -c "$GAMEDIR/nova2.ini" >/dev/null &
  mapper_pid=$!
fi
command -v pm_platform_helper >/dev/null 2>&1 && pm_platform_helper "$GAMEDIR/nova2"
read -r -a taskset_command <<< "${TASKSET:-}"
"${taskset_command[@]}" "$GAMEDIR/nova2" "$GAMEDIR/donor"
status=$?
[ -n "$mapper_pid" ] && kill "$mapper_pid" 2>/dev/null
[ -n "$mapper_pid" ] && wait "$mapper_pid" 2>/dev/null
pm_finish
exit "$status"
