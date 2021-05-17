try {
  # die Exception('First exception thrown')
  try {
    die Exception('Second exception thrown')
  } catch Exception as e {
    echo e.message
  }
} catch Exception as e {
  echo e.message
}

try {
  var i = 10
  echo i

  try {
    echo [1,2,3][-10]
  } catch Exception as e {
    echo 'Message: ${e.message}\nTrace: \n${e.stacktrace}'
  }
} catch Exception as e {
  echo e
}

try {
  die Exception('I am a thrown exception')
  die Exception('Second exception we will never reach')
} catch Exception as e {
  echo '\n\nCatching exception...'
  echo 'Exception message: ${e.message}'
  echo 'Exception trace: ${e.stacktrace}'
}