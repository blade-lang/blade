[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/blade-lang/nyssa/blob/master/LICENSE)
[![Coverage Status](https://coveralls.io/repos/github/blade-lang/nyssa/badge.svg?branch=main)](https://coveralls.io/github/blade-lang/nyssa?branch=main)
[![Version](https://img.shields.io/badge/version-0.1.4-green)](https://github.com/blade-lang/nyssa)

# Nyssa

Nyssa is the official package manager for the Blade programming language. It is also a self-hostable repository server that allows you to easily manage and distribute packages for your Blade projects.

#### The CLI

![Nyssa CLI](https://raw.githubusercontent.com/blade-lang/nyssa/main/nyssa-cli.png)

#### The browsable repository website.

![Nyssa Repository](https://raw.githubusercontent.com/blade-lang/nyssa/main/nyssa.png)


## Features

- [x] Create packages.
- [x] Manage application dependencies 
  - [x] Install package
  - [x] Uninstall package
  - [x] Update (Install without specifying a version)
  - [x] Restore package
  - [x] Publish package
- [x] Built-in hostable repository server.
- [x] Publish package to public and private repositories.
- [x] Nyssa repository server public API.
- [x] Nyssa repository server searchable frontend website.
- [x] Manage publisher accounts.
  - [x] Create publisher account
  - [x] Login to publisher account
  - [x] Logout from publisher account
- [x] Custom Post-Installation script support.
- [x] Custom Pre-Uninstallation script support.
- [ ] Generate application/library documentation.


## Installation

- Clone the repository to the desired host directory using the command below:
  
  ```sh
  git clone https://github.com/blade-lang/nyssa.git
  ```

- Add the full path to `nyssa` to path (steps depend on operating system).


## Documentation

To read the documentation, see the [Getting started](https://nyssa.bladelang.com/docs) guide on the website.


## Contributing

We welcome contributions from the community. Please see the [CONTRIBUTING.md](CONTRIBUTING.md) file for more information on how you can help.


## License

Nyssa is released under the [MIT license](LICENSE).
