//! Rewrites the loops the parser keeps as written into the plain
//! `while` loops code generation knows how to emit.
//!
//! The parser hands back `for`, `iter` and `do..while` as statements of
//! their own, so anything reading the tree sees the program the way it
//! was typed. The compiler only emits `while`, so all three are lowered
//! here, once, before the compiler looks at anything. Nothing past this
//! point ever meets a `For`, `Iter` or `DoWhile`.
//!
//! The same goes for the shorthands among the expressions. Compound
//! assignment, `++` and `--` become a plain assignment of an arithmetic
//! result, an interpolated string becomes a chain of `+`, and a `{ name }`
//! dictionary value becomes a read of `name`.
//!
//! A node made up here takes the span of the construct it stands in for.
//! Every name made up here starts with `$`, which the lexer never
//! produces from source, so none of them can collide with a name the
//! program declares.

use crate::compiler::ast::{
  Decl, DeclKind, Expr, ExprKind, Segment, Span, Stmt, StmtKind, WhenArm,
};
use crate::compiler::token::{Token, TokenKind, get_assignment_alt};

/// Lowers a whole program, including the bodies of its functions,
/// methods, classes and anonymous functions.
pub fn lower(decls: Vec<Decl>) -> Vec<Decl> {
  decls.into_iter().map(decl).collect()
}

fn decl(d: Decl) -> Decl {
  let kind = match d.kind {
    DeclKind::Stmt(s) => DeclKind::Stmt(boxed_stmt(s)),
    DeclKind::Function(name, params, body, is_variadic) => {
      DeclKind::Function(name, exprs_of(params), boxed_stmt(body), is_variadic)
    },
    DeclKind::Method(name, params, body, is_variadic, is_static) => DeclKind::Method(
      name,
      exprs_of(params),
      boxed_stmt(body),
      is_variadic,
      is_static,
    ),
    DeclKind::Property(name, value, type_hint, is_static, is_constant) => DeclKind::Property(
      name,
      boxed_expr(value),
      boxed_expr(type_hint),
      is_static,
      is_constant,
    ),
    DeclKind::Class(name, superclass, properties, methods, is_extension) => DeclKind::Class(
      name,
      superclass.map(boxed_expr),
      properties.into_iter().map(decl).collect(),
      methods.into_iter().map(decl).collect(),
      is_extension,
    ),
    DeclKind::Trivia(token) => DeclKind::Trivia(token),
  };

  Decl::new(kind, d.span)
}

