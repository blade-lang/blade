# lsp

The Zuri language server. It speaks the Language Server Protocol over
standard input and output, so any editor that speaks it gets completion,
navigation, diagnostics as you type, refactoring, formatting and test
running for Zuri. The Editor Support chapter of the book describes every
feature and shows how to set up each editor.

## Running it

Editors start it themselves:

```sh
zuri lsp [--stdio] [--log <path>] [--log-level <level>]
```

`--stdio` is accepted because editors pass it; standard input and output
are the only way the server talks. `--log` appends the log to a file as
well as sending it to the editor. `--log-level` is `error`, `warn`, `info`
(the default), `debug`, or `trace`, which also writes every message
exchanged to the log file.

The process exits with `0` after an orderly `shutdown` and `exit`, and
with `1` when the connection ends otherwise or the editor that started it
exits.

## How it works

One isolate owns all of the server's state and answers every message.
Others read standard input, index the files nobody has open, lay out files
for formatting, and run tests; each reports back over a channel, and the
owning isolate waits on all of them and its own timers at once. Before it
handles anything it takes every message that has arrived, so a request
cancelled while it waited is answered as cancelled without being run, and
an answer about a document that changed underneath it is answered as
modified.

A file is understood in three steps. `zuri.parse_partial()` reads it into
a syntax tree, recovering from syntax errors. The walk in
`analysis/scope.zu` resolves every name the way the compiler does and
keeps the result as a summary of plain lists and dictionaries, which is
what lets an indexing isolate build one and send it across a channel. The
linker in `analysis/linker.zu` answers what no single file can: what a
module exports, what an imported name stands for, what a deferred type
turns out to be, and which members a class has once its superclasses and
extensions are counted.

Imports resolve in the runtime's own order (`resolve.zu`), and the
built-in globals and types come from the stubs beside the standard
library (`analysis/builtins.zu`), which the runtime's own tests hold to
what it defines.

## Layout

- `index.zu`: the command, its flags and the log set-up.
- `server.zu`: the loop, the lifecycle, documents, diagnostics timing,
  indexing, settings, formatting jobs and test runs.
- `state.zu`: open documents and the settings.
- `text.zu`: line indexes, the three position encodings, incremental
  edits, URIs, and the line diff formatting answers with.
- `workspace.zu`: the files of a workspace folder.
- `resolve.zu`: import resolution.
- `logging.zu`: the server's log, on the `log` module.
- `indexer.zu`, `formatter.zu`: the isolates that index and format.
- `analysis/`: the summary of a file (`summary.zu`, `scope.zu`,
  `infer.zu`), the linker, the built-ins, questions about a position
  (`query.zu`), and doc blocks as markdown (`render.zu`).
- `features/`: one file for each part of the protocol the server answers.

## Tests

The suite in `tests/` drives a real `zuri lsp` through `rpc`, as an
editor does, against the small project in `tests/_fixture/`:

```sh
zuri test cmds/lsp/tests
```
