# Installing Nyssa

Setting up Nyssa is a very straight forward process that involves three easy steps.

1. Download the Nyssa package manager from [the official Nyssa website](https://nyssa.bladelang.com) or from [the Github repository releases page](https://github.com/blade-lang/nyssa/releases). It is recommended to download from the Nyssa website as the website will only have the most stable release for download.
2. Extract the downloaded `.zip` file to a choice location on your device.
3. Add the location of the extracted package to your system environment path to make the `nyssa` command available system-wide.

Various operating systems provides different mechanisms for adding a path to the environment so the steps may vary for your specific operating system.

Here's a few links for different operating systems showing how to do this.

| Operating systems | Instruction Link |
|------------------|------------------|
| Linux, macOS | [https://opensource.com/article/17/6/set-path-linux](https://opensource.com/article/17/6/set-path-linux) |
| Windows | [https://www.wikihow.com/Change-the-PATH-Environment-Variable-on-Windows](https://www.wikihow.com/Change-the-PATH-Environment-Variable-on-Windows) |


### Testing your installation

If you followed all the steps and successfully added the download directory to system path, open a new terminal session (may be required) and run the command `nyssa --version`.

You should see an output similar to the below.

```
Nyssa 0.1.0
Blade 0.0.75 (running on BladeVM 0.0.8)
```

You can also run the `nyssa` command without any arguments to see the full help information.

```
Usage: nyssa [ [-h] | [-v] ] [COMMAND]

OPTIONS:
  -h, --help                Show this help message and exit
  -v, --version             Show Nyssa version

COMMANDS:
  account <choice>          Manages a Nyssa publisher account
    create                    creates a new publisher account
    login                     login to a publisher account
    logout                    log out of a publisher account
    -r, --repo <value>        the repo where the account is located
  clean                     Clear Nyssa storage
    -c, --cache               clean packages cache
    -l, --logs                clean logs
    -a, --all                 clean everything
  info                      Shows current project information
  init                      Creates a new package in current directory
    -n, --name <value>        the name of the package
  install <value>           Installs a Blade package
    -g, --global              installs the package globally
    -c, --use-cache           enables the cache
    -r, --repo <value>        the repository to install from
  publish                   Publishes a repository
    -r, --repo <value>        repository url
  restore                   Restores all project dependencies
    -x, --no-cache            disables the cache
  serve                     Starts a local Nyssa repository server
    -p, --port <value>        port of the server (default: 3000)
    -n, --host <value>        the host ip (default: 127.0.0.1)
  uninstall <value>         Uninstalls a Blade package
    -g, --global              package is a global package
```

If you can see this, then you're all good.