fn stmt(s: Stmt) -> Stmt {
  let span = s.span;

  let kind = match s.kind {
    StmtKind::For(key, value, iterable, body) => {
      return for_loop(span, key, value, expr(*iterable), stmt(*body));
    },
    StmtKind::Iter(initializer, condition, steps, body) => {
      return iter_loop(
        span,
        initializer.map(|s| stmt(*s)),
        condition.map(|e| expr(*e)),
        exprs_of(steps),
        stmt(*body),
      );
    },
    StmtKind::DoWhile(body, condition) => {
      let body = stmt(*body);
      let looped = StmtKind::While(boxed_expr(condition), Box::new(body.clone()));

      StmtKind::Block(vec![body, Stmt::new(looped, span)])
    },
    StmtKind::Echo(e) => StmtKind::Echo(boxed_expr(e)),
    StmtKind::Expression(e) => StmtKind::Expression(boxed_expr(e)),
    StmtKind::If(condition, then_branch, else_branch) => StmtKind::If(
      boxed_expr(condition),
      boxed_stmt(then_branch),
      else_branch.map(boxed_stmt),
    ),
    StmtKind::While(condition, body) => StmtKind::While(boxed_expr(condition), boxed_stmt(body)),
    StmtKind::Raise(e) => StmtKind::Raise(boxed_expr(e)),
    StmtKind::Return(keyword, e) => StmtKind::Return(keyword, boxed_expr(e)),
    StmtKind::Assert(condition, message) => {
      StmtKind::Assert(boxed_expr(condition), message.map(boxed_expr))
    },
    StmtKind::Using(subject, arms, default) => StmtKind::Using(
      boxed_expr(subject),
      arms
        .into_iter()
        .map(|arm| WhenArm {
          labels: exprs_of(arm.labels),
          body: stmt(arm.body),
          span: arm.span,
        })
        .collect(),
      default.map(boxed_stmt),
    ),
    StmtKind::Import(path, path_tokens, name, elements, imports_all, exported) => StmtKind::Import(
      path,
      path_tokens,
      boxed_expr(name),
      exprs_of(elements),
      imports_all,
      exported,
    ),
    StmtKind::Catch(body, handler, error_var) => StmtKind::Catch(
      boxed_stmt(body),
      handler.map(boxed_stmt),
      error_var.map(boxed_expr),
    ),
    StmtKind::Block(stmts) => StmtKind::Block(stmts_of(stmts)),
    StmtKind::Decl(d) => StmtKind::Decl(Box::new(decl(*d))),
    StmtKind::Var(name, value, type_hint, is_constant) => StmtKind::Var(
      name,
      boxed_expr(value),
      type_hint.map(boxed_expr),
      is_constant,
    ),
    StmtKind::VarList(stmts) => StmtKind::VarList(stmts_of(stmts)),
    StmtKind::None => StmtKind::None,
    StmtKind::FixContinue => StmtKind::FixContinue,
    StmtKind::Continue(keyword) => StmtKind::Continue(keyword),
    StmtKind::Break(keyword) => StmtKind::Break(keyword),
    StmtKind::Trivia(token) => StmtKind::Trivia(token),
  };

  Stmt::new(kind, span)
}

fn expr(e: Expr) -> Expr {
  let span = e.span;

  let kind = match e.kind {
    ExprKind::CompoundAssign(target, op, value, line) => {
      return assign_with(span, *target, get_assignment_alt(op), expr(*value), line);
    },
    ExprKind::Update(target, op, line) => {
      let op = match op {
        TokenKind::Increment => TokenKind::Plus,
        _ => TokenKind::Minus,
      };

      return assign_with(
        span,
        *target,
        op,
        Expr::new(ExprKind::Integer(1), span),
        line,
      );
    },
    ExprKind::Interpolation(segments) => return interpolation(span, segments),
    ExprKind::Shorthand(name) => ExprKind::Identifier(name),
    ExprKind::Unary(op, operand, line) => ExprKind::Unary(op, boxed_expr(operand), line),
    ExprKind::Binary(left, op, right, line) => {
      ExprKind::Binary(boxed_expr(left), op, boxed_expr(right), line)
    },
    ExprKind::Logical(left, op, right, line) => {
      ExprKind::Logical(boxed_expr(left), op, boxed_expr(right), line)
    },
    ExprKind::Circuit(left, op, right) => {
      ExprKind::Circuit(boxed_expr(left), op, boxed_expr(right))
    },
    ExprKind::Grouping(inner) => ExprKind::Grouping(boxed_expr(inner)),
    ExprKind::Range(lower, upper, line) => {
      ExprKind::Range(boxed_expr(lower), boxed_expr(upper), line)
    },
    ExprKind::Condition(condition, truth, falsy) => {
      ExprKind::Condition(boxed_expr(condition), boxed_expr(truth), boxed_expr(falsy))
    },
    ExprKind::Call(callee, args, line) => ExprKind::Call(boxed_expr(callee), exprs_of(args), line),
    ExprKind::Get(object, name) => ExprKind::Get(boxed_expr(object), name),
    ExprKind::Set(object, name, value) => {
      ExprKind::Set(boxed_expr(object), name, boxed_expr(value))
    },
    ExprKind::Index(object, index, line) => {
      ExprKind::Index(boxed_expr(object), boxed_expr(index), line)
    },
    ExprKind::Slice(object, lower, upper, line) => ExprKind::Slice(
      boxed_expr(object),
      boxed_expr(lower),
      boxed_expr(upper),
      line,
    ),
    ExprKind::List(items) => ExprKind::List(exprs_of(items)),
    ExprKind::Dict(keys, values) => ExprKind::Dict(exprs_of(keys), exprs_of(values)),
    ExprKind::Assign(target, value) => ExprKind::Assign(boxed_expr(target), boxed_expr(value)),
    ExprKind::Anonymous(function) => ExprKind::Anonymous(Box::new(decl(*function))),
    ExprKind::Argument(name, type_hint) => ExprKind::Argument(name, boxed_expr(type_hint)),
    leaf @ (ExprKind::Nil
    | ExprKind::Bool(_)
    | ExprKind::Integer(_)
    | ExprKind::Float(_)
    | ExprKind::BigNumber(_)
    | ExprKind::Literal(_)
    | ExprKind::Identifier(_)
    | ExprKind::Parent(_)
    | ExprKind::Self_(_)
    | ExprKind::TypeHint(..)) => leaf,
  };

  Expr::new(kind, span)
}

