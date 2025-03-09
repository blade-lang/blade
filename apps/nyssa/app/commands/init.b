import args
import os
import io
import colors
import json
import log
import ..config {
  Config
}
import ..setup

def parse(parser) {
  parser.add_command(
    'init', 
    'Creates a new package in current directory'
  ).add_option(
    'name', 
    'the name of the package', 
    {
      short_name: 'n',
      type: args.OPTIONAL,
    }
  )
}

def run(value, options, success, error) {

  # Declare locations.
  var here = os.cwd(),
      test_dir = os.join_paths(here, setup.TEST_DIR),
      app_dir = os.join_paths(here, setup.APP_DIR)
  var test_ignore = test_dir + os.path_separator + '.gitignore',
      index = here + os.path_separator + setup.INDEX_FILE,
      app_index = app_dir + os.path_separator + setup.INDEX_FILE,
      readme = here + os.path_separator + setup.README_FILE,
      config_file = here + os.path_separator + setup.CONFIG_FILE,
      attr_file = here + os.path_separator + '.gitattributes',
      ignore_file = here + os.path_separator + '.gitignore'

  var test_config_file = file(config_file)
  if test_config_file.exists() and test_config_file.read().trim().length() > 0 {
    error('Cannot create new package where one exists.')
    return
  }

  var config = get_package_config()
  config.sources = [setup.DEFAULT_REPOSITORY]

  if !config.name {
    error('Package must specify a name.')
  }

  log.info('Initializing package ${config.name}')
  log.info('Creating directories')

  # Create tests and examples directory
  if !os.dir_exists(test_dir) os.create_dir(test_dir)

  # Create .gitignore files in tests and examples directory for 
  # git compartibility.
  log.info('Creating required git files')
  var tf = file(test_ignore, 'w+')
  tf.open(); tf.close()

  # increase Blade visibility by setting the attribute file properties
  # to allow Github identify it as a Blade project.
  var test_attr_file = file(attr_file)
  var attr_content_test = '/\\*\\.b linguist\\-language=Blade/'
  if !test_attr_file.exists() or !test_attr_file.read().match(attr_content_test) {
    var start_line = test_attr_file.exists() ? '\n' : ''
    file(attr_file, 'w+').write(
      start_line +
      '*.b linguist-detectable\n' +
      '*.b linguist-language=Blade\n'
    )
  }

  # create default gitignore file to disable popular editor extensions
  # and the .blade directory.
  # 
  # this will only happen if the file does not exist or does not contain 
  # the blade default ignore definitions.
  var test_ingore_file = file(ignore_file)
  var ignore_start_line = '# blade packages directory and files'
  if !test_ingore_file.exists() or !test_ingore_file.read().match(
    '/${ignore_start_line}/'
  ) {
    log.info('Initializing Gitignore')

    var start_line = test_ingore_file.exists() ? '\n' : ''
    file(ignore_file, 'w+').write(
      start_line +
      '${ignore_start_line}\n' +
      '.blade/\n' +
      '*.nyp\n' +   # nyssa package object file
      '\n' +
      '# popular editors\n' +
      '.vscode/\n' +
      '.idea/\n' +
      '.vs/\n' +
      '\n' +
      '# c object files (for C extensions)\n' +
      '*.o\n' +
      '*.ko\n' +
      '*.obj\n' +
      '*.elf\n' +
      '\n' +
      '# log files\n' +
      '*.log\n'
    )
  }

  # Create the README.md file if one does not exists.
  if !file(readme).exists() {
    log.info('Generating README.md for project')

    file(readme, 'w').write(
      '# ${config.name}\n' + 
      '\n' +
      '${config.description or "_Package description goes here._"}\n' +
      '\n' +
      '### Package Information\n' + 
      '\n' + 
      '- **Name:** ${config.name}\n' +
      '- **Version:** ${config.version}\n' +
      '- **Homepage:** ${config.homepage or "_Homepage goes here._"}\n' +
      '- **Tags:** ${", ".join(config.tags) or "_Tags goes here._"}\n' +
      '- **Author:** ${config.author or "_Author info goes here._"}\n' +
      '- **License:** ${config.license or "_License name or link goes here._"}\n' +
      '\n'
    )
  } else {
    log.info('Existing README.md. Skipping')
  }

  # Create the nyssa.json file.
  log.info('Generating Nyssa config file')
  file(config_file, 'w').write(json.encode(config, false))

  if !os.dir_exists(app_dir) {
    log.info('Creating application directory')
    os.create_dir(app_dir)
  }

  # create the app index file
  var app_index_test_file = file(app_index)
  if !app_index_test_file.exists() or !app_index_test_file.read().trim().length() == 0 {
    log.info('Creating application files')
    file(app_index, 'w+').write("echo 'Welcome to Nyssa. Magic begins here!'")
  }

  # create the index file
  var index_test_file = file(index)
  if !index_test_file.exists() or index_test_file.read().trim().length() == 0 {
    log.info('Finalizing package initialization')
    file(index, 'w+').write('import .app { * }')
  }

  success('Package ${config.name} created!')
}

def _default(t) {
  return colors.text(colors.text('(default: ${t})', colors.text_color.dark_grey), colors.style.italic)
}

def get_package_config() {
  var default_name = os.base_name(os.cwd())
  return Config.from_dict({
    name: io.readline('package name ${_default(default_name)}:').trim() or default_name,
    version: io.readline('version ${_default("1.0.0")}:').trim() or '1.0.0',
    description: io.readline('description:').trim(),
    homepage: io.readline('homepage:').trim(),
    tags: io.readline('tags:').trim().split(',').map(@(x) {
      return x.trim()
    }),
    author: io.readline('author:').trim(),
    license: io.readline('license ${_default("ISC")}:').trim() or 'ISC',
  })
}
