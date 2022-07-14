#!-- part of the sqlite module


/**
 * General Exception for SQLite
 */
class SQLiteException < Exception {
  /**
   * SQLiteException(message: string)
   * @constructor
   */
  SQLiteException(message) {
    parent(message)
  }
}

