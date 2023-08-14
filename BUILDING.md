# Building Blade

Blade itself is a C11 application using the CMake build system, and therefore, building Blade should be no different to
any other application built using the same tools.

## Prerequisites

### macOS

Make sure you have the Xcode CLT installed. The [Homebrew package manager](https://brew.sh) is the preferred way to
install dependencies on macOS.

```sh
sudo xcode-select    # prompt the user to install Xcode CLT if it is not already installed
sudo brew update
sudo brew install openssl cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

### Linux

#### Debian, Ubuntu and their derivatives

Install the required dependencies using the `apt` package manager.

```sh
sudo apt update
sudo apt install build-essential libssl-dev cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

#### Arch Linux

Install the required dependencies using the `pacman` package manager.

```sh
sudo pacman -Sy
sudo pacman -S --needed --noconfirm base-devel openssl cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

#### RedHat, Fedora, CentOS and their derivatives

Install the required dependencies using the `yum` package manager.

```sh
sudo yum check-update
sudo yum groupinstall 'Development Tools'
sudo yum install -y openssl-devel cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

### Windows

> Starting from the 3rd of February 2021, Blade's officially supported compilers for the Windows environment is now the 
> WinLibs and TDM-GCC compiler. The decision to change the official compiler from Visual Studio and MSYS2 to WinLibs and TDM-GCC 
> is to allow for minial configuration effort while installing Blade as well as to allow us to develop Blade faster as trying to 
> be cross-compatible with Visual Studio has proven to slow down the growth of the language and the ecosystem and setting up 
> MSYS2 environment to compile Blade is more work than required for either WinLibs or TDM-GCC.
> 
> This also allows us to build valid Blade C extensions on Windows with less hassle. 
> Check out the [blade-ext-demo](https://github.com/blade-lang/blade-ext-demo) or any of the extension in the 
> [packages](https://github.com/blade-lang/blade/packages) directory for more info on how to write a valid C extension for Blade.

#### Using WinLibs or TDM-GCC with vcpkg

To install Blade with WinLibs or TDM-GCC, install 
[WinLibs](https://github.com/brechtsanders/winlibs_mingw/releases/download/11.2.0-9.0.0-msvcrt-r5/winlibs-x86_64-posix-seh-gcc-11.2.0-mingw-w64-9.0.0-r5.zip) 
or [TDM-GCC](https://github.com/jmeubank/tdm-gcc/releases/download/v10.3.0-tdm64-2/tdm64-gcc-10.3.0-2.exe) 
via the given links. Add WinLibs or TDM-GCC `bin` directory to your environment path. TDM-GCC also allows you to add to path during its installation. 

Next, install [vcpkg](https://vcpkg.io/en/index.html) following the instruction [here](https://vcpkg.io/en/getting-started.html)
and add `vcpkg` to your environment. After this, run the commands below to install the required dependencies:

```bat
vcpkg install curl:x64-windows libffi:x64-windows openssl:x64-windows
```

If you are on an `x86` system, you can also install the x86 versions of the dependencies using the command:

```shell
vcpkg install curl:x86-windows libffi:x86-windows openssl:x86-windows
```

## Automated Build and Install (Works on all OSX and Linux)

```shell
bash <(curl -s https://raw.githubusercontent.com/blade-lang/blade/main/scripts/install.sh)
```

## Configure

Configure CMake by creating an empty `build` directory in the source root directory, and running the appropriate command.

> During configure, other required dependencies will be downloaded automatically the first time you do a configure so you should ensure you have a good internet connection if you are running configure for the first time.

### Unix (Linux, macOS)

```sh
cmake -B build
```

Optionally, if you have `ninja-build` installed, you may use `-G Ninja` to speed up compile times. For example:

```sh
cmake -B build -G Ninja
```

CMake will have generated a Unix Makefiles or Ninja build files in the `build` directory.

If you get an error regards missing `OPENSSL_ROOT_DIR` on macOS add the path to `openssl` which should be located at `/usr/local/opt/openssl` or somewhere in `/opt/homebrew/Cellar/` 
(for example `/opt/homebrew/Cellar/openssl@3/3.0.7`). Your configure command should look like this:

```sh
cmake -B build -DOPENSSL_ROOT_DIR=/usr/local/opt/openssl
```

### Windows

If you are configuring on Windows, you'll need to specify the `make` program as well as the `vcpkg` toolchain file.

```shell
cmake -B build -DCMAKE_MAKE_PROGRAM=mingw32-make -G "Unix Makefiles" -DCMAKE_TOOLCHAIN_FILE="${PATH_TO_VCPKG}/scripts/buildsystems/vcpkg.cmake"
```

On Windows, it is also mostly common to use the `ninja` build system when available. For this, you'll only need to replace the `mingw32-make` with `ninja` in the command.

## Build

Once the build files have been generated, either change to the `build` directory and run the appropriate `make` or 
`ninja` tool (the latter using the 'Developer Tools Command Prompt' on Windows), or ask CMake to call the correct 
tool for you:

```sh
cmake --build build
```

Blade will be built into a directory called `blade` which will be located in the `build` folder.
