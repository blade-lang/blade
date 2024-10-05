import os
import json

# general
var NYSSA_VERSION = '0.0.0'

# directories
var APP_DIR = 'app'
var TEST_DIR = 'tests'
var STATIC_DIR = 'public'
var TEMPLATES_DIR = 'templates'
var STORAGE_DIR = 'storage'
var DOCS_DIR = 'docs'
var LOGS_DIR = '${STORAGE_DIR}/logs'
var SOURCES_DIR = '${STORAGE_DIR}/sources'
var DATABASE_DIR = '${STORAGE_DIR}/db'
var CACHE_DIR = '${STORAGE_DIR}/cache'

# files
var INDEX_FILE = 'index.b'
var README_FILE = 'README.md'
var CONFIG_FILE = 'nyssa.json'
var DATABASE_FILE = '${DATABASE_DIR}/nyssa.db'
var STATE_FILE = '${STORAGE_DIR}/config.json'

# repository
var REPOSITORY_HOST = '127.0.0.1'
var REPOSITORY_PORT = 3000

var DEFAULT_REPOSITORY = 'https://nyssa.bladelang.org'

# frontend
var PACKAGES_PER_PAGE = 10
var SESSION_NAME = 'NYSSA-SESSION-ID'

# Nyssa directory
var NYSSA_DIR = os.dir_name(os.dir_name(__file__))

var config_file = os.join_paths(NYSSA_DIR, CONFIG_FILE)
if (config_file = file(config_file)) and config_file.exists() {
  var conf = json.decode(config_file.read())
  if is_dict(conf) {
    NYSSA_VERSION = conf.get('version', NYSSA_VERSION)
  }
}

