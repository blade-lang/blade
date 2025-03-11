import os
import io
import args
import qi
import log
import ..setup

def parse(parser) {
  parser.add_command(
    'bundle', 
    'Creates a standalone application from current project'
  )
}
def run(value, options, success, error) {
  
}
