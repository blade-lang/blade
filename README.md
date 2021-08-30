<div align="center">
<p>
    <img width="120" src="./blade-icon.png?sanitize=true">
</p>
<h1>The Blade Programming Language</h1>

[Blade Language Website](https://bladelang.com) |
[Documentation](https://bladelang.com)

<div>

![code analysis](https://github.com/blade-lang/blade/actions/workflows/codeql.yml/badge.svg)
![linux build](https://github.com/blade-lang/blade/actions/workflows/linux-build.yml/badge.svg)
![osx build](https://github.com/blade-lang/blade/actions/workflows/osx-build.yml/badge.svg)
![windows build](https://github.com/blade-lang/blade/actions/workflows/windows-build.yml/badge.svg)

[![Gitter](https://badges.gitter.im/blade-lang/community.svg)](https://gitter.im/blade-lang/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

</div>

</div>

<br>

## What's Blade

Blade is a simple, fast, clean and dynamic language that allows you to develop complex applications 
quickly. Blade emphasises algorithm over syntax and for this reason, it has a very small but powerful 
syntax set with a very natural feel.

<br>

> ### HELP IS URGENTLY NEEDED TO TEST BLADE WELL ON WINDOWS.
> I currently lack a Windows device to test Blade on Windows.
> While I expect most features to work, I cannot guarantee that they do
> and therefore solicit the help of fellow and willing contributors who
> can actively test Blade on Windows devices.
> 
> Thanks!


## Features

- Simple syntax and minimal keywords.
- Dynamically typed.
- Fast.
- Comprehensive builtin functions.
- Object-oriented (only supports single inheritance for now.
  Multiple inheritance is open for discussion).
- Exceptions.
- Closures.
- Custom iterable classes.
- Garbage collection.
- Stack-based VM.
- REPL
- Lightweight.
- Highly portable.


## Documentation

Documentation is currently in-progress in the [blade-docs](https://github.com/blade-lang/blade-docs) repo.
You can read the [Blade language documentation](https://bladelang.com) online at [bladelang.com](https://bladelang.com).



## Directory Structure

| Directory | Contents 
|-----------|----------
| `benchmarks` | Contains the sample benchmarks for Blade (some are based on the Benchmark games).
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
