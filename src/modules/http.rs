//! `_http` builtin module: high-performance native HTTP request head parsing.
//!
//! Powered by `httparse` with SIMD-accelerated zero-copy parsing of HTTP/1.x
//! request lines and headers directly from socket byte buffers.

use smallvec::{SmallVec, smallvec};
use std::borrow::Cow;

use crate::builtins::enforce::ArgType;
use crate::modules::{BuiltinModuleDef, native};
use crate::vm::object::ZuriContext;
use crate::vm::value::Value;
use crate::vm::vm::VM;
use crate::{enforce_arg_count, enforce_arg_type, enforce_arg_type_any_of};

pub static MODULE: BuiltinModuleDef = BuiltinModuleDef {
  name: "_http",
  build,
};

fn build(vm: &mut VM) -> Vec<(&'static str, Value)> {
  vec![(
    "parse_request_head",
    native(vm, "parse_request_head", 5, false, parse_request_head_fn),
  )]
}

#[inline]
fn is_singleton(name: &str) -> bool {
  name.eq_ignore_ascii_case("content-length")
    || name.eq_ignore_ascii_case("content-type")
    || name.eq_ignore_ascii_case("host")
    || name.eq_ignore_ascii_case("authorization")
    || name.eq_ignore_ascii_case("proxy-authorization")
    || name.eq_ignore_ascii_case("from")
    || name.eq_ignore_ascii_case("location")
    || name.eq_ignore_ascii_case("max-forwards")
    || name.eq_ignore_ascii_case("referer")
    || name.eq_ignore_ascii_case("user-agent")
    || name.eq_ignore_ascii_case("if-modified-since")
    || name.eq_ignore_ascii_case("if-unmodified-since")
    || name.eq_ignore_ascii_case("age")
    || name.eq_ignore_ascii_case("expires")
    || name.eq_ignore_ascii_case("date")
    || name.eq_ignore_ascii_case("retry-after")
    || name.eq_ignore_ascii_case("last-modified")
    || name.eq_ignore_ascii_case("server")
}

#[inline]
fn canonical_lower(name: &str) -> Cow<'_, str> {
  if name.eq_ignore_ascii_case("host") {
    Cow::Borrowed("host")
  } else if name.eq_ignore_ascii_case("user-agent") {
    Cow::Borrowed("user-agent")
  } else if name.eq_ignore_ascii_case("accept") {
    Cow::Borrowed("accept")
  } else if name.eq_ignore_ascii_case("content-length") {
    Cow::Borrowed("content-length")
  } else if name.eq_ignore_ascii_case("content-type") {
    Cow::Borrowed("content-type")
  } else if name.eq_ignore_ascii_case("connection") {
    Cow::Borrowed("connection")
  } else if name.eq_ignore_ascii_case("accept-encoding") {
    Cow::Borrowed("accept-encoding")
  } else if name.eq_ignore_ascii_case("authorization") {
    Cow::Borrowed("authorization")
  } else if name.eq_ignore_ascii_case("cookie") {
    Cow::Borrowed("cookie")
  } else if name.bytes().any(|b| b.is_ascii_uppercase()) {
    Cow::Owned(name.to_ascii_lowercase())
  } else {
    Cow::Borrowed(name)
  }
}

struct HeaderEntry<'a> {
  orig: &'a str,
  values: SmallVec<[&'a str; 2]>,
}

