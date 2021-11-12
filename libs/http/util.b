#!-- part of the http module

def iso_8859_1_clean(str) {
  return '\n'.join(str.split('\n')[1,-1])
}