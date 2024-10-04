import args
import hash
import os
import colors
import http
import json
import io
import zip
import ..setup
import ..log
import ..config { Config }
import ..util { setup_cli }


var cache_dir = os.join_paths(setup.NYSSA_DIR, setup.CACHE_DIR)
if !os.dir_exists(cache_dir)
  os.create_dir(cache_dir)

var config_file = os.join_paths(os.cwd(), setup.CONFIG_FILE)

def bold(t) {
  return colors.text('${t} ', colors.style.bold)
}

def green(t) {
  return colors.text(t, colors.text_color.green)
}

def parse(parser) {
  parser.add_command(
    'install', 
    'Installs a Blade package', 
    {
      type: args.STRING,
    }
  ).add_option(
    'global', 
    'installs the package globally', 
    {
      short_name: 'g',
    }
  ).add_option(
    'use-cache', 
    'enables the cache', 
    {
      short_name: 'c',
    }
  ).add_option(
    'repo', 
    'the repository to install from', 
    {
      short_name: 'r',
      type: args.OPTIONAL
    }
  )
}

/**
 * @DONE:
 * - Support for running custom setup script on installation.
 * - Add support for copying binary files to bin directory.
 */
def configure(config, repo, full_name, name, version, path, is_global, with_cache, progress, error) {
  log.info('Installing ${full_name}')

  var blade_exe = os.args[0]

  var destination
  if !is_global destination = os.join_paths(os.cwd(), '.blade/libs/${name}')
  else destination = os.join_paths(os.dir_name(blade_exe), 'vendor/${name}')

  # create the packages directory if not exists
  log.info('Creating package directory for ${full_name}')
  if os.dir_exists(destination)
    os.remove_dir(destination, true)
  os.create_dir(destination)

  # extract
  log.info('Extracting artefact for ${full_name}')
  if zip.extract(path, destination) {

    # add to list of installed packages
    progress.set(name, version)

    var package_config_file = os.join_paths(destination, setup.CONFIG_FILE)
    var package_config = Config.from_dict(json.decode(file(package_config_file).read()))

    # install dependencies before running post-install in case post-install
    # depends on a dependency.
    if package_config.deps {
      echo ''
      log.info('Fetching dependencies for ${full_name}...')
      echo '--------------------------${"-" * full_name.length()}---'

      for dep, ver in package_config.deps {
        var dep_full_name = ver ? '${dep}@${ver}' : dep
        if !progress.contains(dep) {
          install(config, repo, dep_full_name, dep, ver, is_global, with_cache, progress, error)
        }
      }
    }

    # run post install script if it exists
    if package_config.post_install {
      log.info('Running post install for ${full_name}')

      # cd into the destination before running post_install so 
      # that post_install will run relative to the package.
      var this_dir = os.cwd()
      os.change_dir(destination)

      # run the script
      os.exec('${blade_exe} ${package_config.post_install}')

      # return to current directory
      os.change_dir(this_dir)
    }

    # create cli if required
    if package_config.cli {
      var cli_path = os.join_paths(destination, package_config.cli)
      log.info('Creating CLI for ${name} at ${cli_path}')
      setup_cli(name, destination, cli_path)
    }
  } else {
    error('${name} installation failed:\n  Failed to extract package source')
  }
}

def install(config, repo, full_name, name, version, is_global, with_cache, progress, error) {

  if with_cache log.info('Checking local cache for ${full_name}')
  var cache_id = hash.sha1(repo + name + version)
  var cache_path = os.join_paths(cache_dir, '${cache_id}.nyp')

  catch {

    # check local cache first to avoid redownloading all the time...
    if file(cache_path).exists() and with_cache {
      configure(config, repo, full_name, name, nil, cache_path, is_global, with_cache, progress, error)
      return
    }

    # fresh install
    log.info('>>> Fetching package metadata for ${full_name}')
    var res = http.get('${repo}/api/get-package/${full_name}')
    var body = json.decode(res.body.to_string())

    if res.status == 200 {
      if !progress {
        echo ''
        echo green(bold('PACKAGE FOUND'))
        echo green('-------------')
        echo bold('Name:') + body.name
        echo bold('Version:') + body.version
        echo bold('Description:') + body.description
        echo bold('Homepage:') + body.homepage
        echo bold('Author:') + body.author
        echo bold('License:') + body.license
        echo bold('Publisher:') + body.publisher
        echo ''
      } else {
        log.info('${name} dependency ${body.name}@${body.version} found')
      }

      # download source
      log.info('Downloading package source for ${full_name}...')
      var download_url = repo + '/source/' + body.source
      var download_req = http.get(download_url)
      if download_req.status == 200 {
        # save the file to cache
        log.info('Caching download for ${full_name}')
        file(cache_path, 'wb').write(download_req.body)

        # do package configuration
        configure(config, repo, full_name, body.name, body.version, cache_path, is_global, with_cache, progress, error)
      } else {
        error('package source not found')
      }
    } else {
      error('${full_name} installation failed:\n  ${body.error}')
    }
  } as e

  if e {
    error('${full_name} installation failed:\n  ${e.message}')
  }
}

def run(value, options, success, error) {
  var repo = options.get('repo', setup.DEFAULT_REPOSITORY),
      is_global = options.get('global', false),
      with_cache = options.get('use-cache', false),
      progress = {},
      config_exists = file(config_file).exists()

  if !config_exists and !is_global
    error('Not in a Nyssa project')
  
  var config
  if !config_exists and is_global {
    config = Config()
  } else {
    config = json.decode(file(config_file).read())
    if !is_dict(config) or !config.get('name', nil) or !config.get('version', nil)
      error('Not in a Nyssa project')
    config = Config.from_dict(config)
  }

  var ns = value.split('@'),
      name = ns[0],
      version = ns.length() > 1 ? ns[1] : nil,
      full_name = version ? '${name}@${version}' : name

  var config_check = config.deps.get(name, nil)
  if config_check
    if version == nil or config_check == version
      success('${value} is already installed.')

  install(config, repo, full_name, name, version, is_global, with_cache, progress, error)

  catch {
    if progress.length() >= 1 {
      log.info('Updating dependency state for project')

      version = progress[name]

      if config.deps.contains(name) and version == nil {
        # do nothing...
      } else {
        config.deps[name] = version
      }

      # update sources if not already listed
      if !config.sources.contains(repo) {
        config.sources.append(repo)
      }

      if config_exists {
        file(config_file, 'w').write(json.encode(config, false))
      }
    }
  } as e

  if e {
    echo colors.text(
      'Dependency state update failed!\n' + 
      'You can manually fix it by adding the following to the dependency section of you ' + setup.CONFIG_FILE + ' file.\n' +
      '\n\t"${name}": ' + (version ? '"${version}"' : "null") +  '\n\n' +
      'If the section is not empty add a comma (,) and press ENTER before adding the fix.\n', 
      colors.text_color.orange
    )
    error(e.message)
  }

  success('${full_name} installed successfully!')
}
