#!/usr/bin/python

import re
import sys
from subprocess import call

is_host=False

armv7_flags=["-march=armv7-a", "-mfloat-abi=softfp", "-mfpu=vfpv3-d16", "-Wl,--fix-cortex-a8"]

match_pattern=[r'.*asn1/gen_template.*\.o', r'.*compile_et.*\.o', r'.*print_version.*\.o', r'.*version_128\.o']

match_pattern=[re.compile(pattern) for pattern in match_pattern]

for i in sys.argv:
  if i == '-D_SAMBA_HOSTCC_':
    is_host=True

  for pattern in match_pattern:
    if pattern.match(i):
      is_host=True

if is_host:
  cmd=['gcc'];
  for arg in sys.argv[2:]:
    append=True
    if arg.startswith('--sysroot'):
      append=False

    for flag in armv7_flags:
      if arg == flag:
        append=False

    if append:
      cmd.append(arg)

  call(cmd, stdout=sys.stdout, stderr=sys.stderr)
else:
  call(sys.argv[1:], stdout=sys.stdout, stderr=sys.stderr)
