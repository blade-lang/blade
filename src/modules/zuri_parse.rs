//! `_zuri_parse` builtin module; the native backing for `zuri.parse()`.
//!
//! Runs the real `Lexer`/`Parser` pipeline (the SAME grammar the
//! compiler itself uses; no second, parallel parser to keep in sync)
//! and converts the resulting `Vec<Decl>` into a list of nodes, built
//! directly as instances of the `Node` class `libs/zuri/ast.zu` passes
//! in: `kind`, the six position fields, and a `fields` dict holding
//! the kind's own children, already nodes themselves.
//!
//! Kinds, field names and operator names come from a fixed vocabulary
//! and are interned, so every node shares them with each other and
//! with the same literals in Zuri code. A name the source spells is
//! made once per parse, however often the file uses it.
//!
//! A node's position is the whole of the source it was read from:
//! `start` and `end` are char offsets, the first character and the one
//! just past the last, and `line`/`col` and `end_line`/`end_col` are
//! the same two points as one-based lines and columns. A declaration
//! starts at its keyword, so `var x = 1` starts at `var`, and the name it
//! declares comes with a `name_span` of its own in `fields`, shaped like
//! a node's position without the `kind` and `fields`.
//!
//! Every comment and doc block the source contains comes back too, as
//! its own `"Comment"`/`"DocBlock"`-kind node sitting exactly where it
//! appeared lexically (see `DeclKind::Trivia`/`StmtKind::Trivia` in
//! `src/compiler/ast.rs`, and the parser's own trivia-draining logic in
//! `src/compiler/parser.rs`); this is what makes the result "lossless
//! enough to rewrite the file from" for anything at statement/
//! declaration granularity. A comment written INSIDE an expression
//! (`foo(a, # x` on one line, `b)` on the next, ...) has no node kind of
//! its own inside that expression. It's still captured, not dropped:
//! the parser's trivia buffer doesn't distinguish "seen while parsing
//! this statement's own expression" from "seen while chasing trailing
//! whitespace toward the next one", so it comes back as an ordinary
//! sibling node immediately after the smallest enclosing statement/
//! declaration/class-member, in that statement's own surrounding list,
//! rather than nested inside the expression it visually sat in. Source
//! that keeps each comment on a line of its own, between statements,
//! never meets this; elsewhere it costs only where the comment ends up
//! in the tree, never the comment itself.

use crate::builtins::enforce::ArgType;
use crate::compiler::ast::{
  Decl, DeclKind, Expr, ExprKind, Segment, Span, Stmt, StmtKind, Type, TypeName,
};
use crate::compiler::lexer::Lexer;
use crate::compiler::parser::{Parser, ParserError};
use crate::compiler::token::{Token, TokenKind};
use crate::enforce_arg_count;
use crate::enforce_arg_type;
use crate::modules::zuri_lex::token_kind_name;
use crate::modules::{BuiltinModuleDef, Shape, native};
use crate::vm::object::ZuriContext;
use crate::vm::value::Value;
use crate::vm::vm::VM;
use rustc_hash::FxHashMap;

pub static MODULE: BuiltinModuleDef = BuiltinModuleDef {
  name: "_zuri_parse",
  build,
};

fn build(vm: &mut VM) -> Vec<(&'static str, Value)> {
  vec![
    ("parse", native(vm, "parse", 2, false, parse_fn)),
    (
      "parse_partial",
      native(vm, "parse_partial", 2, false, parse_partial_fn),
    ),
  ]
}

/// The fields of a `Node` the writer fills, in the order `Writer::node`
/// passes them.
const NODE_FIELDS: [&str; 8] = [
  "kind", "start", "end", "line", "col", "end_line", "end_col", "fields",
];

