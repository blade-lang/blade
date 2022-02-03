#!-- part of the sqlite module


/**
 * @class SQLiteException
 * 
 * General Exception for SQLite
 */
class SQLiteException < Exception {

  /**
   * @constructor SQLiteException
   */
  SQLiteException(message) {
    parent(message)
  }
}

