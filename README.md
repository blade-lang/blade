What's Bird
============
Bird is an interpreted high-level general-purpose multi-paradigm 
programming language with an emphasis on simplicity, flexibility 
and expressiveness (the core pillars). It aims to maintain the 
core pillars of the language while still being powerful enough to
do anything.

### Features

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

### Directory Structure

| Directory | Contents 
|-----------|----------
| `benchmarks` | Contains the sample benchmarks for Bird (some are based on the Benchmarks games).
| `birdy-vscode-ext` | The visual studio code extension for Bird that enables syntax highlighting for the language.
| `deps` | Contains static libraries to be used for compiling when matching libraries are not found on the system.
| `docs` | Contains the source code Sphinx documentation for Bird.
| `libs` | Contains the Bird standard library.
| `src` | The source code of the Bird language including the native implementation of some Bird library classes and functions in the modules directory.
| `tests` | A few test cases that Bird implementation must pass.
| `winbuild` | Contains the windows build files for Bird.

### How to compile

For now, Bird can only build on one Windows, Linux and Macosx devices.

-   #### For Linux and OSX
  
    Simply run one of `make debug` or `make release` as 
    desired and you will get the compiled output in the `build` 
    directory. You may customize the output directory by simply
    changing it in the `Makefile`.
    
-   #### For Windows
    
    Open the solution file in `winbuild` in Visual Studio 2017 
    and above and build your desired release (`Debug` or `Release`).

### How to contribute

The standard. The general workflow is as follows.

1. Find/file an issue on the Issues tab.
2. Fork the repo.
3. Push your changes to a branch in your forked repo. For coding guidelines, see below.
4. Submit a pull request to Bird from your forked repo.

You can also contribute by finding bugs but not fixing them.
To simply submit a bug from your own repo, simply modify the
`BUGS.md` file in a new branch a submit a PR.

> NOTE: Bugs submitted via the `BUGS.md` file take priority.

You can also just mail your issues to [me](mailto:eqliqandfriends@gmail.com) directly.

### Coding Standard

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

### Sponsors

![JetBrains Logo](jetbrains.png)
