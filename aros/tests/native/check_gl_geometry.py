#!/usr/bin/env python3
"""Reject native GL renderer creation that loses the game's fitted client area."""
import argparse
from pathlib import Path
import re
p=argparse.ArgumentParser(description=__doc__)
p.add_argument('log',type=Path)
a=p.parse_args();text=a.log.read_text()
assert 'result=0 error=' in text,'probe did not finish successfully'
phases={}
for line in text.splitlines():
 m=re.match(r'(created|post-fit|renderer-created|first-present|finished) .*?window=(\d+)x(\d+).*?native=(\S+)',line)
 if m: phases[m[1]]=(int(m[2]),int(m[3]),m[4])
assert set(phases)=={'created','post-fit','renderer-created','first-present','finished'},'missing geometry phases'
expected=phases['post-fit'][:2]
assert phases['first-present'][:2]==expected, f"client changed when enabling GL: {expected} -> {phases['first-present'][:2]}"
assert phases['finished'][:2]==expected,'final geometry differs from fitted client'
assert phases['post-fit'][2]==phases['renderer-created'][2],'renderer recreated the native window'
assert 'backend_full=0 mon=(none) custom=0000000000000000' in text,'unexpected fullscreen/custom screen'
print('PASS: fitted client and native window survive renderer creation:',expected)
