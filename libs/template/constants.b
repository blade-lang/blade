#!-- part of the html module

import os

var DEFUALT_ROOT_DIR = os.join_paths(os.cwd(), 'templates')

var VAR_RE = '/(?<!%)\{\{\s*(?P<variable>([a-z_][a-z0-9_\-|="\']*(\.[a-z0-9_\-|="\']+)*))\s*\}\}/i'
var FUNCTION_RE = '/(?<!%)\{\!\s*(?P<fn>[a-z0-9_]+)\s*\!\}/i'
var EXT_RE = '/[.][a-zA-Z]+$/'
var COMMENT_RE = '/(?=<!--)([\s\S]*?)-->\\n*/m'
var NUMBER_RE = '/^[-]?\d+$/'
var NUMBER_WITH_DECIMAL_RE = '/^[-]?\d+([.]\d+)$/'
var QUOTE_VALUE_RE = '/([\'"]).*\\1/'

var IF_ATTR = 'w-if'
var NOT_ATTR = 'w-not'
var FOR_ATTR = 'w-for'
var KEY_ATTR = 'w-key'
var VALUE_ATTR = 'w-value'
var INCLUDE_TAG = 'include'
var PATH_ATTR = 'w-path'
var DEFAULT_EXT = '.html'
