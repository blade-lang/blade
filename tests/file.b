
echo 'Copying file me.png to me2.png'
var start = time()

var f = file('jetbrains.png', 'rb')
var data = f.read()

var f2 = file('jetbrains-new.png', 'wb')
f2.write(data)

echo 'Successfully copied file me.png to me2.png in ${time() - start}s'

var f3 = file('README.md')
echo f3.read()

# file('test', 'w').write('It works')
