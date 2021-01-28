var f = file('pic.jpg', 'rb')
var data = f.read()

echo f.is_open()

var f2 = file('pic2.jpg', 'wb')
f2.write(data)

var f3 = file('README.md')
echo f3.read()