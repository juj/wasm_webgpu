// Calculates all the possible sub-strings of 's' of length 'n', and the frequency (count) that they occur.
function findSubStringsOfLengthN(n, s) {
  var subs = {};
  for(var i = 0; i + n <= s.length; ++i) {
    var sub = s.substring(i, i + n);
    if (!subs[sub]) subs[sub] = 1;
    else ++subs[sub];
  }
  return subs;
}

// Replaces all occurrences of substring 'find' in string 'str' with string 'replace'.
function replaceAll(str, find, replace) {
  let iStart = 0;
  for(;;) {
    let i = str.indexOf(find, iStart);
    if (i < 0) return str;
    str = str.slice(0, i) + replace + str.slice(i + find.length);
    iStart = i + replace.length;
  }
//  return str.replace(new RegExp(find, 'g'), replace);
}

// The runtime decoding function that the WebGPU library will use.
function wgpuDecodeStrings(s, c, ch) {
  ch = ch || 65;
  c = c.split('|');
  while(c.length) {
    s = replaceAll(s, String.fromCharCode(ch++), c.pop());
  }
  return s;
}

let DEBUG = 0;

function optimizeStringOnce(s, subCharStart, subCharEnd) {
  if (DEBUG) console.log(`${s.length} chars: '${s}'`);

  var origS = s;
  var nextSubChar = subCharStart;
  var numSubs = 0;
  var codeArray = [];
  for(;;) {
    var winnings = [];

    // Find the substring in the code that, if compressed to a single character substitution, will yield
    // the best space saving win to the string.
    for(var len = 2;; ++len) {
      var maxCount = 0;
      var subs = findSubStringsOfLengthN(len, s);
      for(var i in subs) {
        maxCount = Math.max(maxCount, subs[i]);
        var win = subs[i] * (len-1) - (len+1) // must add |oldString to the code;
        if (win > 0) {
          winnings.push({
            sub: i,
            len: len,
            count: subs[i],
            win: win
          });
        }
      }
      if (maxCount == 1)
        break;
    }

    // If we don't win any space with any substitution, we are done
    if (winnings.length == 0 || nextSubChar == subCharEnd || s.includes(String.fromCharCode(nextSubChar))) {
      if (codeArray.length == 0) return s; // If no optimization opportunity, return original string
      // Re-encode string with fixed indexes
      s = origS;
      nextSubChar = subCharStart + codeArray.length - 1;
      for(var i = 0; i < codeArray.length; ++i) {
        s = replaceAll(s, codeArray[i], String.fromCharCode(nextSubChar--));
      }
      var maybeCh = subCharStart != 65 ? `, ${subCharStart}` : '';
      return `wgpuDecodeStrings('${s}', '${codeArray.join("|")}'${maybeCh})`;
    }

    winnings.sort((a,b) => { return b.win - a.win; });

  //  for(var w of winnings) {
  //    console.log(`Substituting '${w.sub}' (len=${w.len}, count=${w.count} would win ${w.win} chars.`);
  //  }

    // Choose a random substitution
    var rng = Math.min(Math.random(), Math.random());
    var idx = Math.floor(rng * Math.min(5, winnings.length));
//    console.log(`'${winnings[0].sub}' -> '${subChars[0]}'`);
    s = replaceAll(s, winnings[idx].sub, String.fromCharCode(nextSubChar++));
    codeArray.push(winnings[idx].sub);
//    console.log(`${s.length} chars: '${s}'`);
  }
}

function searchOptimizedStringArrayRepresentation(s, numIterations, subCharStart, subCharEnd) {
  var best = s.length + 100;
  var bestStr = s;
  for(var i = 0; i < numIterations; ++i) {
    var newStr = optimizeStringOnce(s, subCharStart, subCharEnd);
    if (newStr.length < best && newStr.includes('wgpuDecodeStrings')) {
      if (DEBUG) console.log(`new best: ${newStr.length}, str: ${newStr}`);
      best = newStr.length;
      bestStr = newStr;
      var testS = eval(newStr);
      if (testS != s) {
        console.log('Got: ' + testS);
        console.log('Exp: ' + s);
        throw 'Internal error! Encoding and decoding did not match!';
      }
  //    console.log('decoded:' + testS);
    }
  }
  return bestStr;
}

