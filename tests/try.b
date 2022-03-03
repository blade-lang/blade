try {
  # die Exception('First exception thrown')
  try {
    die Exception('Second exception thrown')
  } catch Exception e {
    echo e.message
  }
} catch Exception e {
  echo e.message
}

try {
  var i = 10
  echo i

  try {
    echo [1,2,3][10]
  } catch Exception e {
    echo '\nMessage: ${e.message}'
  } finally {
    echo 'Despite the error, I run because I am in finally'
  }
} catch Exception e {
  echo e
}

try {
  die Exception('I am a thrown exception')
  die Exception('Second exception we will never reach')
} catch Exception e {
  echo '\nCatching exception...'
  echo 'Exception message: ${e.message}'
  echo 'Exception trace: ${e.stacktrace}'
}

try {
  echo '\nTry block called'
} finally {
  echo 'Final block called\n'
}

try {
  echo [][10]
} finally {
  echo 'Error occurred, but I will still run'
}

# this code should never run...
# because we didn't catch the last exception.
echo 500
