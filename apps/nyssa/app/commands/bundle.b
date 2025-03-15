import os
import io
import args
import qi
import log
import http
import zip
import json

import ..setup

/**
 * The bundle command can be configured in the nyssa.json file and takes the following format:
 * 
 * "bundle": {
 *    "name": "The app name ", # If missing, it defaults to the Nyssa package name
 *    "version": "The app version", # # If missing, it defaults to the Nyssa package version
 *    "icns": "/path/to/icon.icns", # custom app icon only used by a `macos` bundle
 *    "plist": "/path/to/plist/file", # custom Info.plist file to be used by a `macos` bundle
 * }
 */

# ...
var storage_dir = os.join_paths(setup.NYSSA_DIR, setup.STORAGE_DIR)
var cache_dir = os.join_paths(setup.NYSSA_DIR, setup.CACHE_DIR)
var tmp_dir = os.join_paths(storage_dir, '.tmp')
var config_file = os.join_paths(os.cwd(), setup.CONFIG_FILE)

var arch = os.info().machine

var _supported_platforms = [
  'linux',
  'linux-aarch64',
  'macos',
  'osx',
  'windows',
]


def parse(parser) {

  var default_platform = os.platform
  if default_platform == 'linux' and arch == 'arm64' {
    default_platform = 'linux-aarch64'
  } else if default_platform == 'osx' {
    default_platform = 'macos'
  }

  var platform_list = ', '.join(
    _supported_platforms.map(@(x) {
      return x == default_platform ? '${x} - default' : x
    })
  )

  parser.add_command(
    'bundle', 
    'Creates a standalone application bundle from any Blade project'
  ).add_option(
    'os',
    'the target operating system (${platform_list})',
    {
      short_name: 's',
      type: args.OPTIONAL,
      value: default_platform,
    }
  ).add_option(
    'dest',
    'the output directory to store the bundle',
    {
      short_name: 'd',
      type: args.OPTIONAL,
      value: os.cwd(),
    }
  )
}

def get_file_list(root_dir, main_root) {
  if !main_root {
    main_root = root_dir
  }

  if os.dir_exists(root_dir) {
    return os.read_dir(root_dir).filter(@(f) { 
      return !f.starts_with('.')
    }).reduce(@(list, file) {
      var full_path = os.join_paths(root_dir, file)
      if os.dir_exists(full_path) {
        list.extend(get_file_list(full_path, main_root))
      } else {
        list.append(full_path[main_root.length(),])
      }
      return list
    }, [])
  }

  return []
}

def copy_directory(src_dir, dest_dir) {
  var fls = get_file_list(src_dir).filter(@(x) {
    return x != '.' and x != '..' and 
      !x.match('/(\\/|\\\\)[.]git\\1/')
  })

  for fl in  fls {
    var src = os.join_paths(src_dir, fl)
    var dest = os.join_paths(dest_dir, fl)

    if os.dir_exists(src) {
      os.create_dir(dest)
    } else {
      # create the directory if its missing
      os.create_dir(os.dir_name(dest))

      file(src).copy(dest)
    }
  }
}