fn parse_data(
  ctx: &mut ZuriContext,
  data: &[u8],
  pos: usize,
  max_line: usize,
  max_header_size: usize,
  max_header_count: usize,
) -> Result<Value, String> {
  if pos >= data.len() {
    return Ok(Value::nil());
  }

  let raw = &data[pos..];

  const STACK_HEADERS: usize = 128;
  let mut stack_headers = [httparse::EMPTY_HEADER; STACK_HEADERS];
  let mut heap_headers;
  let headers_slice: &mut [httparse::Header] = if max_header_count <= STACK_HEADERS {
    &mut stack_headers[..max_header_count]
  } else {
    heap_headers = vec![httparse::EMPTY_HEADER; max_header_count];
    &mut heap_headers[..]
  };

  let mut req = httparse::Request::new(headers_slice);
  let status = match req.parse(raw) {
    Ok(s) => s,
    Err(httparse::Error::TooManyHeaders) => {
      let err_tag = ctx.heap().intern_str("error");
      let msg = ctx
        .heap()
        .alloc_string(format!("more than {max_header_count} header fields"));
      return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
    },
    Err(err) => {
      let err_tag = ctx.heap().intern_str("error");
      let msg = ctx
        .heap()
        .alloc_string(format!("malformed HTTP request: {err}"));
      return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
    },
  };

  if status.is_partial() {
    if let Some(p) = raw.iter().position(|&b| b == b'\n') {
      let line_len = if p > 0 && raw[p - 1] == b'\r' {
        p - 1
      } else {
        p
      };
      if line_len > max_line {
        let err_tag = ctx.heap().intern_str("error");
        let msg = ctx
          .heap()
          .alloc_string(format!("line exceeds the {max_line} byte limit"));
        return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
      }
    } else if raw.len() > max_line {
      let err_tag = ctx.heap().intern_str("error");
      let msg = ctx
        .heap()
        .alloc_string(format!("line exceeds the {max_line} byte limit"));
      return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
    }
    if raw.len() > max_line + max_header_size {
      let err_tag = ctx.heap().intern_str("error");
      let msg = ctx
        .heap()
        .alloc_string(format!("header section exceeds {max_header_size} bytes"));
      return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
    }
    return Ok(Value::nil());
  }

  let amt = status.unwrap();

  let first_lf = raw[..amt].iter().position(|&b| b == b'\n').unwrap();
  let req_line_len = if first_lf > 0 && raw[first_lf - 1] == b'\r' {
    first_lf - 1
  } else {
    first_lf
  };
  if req_line_len > max_line {
    let err_tag = ctx.heap().intern_str("error");
    let msg = ctx
      .heap()
      .alloc_string(format!("line exceeds the {max_line} byte limit"));
    return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
  }

  let headers_raw_len = amt - (first_lf + 1);
  if headers_raw_len > max_header_size {
    let err_tag = ctx.heap().intern_str("error");
    let msg = ctx
      .heap()
      .alloc_string(format!("header section exceeds {max_header_size} bytes"));
    return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
  }

  let method = req.method.unwrap();
  let target = req.path.unwrap();
  let version = match req.version.unwrap() {
    1 => "1.1",
    0 => "1.0",
    _ => {
      let err_tag = ctx.heap().intern_str("error");
      let msg = ctx.heap().alloc_string("unsupported HTTP version");
      return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
    },
  };

  let mut host_count = 0;
  let mut authority: Option<&str> = None;
  let mut cookie: Option<String> = None;
  let mut wire_size: usize = 0;

  let mut headers: SmallVec<[HeaderEntry; 8]> = SmallVec::new();

  for h in req.headers.iter() {
    let name = h.name;
    let val_str = match std::str::from_utf8(h.value) {
      Ok(s) => s,
      Err(_) => {
        let err_tag = ctx.heap().intern_str("error");
        let msg = ctx.heap().alloc_string("invalid header field value");
        return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
      },
    };

    if name.eq_ignore_ascii_case("host") {
      host_count += 1;
      if authority.is_none() {
        authority = Some(val_str);
      }
      if host_count > 1 {
        let err_tag = ctx.heap().intern_str("error");
        let msg = ctx
          .heap()
          .alloc_string("request has more than one Host header");
        return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
      }
    } else if name.eq_ignore_ascii_case("cookie") {
      match &mut cookie {
        None => cookie = Some(val_str.to_string()),
        Some(existing) => {
          existing.push_str("; ");
          existing.push_str(val_str);
        },
      }
    }

    if is_singleton(name) {
      if let Some(existing) = headers.iter().find(|e| e.orig.eq_ignore_ascii_case(name)) {
        if let Some(&first_val) = existing.values.first() {
          if first_val != val_str {
            let err_tag = ctx.heap().intern_str("error");
            let msg = ctx
              .heap()
              .alloc_string(format!("conflicting duplicate {name} header"));
            return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
          }
          continue;
        }
      }
    }

    wire_size += name.len() + val_str.len() + 4;

    if let Some(existing) = headers
      .iter_mut()
      .find(|e| e.orig.eq_ignore_ascii_case(name))
    {
      existing.values.push(val_str);
    } else {
      headers.push(HeaderEntry {
        orig: name,
        values: smallvec![val_str],
      });
    }
  }

  if version == "1.1" && host_count == 0 {
    let err_tag = ctx.heap().intern_str("error");
    let msg = ctx
      .heap()
      .alloc_string("HTTP/1.1 request has no Host header");
    return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
  }
  if host_count > 1 {
    let err_tag = ctx.heap().intern_str("error");
    let msg = ctx
      .heap()
      .alloc_string("request has more than one Host header");
    return Ok(ctx.heap().alloc_list(vec![err_tag, msg]));
  }

  let method_val = ctx.heap().intern_str(method);
  let target_val = ctx.heap().alloc_string(target);
  let version_val = ctx.heap().intern_str(version);

  let mut pairs = Vec::with_capacity(headers.len());
  for item in headers {
    let lower_str = canonical_lower(item.orig);
    let k_val = ctx.heap().intern_str(lower_str.as_ref());
    let orig_val = ctx.heap().intern_str(item.orig);

    let val_list = if item.values.len() == 1 {
      let v_val = ctx.heap().alloc_string(item.values[0]);
      ctx.heap().alloc_list(vec![v_val])
    } else {
      let mut val_items = Vec::with_capacity(item.values.len());
      for v in item.values {
        val_items.push(ctx.heap().alloc_string(v));
      }
      ctx.heap().alloc_list(val_items)
    };

    let entry = ctx.heap().alloc_list(vec![orig_val, val_list]);
    pairs.push((k_val, entry));
  }

  let fields_dict = ctx.heap().alloc_dict(pairs);

  let authority_val = match authority {
    Some(s) => ctx.heap().intern_str(s),
    None => Value::nil(),
  };

  let cookie_val = match cookie {
    Some(s) => ctx.heap().alloc_string(s),
    None => Value::nil(),
  };

  let result_list = ctx.heap().alloc_list(vec![
    method_val,
    target_val,
    version_val,
    fields_dict,
    Value::number(wire_size as f64),
    Value::number(amt as f64),
    authority_val,
    cookie_val,
  ]);

  Ok(result_list)
}

fn parse_request_head_fn(ctx: &mut ZuriContext) -> Result<Value, String> {
  enforce_arg_count!(ctx, 5);
  enforce_arg_type_any_of!(ctx, 0, [ArgType::Bytes, ArgType::String]);
  enforce_arg_type!(ctx, 1, ArgType::Number);
  enforce_arg_type!(ctx, 2, ArgType::Number);
  enforce_arg_type!(ctx, 3, ArgType::Number);
  enforce_arg_type!(ctx, 4, ArgType::Number);

  let pos = ctx.args[1].as_number() as usize;
  let max_line = ctx.args[2].as_number() as usize;
  let max_header_size = ctx.args[3].as_number() as usize;
  let max_header_count = ctx.args[4].as_number() as usize;

  let val = ctx.args[0];
  if val.is_bytes() {
    val.with_bytes(|data| parse_data(ctx, data, pos, max_line, max_header_size, max_header_count))
  } else {
    let s = val.as_str().to_string();
    parse_data(
      ctx,
      s.as_bytes(),
      pos,
      max_line,
      max_header_size,
      max_header_count,
    )
  }
}
