import _thread

# var th = _thread.new(@(i) { echo i }, [1])
# _thread.start(th)
# # _thread.await(th)
# # _thread.dispose(th)

def fib(n) {
  if n < 2 return n
  return fib(n - 2) + fib(n - 1)
}

var start = microtime()
for i in 0..32 {
    fib(i)
}
var non_thread_time = microtime() - start


start = microtime()

var thrds = []
for i in 0..4 {
    thrds.append(
        _thread.run(@(i){
            for j in (i * 8)..((i * 8) + 8) {
                fib(j)
            }
        }, [i])
    )
}

for th in thrds {
    _thread.await(th)
}

var thread_time = microtime() - start

echo 'Raw took ${non_thread_time / 1000000} seconds'
echo 'Threads took ${thread_time / 1000000} seconds'


# for i in 0..1000000 {
#     var th = _thread.new(@(i) { echo i }, [i])
#     _thread.start(th)
#     _thread.await(th)
# #     _thread.dispose(th)
# }
