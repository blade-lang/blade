# Editor Support

Zuri ships a language server. `zuri lsp` speaks the Language Server
Protocol, so any editor that speaks it too gets completion, navigation,
diagnostics as you type, refactoring, formatting and test running for
Zuri, from the same server that ships with the runtime. The server is
written in Zuri, on the `zuri` module's own parser and compiler, so what
it reports is what the runtime would do with the same code.

Visual Studio Code has an extension that sets everything up. Neovim,
Helix, Sublime Text and Emacs each need a few lines of configuration,
shown at the end of this chapter.

- [What the Server Does](#what-the-server-does)
  - [Writing Code](#writing-code)
  - [Finding Your Way](#finding-your-way)
  - [Diagnostics](#diagnostics)
  - [Changing Code](#changing-code)
  - [Running Tests](#running-tests)
  - [Highlighting](#highlighting)
- [How the Server Reads a Project](#how-the-server-reads-a-project)
- [Settings](#settings)
- [Starting the Server](#starting-the-server)
- [Visual Studio Code](#visual-studio-code)
- [Neovim](#neovim)
- [Helix](#helix)
- [Sublime Text](#sublime-text)
- [Emacs](#emacs)
- [When Something Goes Wrong](#when-something-goes-wrong)

## What the Server Does

### Writing Code

**Completion** offers what can be written at the cursor:

- after a `.`, the members of the receiver's type: an instance's fields and
  methods, inherited ones and the ones extensions add, a class's statics, a
  module's exports, a built-in type's methods, and the keys of a dictionary
  written out where its variable was declared;
- elsewhere, the names in scope, the built-in globals, and the keywords that
  can start what is being written;
- after `import`, module paths, and between an import's braces, what the
  module exports;
- at the start of a class member, `@new` and the operator decorators the
  class does not have yet, as method skeletons;
- after the `:` of a type hint, the type names;
- inside a doc block, its tags after `@`, and after `@param` the names of
  the parameters it documents.

A name another module exports is offered with the import it needs: a module
of the standard library or a package as `import json` and `json.encode`, and
a module of the project as `import .shapes { Point }`.

**Signature help** follows a call while its arguments are written. It marks
the parameter the cursor is on and shows its documentation, follows a
constructor to its `@new`, and stays on a variadic parameter for every
argument it takes.

**Hover** shows a declaration as a line of Zuri, its documentation laid out
the way the standard library reference lays it out, the type a variable is
inferred to hold, the value of a constant, and a module's overview.

**Inlay hints** name the parameter each literal argument goes to, and, when
turned on, the type a variable is inferred to hold.

### Finding Your Way

Go to definition follows an imported name back to the module that declares
it, and a built-in to the stub that documents it. Go to declaration stops at
the import that brings a name into the file, and go to type definition goes
to the class of the value a name holds. Go to implementation lists the
methods of subclasses that override a method, or the subclasses of a class.

Find references and the highlights of the current name go by what each name
resolves to, never by its spelling, so a `distance` method of one class is
never confused with another class's.

The call hierarchy shows what calls a function and what it calls; the type
hierarchy, the classes above and below a class. Each file has an outline,
and the workspace symbol search finds a declaration by the letters of its
name in order, so `hrq` finds `HttpRequest`. Each import links to the file
it loads, and each class, function and method shows how often it is used.

### Diagnostics

Every error the server reports is either an error the compiler reports or a
failure the runtime is certain to raise when the code runs:

- a syntax error, with the recovered rest of the file still checked;
- an error from the compiler, such as `break` outside a loop;
- an import of a module that does not exist, or of a name its module does
  not have;
- `module.name` for a name the module does not have;
- a name nothing declares;
- `self.name = value` outside `@new`, for a field no class in the chain
  declares;
- a literal argument of the wrong type, or a missing one, for a typed
  parameter, with the runtime's own message.

A member that an instance of a fully known class does not have is a
warning, since an extension the server has not read could add it. Unused
imports and declarations, and code after a `return`, `raise`, `break` or
`continue`, are shown faded. A use of anything documented `@deprecated` is
struck through.

The server never guesses. A value whose type cannot be told, a module whose
wildcard imports lead somewhere unread, and a class whose superclass cannot
be found are never diagnosed. Arity is not checked either, because Zuri does
not enforce it.

Quick fixes add the import a missing name needs, correct a misspelt name or
member, remove an unused import, and declare a field set outside `@new`.

### Changing Code

**Rename** changes a declaration and every use of it across the workspace.
A `{ name }` dictionary entry keeps its key and becomes `{ name: renamed }`.
A rename is refused, with the reason, when the new name is a keyword, when
it is already declared where the declaration or a use of it would see it,
when it would make private something used from outside, and for what the
standard library or a package declares. A member used through a receiver
whose type cannot be told is left alone, and the editor is told where.

**Extract and inline** refactorings are offered only where they keep what
the program does:

- *Extract variable* moves an expression into a `var` just before its
  statement, never out of a loop's condition, the right of `and` or `or`, a
  branch of `?:`, a `when` label or an `else if` condition, and never past
  something with a side effect that runs first. Every identical copy in the
  block can share the variable when the expression has no side effect.
- *Extract function* turns whole statements of one block into a function.
  The outer locals they use become its parameters, and what they set and is
  read afterwards comes back: one value directly, several in a dictionary.
  Statements that use `self` or `parent` become a private method.
- *Inline variable* replaces each use of a `var` set once and never again
  with its value.
- *Inline function* replaces a call of a function whose body is a single
  `return` with that expression, and removes the function once every call is
  inlined.

**Formatting** runs `zuri fmt` on a file or a selection and sends back only
the lines that differ. A file with a syntax error is left as it is.

Renaming or moving a file updates the relative imports that reach it, in
the files that import it and in the moved file itself.

### Running Tests

The server finds the tests every file of the workspace declares with the
`test` module: each `describe` and `it`, and their `_only`, `_skip`, `_todo`
and `_each` forms, whose name is written as a string. A lens above each one
runs it, and one above the first runs the whole file.

A test runs as `zuri test` would run it: `zuri run` on its file, in a process
of its own, from the root of the project. Each result reaches the editor as
it happens, and each failed assertion is shown on the line it failed at
until the file changes or its tests run again.

### Highlighting

Semantic highlighting colours each name by what it resolves to: a class, a
function, a method, a parameter, a variable, a field, a module or a
decorator, with whatever the standard library defines marked as such. It
works with any editor theme that colours semantic tokens.

## How the Server Reads a Project

The server reads every `.zu` file of the workspace folders the editor opens,
in the background, on isolates of its own, so it answers while it reads.
Inside a git work tree it reads what git tracks and what git would track,
so `.gitignore` is honoured; anything under `.git`, `.zuri` or
`node_modules` is left out, and so is whatever `zuri.exclude` names.

Imports resolve exactly as the runtime resolves them: a relative import
against the importing file, then the project's `.zuri/libs`, then the
standard library, then the native modules, then the packages installed for
the user in `ZURI_HOME/libs`. The standard library is read from the Zuri
installation the server belongs to, or from `zuri.root`, or from
`ZURI_ROOT`. A module the workspace imports is read the first time it is
needed.

## Settings

The server reads its settings from the `zuri` section of the editor's
configuration, as nested objects or dotted names, and follows changes as
they are made.

| Setting | Default | What it does |
| --- | --- | --- |
| `root` | `''` | The installation whose standard library is read; empty uses `ZURI_ROOT`, or the installation beside `zuri` |
| `exclude` | `[]` | Globs, relative to a workspace folder, of files never read |
| `index.workers` | `2` | Isolates that read the workspace in the background |
| `diagnostics.enable` | `true` | Whether problems are reported at all |
| `diagnostics.scope` | `'openFiles'` | `'openFiles'`, or `'workspace'` for every file of the workspace |
| `diagnostics.delay` | `300` | Milliseconds after a change before a file is checked |
| `diagnostics.unusedHints` | `true` | Whether unused declarations are shown faded |
| `inlayHints.parameterNames` | `'literals'` | Which arguments are named: `'none'`, `'literals'` or `'all'` |
| `inlayHints.variableTypes` | `false` | Whether inferred variable types are shown |
| `codeLens.references` | `true` | Whether declarations show how often they are used |
| `completion.autoImport` | `true` | Whether names from modules not imported yet are offered |
| `completion.callParentheses` | `false` | Whether completing a function writes its parentheses |
| `testing.codeLens` | `true` | Whether suites and tests show lenses that run them |

## Starting the Server

Editors start the server themselves, over its standard input and output:

```sh
zuri lsp [--stdio] [--log <path>] [--log-level <level>]
```

`--stdio` names the only way the server talks, and is accepted because
editors pass it. `--log` appends the server's log to a file as well as
sending it to the editor. `--log-level` sets how much is logged: `error`,
`warn`, `info` (the default), `debug`, or `trace`, which also writes every
message the editor and the server exchange to the log file.

The server exits with `0` when the editor shuts it down in order, and with
`1` when the connection ends without that, or when the editor that started
it has exited.

## Visual Studio Code

Install the **Zuri** extension from the marketplace, or from a downloaded
package:

```sh
code --install-extension zuri-vscode-0.2.0.vsix
```

The extension starts the server for every workspace with a `.zu` file. Its
settings are the server's, under `zuri.`, plus two of its own: `zuri.path`,
the `zuri` executable to run, and `zuri.trace.server`, which records the
messages between the editor and the server in the **Zuri Language Server**
output. Tests appear in the Test Explorer, and the commands **Zuri: Restart
Language Server**, **Zuri: Show Language Server Output** and **Zuri: Check
Zuri Installation** are in the command palette.

## Neovim

Neovim 0.11 configures language servers itself:

```lua
vim.filetype.add({ extension = { zu = 'zuri' } })

vim.lsp.config('zuri', {
  cmd = { 'zuri', 'lsp' },
  filetypes = { 'zuri' },
  root_markers = { 'project.toml', '.git' },
  settings = {
    zuri = {
      inlayHints = { variableTypes = true },
    },
  },
})

vim.lsp.enable('zuri')
```

Inlay hints are shown with `vim.lsp.inlay_hint.enable()`, and a code lens
runs with `vim.lsp.codelens.run()`.

## Helix

Add the language and its server to `languages.toml`:

```toml
[[language]]
name = "zuri"
scope = "source.zuri"
file-types = ["zu"]
roots = ["project.toml"]
comment-token = "#"
block-comment-tokens = { start = "/*", end = "*/" }
indent = { tab-width = 2, unit = "  " }
language-servers = ["zuri-lsp"]

[language-server.zuri-lsp]
command = "zuri"
args = ["lsp"]

[language-server.zuri-lsp.config.zuri]
inlayHints = { parameterNames = "literals" }
```

## Sublime Text

With the LSP package installed, add a client to its settings:

```json
{
  "clients": {
    "zuri": {
      "enabled": true,
      "command": ["zuri", "lsp"],
      "selector": "source.zuri",
      "settings": {
        "zuri.diagnostics.scope": "openFiles"
      }
    }
  }
}
```

The selector names the syntax Zuri files open with. The Visual Studio Code
extension's grammar, `zuri.tmLanguage.json`, is a TextMate grammar whose
scope is `source.zuri`; PackageDev's Convert command turns it into a
`.tmLanguage` file for Sublime Text.

## Emacs

Eglot, built into Emacs 29, needs a mode for Zuri files and the command
that starts the server:

```elisp
(define-derived-mode zuri-mode prog-mode "Zuri"
  "A major mode for Zuri source."
  (setq-local comment-start "# "))

(add-to-list 'auto-mode-alist '("\\.zu\\'" . zuri-mode))

(with-eval-after-load 'eglot
  (add-to-list 'eglot-server-programs '(zuri-mode "zuri" "lsp")))

(add-hook 'zuri-mode-hook #'eglot-ensure)
```

Settings go in `eglot-workspace-configuration`:

```elisp
(setq-default eglot-workspace-configuration
              '(:zuri (:inlayHints (:variableTypes t))))
```

## When Something Goes Wrong

The server sends what it logs at its `--log-level` and above, `info` by
default, to the editor's log for language servers, and while the editor asks
for a trace, everything down to `debug` as well. Starting it with `--log` and
`--log-level debug` writes all of it to a file; `--log-level trace` adds
every message exchanged.

When nothing works at all, the editor cannot start `zuri`: running
`zuri --version` in a terminal shows whether it is on `PATH`, and an editor
that takes a path, as `zuri.path` does in Visual Studio Code, can be given
the executable directly.

When the standard library cannot be found, the server says so in its log.
Setting `zuri.root`, or `ZURI_ROOT` for the editor, to the Zuri installation
fixes it.
