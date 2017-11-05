#!/usr/bin/env python3

import os
import sys
import subprocess
import filecmp

def get_subdirs(dir):
    return [name for name in os.listdir(dir)
            if os.path.isdir(os.path.join(dir, name))]

def get_inputs():
    return [name for name in os.listdir(os.getcwd())
            if os.path.isfile(os.path.join(os.getcwd(), name)) and
               name.endswith('.nes')]

root_dir = os.getcwd()
bin_file = os.path.join(root_dir, 'bin', 'dndltr_d')

def setup_suite():
    pass

def run_suite():
    with open(os.devnull, 'w') as FNULL:
        for rom in get_inputs():
            path = os.path.join(os.getcwd(), rom)
            res = subprocess.call([bin_file, path, '-f', '100'], stdout=FNULL)
            if res != 0:
                print('  FAIL: Test', rom, 'failed:', res)
                return res
            if not filecmp.cmp('output.bmp', os.path.join('expected', rom.replace('.nes', '.bmp'))):
                print('  FAIL: Test', rom, 'failed: output does not match expected')
                return -1
            print('  SUCCESS: Test', rom, 'passed')
    return 0

def teardown_suite():
    if os.path.isfile('output.bmp'):
        os.remove('output.bmp')

retcode = 0
for dir in get_subdirs('tests'):
    print('SUITE:', dir)
    os.chdir(os.path.join(root_dir, 'tests', dir))
    setup_suite()
    res = run_suite()
    teardown_suite()
    os.chdir(root_dir)
    if res != 0:
        print('FAIL: Test suite', dir, 'failed:', res)
        retcode = res
        break
    print('SUCCESS: Test suite', dir, 'passed')

if retcode == 0:
    print('SUCCESS: All tests passed')
else:
    print('FAIL: Some or all tests failed')

sys.exit(retcode)
