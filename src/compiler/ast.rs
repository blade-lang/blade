//! The syntax tree the parser builds and the compiler reads.
//!
//! Every expression, statement and declaration is a `kind` plus the
//! `Span` of source it was read from, so anything holding a node can
//! point back at exactly the text it came from. The tree keeps the
//! program the way it was written: loops, compound assignment and the
//! other shorthands stay as themselves until `lower` rewrites them for
//! the compiler.

use crate::compiler::token::{Token, TokenKind};
use num_bigint::BigInt;

/// A stretch of source, as the char offsets of its first character and
/// of the one just past its last. These are the units of `Token::start`
/// and `Token::end`.
#[derive(Debug, Clone, Copy, PartialEq, Eq, Default)]
pub struct Span {
  pub start: usize,
  pub end: usize,
}

impl Span {
  pub fn new(start: usize, end: usize) -> Self {
    Self { start, end }
  }

  /// Exactly the text of one token.
  pub fn of(token: &Token) -> Self {
    Self::new(token.start, token.end)
  }

  /// From the start of this span to the end of `other`.
  pub fn to(self, other: Span) -> Self {
    Self::new(self.start, other.end.max(self.start))
  }
}

/// An expression and the source it was read from.
#[derive(Debug, Clone, PartialEq)]
pub struct Expr {
  pub kind: ExprKind,
  pub span: Span,
}

impl Expr {
  pub fn new(kind: ExprKind, span: Span) -> Self {
    Self { kind, span }
  }
}

/// A statement and the source it was read from.
#[derive(Debug, Clone, PartialEq)]
pub struct Stmt {
  pub kind: StmtKind,
  pub span: Span,
}

impl Stmt {
  pub fn new(kind: StmtKind, span: Span) -> Self {
    Self { kind, span }
  }
}

/// A declaration and the source it was read from.
#[derive(Debug, Clone, PartialEq)]
pub struct Decl {
  pub kind: DeclKind,
  pub span: Span,
}

impl Decl {
  pub fn new(kind: DeclKind, span: Span) -> Self {
    Self { kind, span }
  }

  pub fn is_method(&self, name: &str) -> bool {
    match &self.kind {
      DeclKind::Method(token, ..) => match token.kind.clone() {
        TokenKind::Identifier(token_name) => token_name.eq(name),
        TokenKind::Decorator(token_name) => token_name.eq(name),
        _ => false,
      },
      _ => false,
    }
  }
}

#[derive(Debug, Clone, PartialEq)]
pub enum Type {
  Any,
  Bool,
  Int,
  Number,
  BigInt,
  String,
  Bytes,
  List,
  Dict,
  Range,
  File,
  Function,
  Type,
  Callable,
  Iterable,
  Instance(Token),
}

/// One name in a type hint, `string` or `Point` in `x: ?string|Point`,
/// and where it was written.
#[derive(Debug, Clone, PartialEq)]
pub struct TypeName {
  pub ty: Type,
  pub span: Span,
}

#[derive(Debug, Clone, PartialEq)]
pub enum ExprKind {
  Nil,
  Bool(bool),
  Integer(i64),
  Float(f64),
  BigNumber(BigInt),
  Literal(String),

  /*  Trailing `u32` on the six variants below is the source line of
   the operator/delimiter that makes this node fallible (the `+`,
   the `(`, the `[`, the `..`); not the node's start. Read by the
   compiler at the exact point it emits that node's own instruction,
   after operands are already compiled, so nothing an operand's own
   compilation does to intervening state can clobber it. Get/Set/
   Identifier need no such field; they already carry a `Token`
   with its own `.line`.
  */
  Unary(TokenKind, Box<Expr>, u32),
  Binary(Box<Expr>, TokenKind, Box<Expr>, u32),
  Logical(Box<Expr>, TokenKind, Box<Expr>, u32),
  Circuit(Box<Expr>, TokenKind, Box<Expr>),
  Grouping(Box<Expr>),
  Range(Box<Expr>, Box<Expr>, u32),
  Identifier(Token),
  Condition(Box<Expr>, Box<Expr>, Box<Expr>),
  Call(Box<Expr>, Vec<Expr>, u32),
  Get(Box<Expr>, Token),
  Set(Box<Expr>, Token, Box<Expr>),
  Index(Box<Expr>, Box<Expr>, u32),
  Slice(Box<Expr>, Box<Expr>, Box<Expr>, u32),
  List(Vec<Expr>),
  Dict(Vec<Expr>, Vec<Expr>),
  // `parent` and `self`, each with its keyword.
  Parent(Token),
  Self_(Token),
  Assign(Box<Expr>, Box<Expr>),
  Anonymous(Box<Decl>),
  TypeHint(Vec<TypeName>, bool),
  Argument(Token, Box<Expr>),

