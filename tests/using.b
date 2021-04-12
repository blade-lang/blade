var i = 10
# echo i
using i {
  when 2 {
    echo 'two'
  }
  when 5 {
    echo 'five'
  }
  when 10 {
    var result = 'ten'
    echo result
  }
  default {
    echo 'default'
  }
}
echo 'after'