/// `_zuri_parse.parse(source, Node)`: parses `source` in full and
/// returns its top-level declarations as a list of `Node`s. See this
/// module's own doc comment for the exact shape and what is/isn't
/// preserved. Raises (rather than returning a partial result) if
/// `source` has a syntax error; there is no meaningful AST to hand
/// back for input the parser itself couldn't make sense of.
fn parse_fn(ctx: &mut ZuriContext) -> Result<Value, String> {
  enforce_arg_count!(ctx, 2);
  enforce_arg_type!(ctx, 0, ArgType::String);
  enforce_arg_type!(ctx, 1, ArgType::Class);

  let nodes = Shape::of(ctx.args[1], NODE_FIELDS)?;
  let args = ctx.args;
  let source = args[0].as_str();
  let mut lexer = Lexer::new(source);
  let mut parser = Parser::new(&mut lexer);

  let decls = match parser.parse() {
    Ok(decls) => decls,
    Err(errors) => {
      let msg = errors
        .iter()
        .map(|e| e.to_string())
        .collect::<Vec<_>>()
        .join("\n  ");
      return Err(format!("failed to parse source:\n  {}", msg));
    },
  };

  let mut w = Writer::building(ctx, source, nodes);

  Ok(w.decls(&decls))
}

/// `_zuri_parse.parse_partial(source, Node)`: parses `source` as far as
/// the parser can recover and returns `{ nodes, errors }`, the tree it
/// read and a diagnostic dict for every syntax error. Never raises on
/// bad source.
fn parse_partial_fn(ctx: &mut ZuriContext) -> Result<Value, String> {
  enforce_arg_count!(ctx, 2);
  enforce_arg_type!(ctx, 0, ArgType::String);
  enforce_arg_type!(ctx, 1, ArgType::Class);

  let nodes = Shape::of(ctx.args[1], NODE_FIELDS)?;
  let args = ctx.args;
  let source = args[0].as_str();
  let mut lexer = Lexer::new(source);
  let mut parser = Parser::new(&mut lexer);
  let (decls, errors) = parser.parse_recovering();

  let mut w = Writer::building(ctx, source, nodes);
  let nodes = w.decls(&decls);
  let errors = w.diagnostics(&errors);
  let entries = vec![w.kv("nodes", nodes), w.kv("errors", errors)];

  Ok(w.ctx.heap().alloc_dict(entries))
}

/// Where each line of the source starts, as a char offset, so an offset
/// can be turned into the one-based line and column the lexer counts in.
struct Lines {
  starts: Vec<usize>,
  len: usize,
}

impl Lines {
  fn new(source: &str) -> Self {
    let mut starts = vec![0];
    let mut len = 0;

    for (offset, c) in source.chars().enumerate() {
      if c == '\n' {
        starts.push(offset + 1);
      }
      len = offset + 1;
    }

    Self { starts, len }
  }

  /// The whole of one line, its line end aside. A line past the end of
  /// the source is an empty span at the end.
  fn line(&self, line: usize) -> Span {
    let Some(&start) = self.starts.get(line.saturating_sub(1)) else {
      return Span::new(self.len, self.len);
    };

    let end = match self.starts.get(line) {
      Some(&next) => next - 1,
      None => self.len,
    };

    Span::new(start, end)
  }

  fn position(&self, offset: usize) -> (usize, usize) {
    let index = match self.starts.binary_search(&offset) {
      Ok(index) => index,
      Err(index) => index - 1,
    };

    (index + 1, offset - self.starts[index] + 1)
  }
}

/// Builds the nodes and diagnostic dicts for one source.
pub(crate) struct Writer<'c, 'v> {
  ctx: &'c mut ZuriContext<'v>,
  lines: Lines,
  /// The `Node` class, for a writer that builds nodes.
  nodes: Option<Shape<8>>,
  /// Every name made so far, so each is one string however often the
  /// source uses it.
  names: FxHashMap<String, Value>,
}

impl<'c, 'v> Writer<'c, 'v> {
  /// A writer for diagnostics alone.
  pub(crate) fn new(ctx: &'c mut ZuriContext<'v>, source: &str) -> Self {
    Self {
      ctx,
      lines: Lines::new(source),
      nodes: None,
      names: FxHashMap::default(),
    }
  }

