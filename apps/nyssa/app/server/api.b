# API BACKEND

import hash
import json
import bcrypt
import os
import log
import http.status
import .db
import .util
import .errors
import ..setup
import ..package
import ..publisher

var uploads_dir = os.join_paths(setup.NYSSA_DIR, setup.SOURCES_DIR)
if !os.dir_exists(uploads_dir)
  os.create_dir(uploads_dir)

# API
def create_publisher(req, res) {
  /* Expected request format:
    {
      "name": string,
      "email": string,
      "password": string
    } */
  if !req.body or !is_dict(req.body)
    return res.fail(status.BAD_REQUEST)

  var name = req.body.get('name', nil),
      email = req.body.get('email', nil),
      password = req.body.get('password', nil)

  if !name or !email or !password
    return res.fail(status.BAD_REQUEST)

  # ensure name is in a valid format
  # 1. publisher name can only contain alphanumeric and underscores
  # 2. publisher name must start with an alphabeth
  if !name.match('/^[a-zA-Z][a-zA-Z0-9_]+$/')
    return res.fail(status.BAD_REQUEST, 'invalid publisher name')

  # ensure that name is not taken
  var test = db.get_publisher(name)
  if test return res.fail(status.CONFLICT, 'publisher already exists')

  # generate key
  var key = util.generate_publisher_key(name)

  log.info('Creating publisher ${name}')
  var pub = publisher(name, email, bcrypt.hash(password, 5), key)
  if db.create_publisher(pub) == 0
    return res.fail(status.SERVICE_UNAVAILABLE, 'something went wrong')

  return res.json(pub)
}

def create_package(req, res) {
  /* Expected request format:
    {
      "name": string,
      "version": string,
      "source": file,
      "config": string,
      "readme": string
    } */
  if !req.body or !is_dict(req.body)
    return res.fail(status.BAD_REQUEST)

  var publisher
  if publisher = util.validate_auth_data(req, res) {
    var name = req.body.get('name', nil),
      version = req.body.get('version', nil),
      config = req.body.get('config', nil),
      readme = req.body.get('readme', nil),
      source = req.files.get('source', nil)

    if !name or !version or !config or !source
      return res.fail(status.BAD_REQUEST)

    # disallow creating a package named blade to avoid cli destruction
    # when doing a global installation.
    if name.lower() == 'blade'
      return res.fail(status.BAD_REQUEST)

    config = json.decode(config)
    # Verify config first
    if !is_dict(config) or !config.get('name', nil) or !config.get('version', nil)
      return res.fail(status.BAD_REQUEST)
    if config.name != name or config.version != version
      return res.fail(status.BAD_REQUEST)

    # then avoid conflicts
    var test_pkg = db.get_package(name)
    if test_pkg {
      # now ensure that another publisher isn't publishing it
      if test_pkg.publisher != publisher.username
        return res.fail(status.CONFLICT, 'Package ${name} already exists in this repository')

      # check if package version already exist
      if db.get_package(name, version)
        return res.fail(status.CONFLICT, 'Package ${name} already published version ${version}')
    }

    # write the file next
    var file_name = '${publisher.username}_${name}_${microtime()}.nyp'
    var file_path = os.join_paths(uploads_dir, file_name)
    if !file(file_path, 'wb').write(source.content)
      return res.fail(status.UNPROCESSABLE_ENTITY)  # consider using UNSUPPORTED_MEDIA_TYPE

    var pkg = package(publisher.username, name, version, config, readme, file_name)

    # then write to db
    if db.create_package(pkg) == 0
      return res.fail(status.SERVICE_UNAVAILABLE, 'something went wrong')

    # return the package
    return res.json(pkg)
  }
}

def get_package(req, res) {
  var r = req.params.get('name', '').split('@')
  var name = r[0],
      version = r.length() > 1 ? r[1] : nil

  if !name return res.fail(status.BAD_REQUEST)

  # fetch the package
  var pack = db.get_package(name, version)
  if !pack return res.fail(status.NOT_FOUND)

  db.update_package_download_count(pack.name, pack.version)

  pack.tags = json.decode(pack.tags)
  pack.deps = json.decode(pack.deps)
  pack.remove('config')

  return res.json(pack)
}

def all_package(req, res) {
  # fetch all the package
  var packs = db.get_packages()

  for pack in packs {
    pack.tags = json.decode(pack.tags)
    pack.deps = json.decode(pack.deps)
    pack.remove('config')
  }

  return res.json(packs)
}

def login(req, res) {
  /* Expected request format:
  {
    "username": string,
    "password": string
  } */
  if !req.body or !is_dict(req.body)
    return res.fail(status.BAD_REQUEST)

  var name = req.body.get('username', nil),
      password = req.body.get('password', nil)

  if !name or !password
    return res.fail(status.BAD_REQUEST)

  var pub = db.get_publisher(name)

  # authenticate
  if !pub or !bcrypt.compare(password, pub.password)
    return res.fail(status.FORBIDDEN)


  # transform for return to remove the non-returning fields
  pub = publisher.Publisher.from_dict(pub)

  return res.json(pub)
}
