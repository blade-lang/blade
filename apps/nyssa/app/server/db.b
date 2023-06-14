import sqlite
import os
import json
import math
import ..log
import ..setup

var db_file = os.join_paths(os.args[1], setup.DATABASE_FILE)
var db_dir = os.dir_name(db_file)

if !os.dir_exists(db_dir)
  os.create_dir(db_dir)

var db = sqlite.open(db_file)

def create_tables() {
  # create the publishers table
  db.exec('CREATE TABLE IF NOT EXISTS publishers (' +
      'id INTEGER PRIMARY KEY,' +
      'username TEXT NOT NULL,' +
      'email TEXT NOT NULL,' +
      'password TEXT NOT NULL,' +
      'key TEXT NOT NULL,' +
      'active BOOLEAN DEFAULT TRUE,' +
      'created_at DATETIME DEFAULT CURRENT_TIMESTAMP,' +
      'deleted_at DATETIME NULL' +
    ');')

  # Create the packages table
  db.exec('CREATE TABLE IF NOT EXISTS packages (' +
      'id INTEGER PRIMARY KEY,' +
      'publisher TEXT NOT NULL,' +
      'name TEXT NOT NULL,' +
      'version TEXT NOT NULL,' +
      'source TEXT NOT NULL,' +
      'config TEXT NOT NULL,' +
      'readme TEXT NULL,' +

      # extracted columns start
      'description TEXT GENERATED ALWAYS AS (json_extract(config, \'$.description\')) VIRTUAL,' +
      'homepage TEXT GENERATED ALWAYS AS (json_extract(config, \'$.homepage\')) VIRTUAL,' +
      'author TEXT GENERATED ALWAYS AS (json_extract(config, \'$.author\')) VIRTUAL,' +
      'license TEXT GENERATED ALWAYS AS (json_extract(config, \'$.license\')) VIRTUAL,' +
      'tags TEXT GENERATED ALWAYS AS (json_extract(config, \'$.tags\')) VIRTUAL,' +
      'deps TEXT GENERATED ALWAYS AS (json_extract(config, \'$.deps\')) VIRTUAL,' +
      # extracted columns end

      'downloads INTEGER DEFAULT 0,' +
      'active BOOLEAN DEFAULT TRUE,' +
      'created_at DATETIME DEFAULT CURRENT_TIMESTAMP,' +
      'deleted_at DATETIME NULL' +
    ');')

    # create the sessions table
    db.exec('CREATE TABLE IF NOT EXISTS sessions (' +
        'id INTEGER PRIMARY KEY,' +
        'key TEXT NOT NULL,' +
        'data TEXT DEFAULT \'{}\',' +
        'created_at DATETIME DEFAULT CURRENT_TIMESTAMP' +
      ');')
}

# PUBLISHERS

def get_publishers() {
  return db.query('SELECT * FROM publishers;')
}

def get_publisher(name, key) {
  var res
  if !key {
    res = db.fetch('SELECT * FROM publishers WHERE username = ? ORDER BY id DESC LIMIT 1;', [name])
  } else {
    res = db.fetch('SELECT * FROM publishers WHERE username = ? AND key = ? ORDER BY id DESC LIMIT 1;', [name, key])
  }

  if res return res[0]
  return nil
}

def check_publisher(name) {
  var res = db.fetch('SELECT * FROM publishers WHERE username = ? OR email = ? ORDER BY id DESC LIMIT 1;', [name, name])

  if res return res[0]
  return nil
}

def create_publisher(publisher) {
  if db.exec('INSERT INTO publishers (username, email, password, key) VALUES (?, ?, ?, ?);', [
    publisher.username, 
    publisher.email,
    publisher.password,
    publisher.key
  ])
    return db.last_insert_id()
  return 0
}

def delete_publisher(name) {
  return db.exec('DELETE FROM publishers WHERE username = ?;', [name])
}

def get_publishers_count() {
  var res = db.fetch('SELECT COUNT(*) as count FROM publishers;')
  if res return res[0].count or 0
  return 0
}

# PACKAGES

def get_packages() {
  return db.fetch('SELECT * FROM packages WHERE deleted_at IS NULL;')
}

def get_top_packages(order) {
  if !order order = 'download DESC'
  return db.fetch("SELECT id, name, publisher, description, sum(downloads) as download, replace(created_at, '-', '/') as date_created FROM packages WHERE deleted_at IS NULL GROUP BY name ORDER BY ${order} LIMIT 4;")
}

def get_package(name, version) {
  var res
  if !version {
    res = db.fetch('SELECT * FROM packages WHERE name = ? and deleted_at IS NULL ORDER BY id DESC LIMIT 1;', [name])
  } else {
    res = db.fetch('SELECT * FROM packages WHERE name = ? and version = ? ORDER BY id DESC LIMIT 1;', [name, version])
  }

  if res return res[0]
  return nil
}