  /// A writer that builds nodes as instances of the class `nodes`
  /// describes.
  fn building(ctx: &'c mut ZuriContext<'v>, source: &str, nodes: Shape<8>) -> Self {
    Self {
      nodes: Some(nodes),
      ..Self::new(ctx, source)
    }
  }

  /// A list of `{ message, start, end, line, col, end_line, end_col }`
  /// dicts, one for each error. An error that belongs to no one token
  /// covers its whole line.
  pub(crate) fn diagnostics(&mut self, errors: &[ParserError]) -> Value {
    let mut values = Vec::with_capacity(errors.len());

    for error in errors {
      let span = error
        .span
        .unwrap_or_else(|| self.lines.line(error.line_number));
      let message = self.string(error.message.clone());
      let mut entries = vec![self.kv("message", message)];
      entries.extend(self.position(span));
      values.push(self.ctx.heap().alloc_dict(entries));
    }

    self.ctx.heap().alloc_list(values)
  }

  fn string(&mut self, text: impl Into<String>) -> Value {
    self.ctx.heap().alloc_string(text.into())
  }

  /// A kind, field or operator name, all of which come from the
  /// grammar's own fixed vocabulary.
  fn word(&mut self, text: &str) -> Value {
    self.ctx.heap().intern_str(text)
  }

  /// A name the source spells, shared with every earlier use of it.
  fn name(&mut self, text: String) -> Value {
    if let Some(&known) = self.names.get(&text) {
      return known;
    }

    let value = self.ctx.heap().alloc_string(text.clone());
    self.names.insert(text, value);
    value
  }

  fn kv(&mut self, key: &str, value: Value) -> (Value, Value) {
    (self.word(key), value)
  }

  /// `start`, `end`, `line`, `col`, `end_line` and `end_col` for a span.
  fn position(&mut self, span: Span) -> Vec<(Value, Value)> {
    let (line, col) = self.lines.position(span.start);
    let (end_line, end_col) = self.lines.position(span.end);

    vec![
      self.kv("start", Value::number(span.start as f64)),
      self.kv("end", Value::number(span.end as f64)),
      self.kv("line", Value::number(line as f64)),
      self.kv("col", Value::number(col as f64)),
      self.kv("end_line", Value::number(end_line as f64)),
      self.kv("end_col", Value::number(end_col as f64)),
    ]
  }

  /// A span on its own, as the dict a `name_span` holds.
  fn span(&mut self, span: Span) -> Value {
    let entries = self.position(span);
    self.ctx.heap().alloc_dict(entries)
  }

  fn node(&mut self, kind: &str, span: Span, fields: Vec<(Value, Value)>) -> Value {
    let kind_v = self.word(kind);
    let (line, col) = self.lines.position(span.start);
    let (end_line, end_col) = self.lines.position(span.end);
    let fields_v = self.ctx.heap().alloc_dict(fields);

    let number = |n: usize| Value::number(n as f64);
    let values = [
      kind_v,
      number(span.start),
      number(span.end),
      number(line),
      number(col),
      number(end_line),
      number(end_col),
      fields_v,
    ];

    let nodes = self
      .nodes
      .as_ref()
      .expect("only a writer made by `building` writes nodes");
    nodes.build(self.ctx.vm, values)
  }

  /// The `name` and `name_span` fields of anything that declares or reads
  /// a name through a token of its own.
  fn name_fields(&mut self, token: &Token) -> Vec<(Value, Value)> {
    let name = self.name(token_identifier_name(token));
    let span = self.span(Span::of(token));

    vec![self.kv("name", name), self.kv("name_span", span)]
  }

  fn opt_expr(&mut self, e: &Option<Box<Expr>>) -> Value {
    match e {
      Some(e) => self.expr(e),
      None => Value::nil(),
    }
  }