const stringArrays = {
  // This array combines GPUTextureFormat and GPUVertexFormat values, since they share many of the same strings.
  '$GPUTextureAndVertexFormats': [/*undefined*/,
    // GPUTextureFormat types:

    // 8-bit formats
    "r8unorm",
    "r8snorm",
    "r8uint",
    "r8sint",

    // 16-bit formats
    "r16uint",
    "r16sint",
    "r16float",
    "rg8unorm",
    "rg8snorm",
    "rg8uint",
    "rg8sint",

    // 32-bit formats
    "r32uint",
    "r32sint",
    "r32float",
    "rg16uint",
    "rg16sint",
    "rg16float",
    "rgba8unorm",
    "rgba8unorm-srgb",
    "rgba8snorm",
    "rgba8uint",
    "rgba8sint",
    "bgra8unorm",
    "bgra8unorm-srgb",
    // Packed 32-bit formats
    "rgb9e5ufloat",
    "rgb10a2unorm",
    "rg11b10ufloat",

    // 64-bit formats
    "rg32uint",
    "rg32sint",
    "rg32float",
    "rgba16uint",
    "rgba16sint",
    "rgba16float",

    // 128-bit formats
    "rgba32uint",
    "rgba32sint",
    "rgba32float",

    // Depth/stencil formats
    "stencil8",
    "depth16unorm",
    "depth24plus",
    "depth24plus-stencil8",
    "depth32float",

    // "depth32float-stencil8" feature
    "depth32float-stencil8",

    // BC compressed formats usable if "texture-compression-bc" is both
    // supported by the device/user agent and enabled in requestDevice.
    "bc1-rgba-unorm",
    "bc1-rgba-unorm-srgb",
    "bc2-rgba-unorm",
    "bc2-rgba-unorm-srgb",
    "bc3-rgba-unorm",
    "bc3-rgba-unorm-srgb",
    "bc4-r-unorm",
    "bc4-r-snorm",
    "bc5-rg-unorm",
    "bc5-rg-snorm",
    "bc6h-rgb-ufloat",
    "bc6h-rgb-float",
    "bc7-rgba-unorm",
    "bc7-rgba-unorm-srgb",

    // ETC2 compressed formats usable if "texture-compression-etc2" is both
    // supported by the device/user agent and enabled in requestDevice.
    "etc2-rgb8unorm",
    "etc2-rgb8unorm-srgb",
    "etc2-rgb8a1unorm",
    "etc2-rgb8a1unorm-srgb",
    "etc2-rgba8unorm",
    "etc2-rgba8unorm-srgb",
    "eac-r11unorm",
    "eac-r11snorm",
    "eac-rg11unorm",
    "eac-rg11snorm",

    // ASTC compressed formats usable if "texture-compression-astc" is both
    // supported by the device/user agent and enabled in requestDevice.
    "astc-4x4-unorm",
    "astc-4x4-unorm-srgb",
    "astc-5x4-unorm",
    "astc-5x4-unorm-srgb",
    "astc-5x5-unorm",
    "astc-5x5-unorm-srgb",
    "astc-6x5-unorm",
    "astc-6x5-unorm-srgb",
    "astc-6x6-unorm",
    "astc-6x6-unorm-srgb",
    "astc-8x5-unorm",
    "astc-8x5-unorm-srgb",
    "astc-8x6-unorm",
    "astc-8x6-unorm-srgb",
    "astc-8x8-unorm",
    "astc-8x8-unorm-srgb",
    "astc-10x5-unorm",
    "astc-10x5-unorm-srgb",
    "astc-10x6-unorm",
    "astc-10x6-unorm-srgb",
    "astc-10x8-unorm",
    "astc-10x8-unorm-srgb",
    "astc-10x10-unorm",
    "astc-10x10-unorm-srgb",
    "astc-12x10-unorm",
    "astc-12x10-unorm-srgb",
    "astc-12x12-unorm",
    "astc-12x12-unorm-srgb",

    // GPUVertexFormat types
    "uint8x2",
    "uint8x4",
    "sint8x2",
    "sint8x4",
    "unorm8x2",
    "unorm8x4",
    "snorm8x2",
    "snorm8x4",
    "uint16x2",
    "uint16x4",
    "sint16x2",
    "sint16x4",
    "unorm16x2",
    "unorm16x4",
    "snorm16x2",
    "snorm16x4",
    "float16x2",
    "float16x4",
    "float32",
    "float32x2",
    "float32x3",
    "float32x4",
    "uint32",
    "uint32x2",
    "uint32x3",
    "uint32x4",
    "sint32",
    "sint32x2",
    "sint32x3",
    "sint32x4"
    ],

  'wgpu32BitLimitNames': [
    'maxTextureDimension1D',
    'maxTextureDimension2D',
    'maxTextureDimension3D',
    'maxTextureArrayLayers',
    'maxBindGroups',
    'maxDynamicUniformBuffersPerPipelineLayout',
    'maxDynamicStorageBuffersPerPipelineLayout',
    'maxSampledTexturesPerShaderStage',
    'maxSamplersPerShaderStage',
    'maxStorageBuffersPerShaderStage',
    'maxStorageTexturesPerShaderStage',
    'maxUniformBuffersPerShaderStage',
    'minUniformBufferOffsetAlignment',
    'minStorageBufferOffsetAlignment',
    'maxVertexBuffers',
    'maxVertexAttributes',
    'maxVertexBufferArrayStride',
    'maxInterStageShaderComponents',
    'maxInterStageShaderVariables',
    'maxColorAttachments',
    'maxComputeWorkgroupStorageSize',
    'maxComputeInvocationsPerWorkgroup',
    'maxComputeWorkgroupSizeX',
    'maxComputeWorkgroupSizeY',
    'maxComputeWorkgroupSizeZ'],

  'wgpuFeatures': [
    'depth-clip-control',
    'depth32float-stencil8',
    'texture-compression-bc',
    'texture-compression-etc2',
    'texture-compression-astc',
    'timestamp-query',
    'indirect-first-instance',
    'shader-f16',
    'bgra8unorm-storage',
    'rg11b10ufloat-renderable'],

  '$GPUBlendFactors': [/*undefined*/,
    'zero',
    'one',
    'src',
    'one-minus-src',
    'src-alpha',
    'one-minus-src-alpha',
    'dst',
    'one-minus-dst',
    'dst-alpha',
    'one-minus-dst-alpha',
    'src-alpha-saturated',
    'constant',
    'one-minus-constant'],

  '$GPUStencilOperations': [/*undefined*/, 'keep', 'zero', 'replace', 'invert', 'increment-clamp', 'decrement-clamp', 'increment-wrap', 'decrement-wrap'],
  '$GPUCompareFunctions': [/*undefined*/, 'never', 'less', 'equal', 'less-equal', 'greater', 'not-equal', 'greater-equal', 'always'],
  '$GPUBlendOperations': [/*undefined*/, 'add', 'subtract', 'reverse-subtract', 'min', 'max'],
  '$GPUIndexFormats': [/*undefined*/, 'uint16', 'uint32'],
  '$GPUBufferMapStates': [/*undefined*/, 'unmapped', 'pending', 'mapped'],
  '$GPUTextureDimensions': [/*undefined*/, '1d', '2d', '3d'],
  '$GPUTextureViewDimensions': [/*undefined*/, '1d', '2d', '2d-array', 'cube', 'cube-array', '3d'],
  '$GPUAddressModes': [/*undefined*/, 'clamp-to-edge', 'repeat', 'mirror-repeat'],
  '$GPUTextureAspects': [/*undefined*/, 'all', 'stencil-only', 'depth-only'],
  '$GPUPipelineStatisticNames': [/*undefined*/, 'timestamp'],
  '$GPUPrimitiveTopologys': [/*undefined*/, 'point-list', 'line-list', 'line-strip', 'triangle-list', 'triangle-strip'],
  '$GPUBufferBindingTypes': [/*undefined*/, 'uniform', 'storage', 'read-only-storage'],
  '$GPUSamplerBindingTypes': [/*undefined*/, 'filtering', 'non-filtering', 'comparison'],
  '$GPUTextureSampleTypes': [/*undefined*/, 'float', 'unfilterable-float', 'depth', 'sint', 'uint'],
  '$GPUQueryTypes': [/*undefined*/, 'occlusion'],
  '$HTMLPredefinedColorSpaces': [/*undefined*/, 'srgb', 'display-p3'],
  '$GPUFilterModes': [/*undefined*/, 'nearest', 'linear'],
  '$GPUMipmapFilterModes': [/*undefined*/, 'nearest', 'linear'],
  '$GPULoadOps': ['load', 'clear'],
  '$GPUStoreOps': ['store', 'discard'],
  '$GPUAutoLayoutMode': '="auto"',
};

