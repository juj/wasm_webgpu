import argparse, glob, subprocess, os, sys, re, tempfile, shutil, time

parser = argparse.ArgumentParser()

parser.add_argument('--browser',
                    help='Specifies the browser executable to run the tests in.')
parser.add_argument('--bigint', action='store_true',
                    help='If true, runs test suite in Emscripten -sWASM_BIGINT mode.')
parser.add_argument('--wasm4gb', action='store_true',
                    help='If true, runs test suite in 4GB Wasm mode.')
parser.add_argument('--wasm64', action='store_true',
                    help='If true, runs test suite in 64-bit Wasm mode. This implies -sWASM_BIGINT mode.')
parser.add_argument('--sanitize', action='store_true',
                    help='If true, runs with LLVM/Clang sanitizers enabled.')
parser.add_argument('--std_cpp11', action='store_true',
                    help='If true, runs test suite in -std=c++11 mode.')
parser.add_argument('tests_to_run',nargs='*')

options = parser.parse_args(sys.argv[1:])

test_dir = os.path.join(os.path.dirname(os.path.abspath(__file__)), 'out')

if not os.path.exists(test_dir):
    os.makedirs(test_dir)

modes = [
 ['-O0', '-sASSERTIONS=1', '-jsDWEBGPU_DEBUG=1'],
 ['-O3'],
 ['-Oz', '--closure', '1', '--closure-args=--externs=lib/webgpu-closure-externs.js'],
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

cmd = ['em++.bat', 'lib/lib_webgpu.cpp', 'lib/lib_webgpu_cpp11.cpp' if options.std_cpp11 else 'lib/lib_webgpu_cpp20.cpp',
       '-o', output_file, '-Ilib/', '--js-library', 'lib/lib_webgpu.js', '--emrun', '-profiling-funcs', '-Wno-experimental']

if options.wasm64 and options.wasm4gb:
  raise Exception('Testing --wasm64 and --wasm4gb are mutually exclusive!')

if options.sanitize:
  cmd += ['-fsanitize=address', '-fsanitize=undefined']
if options.wasm64:
  cmd += ['-sMEMORY64']
  if not options.sanitize:
    cmd += ['-sINITIAL_MEMORY=4300MB', '-sGLOBAL_BASE=4GB']
elif options.wasm4gb:
  if not options.sanitize:
    cmd += ['-sINITIAL_MEMORY=2300MB', '-sGLOBAL_BASE=2GB']
elif options.bigint:
  cmd += ['-sWASM_BIGINT']

failures = []
passes = 0

def kill_firefox():
  try:
    subprocess.check_call(['taskkill', '/f', '/t', '/im', 'firefox.exe'], timeout=15, stdout=subprocess.PIPE, stderr=subprocess.PIPE)
  except:
    pass

def num_firefox_processes():
  procs = subprocess.check_output(["tasklist","/FI","IMAGENAME eq firefox.exe", '/fo', 'csv']).decode("ascii","ignore")
  return len(procs.splitlines()[1:])

def wait_firefox_quit():
  for i in range(10):
    if num_firefox_processes() == 0:
      return
    time.sleep(0.3)
  raise TimeoutError("Firefox took too long to quit.")

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
      browser_cmd = ['emrun.bat', '--safe-firefox-profile']
      if options.browser: browser_cmd += ['--browser', options.browser]
      browser_cmd += [output_file]
      print(str(browser_cmd))
      subprocess.check_call(browser_cmd, timeout=15)
# Test code logic for interactive test debugging
#      wait_firefox_quit()

      passes += 1
#      shutil.move(t, os.path.join('ok', t))

    except Exception as e:
#      kill_firefox()
      print(str(e))
      failures += [run]

for f in failures:
  cmd = ' '.join(f)
  print(f'FAIL: {cmd}')
print('')
print(f'{passes}/{passes+len(failures)} tests passed.')

