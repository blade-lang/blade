import os
import io

var source = os.args.get(2) or 'ir.sh'

if !file(source).exists() {
  io.stderr.write('IR file (${source}) not found.\n')
  echo 'Usage: ir2c.b [ir_file]'
  os.exit(1)
}

var rex = "/^(\d{8})\s+(\d+|\|)\s([a-z]+)(?:\s+(\d+)|\s+(?:(\d+)\s\'([^\']*)\')|\s+\(([^)]+)\)\s+(\d+)\s\'([^\']*)\'|\s+(\d+)\s\->\s(\d+))?$/m"

var op_map = {
  'fjump': 'OP_JUMP_IF_FALSE',
  'jump': 'OP_JUMP',
  'itry': 'OP_TRY',
  'loop': 'OP_LOOP',

  'dglob': 'OP_DEFINE_GLOBAL',
  'gglob': 'OP_GET_GLOBAL',
  'sglob': 'OP_SET_GLOBAL',

  'gloc': 'OP_GET_LOCAL',
  'sloc': 'OP_SET_LOCAL',

  'gprop': 'OP_GET_PROPERTY',
  'gprops': 'OP_GET_SELF_PROPERTY',
  'sprop': 'OP_SET_PROPERTY',

  'gupv': 'OP_GET_UP_VALUE',
  'supv': 'OP_SET_UP_VALUE',

  'ptry': 'OP_POP_TRY',
  'pubtry': 'OP_PUBLISH_TRY',

  'load': 'OP_CONSTANT',

  'eq': 'OP_EQUAL',

  'gt': 'OP_GREATER',
  'less': 'OP_LESS',
  'em': 'OP_EMPTY',
  'nil': 'OP_NIL',
  'true': 'OP_TRUE',
  'false': 'OP_FALSE',
  'add': 'OP_ADD',
  'sub': 'OP_SUBTRACT',
  'mul': 'OP_MULTIPLY',
  'div': 'OP_DIVIDE',
  'fdiv': 'OP_F_DIVIDE',
  'rmod': 'OP_REMINDER',
  'pow': 'OP_POW',
  'neg': 'OP_NEGATE',
  'not': 'OP_NOT',
  'bnot': 'OP_BIT_NOT',
  'band': 'OP_AND',
  'bor': 'OP_OR',
  'bxor': 'OP_XOR',
  'lshift': 'OP_LSHIFT',
  'rshift': 'OP_RSHIFT',
  'one': 'OP_ONE',

  'cimport': 'OP_CALL_IMPORT',
  'nimport': 'OP_NATIVE_MODULE',
  'simport': 'OP_SELECT_IMPORT',
  'snimport': 'OP_SELECT_NATIVE_IMPORT',
  'eimport': 'OP_EJECT_IMPORT',
  'enimport': 'OP_EJECT_NATIVE_IMPORT',
  'aimport': 'OP_IMPORT_ALL',
  'animport': 'OP_IMPORT_ALL_NATIVE',

  'echo': 'OP_ECHO',
  'str': 'OP_STRINGIFY',
  'cho': 'OP_CHOICE',
  'raise': 'OP_RAISE',
  'pop': 'OP_POP',
  'clupv': 'OP_CLOSE_UP_VALUE',
  'dup': 'OP_DUP',
  'assrt': 'OP_ASSERT',
  'popn': 'OP_POP_N',

  # non-user objects...
  'sw': 'OP_SWITCH',

  # data container manipulators
  'rng': 'OP_RANGE',
  'list': 'OP_LIST',
  'dict': 'OP_DICT',
  'gind': 'OP_GET_INDEX',
  'grind': 'OP_GET_RANGED_INDEX',
  'sind': 'OP_SET_INDEX',
  'call': 'OP_CALL',
  'invk': 'OP_INVOKE',
  'invks': 'OP_INVOKE_SELF',
  'ret': 'OP_RETURN',

  'class': 'OP_CLASS',
  'meth': 'OP_METHOD',
  'clprop': 'OP_CLASS_PROPERTY',
  'gsup': 'OP_GET_SUPER',
  'inher': 'OP_INHERIT',
  'sinvk': 'OP_SUPER_INVOKE',
  'sinvks': 'OP_SUPER_INVOKE_SELF',
}

var last_line = 0, tagged_count = 0
var dumps = file(source).read().split('\n')

for dump in dumps {
  var match = dump.match(rex + '\n')
  if match {
    last_line = match[2] == '|' ? last_line : last_line++
    var op = op_map.get(match[3])

    echo '\n// ${match[0]}'

    # echo match
    if match.contains(11) {
      # it's a jump
      echo 'write_blob(vm, &function->blob, ${op}, ${last_line});'
      echo 'write_blob(vm, &function->blob, (${match[11]} >> 8) & 0xff, ${last_line});'
      echo 'write_blob(vm, &function->blob, ${match[11]} & 0xff, ${last_line});'
    } else if match.contains(7) {
      # its a call
      var count = match[7].split(' ')[0]
      echo 'int tagged${match[8]}_const = add_constant(vm, &function->blob, STRING_L_VAL("${match[9]}", ${match[9].length()}));'
      echo 'write_blob(vm, &function->blob, ${op}, ${last_line});'
      echo 'write_blob(vm, &function->blob, (tagged${match[8]}_const >> 8) & 0xff, ${last_line});'
      echo 'write_blob(vm, &function->blob, tagged${match[8]}_const & 0xff, ${last_line});'
      echo 'write_blob(vm, &function->blob, ${count}, ${last_line});'
      tagged_count++
    } else if match.contains(5) {
      # its a tagged load
      echo 'int tagged${match[5]}_const = add_constant(vm, &function->blob, STRING_L_VAL("${match[6]}", ${match[6].length()}));'
      echo 'write_blob(vm, &function->blob, ${op}, ${last_line});'
      echo 'write_blob(vm, &function->blob, (tagged${match[5]}_const >> 8) & 0xff, ${last_line});'
      echo 'write_blob(vm, &function->blob, tagged${match[5]}_const & 0xff, ${last_line});'
      tagged_count++
    } else if match.contains(4) {
      # its a load
      echo 'write_blob(vm, &function->blob, ${op}, ${last_line});'
      echo 'write_blob(vm, &function->blob, (${match[4]} >> 8) & 0xff, ${last_line});'
      echo 'write_blob(vm, &function->blob, ${match[4]} & 0xff, ${last_line});'
    } else {
      # it's an op
      echo 'write_blob(vm, &function->blob, ${op}, ${last_line});'
    }
  }
}