def _get_blade_bundler(source_os, target_os, config) {
  var blade_zip_target = os.join_paths(tmp_dir, '${config.name}-${config.version}')
  if os.dir_exists(blade_zip_target) {
    os.remove_dir(blade_zip_target, true)
  }

  var blade_zip_tmp_target = os.join_paths(tmp_dir, '__${config.name}-${config.version}')
  if os.dir_exists(blade_zip_tmp_target) {
    os.remove_dir(blade_zip_tmp_target, true)
  }

  var runtime_dir = os.join_paths(blade_zip_target, 'runtime')

  # if we are generating for the same platform we are running on, we'll 
  # be using a copy of our local installation.
  if source_os == target_os {
    copy_directory(os.dir_name(os.exe_path), runtime_dir)
    log.info('Successfully copied bundler...')
  } else {
    var blade_zip = os.join_paths(cache_dir, 'blade-${target_os}.zip')

    # create/recreate temporary target directory
    os.create_dir(blade_zip_tmp_target)

    # if we do not have a local copy of the Blade bundler, then we download a 
    # Bundler for the current Blade version from Github.
    if !file(blade_zip).exists() {
      var version_response = os.exec('"${os.exe_path}" -v')
      if !version_response {
        raise Exception('Failed to determine Blade version')
      }

      var blade_version = version_response.match('/(?<=Blade)\s*(\d+[.]\d+[.]\d+)/')
      if !blade_version {
        raise Exception('Failed to determine Blade version')
      }

      var version = blade_version[1]
      var download_link = 'https://github.com/blade-lang/blade/releases/download/v${version}/blade-${target_os}-v${version}.zip'
      log.info('Getting Blade bundle from ${download_link}')

      var download_result = http.get(download_link)

      if download_result.status == 200 {
        file(blade_zip, 'wb').write(download_result.body)
        log.info('Saved downloaded bundler ${target_os}-${version}')
      } else if download_result.status == 404 {
        raise Exception('Bundler for OS ${target_os} not found for current Blade version!')
      } else  {
        raise Exception('Failed to get bundler for OS ${target_os}: ${download_result.as_text()}')
      }
    }

    zip.extract(blade_zip, blade_zip_tmp_target)

    # create/recreate target directory
    if os.dir_exists(blade_zip_target) {
      os.remove_dir(blade_zip_target, true)
    }
    os.create_dir(blade_zip_target)

    # move blade to runtime directory
    os.rename(
      os.real_path(os.join_paths(blade_zip_tmp_target, 'blade')),
      runtime_dir
    )

    # remove intermediate extraction directory
    os.remove_dir(blade_zip_tmp_target, true)
    log.info('Successfully extracted bundler...')
  }

  # streamline runtime
  # 1. Remove nyssa and its binaries since its not accessible in userspace
  os.remove_dir(os.join_paths(runtime_dir, 'apps'), true)
  file(os.join_paths(runtime_dir, 'nyssa')).delete()
  file(os.join_paths(runtime_dir, 'nyssa.cmd')).delete()

  # 2. Extract bundle.zip from the runtime into the target_directory so that 
  # we can use it later then remove the bundle.zip file.
  zip.extract(
    os.join_paths(runtime_dir, 'bundle.zip'),
    os.join_paths(blade_zip_target, 'bundle')
  )
  file(os.join_paths(runtime_dir, 'bundle.zip')).delete()

  log.info('Successfully streamlined bundler...')

  return blade_zip_target
}

