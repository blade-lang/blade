import _thread

var s = []
for i in 0..60000 {
    var th = _thread.run(@(i){ echo i }, [i])
    if(th) {
        s.append(th)
    }
}

for i in 0..60000 {
    _thread.await(s[i])
}
