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
//! Every name made up here starts with `$`, which the lexer never
//! produces from source, so none of them can collide with a name the
//! program declares.

use crate::compiler::ast::{Decl, Expr, Segment, Stmt, WhenArm};
use crate::compiler::token::{Token, TokenKind, get_assignment_alt};

/// Lowers a whole program, including the bodies of its functions,
/// methods, classes and anonymous functions.
pub fn lower(decls: Vec<Decl>) -> Vec<Decl> {
  decls.into_iter().map(decl).collect()
}

fn decl(d: Decl) -> Decl {
  match d {
    Decl::Stmt(s) => Decl::Stmt(boxed_stmt(s)),
    Decl::Block(stmts) => Decl::Block(stmts_of(stmts)),
    Decl::Import(path, name, elements, exported) => {
      Decl::Import(path, boxed_expr(name), exprs_of(elements), exported)
    },
    Decl::Function(name, params, body, is_variadic) => {
      Decl::Function(name, exprs_of(params), boxed_stmt(body), is_variadic)
    },
    Decl::Method(name, params, body, is_variadic, is_static) => Decl::Method(
      name,
      exprs_of(params),
      boxed_stmt(body),
      is_variadic,
      is_static,
    ),
    Decl::Property(name, value, type_hint, is_static, is_constant) => Decl::Property(
      name,
      boxed_expr(value),
      boxed_expr(type_hint),
      is_static,
      is_constant,
    ),
    Decl::Class(name, superclass, properties, methods, is_extension) => Decl::Class(
      name,
      superclass.map(boxed_expr),
      properties.into_iter().map(decl).collect(),
      methods.into_iter().map(decl).collect(),
      is_extension,
    ),
    Decl::None => Decl::None,
    Decl::Trivia(token) => Decl::Trivia(token),
  }
}

fn stmt(s: Stmt) -> Stmt {
  match s {
    Stmt::For(key, value, iterable, body) => for_loop(key, value, expr(*iterable), stmt(*body)),
    Stmt::Iter(initializer, condition, steps, body) => iter_loop(
      initializer.map(|s| stmt(*s)),
      condition.map(|e| expr(*e)),
      exprs_of(steps),
      stmt(*body),
    ),
    Stmt::DoWhile(body, condition) => {
      let body = stmt(*body);

      Stmt::Block(vec![
        body.clone(),
        Stmt::While(boxed_expr(condition), Box::new(body)),
      ])
    },
    Stmt::Echo(e) => Stmt::Echo(boxed_expr(e)),
    Stmt::Expression(e) => Stmt::Expression(boxed_expr(e)),
    Stmt::If(condition, then_branch, else_branch) => Stmt::If(
      boxed_expr(condition),
      boxed_stmt(then_branch),
      else_branch.map(boxed_stmt),
    ),
    Stmt::While(condition, body) => Stmt::While(boxed_expr(condition), boxed_stmt(body)),
    Stmt::Raise(e) => Stmt::Raise(boxed_expr(e)),
    Stmt::Return(e) => Stmt::Return(boxed_expr(e)),
    Stmt::Assert(condition, message) => {
      Stmt::Assert(boxed_expr(condition), message.map(boxed_expr))
    },
    Stmt::Using(subject, arms, default) => Stmt::Using(
      boxed_expr(subject),
      arms
        .into_iter()
        .map(|arm| WhenArm {
          labels: exprs_of(arm.labels),
          body: stmt(arm.body),
        })
        .collect(),
      default.map(boxed_stmt),
    ),
    Stmt::Import(path, path_tokens, name, elements, imports_all, exported) => Stmt::Import(
      path,
      path_tokens,
      boxed_expr(name),
      exprs_of(elements),
      imports_all,
      exported,
    ),
    Stmt::Catch(body, handler, error_var) => Stmt::Catch(
      boxed_stmt(body),
      handler.map(boxed_stmt),
      error_var.map(boxed_expr),
    ),
    Stmt::Block(stmts) => Stmt::Block(stmts_of(stmts)),
    Stmt::Decl(d) => Stmt::Decl(Box::new(decl(*d))),
    Stmt::Var(name, value, type_hint, is_constant) => Stmt::Var(
      name,
      boxed_expr(value),
      type_hint.map(boxed_expr),
      is_constant,
    ),
    Stmt::VarList(stmts) => Stmt::VarList(stmts_of(stmts)),
    Stmt::None => Stmt::None,
    Stmt::FixContinue => Stmt::FixContinue,
    Stmt::Continue => Stmt::Continue,
    Stmt::Break => Stmt::Break,
    Stmt::Trivia(token) => Stmt::Trivia(token),
  }
}

