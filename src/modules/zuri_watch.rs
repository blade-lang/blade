//! `_zuri_watch` builtin module; the native backing for `zuri.watch`.
//!
//! What a running program tells `zuri run --watch` about the files it
//! depends on. The report itself lives in `crate::watch`.

use crate::builtins::enforce::ArgType;
use crate::enforce_arg_count;
use crate::enforce_arg_type;
use crate::modules::{BuiltinModuleDef, native};
use crate::vm::object::ZuriContext;
use crate::vm::value::Value;
use crate::vm::vm::VM;

pub static MODULE: BuiltinModuleDef = BuiltinModuleDef {
  name: "_zuri_watch",
  build,
};

fn build(vm: &mut VM) -> Vec<(&'static str, Value)> {
  vec![
    (
      "depends_on",
      native(vm, "depends_on", 1, false, depends_on_fn),
    ),
    ("active", native(vm, "active", 0, false, active_fn)),
  ]
}

fn depends_on_fn(ctx: &mut ZuriContext) -> Result<Value, String> {
  enforce_arg_count!(ctx, 1);
  enforce_arg_type!(ctx, 0, ArgType::String);

  let path = ctx.args[0].as_str().to_string();
  Ok(Value::bool(crate::watch::depends_on(&path)))
}

fn active_fn(ctx: &mut ZuriContext) -> Result<Value, String> {
  enforce_arg_count!(ctx, 0);
  Ok(Value::bool(crate::watch::active()))
}