fn boxed_expr(e: Box<Expr>) -> Box<Expr> {
  Box::new(expr(*e))
}

fn boxed_stmt(s: Box<Stmt>) -> Box<Stmt> {
  Box::new(stmt(*s))
}

fn exprs_of(items: Vec<Expr>) -> Vec<Expr> {
  items.into_iter().map(expr).collect()
}

fn stmts_of(items: Vec<Stmt>) -> Vec<Stmt> {
  items.into_iter().map(stmt).collect()
}

/// `target = target op operand`, the long form of `target op= operand`
/// and of `++` and `--`. A property is read and written through the same
/// receiver expression, a name or an index through itself.
fn assign_with(span: Span, target: Expr, op: TokenKind, operand: Expr, line: u32) -> Expr {
  let target_span = target.span;

  match target.kind {
    ExprKind::Get(object, name) => {
      let object = boxed_expr(object);
      let current = Expr::new(ExprKind::Get(object.clone(), name.clone()), target_span);
      let updated = ExprKind::Binary(Box::new(current), op, Box::new(operand), line);

      Expr::new(
        ExprKind::Set(object, name, Box::new(Expr::new(updated, span))),
        span,
      )
    },
    kind => {
      let target = expr(Expr::new(kind, target_span));
      let updated = ExprKind::Binary(Box::new(target.clone()), op, Box::new(operand), line);

      Expr::new(
        ExprKind::Assign(Box::new(target), Box::new(Expr::new(updated, span))),
        span,
      )
    },
  }
}

/// An interpolated string as the chain of `+` it compiles to: its
/// leading text, then every `${...}` and every run of text after it,
/// added on in order. A `${...}` takes the line of the text before it.
fn interpolation(span: Span, segments: Vec<Segment>) -> Expr {
  let mut chain: Option<Expr> = None;
  let mut line = 0;

  for segment in segments {
    let (part, part_line) = match segment {
      Segment::Text(token) => {
        line = token.line as u32;
        let text = Expr::new(ExprKind::Literal(text_of(&token)), Span::of(&token));

        (text, line)
      },
      Segment::Value(value) => (expr(value), line),
    };

    chain = Some(match chain {
      None => part,
      Some(chain) => {
        let joined = ExprKind::Binary(Box::new(chain), TokenKind::Plus, Box::new(part), part_line);
        Expr::new(joined, span)
      },
    });
  }

  chain.expect("an interpolated string starts with text")
}

fn text_of(token: &Token) -> String {
  match &token.kind {
    TokenKind::Literal(text) | TokenKind::Interpolation(text) => text.clone(),
    _ => token.to_string(),
  }
}

fn named(token: &Token, name: &str) -> Token {
  token.copy_to(TokenKind::Identifier(name.to_string()))
}

/// Builds the nodes of one lowered construct, every one of them placed
/// at the construct's own span.
struct At(Span);

impl At {
  fn expr(&self, kind: ExprKind) -> Box<Expr> {
    Box::new(Expr::new(kind, self.0))
  }

  fn stmt(&self, kind: StmtKind) -> Stmt {
    Stmt::new(kind, self.0)
  }

