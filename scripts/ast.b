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
      Identifier: ['value'],
      Unary: ['op', 'right'],
      Condition: ['expr', 'truth', 'falsy'],
      Call: ['callee', 'args'],
      Get: ['expr', 'name'],
      Set: ['expr', 'name', 'value'],
      Index: ['args'],
      List: ['items'],
      Dict: ['keys', 'values'],
      Interpolation: ['data']
    },
    props: []
  },
  Stmt: {
    file: 'stmt.b',
    tree: {
      Echo: ['value'],
      Expr: ['expr'],
      If: ['condition', 'truth', 'falsy'],
      Iter: ['declaration', 'condition', 'iterator', 'body'],
      While: ['condition', 'body'],
      For: ['vars', 'iterable', 'body'],
      Continue: [],
      Break: [],
      Die: ['exception'],
      Return: ['value'],
      Assert: ['expr', 'message'],
      Using: ['expr', 'cases', 'default_case'],
      Import: ['path', 'elements'],
      Catch: ['type', 'var_name', 'body'],
      Finally: ['body'],
      Try: ['body', 'catch_stmt', 'finally_stmt'],
      Comment: ['data'],
      Block: ['body'],
      Assign: ['expr', 'type', 'value']
    },
    props: []
  },
  Decl: {
    file: 'decl.b',
    tree: {
      Var: ['name', 'value'],
      Function: ['name', 'params', 'body'],
      Method: ['name', 'params', 'body', 'is_static'],
      Property: ['name', 'value', 'is_static'],
      Class: ['name', 'superclass', 'properties', 'methods'],
    },
    props: ['doc']
  },
  Defn: {
    file: 'defn.b',
    tree: {
      Doc: ['data']
    },
    props: []
  }
}

for ast, members in asts {
  var f = file('${output_dir}${os.path_separator}${members.file}', 'w+')
  if f.exists() f.delete()

  f.write('#!-- This file is autogenerated by scripts/ast.b\n/**\n * @class ${ast}\n * base ${ast} class\n */\n')
  f.write('class ${ast} {\n')
  for p in members.props {
    f.write('  var ${p}\n')
  }
  f.write('}\n\n')
  for cl, attr in members.tree {
    f.write('/**\n * @class ${cl}${ast}\n */\n')
    f.write('class ${cl}${ast} < ${ast} {\n\n')
    var setter, json_body
    for k in attr {
      setter += '    self.${k} = ${k}\n'
      json_body += '      ${k}: self.${k},\n'
    }
    if setter {
      var params = ', '.join(attr)
      f.write('  /**\n   * @constructor ${cl}\n   */\n')
      f.write('  ${cl}${ast}(${params}) {\n')
      f.write(setter)
      f.write('  }\n\n')
    }

    f.write('  @to_json() {\n')
    f.write('    return {\n')
    f.write(json_body ? json_body : ' ')
    f.write('    }\n')
    f.write('  }\n')

    f.write('}\n\n')
  }
}

