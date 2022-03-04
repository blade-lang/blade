# BUGS

- `io.TTY.set_raw()` stopped working correctly. Run io test to confirm.
- The `catch` block in `try...catch...` fails outside a global scope or when triggered from a descendant function.