fn expr(e: Expr) -> Expr {
  match e {
    Expr::Unary(op, operand, line) => Expr::Unary(op, boxed_expr(operand), line),
    Expr::Binary(left, op, right, line) => {
      Expr::Binary(boxed_expr(left), op, boxed_expr(right), line)
    },
    Expr::Logical(left, op, right, line) => {
      Expr::Logical(boxed_expr(left), op, boxed_expr(right), line)
    },
    Expr::Circuit(left, op, right) => Expr::Circuit(boxed_expr(left), op, boxed_expr(right)),
    Expr::Grouping(inner) => Expr::Grouping(boxed_expr(inner)),
    Expr::Range(lower, upper, line) => Expr::Range(boxed_expr(lower), boxed_expr(upper), line),
    Expr::Condition(condition, truth, falsy) => {
      Expr::Condition(boxed_expr(condition), boxed_expr(truth), boxed_expr(falsy))
    },
    Expr::Call(callee, args, line) => Expr::Call(boxed_expr(callee), exprs_of(args), line),
    Expr::Get(object, name) => Expr::Get(boxed_expr(object), name),
    Expr::Set(object, name, value) => Expr::Set(boxed_expr(object), name, boxed_expr(value)),
    Expr::Index(object, index, line) => Expr::Index(boxed_expr(object), boxed_expr(index), line),
    Expr::Slice(object, lower, upper, line) => Expr::Slice(
      boxed_expr(object),
      boxed_expr(lower),
      boxed_expr(upper),
      line,
    ),
    Expr::List(items) => Expr::List(exprs_of(items)),
    Expr::Dict(keys, values) => Expr::Dict(exprs_of(keys), exprs_of(values)),
    Expr::Assign(target, value) => Expr::Assign(boxed_expr(target), boxed_expr(value)),
    Expr::Anonymous(function) => Expr::Anonymous(Box::new(decl(*function))),
    Expr::Argument(name, type_hint) => Expr::Argument(name, boxed_expr(type_hint)),
    Expr::CompoundAssign(target, op, value, line) => {
      assign_with(*target, get_assignment_alt(op), expr(*value), line)
    },
    Expr::Update(target, op, line) => {
      let op = match op {
        TokenKind::Increment => TokenKind::Plus,
        _ => TokenKind::Minus,
      };

      assign_with(*target, op, Expr::Integer(1), line)
    },
    Expr::Interpolation(segments) => interpolation(segments),
    Expr::Shorthand(name) => Expr::Identifier(name),
    leaf @ (Expr::Nil
    | Expr::Bool(_)
    | Expr::Integer(_)
    | Expr::Float(_)
    | Expr::BigNumber(_)
    | Expr::Literal(_)
    | Expr::Identifier(_)
    | Expr::Parent
    | Expr::Self_
    | Expr::TypeHint(..)) => leaf,
  }
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
fn assign_with(target: Expr, op: TokenKind, operand: Expr, line: u32) -> Expr {
  match target {
    Expr::Get(object, name) => {
      let object = boxed_expr(object);
      let current = Expr::Get(object.clone(), name.clone());
      let updated = Expr::Binary(Box::new(current), op, Box::new(operand), line);

      Expr::Set(object, name, Box::new(updated))
    },
    target => {
      let target = expr(target);
      let updated = Expr::Binary(Box::new(target.clone()), op, Box::new(operand), line);

      Expr::Assign(Box::new(target), Box::new(updated))
    },
  }
}

/// An interpolated string as the chain of `+` it compiles to: its
/// leading text, then every `${...}` and every run of text after it,
/// added on in order. A `${...}` takes the line of the text before it.
fn interpolation(segments: Vec<Segment>) -> Expr {
  let mut chain: Option<Expr> = None;
  let mut line = 0;

  for segment in segments {
    let (part, part_line) = match segment {
      Segment::Text(token) => {
        line = token.line as u32;
        (Expr::Literal(text_of(&token)), line)
      },
      Segment::Value(value) => (expr(value), line),
    };

    chain = Some(match chain {
      None => part,
      Some(chain) => Expr::Binary(Box::new(chain), TokenKind::Plus, Box::new(part), part_line),
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

fn id(token: &Token) -> Box<Expr> {
  Box::new(Expr::Identifier(token.clone()))
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
fn for_loop(key: Option<Token>, value: Token, iterable: Expr, body: Stmt) -> Stmt {
  let iterable = match (key.is_none(), iterable) {
    (true, Expr::Range(lower, upper, line)) => {
      return counted_range_loop(value, *lower, *upper, line, body);
    },
    (_, iterable) => iterable,
  };

  let first = key.clone().unwrap_or_else(|| value.clone());

  let (key_track, key_decl) = match key {
    Some(key) => (
      key.clone(),
      Stmt::Var(key, Box::new(Expr::Nil), None, false),
    ),
    None => {
      let hidden = named(&value, "$key");
      (
        hidden.clone(),
        Stmt::Var(hidden, Box::new(Expr::Nil), None, false),
      )
    },
  };

  let value_decl = Stmt::Var(value.clone(), Box::new(Expr::Nil), None, false);

  let iter_name = named(&first, "$iter");
  let iter_decl = Stmt::Var(iter_name.clone(), Box::new(iterable), None, false);

  // key = $iter.@key(key)
  let key_getter = Expr::Get(id(&iter_name), named(&key_track, "@key"));
  let next_key = Expr::Call(
    Box::new(key_getter),
    vec![Expr::Identifier(key_track.clone())],
    key_track.line as u32,
  );
  let advance = Stmt::Expression(Box::new(Expr::Assign(id(&key_track), Box::new(next_key))));

  // if key == nil { break }
  let exhausted = Expr::Logical(
    id(&key_track),
    TokenKind::EqualEq,
    Box::new(Expr::Nil),
    key_track.line as u32,
  );
  let stop = Stmt::If(Box::new(exhausted), Box::new(Stmt::Break), None);

  // value = $iter.@value(key)
  let value_getter = Expr::Get(id(&iter_name), named(&value, "@value"));
  let next_value = Expr::Call(
    Box::new(value_getter),
    vec![Expr::Identifier(key_track.clone())],
    value.line as u32,
  );
  let fetch = Stmt::Expression(Box::new(Expr::Assign(id(&value), Box::new(next_value))));

  let loop_body = Stmt::Block(vec![advance, stop, fetch, body]);
  let looped = Stmt::While(Box::new(Expr::Bool(true)), Box::new(loop_body));

  Stmt::Block(vec![iter_decl, key_decl, value_decl, looped])
}

/// `for x in LOWER..UPPER { body }` as a counting loop, rather than
/// allocating a range and calling `@key`/`@value` through the generic
/// method dispatch on every pass.
///
/// Only the single-variable form takes this path. A range's keys and
/// values come in a different order once an index is asked for too, so
/// `for k, v in a..b` always goes through `for_loop`'s general path.
fn counted_range_loop(var: Token, lower: Expr, upper: Expr, line: u32, body: Stmt) -> Stmt {
  let lower_name = named(&var, "$lower");
  let upper_name = named(&var, "$upper");
  let count_name = named(&var, "$count");
  let step_name = named(&var, "$step");
  let n_name = named(&var, "$n");

  // var $lower = LOWER ; var $upper = UPPER, each evaluated once and in
  // the order a range expression would have evaluated them.
  let lower_decl = Stmt::Var(lower_name.clone(), Box::new(lower), None, false);
  let upper_decl = Stmt::Var(upper_name.clone(), Box::new(upper), None, false);

  // $upper >= $lower settles the direction, and with it which of the
  // two differences becomes $count.
  let ascending = Expr::Logical(id(&upper_name), TokenKind::GreaterEq, id(&lower_name), line);
  let rising = Expr::Binary(id(&upper_name), TokenKind::Minus, id(&lower_name), line);
  let falling = Expr::Binary(id(&lower_name), TokenKind::Minus, id(&upper_name), line);

  // var $count = $upper >= $lower ? $upper - $lower : $lower - $upper
  let count_value = Expr::Condition(Box::new(ascending), Box::new(rising), Box::new(falling));
  let count_decl = Stmt::Var(count_name.clone(), Box::new(count_value), None, false);

  // var $step = $upper >= $lower ? 1 : -1
  let ascending = Expr::Logical(id(&upper_name), TokenKind::GreaterEq, id(&lower_name), line);
  let step_value = Expr::Condition(
    Box::new(ascending),
    Box::new(Expr::Integer(1)),
    Box::new(Expr::Integer(-1)),
  );
  let step_decl = Stmt::Var(step_name.clone(), Box::new(step_value), None, false);

  // var $n = 0
  let n_decl = Stmt::Var(n_name.clone(), Box::new(Expr::Integer(0)), None, false);

  // var VAR = $lower
  let var_decl = Stmt::Var(var.clone(), id(&lower_name), None, false);

  // VAR += $step and $n += 1 come after the FixContinue, as an `iter`
  // loop's steps do, so a `continue` in the body still advances the
  // loop variable instead of jumping straight back to the condition.
  let var_advance = Stmt::Expression(Box::new(Expr::Assign(
    id(&var),
    Box::new(Expr::Binary(
      id(&var),
      TokenKind::Plus,
      id(&step_name),
      line,
    )),
  )));
  let n_advance = Stmt::Expression(Box::new(Expr::Assign(
    id(&n_name),
    Box::new(Expr::Binary(
      id(&n_name),
      TokenKind::Plus,
      Box::new(Expr::Integer(1)),
      line,
    )),
  )));

  let loop_body = Stmt::Block(vec![body, Stmt::FixContinue, var_advance, n_advance]);

  // while $n < $count { ... }
  let condition = Expr::Logical(id(&n_name), TokenKind::Less, id(&count_name), line);
  let looped = Stmt::While(Box::new(condition), Box::new(loop_body));

  Stmt::Block(vec![
    lower_decl, upper_decl, count_decl, step_decl, n_decl, var_decl, looped,
  ])
}

/// `iter initializer; condition; steps { body }` as its initializer
/// followed by a `while` whose body ends in the steps. The steps sit
/// after a `FixContinue`, so a `continue` in the body still runs them.
fn iter_loop(
  initializer: Option<Stmt>,
  condition: Option<Expr>,
  steps: Vec<Expr>,
  body: Stmt,
) -> Stmt {
  let mut loop_body = vec![body, Stmt::FixContinue];
  loop_body.extend(
    steps
      .into_iter()
      .map(|step| Stmt::Expression(Box::new(step))),
  );

  let condition = condition.unwrap_or(Expr::Bool(true));
  let looped = Stmt::While(Box::new(condition), Box::new(Stmt::Block(loop_body)));

  let mut lowered = Vec::new();

  if let Some(initializer) = initializer {
    lowered.push(initializer);
  }

  lowered.push(looped);

  Stmt::Block(lowered)
}
