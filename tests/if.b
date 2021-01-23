if 10 > 5 echo 'It works\n'


if 0 > 5 echo 'It works\n'
else echo 'Nope\n'


if 1 > 5 {
  echo '1 is greater than 5\n'
} else if 2 < 5 {
  echo '2 is less than 5\n'
} else {
  echo '1 is not greater than 5\n'
}

if 1 > 5 or 2 < 5 echo 'Ok\n'

if 1 > 5 and 2 < 5 { echo 'Ok 2\n' } else { echo 'No\n' }