import args
import os
import ..server
import ..setup

def parse(parser) {
  parser.add_command(
    'serve', 
    'Starts a local Nyssa repository server'
  ).add_option(
    'port', 
    'port of the server (default: ${setup.REPOSITORY_PORT})', 
    {
      short_name: 'p',
      type: args.OPTIONAL,
    }
  ).add_option(
    'host',
    'the host ip (default: ${setup.REPOSITORY_HOST})',
    {
      short_name: 'n',
      type: args.OPTIONAL,
    }
  )
}

def run(value, options, success, error) {
  var port = to_number(options.get('port', setup.REPOSITORY_PORT))
  var host = options.get('host', setup.REPOSITORY_HOST)
  server(host, port)
}
