try {
  var i = 10
  echo i

  try {
    echo [1,2,3][-10]
  } catch e {
    echo e
  }
} catch e {
  echo e
}