  fn opt_stmt(&mut self, s: &Option<Box<Stmt>>) -> Value {
    match s {
      Some(s) => self.stmt(s),
      None => Value::nil(),
    }
  }

  fn exprs(&mut self, items: &[Expr]) -> Value {
    let vals: Vec<Value> = items.iter().map(|item| self.expr(item)).collect();
    self.ctx.heap().alloc_list(vals)
  }

  fn stmts(&mut self, items: &[Stmt]) -> Value {
    let vals: Vec<Value> = items.iter().map(|item| self.stmt(item)).collect();
    self.ctx.heap().alloc_list(vals)
  }

  fn decls(&mut self, items: &[Decl]) -> Value {
    let vals: Vec<Value> = items.iter().map(|item| self.decl(item)).collect();
    self.ctx.heap().alloc_list(vals)
  }

  fn type_name(&mut self, name: &TypeName) -> Value {
    let (kind, class_name) = match &name.ty {
      Type::Any => ("Any", None),
      Type::Bool => ("Bool", None),
      Type::Int => ("Int", None),
      Type::Number => ("Number", None),
      Type::BigInt => ("BigInt", None),
      Type::String => ("String", None),
      Type::Bytes => ("Bytes", None),
      Type::List => ("List", None),
      Type::Dict => ("Dict", None),
      Type::Range => ("Range", None),
      Type::File => ("File", None),
      Type::Function => ("Function", None),
      Type::Type => ("Type", None),
      Type::Callable => ("Callable", None),
      Type::Iterable => ("Iterable", None),
      Type::Instance(tok) => ("Instance", Some(token_identifier_name(tok))),
    };

    let class_name_v = match class_name {
      Some(n) => self.name(n),
      None => Value::nil(),
    };
    let fields = vec![self.kv("class_name", class_name_v)];

    self.node(kind, name.span, fields)
  }

  /// `left`, `op` and `right`, for the binary-shaped nodes.
  fn operands(&mut self, left: &Expr, op: &TokenKind, right: &Expr) -> Vec<(Value, Value)> {
    let op_v = self.word(token_kind_name(op));
    let left_v = self.expr(left);
    let right_v = self.expr(right);

    vec![
      self.kv("left", left_v),
      self.kv("op", op_v),
      self.kv("right", right_v),
    ]
  }