def get_package_for_view(name, version) {
  var res
  if !version {
    res = db.fetch('SELECT * FROM packages WHERE name = ? ORDER BY id DESC LIMIT 1;', [name])
    if res {
      res[0].download = db.fetch('SELECT SUM(downloads) as downloads FROM packages WHERE name = ?', [name])[0].downloads
    }
  } else {
    res = db.fetch('SELECT *, downloads as download FROM packages WHERE name = ? and version = ? ORDER BY id DESC LIMIT 1;', [name, version])
  }

  if res return res[0]
  return nil
}

def get_package_versions(name) {
  return db.fetch('SELECT id, version FROM packages WHERE name = ? and deleted_at IS NULL ORDER BY id DESC', [name])
}

def search_package(query, page, order) {
  if !page page = 1
  if !order order = 'downloads'
  var per_page = setup.PACKAGES_PER_PAGE or 10

  var lower_limit = (page - 1) * per_page # 10 per page

  var count = db.fetch('SELECT count(*) as count FROM (SELECT * FROM packages WHERE (name LIKE :query OR description LIKE :query OR publisher LIKE :query OR tags LIKE :query) AND deleted_at IS NULL GROUP BY name);', {':query': query})
  if !count count = 0
  else count = count[0].count

  var result = db.fetch('SELECT *, sum(downloads) as download FROM (SELECT * FROM packages WHERE (name LIKE :query OR description LIKE :query OR publisher LIKE :query OR tags LIKE :query) AND deleted_at IS NULL ORDER BY id DESC) GROUP BY name ORDER BY ${order} LIMIT ${lower_limit}, ${per_page};', {':query': query})
  var start = per_page * (page -1)

  return {
    packages: result,
    page: page,
    pages: math.ceil(count / per_page),
    start: start,
    end: start + result.length(),
    total: count,
  }
}

def get_user_packages(username, page) {
  if !page page = 1
  var per_page = setup.PACKAGES_PER_PAGE or 10

  var lower_limit = (page - 1) * per_page # 10 per page

  var count = db.fetch('SELECT count(*) as count FROM (SELECT * FROM packages WHERE publisher = :username GROUP BY name);', {':username': username})
  if !count count = 0
  else count = count[0].count

  var result = db.fetch('SELECT * FROM (SELECT * FROM packages WHERE publisher = :username ORDER BY id DESC) GROUP BY name LIMIT ${lower_limit}, ${per_page};', {':username': username})
  var start = per_page * (page -1)

  return {
    packages: result,
    page: page,
    pages: math.ceil(count / per_page),
    start: start,
    end: start + result.length(),
    total: count,
  }
}

def create_package(package) {
  if db.exec('INSERT INTO packages (publisher, name, version, source, config, readme) VALUES (?, ?, ?, ?, ?, ?);', [
    package.publisher,
    package.name,
    package.version,
    package.source,
    json.encode(package.config),
    package.readme
  ]) return db.last_insert_id()
  return 0
}

def delete_package(name) {
  return db.exec('DELETE FROM packages WHERE name = ?;', [name])
}

def update_package_download_count(name, version) {
  return db.exec('UPDATE packages SET downloads = downloads + 1 WHERE name = ? and version = ?;', [name, version])
}

def get_all_packages_count() {
  var res = db.fetch('SELECT COUNT(*) as count FROM packages;')
  if res return res[0].count or 0
  return 0
}

def get_packages_count() {
  var res = db.fetch('SELECT COUNT(DISTINCT name) as count FROM packages;')
  if res return res[0].count or 0
  return 0
}

def get_all_download_count(name) {
  var res
  if name {
    res = db.fetch('SELECT SUM(downloads) as count FROM packages WHERE name = ?', [name])
  } else {
    res = db.fetch('SELECT SUM(downloads) as count FROM packages')
  }
  if res return res[0].count or 0
  return 0
}

def revert_package(name, version) {
  return db.exec('UPDATE packages SET deleted_at=CURRENT_TIMESTAMP WHERE name = ? and id > ?;', [name, version])
}

def archive_package(name) {
  return db.exec('UPDATE packages SET deleted_at=CURRENT_TIMESTAMP WHERE name = ?;', [name])
}

# SESSIONS

def get_session(key) {
  var res = db.fetch('SELECT data FROM sessions WHERE key = ? ORDER BY id DESC LIMIT 1;', [key])
  if res return json.decode(res[0].data)
  return false
}

def create_session(key) {
  if db.exec('INSERT INTO sessions (key) VALUES (?);', [key])
    return db.last_insert_id()
  return 0
}

def update_session(key, value) {
  if db.exec('UPDATE sessions SET data = ? WHERE key = ?;', [value, key])
    return true
  return false
}

def delete_session(key) {
  return db.exec('DELETE FROM sessions WHERE key = ?;', [key])
}


# create tables if not exists...
create_tables()