  fn id(&self, token: &Token) -> Box<Expr> {
    self.expr(ExprKind::Identifier(token.clone()))
  }

  fn var(&self, name: Token, value: Box<Expr>) -> Stmt {
    self.stmt(StmtKind::Var(name, value, None, false))
  }

  fn assign(&self, target: &Token, value: ExprKind) -> Stmt {
    let assign = ExprKind::Assign(self.id(target), self.expr(value));
    self.stmt(StmtKind::Expression(self.expr(assign)))
  }
}

/// `for key, value in iterable { body }`, or `for value in iterable`
/// with the key kept in a hidden `$key`, as a `while` loop over the
/// iterable's `@key` and `@value` methods.
///
/// The iterable is evaluated once, into `$iter`, before the loop.
/// Reading it inside the loop would evaluate it again on every pass,
/// allocating a list or range literal afresh each time and repeating
/// whatever side effects it has, and a generator's internal state
/// would never advance.
///
/// None of the scaffolding corresponds to a token in the source, so
/// all of it is placed at the loop variable, the nearest honest
/// position for "the for statement". The body is the program's own
/// code and keeps its own positions.
fn for_loop(span: Span, key: Option<Token>, value: Token, iterable: Expr, body: Stmt) -> Stmt {
  let at = At(span);

  let iterable = match (key.is_none(), iterable.kind) {
    (true, ExprKind::Range(lower, upper, line)) => {
      return counted_range_loop(span, value, *lower, *upper, line, body);
    },
    (_, kind) => Expr::new(kind, iterable.span),
  };

  let first = key.clone().unwrap_or_else(|| value.clone());

  let (key_track, key_decl) = match key {
    Some(key) => (key.clone(), at.var(key, at.expr(ExprKind::Nil))),
    None => {
      let hidden = named(&value, "$key");
      (hidden.clone(), at.var(hidden, at.expr(ExprKind::Nil)))
    },
  };

  let value_decl = at.var(value.clone(), at.expr(ExprKind::Nil));

  let iter_name = named(&first, "$iter");
  let iter_decl = at.var(iter_name.clone(), Box::new(iterable));

  // key = $iter.@key(key)
  let key_getter = ExprKind::Get(at.id(&iter_name), named(&key_track, "@key"));
  let next_key = ExprKind::Call(
    at.expr(key_getter),
    vec![*at.id(&key_track)],
    key_track.line as u32,
  );
  let advance = at.assign(&key_track, next_key);

  // if key == nil { break }
  let exhausted = ExprKind::Logical(
    at.id(&key_track),
    TokenKind::EqualEq,
    at.expr(ExprKind::Nil),
    key_track.line as u32,
  );
  let stop = at.stmt(StmtKind::If(
    at.expr(exhausted),
    Box::new(at.stmt(StmtKind::Break(key_track.clone()))),
    None,
  ));

  // value = $iter.@value(key)
  let value_getter = ExprKind::Get(at.id(&iter_name), named(&value, "@value"));
  let next_value = ExprKind::Call(
    at.expr(value_getter),
    vec![*at.id(&key_track)],
    value.line as u32,
  );
  let fetch = at.assign(&value, next_value);

  let loop_body = at.stmt(StmtKind::Block(vec![advance, stop, fetch, body]));
  let looped = at.stmt(StmtKind::While(
    at.expr(ExprKind::Bool(true)),
    Box::new(loop_body),
  ));

  at.stmt(StmtKind::Block(vec![
    iter_decl, key_decl, value_decl, looped,
  ]))
}

