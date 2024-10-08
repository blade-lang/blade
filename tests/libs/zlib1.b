import zlib

var d = zlib.compress('Hello, World')
echo d

var f = zlib.uncompress(d)
echo f.to_string()
