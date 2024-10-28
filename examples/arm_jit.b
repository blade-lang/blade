/**
 * ARM64 JIT compilation example.
 */


# -------------- STANDARD IMPLEMENTATION BEGINS ----------------------------

def square2(x) {
    var y = 0
    for i in 0..1000000 {
       y += x
    }
    return y
}

var start = microtime()
echo square2(9)
echo 'Default: ${(microtime() - start)} microseconds'

# -------------- STANDARD IMPLEMENTATION ENDS ------------------------------

# ------------------- JIT IMPLEMENTATION BEGINS ----------------------------

import process
import clib
import convert

var code_address = process.PagedValue(true, true)

# long square(long num) {
#     long y = 0;
#     for(int i = 0; i < 1000000; i++) {
#         y += num;
#     }
#     return y;
# }
# gcc -g -O3 arm64
# square:
#     mov     x1, #0x4240
#     movk    x1, #0xf, lsl #16
#     mul     x0, x0, x1
#     ret
var data = convert.hex_to_bytes(hex(0xd2884801))
data.extend(convert.hex_to_bytes(hex(0xf2a001e1)))
data.extend(convert.hex_to_bytes(hex(0x9b017c00)))
data.extend(convert.hex_to_bytes(hex(0xd65f03c0)))

code_address.set(data)

var square3 = clib.function_handle(code_address.raw_pointer(), clib.long, clib.long)

start = microtime()
echo square3(9)
echo 'JITed: ${(microtime() - start)} microseconds'

# ------------------- JIT IMPLEMENTATION ENDS ------------------------------

