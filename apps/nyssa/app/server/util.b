import hash
import http.status
import .db
import ..log

def generate_publisher_key(name) {
  var table = bytes(0)
  iter var i = 0; i < 64; i++ {
    table.append(rand(0, 255))
  }

  var mcbytes = to_string(microtime()).to_bytes()
  table.extend(mcbytes)
  mcbytes.dispose()
  var name_bytes = name.to_bytes()
  table.extend(name_bytes)
  name_bytes.dispose()

  var h = hash.sha1(table)
  table.dispose()

  return h
}

def validate_auth_data(req, res) {
  if !req.headers return res.fail(status.UNAUTHORIZED)

  var key = req.headers.get('Nyssa-Publisher-Key', nil)
  if !key return res.fail(status.UNAUTHORIZED)

  var name = req.headers.get('Nyssa-Publisher-Name', nil)
  if !name return res.fail(status.UNAUTHORIZED)

  var publisher = db.get_publisher(name, key)
  if !publisher return res.fail(status.UNAUTHORIZED)
  
  log.info('Publisher "${publisher.username}" authenticated!')
  return publisher
}

def format_number(number) {
  var parts = to_string(number).split('.')
  parts[0] = parts[0].replace('/\B(?=(\d{3})+(?!\d))/', ',')
  return '.'.join(parts)
}
