import argparse, glob, subprocess, os, sys, re, tempfile, shutil

parser = argparse.ArgumentParser()

parser.add_argument('--browser',
                    help='Specifies the browser executable to run the tests in.')
parser.add_argument('tests_to_run',nargs='*')

options = parser.parse_args(sys.argv[1:])

test_dir = tempfile.TemporaryDirectory().name

if not os.path.exists(test_dir):
    os.makedirs(test_dir)

modes = [
 ['-O0', '-sASSERTIONS=1', '-jsDWEBGPU_DEBUG=1'],
# ['-O3'],
# ['-Oz', '--closure', '1', '--closure-args=--externs=lib/webgpu-closure-externs.js'],
]

# Uncomment for quick testing in one mode.
#modes = [['-O3', '-g2']]

def contains_substring(s, arr):
  for sub in arr:
    if sub in s:
      return True

tests = glob.glob('test/*.cpp')
if len(options.tests_to_run) > 0:
  tests = filter(lambda t: contains_substring(t, options.tests_to_run), tests)

output_file = os.path.join(test_dir, 'test.html')

cmd = ['em++.bat', 'lib/lib_webgpu.cpp', 'lib/lib_webgpu_cpp20.cpp', '-o', output_file, '-Ilib/', '--js-library', 'lib/lib_webgpu.js', '--emrun', '-profiling-funcs', '-Wno-experimental']

failures = []
passes = 0

for m in modes:
  for t in tests:
    print('')
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
      browser_cmd += [output_file]
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

shutil.rmtree(test_dir)