def bundle_app(source_os, target_os, config, dest_dir) {
  var target_src = _get_blade_bundler(source_os, target_os, config)

  # remove any existing application.
  if os.dir_exists(target_src + '.app') {
    os.remove_dir(target_src + '.app', true)
  }

  var runtime_dir = os.join_paths(target_src, 'runtime')
  var bundle_dir = os.join_paths(target_src, 'bundle')

  var contents_dir = os.join_paths(target_src, 'Contents')
  
  # create the MacOS directory
  var macos_dir = os.join_paths(contents_dir, 'MacOS')
  os.create_dir(macos_dir)

  # create the Resources directory
  var resources_dir = os.join_paths(contents_dir, 'Resources')
  os.create_dir(resources_dir)

  # create plist file
  var target_plist_path = os.join_paths(contents_dir, 'Info.plist')

  # Allow plist to be overriden in the nyssa config using the "plist" key.
  var src_plist = file(
    config.contains('plist') ? config.plist : os.join_paths(bundle_dir, 'Info.plist')
  )

  src_plist.copy(target_plist_path)
  file(target_plist_path, 'w').write(
    src_plist.read().replace('\${EXE}', config.name, false)
  )

  log.info('Successfully created target Plist.')

  # move runtime directory to resources
  os.rename(
    os.real_path(runtime_dir),
    os.join_paths(resources_dir, 'runtime')
  )
  log.info('Successfully packaged runtime')

  # copy the current directory to the resources app directory
  copy_directory(
    os.cwd(),
    os.join_paths(resources_dir, 'app')
  )
  log.info('Successfully packaged application')
  
  file(
    config.contains('icns') ? config.icns : os.join_paths(bundle_dir, 'icon.icns')
  ).copy(
    os.join_paths(resources_dir, 'icon.icns')
  )
  log.info('Successfully packaged application icon')

  # DO THIS LAST
  # move the bundler executable to the MacOS folder and rename to the app name
  file(os.join_paths(bundle_dir, 'bundle')).rename(
    os.join_paths(macos_dir, config.name)
  )
  file(os.join_paths(macos_dir, 'macos'), 'w') # we only need the file to exist.
  file(os.join_paths(macos_dir, config.name)).chmod(0c755)
  file(os.join_paths(resources_dir, 'runtime', 'blade')).chmod(0c755)

  log.info('Successfully packaged launcher')

  # remove the bundler directory
  os.remove_dir(bundle_dir, true)

  os.rename(
    target_src,
    os.join_paths(dest_dir, '${config.name}-${config.version}.app')
  )
}

def bundle_zip(source_os, target_os, config, dest_dir) {
  var target_src = _get_blade_bundler(source_os, target_os, config)

  var bundle_name = target_os == 'windows' ? 'bundle.exe' : 'bundle'
  var blade_exe_name = target_os == 'windows' ? 'blade.exe' : 'blade'

  var target_bundle_name = target_os == 'windows' ? config.name + '.exe' : config.name
  var bundle_dir = os.join_paths(target_src, 'bundle')

  # copy the current directory to the resources app directory
  copy_directory(
    os.cwd(),
    os.join_paths(target_src, 'app')
  )
  log.info('Successfully packaged application')

  # DO THIS LAST
  # move the bundler executable to the MacOS folder and rename to the app name
  file(os.join_paths(bundle_dir, 'bundle')).rename(
    os.join_paths(target_src, target_bundle_name)
  )
  file(os.join_paths(target_src, target_bundle_name)).chmod(0c755)
  file(os.join_paths(target_src, 'runtime', blade_exe_name)).chmod(0c755)

  log.info('Successfully packaged launcher')

  # remove the bundler directory
  os.remove_dir(bundle_dir, true)

  zip.compress(
    target_src,
    os.join_paths(dest_dir, '${config.name}-${config.version}.zip'),
    nil,
    true 
  )
}

def run(value, options, success, error) {
  var source_os = os.platform == 'osx' ? 'macos' : os.platform
  var target_os =  options.os == 'osx' ? 'macos' : options.os

  # ensure that target location exists
  options.dest = os.real_path(options.dest)

  var config
  catch {
    config = json.parse(config_file)
  } as config_error

  if config_error {
    log.error('Not in a Nyssa project!')
    return
  }

  # if the "bundle" key is given in nyssa.json and it contains a valid dictionary, 
  # we'll assume that our configuration is coming from there.
  if config.contains('bundle') and is_dict(config.bundle) {
    var tmp_config = config.bundle
    if !tmp_config.contains('name') {
      tmp_config['name'] = config.name
    }
    if !tmp_config.contains('version') {
      tmp_config['version'] = config.version
    }

    config = tmp_config
  }


  if !os.dir_exists(cache_dir) {
    os.create_dir(cache_dir)
  }
  if !os.dir_exists(tmp_dir) {
    os.create_dir(tmp_dir)
  }

  catch {
    using options.os {
      when 'macos' bundle_app(source_os, target_os, config, options.dest)
      default bundle_zip(source_os, target_os, config, options.dest)
    }
  } as error

  if error {
    log.exception(error)
  } else {
    log.info('Bundle generated successfully!')
  }
}
