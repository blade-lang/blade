<div align="center">
    <img height="96" alt="Blade Logo" src="https://raw.githubusercontent.com/blade-lang/blade/main/blade.png">
</div>

# Blade Programming Language

[![Build Status](https://github.com/blade-lang/blade/actions/workflows/ci.yml/badge.svg)](https://github.com/blade-lang/blade/actions)
[![Gitter](https://badges.gitter.im/blade-lang/community.svg)](https://gitter.im/blade-lang/community)
[![License](https://img.shields.io/badge/License-MIT-blue.svg)](https://github.com/blade-lang/blade/blob/master/LICENSE)
[![Coverage Status](https://coveralls.io/repos/github/blade-lang/blade/badge.svg?branch=main)](https://coveralls.io/github/blade-lang/blade?branch=main)
[![Version](https://img.shields.io/badge/version-0.0.8-green)](https://github.com/blade-lang/blade)

Blade is a simple, clean, and embeddable dynamic programming language created to be simple enough for beginners, yet powerful and expressive for professionals. It has a very small syntax set with a very low learning curve. Blade improves upon the best features of JavaScript, Python, and Ruby to give developers a familiar and powerful system that feels native for developers coming from any of these languages and an easy way to leverage the strength of any.

Blade also comes with [Nyssa](https://github.com/blade-lang/nyssa), the official package manager, which makes it easy to install and manage packages and modules.

## Features

- **First-class package management**: Package management is built into the language module system.
- **Iterable classes**: Blade has built-in support for iterable classes, making it easy to work with collections of data.
- **Backend development**: Blade can be used for backend development without any external dependencies.
- **Function promotion**: Blade allows for function promotion, making it easy to reuse any piece of code from an imported module.
- **Default exports and imports**: Blade supports default exports and imports, making it easy to organize code.
- **Anonymous functions**: Blade supports anonymous functions, making it easy to write functional code.
- **Access modifiers**: Unlike any of JavaScript, Python and Ruby, Blade supports access modifiers for variables, properties, functions, classes, modules, etc.
- **Decorator functions**: Blade supports decorator functions, making it easy to add functionality to existing code.
- **Function overrides**: Blade supports function overrides in classes, allowing a class to choose the result to return for a function if the function allows it.
- **Easy to extend with C modules**: Blade supports external extensions built in C, making it easy to extend the language with C modules.

## Use Cases

Blade can be used for a wide range of tasks, including:

- **Web development**: Blade can be used to create web applications and web services.
- **Data science**: Blade can be used to perform data analysis and machine learning tasks.
- **Scripting**: Blade can be used to write scripts to automate tasks.
- And more...

## Showcase

- **[Nyssa](https://github.com/blade-lang/nyssa)**: The Nyssa self-hostable repository server and package manager is written in Blade.
- **[jsonrpc](https://github.com/mcfriend99/jsonrpc)**: A JSON-RPC library for Blade programming language.
- **[wire](https://github.com/mcfriend99/wire)**: Dynamic HTML template engine.
- **[tar](https://github.com/mcfriend99/tar)**: Pure Blade library for creating and extracting TAR archives.

## Example

The following code implements a simple backend API that runs on port 3000:

```js
import http
import json

var server = http.server(3000)
server.on_receive(|request, response| {
  response.headers['Content-Type'] = 'application/json'
  response.write(json.encode(request))
})

echo 'Listening on Port 3000...'
server.listen()
```

## How does it differ from Python and Ruby?

- First-class package management (Package management is built into the language module system).
- Iterable classes.
- Backend development without any external dependencies.
- [Function promotion](https://bladelang.com/tutorial/modules.html#function-promotion).
- Default exports and imports.
- Anonymous functions.
- Access modifiers for variables, properties, function, class, modules, etc.
- Decorator functions.
- Function overrides in classes &mdash; A class can chose the result to return for a function if the function allows it.
- Easy to extend with C modules with a familiar API to Wren.

## Installation

To install Blade, please follow the instructions in the [Building](./BUILDING.md) guide.

## Usage

To start using Blade, please refer to the [Getting Started](https://bladelang.com/tutorial/getting-started.html) guide in the documentation.

## API Documentaion

API documentation for Blade which is under active development can be found at [bladelang.com](https://bladelang.com/docs/).

## Community

- Join the conversation on [Gitter](https://gitter.im/blade-lang/community)
<!-- - Follow us on Twitter -->
- Submit a [feature request](https://github.com/blade-lang/blade/issues/new?labels=feature-request) or [bug report](https://github.com/blade-lang/blade/issues/new?labels=bug).

## Contributing

Blade desire to make the community around it as friendly and welcoming as possible. Therefore, all forms of contributions from pull requests, suggestions, typo fixes in documentation, feature request, bug reports and any contribution at all is highly appreciated. Please refer to the [Contributing](./CONTRIBUTING.md) guide for more information.

## License

Blade is licensed under the [2-clause BSDL License](https://github.com/blade-lang/blade/blob/master/LICENSE).
