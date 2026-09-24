"""Regenerate file-resource IDs from the owned APK; not needed for normal builds."""
import argparse, re, subprocess
from pathlib import Path
parser=argparse.ArgumentParser()
parser.add_argument('apk')
args=parser.parse_args()
text=subprocess.check_output(['aapt','dump','--values','resources',args.apk],text=True)
mapping={}
resource=None
for line in text.splitlines():
    found=re.match(r'\s+resource (0x[0-9a-f]+) .*',line)
    if found:resource=found[1]
    found=re.search(r'\(string8\) "(res/[^"\n]+)"',line)
    if found and resource:mapping.setdefault(resource,found[1])
if not mapping:raise SystemExit('No file resources found')
lines=['// Resource identifiers and paths from the supported APK.','#include <cstddef>',
       'const char *resource_name(unsigned id) {','    switch(id) {']
for ident,path in sorted(mapping.items()):lines.append(f'        case {ident}: return "{path}";')
lines+=['        default:return nullptr;','    }','}','']
(Path(__file__).resolve().parents[1]/'game/resources.cpp').write_text('\n'.join(lines),encoding='utf-8')
print('Mapped',len(mapping),'file resources')
