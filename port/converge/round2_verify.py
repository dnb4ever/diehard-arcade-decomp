#!/usr/bin/env python3
"""Round-2 schedules with the same explicit SMPC clock input as the stock gate."""
from pathlib import Path
import hashlib
import os
import subprocess
import tempfile
import verify
verify.WORK=Path('work/converge/round2')

def clock_input():
    # The original gate ran on Sunday, November 8. The cart stores the host
    # weekday at 06098803. Pin the input; never mask that RAM byte in comparison.
    source=Path(__file__).resolve().with_name('clock_input.c')
    digest=hashlib.sha256(source.read_bytes()).hexdigest()
    library=(verify.WORK/f'clock_input-{digest[:16]}.so').resolve()
    if not library.exists():
        # Publish atomically; never truncate a library mapped by another replay.
        with tempfile.TemporaryDirectory(dir=verify.WORK) as tmp:
            staging=Path(tmp)/'clock.so'
            subprocess.run(['cc','-shared','-fPIC',str(source),'-o',str(staging)],check=True)
            os.replace(staging,library)
    os.environ['TZ']='America/New_York'
    os.environ['LD_PRELOAD']=str(library)

if __name__=='__main__':
    clock_input()
    verify.main()
