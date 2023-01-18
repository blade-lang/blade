<div align="center">
<p>
    <img width="128" alt="Blade Logo" src="./blade-icon.png?sanitize=true">
</p>
<h1>The Blade Programming Language</h1>

**Quick links**: ![CI](https://github.com/blade-lang/blade/actions/workflows/ci.yml/badge.svg)  | 
[Building](./BUILDING.md)  |  [Contributing](./CONTRIBUTING.md)  |  [Documentation](https://bladelang.com)  |  [License](./LICENSE)

[comment]: <> ([![Chat on Gitter]&#40;https://badges.gitter.im/blade-lang/community.svg&#41;]&#40;https://gitter.im/blade-lang/community?utm_source=badge&utm_medium=badge&utm_campaign=pr-badge&#41; |)

</div>

<br>

## tl;dr

Blade is a simple, clean and embeddable dynamic programming language created to be simple enough for beginners, 
yet powerful and expressive for professionals and for this reason it has a very small syntax set with a very low 
learning curve. 

Blade improves upon the best features of JavaScript, Python and Ruby to give developers a familiar and powerful 
system that feels native for developers coming from any of these languages and an easy way to leverage the strength 
of any.

> Introducing [Nyssa Package Manager](https://nyssa.bladelang.com). The official package manager for the Blade programming language.

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


## Example

The following implements a simple HTTP server that listens on port 3000.

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

## Documentation

Documentation for the Blade programming language currently lives at [bladelang.com](https://bladelang.com) and the 
source is hosted on GitHub at [blade-lang/blade-docs](https://github.com/blade-lang/blade-docs).

## C Extensions to Blade

Blade supports external extensions built in C. While the documentation is yet to include documentation on writing C 
extensions, there is an easy-to-pick-up example in the [blade-ext-demo](https://github.com/blade-lang/blade-ext-demo) 
repository and many more in the `packages` directory.


## Directory Structure

| Directory | Contents |
|-----------|----------|
| `benchmarks` | Contains the sample benchmarks for Blade (some are based on the Benchmark games). |
| `libs` | Contains the Blade standard library. |
| `packages` | Contains the Blade standard library members built as packages because they depend on other open-source tools. |
| `src` | The source code of the Blade language including the native implementation of some Blade library classes and functions in the modules directory. |
| `scripts` | Helper scripts for various uses such as automated installation. |
| `tests` | A few test cases that Blade implementation must pass. |
| `thirdparty` | Contains open-source libraries and packages used by Blade. |


## How to contribute

Blade desire to make the community around it as friendly and welcoming as possible. Therefore, all forms of 
contributions from pull requests, suggestions, typo fixes in documentation, feature request, bug reports and any 
contribution at all is greatly welcomed and appreciated.

> WE NEED HELP! From review of the documentation, to suggestions on the core features of Blade,
testing of Blade features, writing more comprehensive tests, bug detection, code fixes and more.
PLEASE CONTRIBUTE!

### Contributing code, fixes and features

The standard. The general workflow is as follows.

1. Find/file an issue on the Issues tab.
2. Fork the repo and make your changes.
3. Push your changes to a branch in your forked repo.
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
    braces `{`/`}` on separate line thing. It doesn't look really nice in 
    my own opinion.
    I advise you use the JetBrains CLion or Visual Studio Code
    IDE(s) to format your code before submitting for PR.
    
It's that simple!