  fn expr(&mut self, e: &Expr) -> Value {
    let span = e.span;

    let (kind, fields) = match &e.kind {
      ExprKind::Nil => ("Nil", vec![]),
      ExprKind::Bool(b) => ("Bool", vec![self.kv("value", Value::bool(*b))]),
      ExprKind::Integer(n) => ("Integer", vec![self.kv("value", Value::number(*n as f64))]),
      ExprKind::Float(n) => ("Float", vec![self.kv("value", Value::number(*n))]),
      ExprKind::BigNumber(n) => {
        let v = self.ctx.heap().alloc_bigint(n.clone());
        ("BigNumber", vec![self.kv("value", v)])
      },
      ExprKind::Literal(s) => {
        let v = self.string(s.clone());
        ("Literal", vec![self.kv("value", v)])
      },
      ExprKind::Unary(op, inner, _) => {
        let op_v = self.word(token_kind_name(op));
        let inner_v = self.expr(inner);
        (
          "Unary",
          vec![self.kv("op", op_v), self.kv("operand", inner_v)],
        )
      },
      ExprKind::Binary(l, op, r, _) => ("Binary", self.operands(l, op, r)),
      ExprKind::Logical(l, op, r, _) => ("Logical", self.operands(l, op, r)),
      ExprKind::Circuit(l, op, r) => ("Circuit", self.operands(l, op, r)),
      ExprKind::Grouping(inner) => {
        let v = self.expr(inner);
        ("Grouping", vec![self.kv("inner", v)])
      },
      ExprKind::Range(lo, hi, _) => {
        let lo_v = self.expr(lo);
        let hi_v = self.expr(hi);
        (
          "Range",
          vec![self.kv("lower", lo_v), self.kv("upper", hi_v)],
        )
      },
      ExprKind::Identifier(tok) => {
        let name = self.name(token_identifier_name(tok));
        ("Identifier", vec![self.kv("name", name)])
      },
      ExprKind::Condition(cond, then_e, else_e) => {
        let cond_v = self.expr(cond);
        let then_v = self.expr(then_e);
        let else_v = self.expr(else_e);
        (
          "Condition",
          vec![
            self.kv("condition", cond_v),
            self.kv("then", then_v),
            self.kv("otherwise", else_v),
          ],
        )
      },
      ExprKind::Call(callee, args, _) => {
        let callee_v = self.expr(callee);
        let args_v = self.exprs(args);
        (
          "Call",
          vec![self.kv("callee", callee_v), self.kv("arguments", args_v)],
        )
      },
      ExprKind::Get(obj, tok) => {
        let obj_v = self.expr(obj);
        let mut fields = vec![self.kv("object", obj_v)];
        fields.extend(self.name_fields(tok));
        ("Get", fields)
      },
      ExprKind::Set(obj, tok, val) => {
        let obj_v = self.expr(obj);
        let val_v = self.expr(val);
        let mut fields = vec![self.kv("object", obj_v)];
        fields.extend(self.name_fields(tok));
        fields.push(self.kv("value", val_v));
        ("Set", fields)
      },
      ExprKind::Index(obj, idx, _) => {
        let obj_v = self.expr(obj);
        let idx_v = self.expr(idx);
        (
          "Index",
          vec![self.kv("object", obj_v), self.kv("index", idx_v)],
        )
      },
      ExprKind::Slice(obj, lo, hi, _) => {
        let obj_v = self.expr(obj);
        let lo_v = self.expr(lo);
        let hi_v = self.expr(hi);
        (
          "Slice",
          vec![
            self.kv("object", obj_v),
            self.kv("lower", lo_v),
            self.kv("upper", hi_v),
          ],
        )
      },
      ExprKind::List(items) => {
        let items_v = self.exprs(items);
        ("List", vec![self.kv("items", items_v)])
      },
      ExprKind::Dict(keys, values) => {
        let keys_v = self.exprs(keys);
        let values_v = self.exprs(values);
        (
          "Dict",
          vec![self.kv("keys", keys_v), self.kv("values", values_v)],
        )
      },
      ExprKind::Parent(_) => ("Parent", vec![]),
      ExprKind::Self_(_) => ("Self", vec![]),
      ExprKind::Assign(target, value) => {
        let target_v = self.expr(target);
        let value_v = self.expr(value);
        (
          "Assign",
          vec![self.kv("target", target_v), self.kv("value", value_v)],
        )
      },
      ExprKind::Anonymous(decl) => {
        let decl_v = self.decl(decl);
        ("Anonymous", vec![self.kv("declaration", decl_v)])
      },
      ExprKind::TypeHint(types, nullable) => {
        let names: Vec<Value> = types.iter().map(|name| self.type_name(name)).collect();
        let types_v = self.ctx.heap().alloc_list(names);
        (
          "TypeHint",
          vec![
            self.kv("types", types_v),
            self.kv("nullable", Value::bool(*nullable)),
          ],
        )
      },
      ExprKind::Argument(tok, type_hint) => {
        let type_v = self.expr(type_hint);
        let mut fields = self.name_fields(tok);
        fields.push(self.kv("type_hint", type_v));
        ("Argument", fields)
      },
      ExprKind::CompoundAssign(target, op, value, _) => {
        let target_v = self.expr(target);
        let op_v = self.word(token_kind_name(op));
        let value_v = self.expr(value);
        (
          "CompoundAssign",
          vec![
            self.kv("target", target_v),
            self.kv("op", op_v),
            self.kv("value", value_v),
          ],
        )
      },
      ExprKind::Update(target, op, _) => {
        let target_v = self.expr(target);
        let op_v = self.word(token_kind_name(op));
        (
          "Update",
          vec![self.kv("target", target_v), self.kv("op", op_v)],
        )
      },
      ExprKind::Interpolation(segments) => {
        let parts: Vec<Value> = segments
          .iter()
          .map(|segment| match segment {
            Segment::Text(token) => self.string(segment_text(token)),
            Segment::Value(value) => self.expr(value),
          })
          .collect();
        let parts_v = self.ctx.heap().alloc_list(parts);
        ("Interpolation", vec![self.kv("parts", parts_v)])
      },
      ExprKind::Shorthand(tok) => {
        let name = self.name(token_identifier_name(tok));
        ("Shorthand", vec![self.kv("name", name)])
      },
    };

    self.node(kind, span, fields)
  }

