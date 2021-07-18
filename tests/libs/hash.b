import hash

var start = time()

for i in 1..1000001 {
  hash.adler32('hello, world')
}

echo '1 million adler32 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.crc32('hello, world')
}

echo '1 million crc32 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.md2('hello, world')
}

echo '1 million md2 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.md4('hello, world')
}

echo '1 million md4 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.md5('hello, world')
}

echo '1 million md5 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.sha1('hello, world')
}

echo '1 million sha1 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.sha224('hello, world')
}

echo '1 million sha224 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.sha256('hello, world')
}

echo '1 million sha256 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.sha384('hello, world')
}

echo '1 million sha384 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.sha512('hello, world')
}

echo '1 million sha512 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.fnv1a('hello, world')
}

echo '1 million fnv1a in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.fnv1a_64('hello, world')
}

echo '1 million fnv1a_64 in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.whirlpool('hello, world')
}

echo '1 million whirlpool in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.snefru('hello, world')
}

echo '1 million snefru in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.gost('hello, world')
}

echo '1 million gost in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.siphash('0123456789ABCDEF', 'hello, world')
}

echo '1 million siphash in ${time() - start}s'
start = time()

for i in 1..1000001 {
  hash.hmac_sha256('0123456789ABCDEF', 'hello, world')
}

echo '1 million hmac_sha256 in ${time() - start}s'