  // `target op= value` for any compound assignment operator, kept as the
  // operator written (`+=`, not `+`). The target is a name, an index or a
  // property, and the `u32` is the line the arithmetic is blamed on.
  CompoundAssign(Box<Expr>, TokenKind, Box<Expr>, u32),

  // `target++` or `target--`, with the operator's line.
  Update(Box<Expr>, TokenKind, u32),

  // A string with `${...}` in it, as the text and the expressions it
  // alternates between, in source order. It always starts with text.
  Interpolation(Vec<Segment>),

  // A dictionary value written as nothing but its key, the `name` in
  // `{ name }`. It reads the variable of that name.
  Shorthand(Token),
}

/// One piece of an interpolated string.
#[derive(Debug, Clone, PartialEq)]
pub enum Segment {
  /// Text from the string itself, as the token the lexer produced for it.
  Text(Token),
  /// The expression inside one `${...}`.
  Value(Expr),
}

/// One `when` arm of a `using` statement: every label it lists and the
/// body they share, with the span from its `when` to the end of the body.
#[derive(Debug, Clone, PartialEq)]
pub struct WhenArm {
  pub labels: Vec<Expr>,
  pub body: Stmt,
  pub span: Span,
}

#[derive(Debug, Clone, PartialEq)]
pub enum StmtKind {
  None,
  FixContinue,
  Echo(Box<Expr>),
  Expression(Box<Expr>),
  If(Box<Expr>, Box<Stmt>, Option<Box<Stmt>>),
  While(Box<Expr>, Box<Stmt>),

  // `for value in iterable` or `for key, value in iterable`, with the
  // key absent in the first form. The compiler lowers it to a `while`
  // over the iterable's `@key` and `@value` methods.
  For(Option<Token>, Token, Box<Expr>, Box<Stmt>),

  // `iter initializer; condition; steps { body }`. Every clause may be
  // left empty, and a missing condition loops until something breaks.
  Iter(Option<Box<Stmt>>, Option<Box<Expr>>, Vec<Expr>, Box<Stmt>),

  // `do body while condition`, whose body runs once before the
  // condition is first tested.
  DoWhile(Box<Stmt>, Box<Expr>),

  // `continue` and `break`, each with its keyword.
  Continue(Token),
  Break(Token),
  Raise(Box<Expr>),

  // The `return` keyword, or the `->` of a one-expression anonymous
  // function, and the value returned.
  Return(Token, Box<Expr>),
  Assert(Box<Expr>, Option<Box<Expr>>),
  Using(Box<Expr>, Vec<WhenArm>, Option<Box<Stmt>>),

  // The joined path, the tokens it was joined from (`.`, `..` and names),
  // the name it binds, the items it picks, whether it takes them all, and
  // whether it re-exports what it brings in.
  Import(String, Vec<Token>, Box<Expr>, Vec<Expr>, bool, bool),
  Catch(Box<Stmt>, Option<Box<Stmt>>, Option<Box<Expr>>),
  Block(Vec<Stmt>),
  Decl(Box<Decl>),
  Var(Token, Box<Expr>, Option<Box<Expr>>, bool),
  VarList(Vec<Stmt>),

  // A standalone comment or doc block, sitting between two statements at
  // its original lexical position. The compiler never emits anything for
  // these; they exist purely so `zuri.parse()` can hand back an AST that
  // still has every comment in it, in place, for the `ast` module. See
  // the matching `DeclKind::Trivia` for the top-level/class-body
  // equivalent.
  Trivia(Token),
}

#[derive(Debug, Clone, PartialEq)]
pub enum DeclKind {
  Stmt(Box<Stmt>),
  Function(Token, Vec<Expr>, Box<Stmt>, bool),
  Method(Token, Vec<Expr>, Box<Stmt>, bool, bool),
  Property(Token, Box<Expr>, Box<Expr>, bool, bool),
  Class(Token, Option<Box<Expr>>, Vec<Decl>, Vec<Decl>, bool),

  // Same purpose as `StmtKind::Trivia`, at top level and inside class
  // bodies.
  Trivia(Token),
}
