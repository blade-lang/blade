import _thread

import http

echo '-------------- SINGLE THREAD ---------------------'

var start = microtime()
for i in 0..10 {
    echo http.get('https://google.com')
}
var normal_ends = microtime() - start


echo '--------------- MULTITHREADING -------------------'

start = microtime()

var thrds = []
for i in 0..10 {
    thrds.append(_thread.run(@{
        echo http.get('https://google.com')
    }, []))
}

for t in thrds {
    _thread.await(t)
}

var thread_ends = microtime() - start

# --------------------- RESULT ---------------------
echo 'Normal = ${normal_ends / 1000000} seconds'
echo 'Thread = ${thread_ends / 1000000} seconds'
