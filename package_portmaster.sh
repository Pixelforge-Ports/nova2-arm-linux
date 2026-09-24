#!/usr/bin/env bash
set -euo pipefail
cd "$(dirname "$0")"
OUT="build/nova2.zip"
STAGE="build/pkg-portmaster"
[ -x build/nova2 ] || { echo "build/nova2 missing; run make first" >&2; exit 1; }
[ -f build/libs.armhf/MANIFEST.txt ] || { echo "run make libs first" >&2; exit 1; }
rm -rf "$STAGE"
rm -f "$OUT"
mkdir -p "$STAGE/nova2/licenses/libraries"
cp "ports/N.O.V.A. 2.sh" "$STAGE/"
cp build/nova2 "$STAGE/nova2/"
cp ports/nova2/nova2.ini ports/nova2/port.json ports/nova2/gameinfo.xml ports/nova2/README.md \
   ports/nova2/CREDITS.md ports/nova2/PUT_NOVA2_DATA_HERE.txt "$STAGE/nova2/"
cp package/nova2/nova2.eapx.json package/nova2/extract_sounds.py "$STAGE/nova2/"
cp tools/eapx.py "$STAGE/nova2/eapx.py"
cp -R build/libs.armhf "$STAGE/nova2/"
cp LICENSE "$STAGE/nova2/licenses/LICENSE-portmaster-port.txt"
cp package/nova2/licenses/LICENSE-eapx.txt "$STAGE/nova2/licenses/"
cp package/nova2/licenses/LICENSE-gptokeyb2.txt "$STAGE/nova2/licenses/"
cp package/nova2/licenses/LICENSE-gmloader.md "$STAGE/nova2/licenses/"
cp package/nova2/licenses/LICENSE-powervr.txt "$STAGE/nova2/licenses/"
cp package/nova2/licenses/LICENSE-stb.md "$STAGE/nova2/licenses/"
cp package/nova2/licenses/NOTICE.md "$STAGE/nova2/licenses/"
mv "$STAGE/nova2/libs.armhf/licenses/"* "$STAGE/nova2/licenses/libraries/"
rmdir "$STAGE/nova2/libs.armhf/licenses"
chmod +x "$STAGE/N.O.V.A. 2.sh" "$STAGE/nova2/nova2" "$STAGE/nova2/eapx.py" "$STAGE/nova2/extract_sounds.py"
(cd "$STAGE" && zip -qr "../../$OUT" .)
unzip -tq "$OUT" >/dev/null
[ "$(unzip -p "$OUT" 'N.O.V.A. 2.sh' | sed -n '2p')" = '# PORTMASTER: nova2.zip, N.O.V.A. 2.sh' ] || { echo 'launcher signature mismatch' >&2; exit 1; }
listing="$(unzip -Z1 "$OUT")"
for required in 'N.O.V.A. 2.sh' nova2/nova2 nova2/port.json nova2/nova2.eapx.json nova2/eapx.py nova2/libs.armhf/MANIFEST.txt nova2/licenses/LICENSE-gptokeyb2.txt; do
  case "$listing" in *"$required"*) ;; *) echo "package missing $required" >&2; exit 1;; esac
done
case "$listing" in *'.apk'*|*'GloftN2HP'*|*'libnova2.so'*) echo 'refusing package containing game data' >&2; exit 1;; esac
echo "$OUT"
