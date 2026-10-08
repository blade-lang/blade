//! Giving memory back to the system.
//!
//! The runtime allocates through mimalloc. Version 3, the one the crate
//! builds, keeps memory it has freed for a second before returning it,
//! and returns it only while later allocation is going on. A program that
//! frees a lot and then goes quiet, a server between requests or an
//! isolate waiting on a channel, would otherwise hold on to its peak for
//! as long as it runs.

use std::ffi::{c_int, c_long};

// Linked for its native library. The two calls below are declared here
// because the crate only binds them behind a feature that brings in
// another dependency.
use mimalloc as _;

/// `mi_option_purge_delay` in mimalloc 3's `mi_option_e`.
const MI_OPTION_PURGE_DELAY: c_int = 15;

/// How long freed memory waits before going back to the system, in
/// milliseconds. Short enough that memory a collection frees is returned
/// while the collection is still freeing; mimalloc 2 used the same.
const PURGE_DELAY_MS: c_long = 10;

unsafe extern "C" {
  fn mi_option_set(option: c_int, value: c_long);
  fn mi_collect(force: bool);
  #[cfg(test)]
  fn mi_option_get(option: c_int) -> c_long;
}

/// Sets how soon freed memory is returned, unless `MIMALLOC_PURGE_DELAY`
/// in the environment already decides it. Called once, at startup.
pub fn configure() {
  if std::env::var_os("MIMALLOC_PURGE_DELAY").is_none() {
    unsafe { mi_option_set(MI_OPTION_PURGE_DELAY, PURGE_DELAY_MS) };
  }
}

/// Returns everything this thread's allocator holds free to the system
/// now, rather than whenever later allocation would have.
pub fn return_free_memory() {
  unsafe { mi_collect(true) };
}

#[cfg(test)]
mod tests {
  use super::*;

  // An upgrade that renumbers mimalloc's options would make `configure`
  // set some other option. Nothing here sets the purge delay, so the
  // option this names must still read as version 3's default.
  #[test]
  fn the_purge_delay_option_is_the_one_named() {
    if std::env::var_os("MIMALLOC_PURGE_DELAY").is_some() {
      return;
    }
    assert_eq!(unsafe { mi_option_get(MI_OPTION_PURGE_DELAY) }, 1000);
  }
}
