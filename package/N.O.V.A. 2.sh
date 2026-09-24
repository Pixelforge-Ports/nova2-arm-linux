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
  python3 "$GAMEDIR/eapx.py" install --recipe "$GAMEDIR/nova2.eapx.json" --game-dir "$GAMEDIR" --input "$GAMEDIR" --abi arm || { pm_message 'N.O.V.A. 2 data import failed. See nova2/log.txt.'; pm_finish; exit 1; }
fi
[ -x "$GAMEDIR/nova2" ] || chmod +x "$GAMEDIR/nova2"
export PORT_32BIT=Y
export LD_LIBRARY_PATH="$GAMEDIR/libs.armhf${LD_LIBRARY_PATH:+:$LD_LIBRARY_PATH}"
export SDL_GAMECONTROLLERCONFIG="$sdl_controllerconfig"
export NOVA2_RESOLUTION="${NOVA2_RESOLUTION:-auto}"
[ -f resolution.txt ] && NOVA2_RESOLUTION="$(tr -d '\r\n' < resolution.txt)"
export NOVA2_RESOLUTION
export NOVA2_FACE_LAYOUT="${NOVA2_FACE_LAYOUT:-nintendo}"
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
