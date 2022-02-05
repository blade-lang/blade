#
# @module sqlite
#  
# provides functionalities for interacting with sqlite3 database
# @ copyright 2021, Ore Richard Muyiwa and Blade contributors
# 

import .sqlite3 { * }

/**
 * open([path: string])
 * 
 * returns an handle to a sqlite3 datbase
 */
def open(path) {
  var sqlite = SQLite3(path)
  sqlite.open()
  return sqlite
}

