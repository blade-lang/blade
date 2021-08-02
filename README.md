What's Blade
============
Blade is an interpreted high-level general-purpose multi-paradigm 
programming language with an emphasis on simplicity, flexibility 
and expressiveness (the core pillars). It aims to maintain the 
core pillars of the language while still being powerful enough to
do anything.

![code analysis](https://github.com/blade-lang/blade/actions/workflows/codeql.yml/badge.svg)
![linux build](https://github.com/blade-lang/blade/actions/workflows/linux-build.yml/badge.svg)
![osx build](https://github.com/blade-lang/blade/actions/workflows/osx-build.yml/badge.svg)
![windows build](https://github.com/blade-lang/blade/actions/workflows/windows-build.yml/badge.svg)




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



## Documentation

Documentation is currently in-progress in the [blade-docs](https://github.com/blade-lang/blade-docs) repo.
You can read the [Blade language documentation](https://bladelang.com) online at [bladelang.com](https://bladelang.com).



## Directory Structure

| Directory | Contents 
|-----------|----------
| `benchmarks` | Contains the sample benchmarks for Blade (some are based on the Benchmark games).
| `bladey-vscode-ext` | The visual studio code extension for Blade that enables syntax highlighting for the language.
| `deps` | Contains static libraries to be used for compiling when matching libraries are not found on the system.
| `docs` | Contains the source code Sphinx documentation for Blade.
| `libs` | Contains the Blade standard library.
| `src` | The source code of the Blade language including the native implementation of some Blade library classes and functions in the modules directory.
| `tests` | A few test cases that Blade implementation must pass.



## How to build

For now, Blade can only build on one of Windows, Linux and Macosx devices.

You'll need `cmake` installed to build Blade. Both `Debug` and `Release` 
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

Along with Blade's goal to be simplicity, flexibility and expressiveness is a strong desire to make the community around it as friendly and welcoming as possible. Therefore, all forms of contributions from pull requests, suggestions, typo fixes in documentation, feature request, bug reports and any contribution at all is greatly welcomed and appreciated.

> WE NEED HELP! From review of this documentation, to suggestions on the core features of Blade,
testing of Blade features, writing more comprehensive tests, bug detection, code fixes and more.
PLEASE CONTRIBUTE!

### Contributing code, fixes and features

The standard. The general workflow is as follows.

1. Find/file an issue on the Issues tab.
2. Fork the repo and make your changes.
3. Push your changes to a branch in your forked repo. For coding guidelines, see the project [README](https://github.com/blade-lang/blade/blob/main/README.md) file.
4. Submit a pull request to Blade from your forked repo.

You can also just mail your issues to [Ore Richard Muyiwa](mailto:eqliqandfriends@gmail.com) directly.


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

[comment]: <> (![JetBrains Logo]&#40;jetbrains.png&#41;)

<img src="./jetbrains.png" width="64" height="64" alt="JetBrains Logo"/>
