# fmt

Formats all Zuri files in the given path, or the current directory, in
the project's own style: two spaces to a level, one statement to a line,
explicit blocks, the spacing a reader expects between tokens, and long
lines broken where their expressions hold together least.

## Running it

```sh
zuri fmt [path]
```

`path` is either a single `.zu` file, which is formatted in place, or a
directory, in which case every `.zu` file inside it is formatted,
subdirectories included. If no path is given, the current directory is
formatted.

```sh
# format one file
zuri fmt libs/date.zu

# format a whole tree
zuri fmt libs

# format the current directory
zuri fmt

# report what would change without writing anything (exit 1 if any would)
zuri fmt --dry-run libs
```

`--dry-run` (or `-n`) is what a commit hook or CI wants: it names the
files that are not already formatted and exits non-zero if there are any.

## What it does

It reads the program the way the compiler does, from its parse tree,
and lays every statement, declaration and expression out in the
project's style. It knows what each piece of code is, so a sign is never
mistaken for an operator, a type's `:` for a dictionary's, or a
subscript for a list.

- **Keeps the lines you wrote** while they fit. A list opened out one
  item to a line, a concatenation carried across several, a call whose
  arguments were spread on purpose: each keeps the breaks it was given
  and is re-indented, never packed back together. A collection written
  on one line stays on one line while it fits. A blank line between two
  statements is kept, one at most.
- **Realigns a list that runs past the width.** Once a line of a list
  you broke holds several items and no longer fits, the whole list is
  laid out again, as many items to a line as fit. A line holding a
  single item too long for any line does not count, since nothing could
  shorten it, and the comments and blank lines between items stay
  where they are. This holds for lists, dictionaries, arguments,
  parameters and an import's names.
- **Indents** two spaces per level of nesting. Everything after a
  statement's first line sits one level under it, however many breaks
  it takes, and a bracket's closer returns to the line its opener is on.
- **Opens every block.** The statements inside a block each get their
  own line, and `;` between two statements becomes a line break. An
  empty block stays as it was written, `{}` or open across two lines.
  `else`, `while` after a `do`, and `as` after a `catch` follow the `}`
  before them on its line.
- **Braces every body.** `if x y = 1` becomes a block, and so does the
  body of an `else`, a `while`, a `for` and a `do`. A body that declares
  a name, `if x var y = 1`, is left as it is, since braces would give the
  name a scope of its own. A `when` or `default` arm keeps an unbraced
  body on its line while the line is at most 60 characters wide, and
  braces it otherwise.
- **Sets declarations apart.** A function, method or class that spans
  several lines has a blank line on either side of it, ahead of the
  comments that lead into it.
- **Spaces** by what each token is: around binary operators, after
  commas, none around `.` or inside a call's parentheses, none around `|`
  in a type, `{ a: 1 }` with spaces inside its braces. An empty `iter`
  clause keeps the space that shows it is there: `iter ; i < n; i++`.
- **Breaks a long line** where its expression holds together least: at
  every `or` of a condition before any `and`, before the `?` and `:` of a
  conditional, before each call of a method chain with two or more, or
  across the brackets of a call or a literal, as many items to a line as
  fit. A dictionary made up mostly of `key: value` pairs takes one pair
  to a line instead, whenever it does not stay on one line, and so does
  one you broke yourself. A broken operator chain leads each line with its operator, except
  a sum holding a `-`, which trails them, since a line opening with `-`
  is a statement of its own. A line breaks only where the parser accepts
  a break and only when the break brings it within the width; one with
  nowhere useful to break is left as it was written.
- **Adds a trailing comma** where a list, dictionary, argument or
  parameter list ends with its closer on a line of its own, so a later
  edit touches one line rather than two. One kept on a single line has
  none.
- **Never adds or drops a subscript's commas.** `s[1]`, `s[1,]`, `s[,3]`
  and `s[1,3]` are four different reads, written `s[1]`, `s[1,]`,
  `s[,3]` and `s[1, 3]`.
- **Keeps comments where they were written**: one that trails code stays
  at the end of its line, one on a line of its own keeps that line, and
  a doc block moves along with the code it documents.
- **Writes strings and numbers exactly as the source does**, and import
  paths with their dots as written: `.. ..shared` and `....shared` both
  stay.
- **Rewrites `name: name` to `name`** inside a dictionary, the shorthand
  the project prefers, and drops what the language allows but the
  project never writes: the parentheses an `iter` may be written with,
  and `def` before a method name.

## What it will not do

It never changes what a program does. Before writing a file, it parses
its own result and checks that the tree is the one it started from and
that every comment is still there, in order. The only differences it
accepts are its own rewrites: the dictionary shorthand, and a body it
braced. If anything else differs, that file is left as it was and
reported as skipped. This is why it can be run across a whole tree
without fear.

A file that does not parse is skipped too, with the line and column
of the first syntax error: there is no tree to lay out.

## How it is built

- `source.zu`: the tokens of the source, for what the tree does not
  keep: where the author broke lines, every comment, and the operators
  and commas between two nodes.
- `doc.zu`: the layout document, text and the places it may break.
- `layout.zu`: the parse tree turned into a layout document, in source
  order so every comment is placed where it was written.
- `printer.zu`: the document turned into lines, each group kept on one
  line when it fits.
- `verify.zu`: the comparison of two trees and their comments.
- `engine.zu`: `format(source)` and the checks around it.
- `index.zu`: the command itself, the entry point `zuri fmt` runs.

The parser is Zuri's own (`zuri.parse` and `zuri.tokenize`), not a
second one to keep in step with the language.

## Tests

A test file sits beside each source file under `tests/`, written with
the `test` module. Run the lot with:

```sh
zuri test cmds/fmt/tests
```

or, from this directory, just `zuri test`.
