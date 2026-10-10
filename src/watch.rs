//! What `zuri run --watch` learns about the files a program depends on.
//!
//! Watch mode runs the program as a child process with `REPORT_VAR`
//! naming a file. Every source file the runtime reads, in any isolate,
//! and every file a library declares through `zuri.watch.depends_on()`,
//! is appended to it as one line: a fingerprint of the content, a tab,
//! and the path. Watch mode reads the lines as they arrive and watches
//! each file they name.
//!
//! The fingerprint is the 64 bit FNV-1a of the bytes, in hex, the value
//! `hash.fnv1a_64()` returns for the same bytes, or `-` for a file that
//! could not be read. Watch mode compares it with the file as it is once
//! the watch is in place, so an edit made between the program reading a
//! file and the watch starting is still seen.
//!
//! Each path is recorded once per process, however many times it is
//! read: a template compiled again, a module loaded again by another
//! isolate, or a file a library declares every time it reads it. The
//! first record is the one watch mode needs, since every change after it
//! reaches watch mode as a filesystem event.
//!
//! The standard library is left out. It changes only when Zuri itself
//! is upgraded.

use std::fs::{File, OpenOptions};
use std::io::Write;
use std::path::{Path, PathBuf};
use std::sync::{LazyLock, Mutex, OnceLock};

use rustc_hash::FxHashSet;

/// The variable watch mode names the report file in.
pub const REPORT_VAR: &str = "ZURI_WATCH_REPORT";

static REPORT: OnceLock<Option<Mutex<File>>> = OnceLock::new();

/// Every path recorded so far, across all isolates.
static RECORDED: LazyLock<Mutex<FxHashSet<PathBuf>>> = LazyLock::new(Default::default);

/// Takes the report file over from the environment.
///
/// Called first thing, before any other thread exists. The variable is
/// removed as it is read, so a program this one starts, another `zuri`
/// included, does not report into the same file.
pub fn init() {
  let target = std::env::var_os(REPORT_VAR);

  if target.is_some() {
    // Nothing else is running yet to be reading the environment.
    unsafe { std::env::remove_var(REPORT_VAR) };
  }

  let file = target.and_then(|path| OpenOptions::new().append(true).create(true).open(path).ok());

  let _ = REPORT.set(file.map(Mutex::new));
}

/// Whether this program is running under `zuri run --watch`.
pub fn active() -> bool {
  matches!(REPORT.get(), Some(Some(_)))
}

/// Records that the program read `path`, which held `content`.
pub fn record_source(path: &Path, content: &[u8]) {
  if !active() || in_standard_library(path) || !first_time(path) {
    return;
  }

  write_line(&fingerprint(content), path);
}

/// Records that the program depends on the file at `path`, reading it
/// for its fingerprint. Returns whether anything was recorded, which it
/// is only under watch mode.
pub fn depends_on(path: &str) -> bool {
  if !active() {
    return false;
  }

  let resolved = crate::builtins::file::canonical_path(path)
    .map(PathBuf::from)
    .unwrap_or_else(|_| std::path::absolute(path).unwrap_or_else(|_| PathBuf::from(path)));

  // Checked before the file is read, so declaring a file again costs a
  // lookup and nothing more.
  if in_standard_library(&resolved) || !first_time(&resolved) {
    return true;
  }

  let print = match std::fs::read(&resolved) {
    Ok(bytes) => fingerprint(&bytes),
    Err(_) => "-".to_string(),
  };

  write_line(&print, &resolved);

  true
}

/// Whether `path` has not been recorded before, marking it recorded.
fn first_time(path: &Path) -> bool {
  RECORDED
    .lock()
    .unwrap_or_else(|e| e.into_inner())
    .insert(path.to_path_buf())
}

fn fingerprint(content: &[u8]) -> String {
  format!("{:016x}", crate::modules::hash::fnv1a_64(content))
}

fn write_line(fingerprint: &str, path: &Path) {
  let path = path.display().to_string();

  // A line is the unit the report is read in, so a name that would break
  // one cannot be reported. No import or template path can carry one.
  if path.contains('\n') {
    return;
  }

  let Some(Some(report)) = REPORT.get() else {
    return;
  };

  let line = format!("{fingerprint}\t{path}\n");
  let mut file = report.lock().unwrap_or_else(|e| e.into_inner());

  // One write per line, so a reader never sees half of one, whichever
  // isolate is reporting.
  let _ = file.write_all(line.as_bytes());
}

fn in_standard_library(path: &Path) -> bool {
  static LIBS: OnceLock<Option<PathBuf>> = OnceLock::new();

  let libs = LIBS.get_or_init(|| {
    crate::vm::modules::install_root_libs().and_then(|libs| {
      crate::builtins::file::canonical_path(&libs.to_string_lossy())
        .ok()
        .map(PathBuf::from)
    })
  });

  libs.as_ref().is_some_and(|libs| path.starts_with(libs))
}
