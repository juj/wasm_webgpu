import sys

sys.path.append('ply')

import WebIDL
import re

p = WebIDL.Parser()
def load_and_fixup_idl(filename):
  src = open(filename, 'r').read()
  src = src.replace('readonly attribute FrozenArray<GPUCompilationMessage> messages;', '')
  src = re.sub(r'\[..*?\]', '', src)
  src = src.replace('float lodMaxClamp = 0xffffffff;', '')
  src = src.replace('''[
    Exposed=(Window, DedicatedWorker)
]''', '')
  src = src.replace('record<DOMString, GPUSize32> nonGuaranteedLimits = {};', '')
  print(src)
  return src

p.parse(load_and_fixup_idl('common.idl'), filename='common.idl')
p.parse('''interface EventTarget {}; interface EventHandler {}; interface Navigator {}; interface WorkerNavigator {};
''' + load_and_fixup_idl('webgpu.idl'), filename='webgpu.idl')
data = p.finish();

enums = {}

for d in data:
  if isinstance(d, WebIDL.IDLIncludesStatement):
    continue

  if isinstance(d, WebIDL.IDLEnum):
    enums[d.identifier.name] = [];
    for v in d.values():
      enums[d.identifier.name] += [str(v)]

# Generate headers
webgpu_strings_h = '''#pragma once

''';

wgpu_strings = [0]

def enum_name_to_cpp_identifier(enum_name, enum_value):
  val = 'WGPU'+re.sub(r'([A-Z])', '_\\1', enum_name.replace('GPU', '')).upper()
  if enum_value: val += '_' + enum_value.upper().replace('-', '_');
  return val

for key, values in enums.items():
  webgpu_strings_h += 'typedef int %s;\n' % enum_name_to_cpp_identifier(key, '')
  webgpu_strings_h += '#define %s 0\n' % enum_name_to_cpp_identifier(key, 'INVALID')

  for v in values:
    try:
      existing_index = wgpu_strings.index(v)
    except:
      existing_index = len(wgpu_strings)
      wgpu_strings += [v]
    webgpu_strings_h += '#define %s %d\n' % (enum_name_to_cpp_identifier(key, v), existing_index)

  webgpu_strings_h += '\n'

open('../lib/lib_webgpu_strings.h', 'w').write(webgpu_strings_h)

wgpu_strings = ["'%s'" % x for x in wgpu_strings]
wgpu_strings[0] = ''
wgpu_strings_string = ','.join(wgpu_strings)

js_lib = open('../lib/lib_webgpu.js', 'r').read()

new_strings = 'wgpuStrings: [%s]' % wgpu_strings_string
js_lib = re.sub(r'wgpuStrings: \[,.*?\]', new_strings, js_lib)
assert(new_strings in js_lib)

new_comment = '// Global constant string table for all WebGPU strings. Contains %d entries, using %d bytes.' % (len(wgpu_strings), len(wgpu_strings_string))
js_lib = re.sub(r'// Global constant string table for all WebGPU strings.*', new_comment, js_lib)
assert(new_comment in js_lib)

open('../lib/lib_webgpu.js', 'w').write(js_lib)
