import os
import io
import iters
import args
import qi
import ..setup

def parse(parser) {
  parser.add_command(
    'test', 
    'Run the tests'
  )
}

def run_test_files(files) {
  for f in files {
    f = os.join_paths(setup.TEST_DIR, f)
    qi.run(f)
  }
  return qi.show_tests_results()
}

def run(value, options, success, error) {
  if os.dir_exists(setup.TEST_DIR) {
    var files = iters.filter(os.read_dir(setup.TEST_DIR), @( x ) { 
      return x != '.' and x != '..' and x.ends_with('.b') 
    })
    if files {
      if !run_test_files(files) {
        os.exit(1)
      }
    } else {
      error('No test files found.')
    }
  } else {
    error('"${setup.TEST_DIR}" directory not found.')
  }

  os.exit(0)
}