  fn stmt(&mut self, s: &Stmt) -> Value {
    let span = s.span;

    let (kind, fields) = match &s.kind {
      StmtKind::None => ("None", vec![]),
      StmtKind::FixContinue => ("FixContinue", vec![]),
      StmtKind::Echo(e) => {
        let v = self.expr(e);
        ("Echo", vec![self.kv("value", v)])
      },
      StmtKind::Expression(e) => {
        let v = self.expr(e);
        ("Expression", vec![self.kv("value", v)])
      },
      StmtKind::If(cond, then_s, else_s) => {
        let cond_v = self.expr(cond);
        let then_v = self.stmt(then_s);
        let else_v = self.opt_stmt(else_s);
        (
          "If",
          vec![
            self.kv("condition", cond_v),
            self.kv("then", then_v),
            self.kv("otherwise", else_v),
          ],
        )
      },
      StmtKind::While(cond, body) => {
        let cond_v = self.expr(cond);
        let body_v = self.stmt(body);
        (
          "While",
          vec![self.kv("condition", cond_v), self.kv("body", body_v)],
        )
      },
      StmtKind::For(key, value, iterable, body) => {
        let (key_v, key_span_v) = match key {
          Some(key) => (
            self.name(token_identifier_name(key)),
            self.span(Span::of(key)),
          ),
          None => (Value::nil(), Value::nil()),
        };
        let value_v = self.name(token_identifier_name(value));
        let value_span_v = self.span(Span::of(value));
        let iterable_v = self.expr(iterable);
        let body_v = self.stmt(body);
        (
          "For",
          vec![
            self.kv("key", key_v),
            self.kv("key_span", key_span_v),
            self.kv("value", value_v),
            self.kv("value_span", value_span_v),
            self.kv("iterable", iterable_v),
            self.kv("body", body_v),
          ],
        )
      },
      StmtKind::Iter(initializer, condition, steps, body) => {
        let initializer_v = self.opt_stmt(initializer);
        let condition_v = self.opt_expr(condition);
        let steps_v = self.exprs(steps);
        let body_v = self.stmt(body);
        (
          "Iter",
          vec![
            self.kv("initializer", initializer_v),
            self.kv("condition", condition_v),
            self.kv("steps", steps_v),
            self.kv("body", body_v),
          ],
        )
      },
      StmtKind::DoWhile(body, condition) => {
        let body_v = self.stmt(body);
        let condition_v = self.expr(condition);
        (
          "DoWhile",
          vec![self.kv("body", body_v), self.kv("condition", condition_v)],
        )
      },
      StmtKind::Continue(_) => ("Continue", vec![]),
      StmtKind::Break(_) => ("Break", vec![]),
      StmtKind::Raise(e) => {
        let v = self.expr(e);
        ("Raise", vec![self.kv("value", v)])
      },
      StmtKind::Return(_, e) => {
        let v = self.expr(e);
        ("Return", vec![self.kv("value", v)])
      },
      StmtKind::Assert(cond, msg) => {
        let cond_v = self.expr(cond);
        let msg_v = self.opt_expr(msg);
        (
          "Assert",
          vec![self.kv("condition", cond_v), self.kv("message", msg_v)],
        )
      },
      StmtKind::Using(subject, arms, default) => {
        let subject_v = self.expr(subject);
        let mut arm_values = Vec::with_capacity(arms.len());
        for arm in arms {
          let labels_v = self.exprs(&arm.labels);
          let body_v = self.stmt(&arm.body);
          let f = vec![self.kv("labels", labels_v), self.kv("body", body_v)];
          arm_values.push(self.node("When", arm.span, f));
        }
        let arms_v = self.ctx.heap().alloc_list(arm_values);
        let default_v = self.opt_stmt(default);
        (
          "Using",
          vec![
            self.kv("subject", subject_v),
            self.kv("arms", arms_v),
            // Not `"default"`: that's a reserved keyword, and a dict
            // field named after one can never be read back with plain
            // dot-access syntax (`node.fields.default` doesn't parse).
            self.kv("default_body", default_v),
          ],
        )
      },
      StmtKind::Import(path, path_tokens, name_expr, elements, imports_all, exported) => {
        let path_v = self.string(path.clone());
        let mut segments = Vec::with_capacity(path_tokens.len());
        for token in path_tokens {
          let text = match &token.kind {
            TokenKind::Dot => ".".to_string(),
            TokenKind::Range => "..".to_string(),
            _ => token_identifier_name(token),
          };
          let text_v = self.name(text);
          let mut entries = vec![self.kv("text", text_v)];
          entries.extend(self.position(Span::of(token)));
          segments.push(self.ctx.heap().alloc_dict(entries));
        }
        let segments_v = self.ctx.heap().alloc_list(segments);
        let aliased = matches!(
          &name_expr.kind,
          ExprKind::Identifier(token) if matches!(token.kind, TokenKind::Identifier(_))
        );
        let name_v = self.expr(name_expr);
        let elements_v = self.exprs(elements);
        (
          "Import",
          vec![
            self.kv("path", path_v),
            self.kv("segments", segments_v),
            self.kv("name", name_v),
            self.kv("aliased", Value::bool(aliased)),
            self.kv("elements", elements_v),
            self.kv("imports_all", Value::bool(*imports_all)),
            self.kv("exported", Value::bool(*exported)),
          ],
        )
      },
      StmtKind::Catch(body, catch_body, error_var) => {
        let body_v = self.stmt(body);
        let catch_v = self.opt_stmt(catch_body);
        let var_v = self.opt_expr(error_var);
        (
          "Catch",
          vec![
            self.kv("body", body_v),
            self.kv("catch_body", catch_v),
            self.kv("error_var", var_v),
          ],
        )
      },
      StmtKind::Block(stmts) => {
        let v = self.stmts(stmts);
        ("Block", vec![self.kv("statements", v)])
      },
      StmtKind::Decl(d) => {
        let v = self.decl(d);
        ("Decl", vec![self.kv("declaration", v)])
      },
      StmtKind::Var(tok, init, type_hint, is_constant) => {
        let init_v = self.expr(init);
        let type_v = self.opt_expr(type_hint);
        let mut fields = self.name_fields(tok);
        fields.push(self.kv("value", init_v));
        fields.push(self.kv("type_hint", type_v));
        fields.push(self.kv("is_constant", Value::bool(*is_constant)));
        ("Var", fields)
      },
      StmtKind::VarList(list) => {
        let v = self.stmts(list);
        ("VarList", vec![self.kv("declarations", v)])
      },
      StmtKind::Trivia(tok) => return self.trivia(tok, span),
    };

    self.node(kind, span, fields)
  }

