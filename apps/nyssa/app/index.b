import args
import io
import colors
import os
import log
import date
import .setup

# initialize storage directory
var storage_dir = os.join_paths(setup.NYSSA_DIR, setup.STORAGE_DIR)
if !os.dir_exists(storage_dir)
  os.create_dir(storage_dir)

# ensure config file exists...
var config_file = os.join_paths(setup.NYSSA_DIR, setup.CONFIG_FILE)
if !file(config_file).exists()
  file(config_file, 'w').write('{}')

# ensure the state file exists
if !file(setup.STATE_FILE).exists()
  file(setup.STATE_FILE, 'w').write('{}')

var logs_dir = os.join_paths(setup.NYSSA_DIR, setup.LOGS_DIR)
if !os.dir_exists(logs_dir)
  os.create_dir(logs_dir)

# setup file logging
var time = date.localtime()
log.add_transport(log.FileTransport(
  '${logs_dir}/${time.year}-${to_string(time.month).lpad(2,'0')}-${to_string(time.day).lpad(2,'0')}.log'
))

# setup console logging
log.default_transport().show_time(false)
log.default_transport().show_level(false)

# import commands...
import .commands.account
import .commands.clean
import .commands.info
import .commands.init
import .commands.install
import .commands.restore
# import .commands.doc
import .commands.uninstall
import .commands.publish
import .commands.serve
import .commands.test
import .commands.bundle

# Import options...
import .options.version

var parser = args.Parser('nyssa', true)

var commands = {
  account,
  bundle,
  clean,
  info,
  init,
  install,
  publish,
  restore,
  serve,
  test,
  uninstall,
}

var options = {
  version: version,
}

for cmd in commands {
  cmd.parse(parser)
}
for o in options {
  o.parse(parser)
}

def success(msg, info) {
  log.info(cmsg)

  if info {
    log.info(info)
  }

  os.exit(0)
}

def error(msg) {
  log.error(msg)
  os.exit(1)
}

var opts = parser.parse()

if opts.options or opts.command {
  var command = opts.command
  opts = opts.options

  if command {
    commands[command.name].run(command.value, opts, success, error)
  } else if opts {
    var key = opts.keys()[0]
    options[key].get(opts[key])
  }
}
