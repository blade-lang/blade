var i = 10

using i {
  when 2 {
    echo 'two'
  }
  when 5 {
    echo 'five'
  }
  when 10 {
    echo 'ten'
  }
  default {
    echo 'default'
  }
}
echo '\nafter\n'