const subCharRanges = {
  'wgpu32BitLimitNames': { 'start': 52 /*4*/, 'end': 65}
};

const defaultSubCharStart = 65; // uppercase 'A'
const defaultSubCharEnd = 91; // '['

console.log('////////////////////////////////////////////////////////////');
console.log('// Automatically generated with scripts/compress_strings.js:');
console.log(`
  $wgpuDecodeStrings: function(s, c, ch) {
    ch = ch || 65;
    for(c = c.split('|'); c[0];) s = s['replaceAll'](String.fromCharCode(ch++), c.pop());
    return [,].concat(s.split(' '));
  },
`);

function addNums(arr) {
  let out = [];
  for(let i = 0; i < arr.length; ++i) out.push(`${arr[i]} (${i})`);
  return out;
}

let numIterations = 10;

for(var i in stringArrays) {
  let strArray = stringArrays[i];
  let origArray = strArray.slice(0);
  if (Array.isArray(strArray)) {
    const firstElemIsUndefined = !strArray[0];
    if (firstElemIsUndefined) strArray.splice(0, 1);
    const undefinedComma = firstElemIsUndefined ? ', ' : '';

    let input = strArray.join(' ');
    let opt = searchOptimizedStringArrayRepresentation(input,
      numIterations,
      subCharRanges[i]?.start || defaultSubCharStart,
      subCharRanges[i]?.end || defaultSubCharEnd,
    );
    if (opt == input) { // no optimization?
      console.log(`  ${i}: [${undefinedComma}'${strArray.join("', '")}'],`);
    } else {
      console.log(`  ${i}__deps: ['$wgpuDecodeStrings'],`);
      console.log(`//${i}: [${addNums(origArray.map(x => `'${x}'`)).join(', ')}],`)
      if (firstElemIsUndefined) console.log(`  ${i}: "${opt}",`);
      else console.log(`  ${i}: "${opt}.slice(1)",`);
    }
  } else {
    console.log(`  ${i}: '${strArray}',`);
  }
  console.log('');
}

console.log('// End of automatically generated with scripts/compress_strings.js');
console.log('//////////////////////////////////////////////////////////////////');
