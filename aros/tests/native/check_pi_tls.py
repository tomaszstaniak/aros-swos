#!/usr/bin/env python3
"""Reject native ELF TLS relocations, unsupported by the AROS Pi loader."""
import argparse
import re
import subprocess
p = argparse.ArgumentParser(description=__doc__)
p.add_argument('--readelf', required=True)
p.add_argument('binary', nargs='+')
a = p.parse_args()
failed = False
for binary in a.binary:
    output = subprocess.check_output([a.readelf, '-r', binary], text=True)
    bad = [line for line in output.splitlines() if re.search(r'R_AARCH64_(?:TLS|TLSDESC)', line)]
    print(binary + ': ' + (str(len(bad)) + ' unsupported TLS relocations' if bad else 'no native ELF TLS relocations'))
    failed |= bool(bad)
raise SystemExit(1 if failed else 0)
