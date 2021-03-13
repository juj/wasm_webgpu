mergeInto(LibraryManager.library, {
  $emscripten_mini_stdio_fprintf_str: '=[0]',
  $emscripten_mini_stdio_fprintf_regex: '=/%l*(u|d)/g',
  emscripten_mini_stdio_fprintf__deps: ['$emscripten_mini_stdio_fprintf_str', '$emscripten_mini_stdio_fprintf_regex'],
  emscripten_mini_stdio_fprintf: function(outputStream, format, varArgs) {
    var ch, val, i = format;
    emscripten_mini_stdio_fprintf_str.length = 1; // Preserve first argument as a placeholder for format string.
    varArgs >>= 2; // Variadic arguments are always at minimum 4-byte aligned, so pre-shift here to save code size.
    while(HEAPU8[i]) {
      if (HEAPU8[i++] == 37 /*'%'*/) {
        while((ch = HEAPU8[i++])) {
          if (ch == 102 /*'f'*/ || (ch == 108 /*'l'*/ && HEAPU8[i] == ch)) {
            varArgs = (varArgs + 1) & -2; // align up to 8 byte boundary.
            val = HEAPU32[varArgs+1];
            emscripten_mini_stdio_fprintf_str.push((ch == 108)
              ? HEAPU32[varArgs] + (HEAPU8[i+1] == 117 /*'u'*/ ? val : val|0) * 4294967296
              : HEAPF64[varArgs >> 1]);
            varArgs += 2;
            break;
          }
          if (ch > 57 /*'9'*/) { // assume it is a 'd', 'i' or 'u' (this also prevents runaway scans)
            val = HEAPU32[varArgs++];
            emscripten_mini_stdio_fprintf_str.push(ch == 115 /*'s'*/
              ? UTF8ToString(val)
              : (ch == 117 /* 'u' */ ? val : val|0));
            break;
          }
        }
      }
    }
    // Swallow last newline since console.log() outputs full lines. Note: if one prints incomplete
    // lines, i.e. printf("something without newline"), a full line will still be printed.
    if (HEAPU8[i-1] == 10 /* \n */) --i;

    // Construct the string to print, but also replace all %u, %lld and %llu's with %d's, since console.log API doesn't
    // recognize them as a format specifier. (those were handled above)
    emscripten_mini_stdio_fprintf_str[0] = (
#if TEXTDECODER == 2
        UTF8Decoder.decode(HEAPU8.subarray(format, i))
#else
        UTF8ArrayToString(HEAPU8, format, i-format)
#endif
        ).replace(emscripten_mini_stdio_fprintf_regex, '%d');
#if MIN_CHROME_VERSION < 46 || MIN_FIREFOX_VERSION < 27 || MIN_IE_VERSION != TARGET_NOT_SUPPORTED || MIN_SAFARI_VERSION < 80000 // https://developer.mozilla.org/en-US/docs/Web/JavaScript/Reference/Operators/Spread_syntax#Spread_in_function_calls
    console[(outputStream>1?'error':'log')].apply(null, emscripten_mini_stdio_fprintf_str);
#else
    console[(outputStream>1?'error':'log')](...emscripten_mini_stdio_fprintf_str);
#endif
  },

  emscripten_mini_stdio_printf__deps: ['emscripten_mini_stdio_fprintf'],
  emscripten_mini_stdio_printf: function(format, varArgs) {
    _emscripten_mini_stdio_fprintf(1/*stdout*/, format, varArgs);
  }

});
