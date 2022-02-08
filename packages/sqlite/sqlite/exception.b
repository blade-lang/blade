#!-- part of the sqlite module


/**
 * General Exception for SQLite
 */
class SQLiteException < Exception {

  /**
   * @constructor
   */
  SQLiteException(message) {
    parent(message)
  }
}

