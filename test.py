import argparse, glob, subprocess, sys, re

parser = argparse.ArgumentParser()

parser.add_argument('--browser',
                    help='Specifies the browser executable to run the tests in.')

options = parser.parse_args(sys.argv[1:])

modes = [
 ['-O0'],
 ['-O3'],
 ['-Oz', '--closure', '1'],
]

# Uncomment for quick testing in one mode.
#modes = [['-O3', '-g2']]

tests = glob.glob('test/*.cpp')

cmd = ['em++.bat', 'lib/lib_webgpu.cpp', 'lib/lib_webgpu_cpp20.cpp', '-o', 'a.html', '-Ilib/', '--js-library', 'lib/lib_webgpu.js', '--emrun']

failures = []
passes = 0

for m in modes:
  for t in tests:
    run = cmd + m + [t]
    flags = re.findall(r"// flags: (.*)", open(t, 'r').read())
    if len(flags) > 0:
      for f in flags:
        run += f.split(' ')
    print(' '.join(run))
    try:
      subprocess.check_call(run)
      browser_cmd = ['emrun.bat']
      if options.browser: browser_cmd += ['--browser', options.browser]
      browser_cmd += ['a.html']
      print(str(browser_cmd))
      subprocess.check_call(browser_cmd)
      passes += 1
    except Exception as e:
      print(str(e))
      failures += [run]

for f in failures:
  cmd = ' '.join(f)
  print(f'FAIL: {cmd}')
print('')
print(f'{passes}/{passes+len(failures)} tests passed.')
