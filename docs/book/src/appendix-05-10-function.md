# Function Methods

Every method on the built-in `function` type, with its signature, what
it returns, and the cases where it does something other than the obvious
thing.

| Method | Returns | Summary |
| --- | --- | --- |
| [`name()`](#name) | `string` | The function's declared name. |
| [`arity()`](#arity) | `number` | How many parameters the function declares, counting a variadic one as a single parameter. |
| [`is_variadic()`](#is_variadic) | `bool` | Whether the last parameter is variadic (`...name`). |
| [`call(...args: list)`](#call) | `any` | Calls the function with the given arguments and returns its result. |
| [`apply(args: list)`](#apply) | `any` | Calls the function with the arguments in a list, and returns its result. |
| [`to_string()`](#to_string) | `string` | The function rendered for display, as `<function NAME(ARITY)>`, with a trailing `...` on the arity when the function is variadic. |

## `name()`

```zuri,ignore
name() -> string
```

The function's declared name. An anonymous function is named `@anon`
followed by a number, counted in the order the compiler met it; a bound
method reports the method's own name, not its class's.

```zuri-repl
%> def named(a, b, ...c) {}
%> named.name()
'named'
%> print.name()
'print'
```

**Returns** `string`

## `arity()`

```zuri,ignore
arity() -> number
```

How many parameters the function declares, counting a variadic one as a
single parameter.

```zuri-repl
%> def named(a, b, ...c) {}
%> named.arity()
3
```

A method read off an instance counts the instance as its first
parameter, so a method declaring two parameters reports `3`. A foreign
function from `ffi` has no receiver and reports its C parameter list.

**Returns** `number`

## `is_variadic()`

```zuri,ignore
is_variadic() -> bool
```

Whether the last parameter is variadic (`...name`).

```zuri-repl
%> def named(a, b, ...c) {}
%> named.is_variadic()
true
```

**Returns** `bool`

## `call()`

```zuri,ignore
call(...args: list) -> any
```

Calls the function with the given arguments and returns its result. The
same as calling it directly; useful when the function is held in a
variable and the call site reads better spelled out.

```zuri-repl
%> def add(a, b) { return a + b }
%> add.call(2, 3)
5
```

**Parameters**

- `args` (`...any`) — The arguments to call with.

**Returns** `any` — Whatever the function returns.

**Raises** anything the called function raises.

## `apply()`

```zuri,ignore
apply(args: list) -> any
```

Calls the function with the arguments in a list, and returns its result.
`call()` takes them written out; this one takes them in a list.

```zuri-repl
%> def add(a, b) { return a + b }
%> add.apply([2, 3])
5
%> def collect(first, ...rest) { return [first, rest] }
%> collect.apply([1, 2, 3])
[1, [2, 3]]
```

A list shorter than the function's arity leaves the remaining parameters
`nil`, exactly as calling it directly with too few arguments does; a
longer one overflows into a variadic parameter, or is discarded when
there is none.

**Parameters**

- `args` (`list`) — The arguments to call with, in order.

**Returns** `any` — Whatever the function returns.

**Raises** `TypeError` when `args` is not a list, and anything the
called function raises.

## `to_string()`

```zuri,ignore
to_string() -> string
```

The function rendered for display, as `<function NAME(ARITY)>`, with a
trailing `...` on the arity when the function is variadic.

```zuri-repl
%> def named(a, b, ...c) {}
%> named.to_string()
'<function named(3...)>'
```

**Returns** `string`
