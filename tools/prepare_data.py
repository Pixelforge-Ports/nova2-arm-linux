"""Plan or import the owned APK and data folder with eapx."""
from pathlib import Path
import argparse, hashlib, json, subprocess, sys
ROOT=Path(__file__).resolve().parents[1]
def main():
    parser=argparse.ArgumentParser()
    parser.add_argument('--source',type=Path,required=True)
    parser.add_argument('--destination',type=Path,default=ROOT/'build/data')
    parser.add_argument('--plan',action='store_true')
    args=parser.parse_args()
    contract=json.loads((ROOT/'donor-contract.json').read_text(encoding='utf-8'))
    candidates=[]
    for apk in args.source.rglob('*.apk'):
        digest=hashlib.sha256()
        with apk.open('rb') as stream:
            for block in iter(lambda:stream.read(1024*1024),b''):digest.update(block)
        sha=digest.hexdigest()
        if sha==contract['apk_sha256']:candidates.append(apk)
    if len(candidates)!=1:parser.error('Expected exactly one APK matching donor-contract.json')
    args.destination.mkdir(parents=True,exist_ok=True)
    recipe=ROOT/'package/nova2/nova2.eapx.json'
    command=[sys.executable,str(ROOT/'tools/eapx.py'),'plan' if args.plan else 'install',
        '--recipe',str(recipe),'--game-dir',str(args.destination),
        '--input',str(candidates[0]),'--input',str(args.source)]
    if not args.plan:command+=['--no-adopt']
    return subprocess.call(command)
if __name__=='__main__':raise SystemExit(main())
