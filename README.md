<div align="center">
    <img height="96" alt="Blade Logo" src="https://raw.githubusercontent.com/blade-lang/blade/main/blade.png">
</div>

# Blade Programming Language

[![Build Status](https://github.com/blade-lang/blade/actions/workflows/ci.yml/badge.svg)](https://github.com/blade-lang/blade/actions)
[![Gitter](https://badges.gitter.im/blade-lang/community.svg)](https://gitter.im/blade-lang/community)
[![License](https://img.shields.io/badge/License-BSD_2--Clause-orange.svg)](https://github.com/blade-lang/blade/blob/master/LICENSE)
[![Version](https://img.shields.io/badge/version-0.0.86-green)](https://github.com/blade-lang/blade)

Blade is a modern general-purpose programming language focused on enterprise Web, IoT, and secure application development. Blade offers a comprehensive set of tools and libraries out of the box leading to reduced reliance on third-party packages. 

Blade comes equipped with an integrated package management system, simplifying the management of both internal and external dependencies and a self-hostable repository server making it ideal for private organizational and personal use. Its intuitive syntax and gentle learning curve ensure an accessible experience for developers of all skill levels. Leveraging the best features from JavaScript, Python, Ruby, and Dart, Blade provides a familiar and robust ecosystem that enables developers to harness the strengths of these languages effortlessly.

## Example

The following code implements a simple backend API that runs on port 3000:

```blade
import http

var server = http.server(3000)
server.handle('GET', '/', @(req, res) {
  res.json(req)
})

echo 'Listening on Port 3000...'
server.listen()
```

## What's interesting about Blade

- **Built-in package manager and repository server:** Package management is built into the language module system. Blade also comes with `Nyssa`. Nyssa is a package manager and self-hosted repository server highly suitable for private use.
- **Zero-dependency full-stack web development**: Blade comes with a built-in web server and a rich set of tools and libraries that support it, making it easy to build composable full-stack web applications out of the box:
  - Built-in Model-View-Template (MVT) based HTTP web server.
  - Built-in testing framework.
  - Built-in support for multiple databases.
  - Built-in web template engine &mdash; `Wire`.
  - Built-in routing library.
  - Built-in mail library with SMTP, IMAP, and POP3 support.
  - Built-in device integrations (such as support for COM/Ports, USB, etc.) &mdash; Planned!
  - Built-in cryptography library.
  - Built-in support for media processing (Image - Done, audio, video, etc.) &mdash; Planned!
  - And more.
- **Function promotion**: A feature of the Blade language that makes it easy to reuse any code from an imported module.
- **Access modifiers**: Unlike JavaScript and Python, Blade supports access modifiers for variables, properties, functions, classes, modules, etc.
- **Decorator functions**: Decorator functions are a set of class methods in Blade that makes extending the functionality of existing code super easy.
- **Easy to extend with C modules**: Blade supports external extensions built in C with a built-in extension compiler via `Nyssa`. This feature makes it easy to extend language features with C modules.

## Showcase of other uses

While Blade focuses on Web and IoT, it is also great for general software development. Below are a few showcases of libraries using Blade for other impressive stuff:

- **[jsonrpc](https://github.com/mcfriend99/jsonrpc)**: A JSON-RPC library for Blade programming language.
- **[tar](https://github.com/mcfriend99/tar)**: Pure Blade library for creating and extracting TAR archives.

## Installation

To install Blade, please follow the instructions in the [Building](./BUILDING.md) guide.

## Usage

To start using Blade, please refer to the [Tutorial](https://bladelang.org/tutorial/index.html) section of the online documentation.

## API Documentation

API documentation for Blade is under active development and can be found at [bladelang.org](https://bladelang.org/standard/index.html).

## Community

- Join the conversation on [Gitter](https://gitter.im/blade-lang/community)
- Submit a [feature request](https://github.com/blade-lang/blade/issues/new?labels=feature-request) or [bug report](https://github.com/blade-lang/blade/issues/new?labels=bug).
<!-- - Follow us on Twitter -->

## Contributing

We need your help to make Blade great! The Blade community is as friendly and welcoming as possible. All kinds of contributions like pull requests, suggestions, typo fixes in documentation, feature request, bug reports, and others are highly appreciated. Please refer to the [Contributing](./CONTRIBUTING.md) guide for more information.

## License

Blade is licensed under the [2-clause BSDL License](https://github.com/blade-lang/blade/blob/master/LICENSE).
