import io
import date
import os
import .setup

var logs_dir = os.join_paths(setup.NYSSA_DIR, setup.LOGS_DIR)

var _logfile
def init() {
  if !os.dir_exists(logs_dir)
    os.create_dir(logs_dir)
    
  var time = date.localtime()
  _logfile = file(
    '${logs_dir}/${time.year}-${to_string(time.month).lpad(2,'0')}-${to_string(time.day).lpad(2,'0')}.log',
    'a+'
  )
}
init()

def _write(type, mes, no_console) {
  var time = date.localtime()
  var message = '${time.year}-${to_string(time.month).lpad(2,'0')}-${to_string(time.day).lpad(2,'0')} ' + 
                '${to_string(time.hour).lpad(2,'0')}:${to_string(time.minute).lpad(2,'0')}:${to_string(time.seconds).lpad(2,'0')}' +
                '.${to_string(time.microseconds).lpad(6,'0')} ${type} ${mes}\n'
  if !no_console print(mes + '\n')
  _logfile.write(message)
  return mes
}

def info(message, no_console) {
  return _write('INFO', message, no_console)
}

def debug(message, no_console) {
  return _write('DEBUG', message, no_console)
}

def warn(message, no_console) {
  return _write('WARN', message, no_console)
}

def error(message, no_console) {
  return _write('ERROR', message, no_console)
}