  /// Shared by `StmtKind::Trivia`/`DeclKind::Trivia`: a `#`-comment
  /// becomes a `"Comment"`-kind node, a `/* ... */` becomes a
  /// `"DocBlock"`-kind node; both carry `{ text }`, the comment's own
  /// content, with the leading `#`/wrapping `/*`/`*/` already stripped
  /// off (same content the raw `Token` itself carries).
  fn trivia(&mut self, tok: &Token, span: Span) -> Value {
    let (kind, text) = match &tok.kind {
      TokenKind::Comment(s) => ("Comment", s.clone()),
      TokenKind::DocBlock(s) => ("DocBlock", s.clone()),
      _ => {
        unreachable!("DeclKind::Trivia/StmtKind::Trivia always wraps a Comment or DocBlock token")
      },
    };
    let text_v = self.string(text);
    let f = vec![self.kv("text", text_v)];

    self.node(kind, span, f)
  }

  fn decl(&mut self, d: &Decl) -> Value {
    let span = d.span;

    let (kind, fields) = match &d.kind {
      DeclKind::Stmt(s) => {
        let v = self.stmt(s);
        ("Stmt", vec![self.kv("statement", v)])
      },
      DeclKind::Function(tok, params, body, is_variadic) => {
        let params_v = self.exprs(params);
        let body_v = self.stmt(body);
        let mut fields = self.name_fields(tok);
        fields.push(self.kv("parameters", params_v));
        fields.push(self.kv("body", body_v));
        fields.push(self.kv("is_variadic", Value::bool(*is_variadic)));
        ("Function", fields)
      },
      DeclKind::Method(tok, params, body, is_variadic, is_static) => {
        let params_v = self.exprs(params);
        let body_v = self.stmt(body);
        let mut fields = self.name_fields(tok);
        fields.push(self.kv("parameters", params_v));
        fields.push(self.kv("body", body_v));
        fields.push(self.kv("is_variadic", Value::bool(*is_variadic)));
        fields.push(self.kv("is_static", Value::bool(*is_static)));
        ("Method", fields)
      },
      DeclKind::Property(tok, value, type_hint, is_static, is_constant) => {
        let value_v = self.expr(value);
        let type_v = self.expr(type_hint);
        let mut fields = self.name_fields(tok);
        fields.push(self.kv("value", value_v));
        fields.push(self.kv("type_hint", type_v));
        fields.push(self.kv("is_static", Value::bool(*is_static)));
        fields.push(self.kv("is_constant", Value::bool(*is_constant)));
        ("Property", fields)
      },
      DeclKind::Class(tok, superclass, properties, methods, is_extension) => {
        let superclass_v = match superclass {
          Some(e) => self.expr(e),
          None => Value::nil(),
        };
        let properties_v = self.decls(properties);
        let methods_v = self.decls(methods);
        let mut fields = self.name_fields(tok);
        fields.push(self.kv("superclass", superclass_v));
        fields.push(self.kv("properties", properties_v));
        fields.push(self.kv("methods", methods_v));
        fields.push(self.kv("is_extension", Value::bool(*is_extension)));
        ("Class", fields)
      },
      DeclKind::Trivia(tok) => return self.trivia(tok, span),
    };

    self.node(kind, span, fields)
  }
}

/// The text a piece of an interpolated string holds, without quotes.
fn segment_text(token: &Token) -> String {
  match &token.kind {
    TokenKind::Literal(text) | TokenKind::Interpolation(text) => text.clone(),
    other => format!("{}", other),
  }
}

/// The plain string an identifier-carrying `Token` names: a variable,
/// a function, a field or a class. The name an import binds without an
/// `as` is carried as a literal, since nothing in the source spelled it.
fn token_identifier_name(tok: &Token) -> String {
  match &tok.kind {
    TokenKind::Identifier(s) | TokenKind::Decorator(s) | TokenKind::Literal(s) => s.clone(),
    // The placeholder for a name the source is missing, in a tree read
    // by `parse_partial`.
    TokenKind::Eof => String::new(),
    other => format!("{}", other),
  }
}