/// `for x in LOWER..UPPER { body }` as a counting loop, rather than
/// allocating a range and calling `@key`/`@value` through the generic
/// method dispatch on every pass.
///
/// Only the single-variable form takes this path. A range's keys and
/// values come in a different order once an index is asked for too, so
/// `for k, v in a..b` always goes through `for_loop`'s general path.
fn counted_range_loop(
  span: Span,
  var: Token,
  lower: Expr,
  upper: Expr,
  line: u32,
  body: Stmt,
) -> Stmt {
  let at = At(span);

  let lower_name = named(&var, "$lower");
  let upper_name = named(&var, "$upper");
  let count_name = named(&var, "$count");
  let step_name = named(&var, "$step");
  let n_name = named(&var, "$n");

  // var $lower = LOWER ; var $upper = UPPER, each evaluated once and in
  // the order a range expression would have evaluated them.
  let lower_decl = at.var(lower_name.clone(), Box::new(lower));
  let upper_decl = at.var(upper_name.clone(), Box::new(upper));

  // $upper >= $lower settles the direction, and with it which of the
  // two differences becomes $count.
  let ascending = || {
    ExprKind::Logical(
      at.id(&upper_name),
      TokenKind::GreaterEq,
      at.id(&lower_name),
      line,
    )
  };
  let rising = ExprKind::Binary(
    at.id(&upper_name),
    TokenKind::Minus,
    at.id(&lower_name),
    line,
  );
  let falling = ExprKind::Binary(
    at.id(&lower_name),
    TokenKind::Minus,
    at.id(&upper_name),
    line,
  );

  // var $count = $upper >= $lower ? $upper - $lower : $lower - $upper
  let count_value = ExprKind::Condition(at.expr(ascending()), at.expr(rising), at.expr(falling));
  let count_decl = at.var(count_name.clone(), at.expr(count_value));

  // var $step = $upper >= $lower ? 1 : -1
  let step_value = ExprKind::Condition(
    at.expr(ascending()),
    at.expr(ExprKind::Integer(1)),
    at.expr(ExprKind::Integer(-1)),
  );
  let step_decl = at.var(step_name.clone(), at.expr(step_value));

  // var $n = 0
  let n_decl = at.var(n_name.clone(), at.expr(ExprKind::Integer(0)));

  // var VAR = $lower
  let var_decl = at.var(var.clone(), at.id(&lower_name));

  // VAR += $step and $n += 1 come after the FixContinue, as an `iter`
  // loop's steps do, so a `continue` in the body still advances the
  // loop variable instead of jumping straight back to the condition.
  let var_advance = at.assign(
    &var,
    ExprKind::Binary(at.id(&var), TokenKind::Plus, at.id(&step_name), line),
  );
  let n_advance = at.assign(
    &n_name,
    ExprKind::Binary(
      at.id(&n_name),
      TokenKind::Plus,
      at.expr(ExprKind::Integer(1)),
      line,
    ),
  );

  let loop_body = at.stmt(StmtKind::Block(vec![
    body,
    at.stmt(StmtKind::FixContinue),
    var_advance,
    n_advance,
  ]));

  // while $n < $count { ... }
  let condition = ExprKind::Logical(at.id(&n_name), TokenKind::Less, at.id(&count_name), line);
  let looped = at.stmt(StmtKind::While(at.expr(condition), Box::new(loop_body)));

  at.stmt(StmtKind::Block(vec![
    lower_decl, upper_decl, count_decl, step_decl, n_decl, var_decl, looped,
  ]))
}

/// `iter initializer; condition; steps { body }` as its initializer
/// followed by a `while` whose body ends in the steps. The steps sit
/// after a `FixContinue`, so a `continue` in the body still runs them.
fn iter_loop(
  span: Span,
  initializer: Option<Stmt>,
  condition: Option<Expr>,
  steps: Vec<Expr>,
  body: Stmt,
) -> Stmt {
  let at = At(span);

  let mut loop_body = vec![body, at.stmt(StmtKind::FixContinue)];
  loop_body.extend(steps.into_iter().map(|step| {
    let step_span = step.span;
    Stmt::new(StmtKind::Expression(Box::new(step)), step_span)
  }));

  let condition = condition.unwrap_or_else(|| *at.expr(ExprKind::Bool(true)));
  let looped = at.stmt(StmtKind::While(
    Box::new(condition),
    Box::new(at.stmt(StmtKind::Block(loop_body))),
  ));

  let mut lowered = Vec::new();

  if let Some(initializer) = initializer {
    lowered.push(initializer);
  }

  lowered.push(looped);

  at.stmt(StmtKind::Block(lowered))
}
