What's Bird
============
Bird is an interpreted high-level general-purpose multi-paradigm 
programming language with an emphasis on simplicity, flexibility 
and expressiveness (the core pillars). It aims to maintain the 
core pillars of the language while still being powerful enough to
do anything.

![code analysis](https://github.com/mcfriend99/Bird/actions/workflows/codeql.yml/badge.svg)
![linux build](https://github.com/mcfriend99/Bird/actions/workflows/linux-build.yml/badge.svg)
![osx build](https://github.com/mcfriend99/Bird/actions/workflows/osx-build.yml/badge.svg)
![windows build](https://github.com/mcfriend99/Bird/actions/workflows/windows-build.yml/badge.svg)




## Features

- Simple syntax and minimal keywords.
- Dynamically typed.
- Comprehensive builtin functions.
- Object-oriented (only supports single inheritance for now.
  Multiple inheritance is open for discussion).
- Exceptions.
- Closures.
- Custom iterable classes.
- Garbage collection.
- Stack-based VM.
- Lightweight.
- Highly portable.



### Documentation



## Directory Structure

| Directory | Contents 
|-----------|----------
| `benchmarks` | Contains the sample benchmarks for Bird (some are based on the Benchmark games).
| `birdy-vscode-ext` | The visual studio code extension for Bird that enables syntax highlighting for the language.
| `deps` | Contains static libraries to be used for compiling when matching libraries are not found on the system.
| `docs` | Contains the source code Sphinx documentation for Bird.
| `libs` | Contains the Bird standard library.
| `src` | The source code of the Bird language including the native implementation of some Bird library classes and functions in the modules directory.
| `tests` | A few test cases that Bird implementation must pass.



## How to build

For now, Bird can only build on one of Windows, Linux and Macosx devices.

You'll need `cmake` installed to build Bird. Both `Debug` and `Release` 
build types are supported.
For example, to build into the directory `build`, you can run the following 
commands on all three platforms to create a `Release` build:

```bash
cmake -B ./build -DCMAKE_BUILD_TYPE=Release
cmake --build ./build --config Release
```

For debug builds, 

```bash
cmake -B ./build -DCMAKE_BUILD_TYPE=Debug
cmake --build ./build --config Debug
```

> NOTE: For Windows, MSVC (Visual Studio) is the supported compiler.
> Other compilers such as MingW or Cygwin may work, but are not
> guaranteed to work.




## How to contribute

The standard. The general workflow is as follows.

1. Find/file an issue on the Issues tab.
2. Fork the repo.
3. Push your changes to a branch in your forked repo. For coding guidelines, see below.
4. Submit a pull request to Bird from your forked repo.

You can also contribute by finding bugs but not fixing them.
To simply submit a bug from your own repo, simply modify the
`BUGS.md` file in a new branch a submit a PR. Kindly ensure
what is being reported as a bug is not already captured in
`TODO.md`

> NOTE: Bugs submitted via the `BUGS.md` file take priority.

You can also just mail your issues to [me](mailto:eqliqandfriends@gmail.com) directly.



## Coding Standard

-   I decided to break from the popular camel case common to C style
    languages and went with snake cases. Honestly speaking, the only
    justifiable reason is because I think it looks cool. I know you
    may have a differing opinion, but I will really appreciate it
    that you keep to that in your PRs. I used this same style for both
    the C source and the core library.
    
    
-   For formatting, simply follow the LLVM guide minus the whole
    braces `{`/`}` on separate line thing. It looks really ugly.
    I advice you use the JetBrains CLion or Visual Studio Code
    IDE(s) to format your code before submitting for PR.
    
That simple!



## Sponsors

![JetBrains Logo](jetbrains.png)
