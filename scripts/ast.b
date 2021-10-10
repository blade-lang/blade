/**
 * This is an helper script for generating two AST modules that describe the Blade language
 * The first is the expr module that describes Blade expressions
 * Second is the stmt module that describes Blade statements
 * The third is the decl module that describes Blade declarations such as classes
 * 
 * This script was created to help with the ast module 
 * @copyright Ore Richard Muyiwa
 */

import os

if os.args.length() < 3 {
    os.stderr.write('Missing argument: output directory\n')
    os.exit(1)
}

var output_dir = os.args[2]

var asts = {
  Expr: {
    file: 'expr.b',
    tree: {
      Binary: ['left', 'op', 'right'],
      Group: ['expression'],
      Literal: ['value'],
      Unary: ['op', 'right'],
      Condition: ['expr', 'truth', 'falsy']
    }
  },
  Stmt: {
    file: 'stmt.b',
    tree: {}
  },
  Decl: {
    file: 'decl.b',
    tree: {}
  }
}

for ast, members in asts {
  var f = file('${output_dir}${os.path_separator}${members.file}', 'w+')
  if f.exists() f.delete()

  f.write('/**\n * @class ${ast}\n * base ${ast} class\n */\n')
  f.write('class ${ast} {}\n\n')
  for cl, attr in members.tree {
    f.write('/**\n * @class ${cl}\n */\n')
    f.write('class ${cl} < ${ast} {\n\n')
    var setter
    for k in attr {
      setter += '    self.${k} = ${k}\n'
    }
    if setter {
      var params = ', '.join(attr)
      f.write('  /**\n   * @constructor ${cl}\n   */\n')
      f.write('  ${cl}(${params}) {\n')
      f.write(setter)
      f.write('  }\n')
    }
    f.write('}\n\n')
  }
}

