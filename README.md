<div align="center">
<p>
    <img width="120" src="./blade-icon.png?sanitize=true">
</p>
<h1>The Blade Programming Language</h1>

**Quick links**: [BUILDING](./BUILDING.md) | [CONTRIBUTING](./CONTRIBUTING.md) | [DOCS](https://bladelang.com) | 
[LICENSE](./LICENSE) | 
[![Chat on Gitter](https://badges.gitter.im/blade-lang/community.svg)](https://gitter.im/blade-lang/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge)

<div>

![code analysis](https://github.com/blade-lang/blade/actions/workflows/codeql.yml/badge.svg)
![Linux build](https://github.com/blade-lang/blade/actions/workflows/linux-build.yml/badge.svg)
![macOS build](https://github.com/blade-lang/blade/actions/workflows/osx-build.yml/badge.svg)
![Windows (MSVC) build](https://github.com/blade-lang/blade/actions/workflows/windows-msvc-build.yml/badge.svg)
![Windows (MinGW64) build](https://github.com/blade-lang/blade/actions/workflows/windows-mingw64-build.yml/badge.svg)

</div>

</div>

<br>

## tl;dr
---

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
---

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


## Example
---

The following code prints the sum of all numbers in an array.
```py
def sum(numbers) {
    var result = 0

    for num in numbers {
        result += num
    }

    echo 'the sum is ${result}'
}

sum([1, 2, 3, 4]) # this prints "the sum is 10"
```


## Documentation
---

Documentation is currently in-progress in the [blade-docs](https://github.com/blade-lang/blade-docs) repo.
You can read the [Blade language documentation](https://bladelang.com) online at [bladelang.com](https://bladelang.com).

## C Extensions to Blade
---

Blade supports external extensions built in C. While the website is yet to include documentation on writing C 
extensions, there is an easy-to-pick-up example in the [blade-ext-demo](https://github.com/blade-lang/blade-ext-demo) repository.

### GOTCHAS!

For some weird reasons yet unknown, thirdparty C modules for Blade don't work correctly on Blade installations
built with MSVC (Visual Studio). To be able to use external C modules on Windows, build or use a Blade installation 
built with either TDM-GCC or MinGW64.


## Directory Structure
---

| Directory | Contents 
|-----------|----------
| `benchmarks` | Contains the sample benchmarks for Blade (some are based on the Benchmark games).
| `libs` | Contains the Blade standard library.
| `src` | The source code of the Blade language including the native implementation of some Blade library classes and functions in the modules directory.
| `scripts` | Helper scripts for various uses such as automated installation.
| `tests` | A few test cases that Blade implementation must pass.
| `thirdparty` | Contains open-source libraries and packages used by Blade


## How to contribute
---

Along with Blade's goal to be simplicity, flexibility and expressiveness is a strong desire to make the community around it as friendly and welcoming as possible. Therefore, all forms of contributions from pull requests, suggestions, typo fixes in documentation, feature request, bug reports and any contribution at all is greatly welcomed and appreciated.

> WE NEED HELP! From review of this documentation, to suggestions on the core features of Blade,
testing of Blade features, writing more comprehensive tests, bug detection, code fixes and more.
PLEASE CONTRIBUTE!

### Contributing code, fixes and features
---

The standard. The general workflow is as follows.

1. Find/file an issue on the Issues tab.
2. Fork the repo and make your changes.
3. Push your changes to a branch in your forked repo. For coding guidelines, see the project [README](https://github.com/blade-lang/blade/blob/main/README.md) file.
4. Submit a pull request to Blade from your forked repo.

You can also just mail your issues to [Ore Richard Muyiwa](mailto:eqliqandfriends@gmail.com) directly.


## Coding Standard
---

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
---

[comment]: <> (![JetBrains Logo]&#40;jetbrains.png&#41;)

<img src="./jetbrains.png" width="64" height="64" alt="JetBrains Logo"/>
