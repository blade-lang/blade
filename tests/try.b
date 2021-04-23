try {
  var i = 10
  echo i

  try {
    echo [1,2,3][-10]
  } catch e {
    echo 'Message: ${e.message}\nTrace: \n${e.trace()}'
  }
} catch e {
  echo e
}

try {
  die Exception('I am a thrown exception')
  die Exception('Second exception we will never reach')
} catch e {
  echo '\n\nCatching exception...'
  echo 'Exception message: ${e.message}'
  echo 'Exception trace: ${e.trace()}'
}