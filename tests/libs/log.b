import log

log.info('It works fine')
log.info('It works ', [1, 2, 3])

var transport = log.default_transport()
transport.show_time(true)
transport.show_name(true)

log.add_transport(transport)

# log.remove_transport(log.default_transport())

var start = time()
for i in 0..1000000 {
  log.info('Finished')
  # echo to_string(date())
}
var end = time()

echo 'Took ${end - start}s'