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
sudo brew install readline cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

### Linux

#### Debian, Ubuntu and their derivatives

Install the required dependencies using the `apt` package manager.

```sh
sudo apt update
sudo apt install build-essential libreadline-dev cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

#### Arch Linux

Install the required dependencies using the `pacman` package manager.

```sh
sudo pacman -Sy
sudo pacman -S --needed --noconfirm base-devel readline cmake
```

Proceed to the [Configure](#configure) section to configure your CMake build.

## Windows

> Starting from the 3rd of February 2021, Blade's officially supported compilers for the Windows environment is now the 
> TDM-GCC and WinLibs compiler. The decision to change the official compiler from Visual Studio and MSYS2 to TDM-GCC and WinLibs 
> is to allow a minial configuration effort while installing Blade as well as to allow us to develop Blade faster as trying to 
> be cross-compatible with Visual Studio has proven to slow down the growth of the language and the ecosystem and setting up 
> MSYS2 environment to compile Blade is more work than required for either TDM-GCC or WinLibs.
> 
> This also allows us to build valid Blade C extensions on Windows with less hassle. 
> Check out the [blade-ext-demo](https://github.com/blade-lang/blade-ext-demo) or any of the extension in the 
> [ext](https://github.com/blade-lang/blade/ext) directory for more info on how to write a valid C extension for Blade.

### Using TDM-GCC or WinLibs

To install Blade with TDM-GCC or WinLibs, install [TDM-GCC](https://github.com/jmeubank/tdm-gcc/releases/download/v10.3.0-tdm64-2/tdm64-gcc-10.3.0-2.exe)
or [WinLibs](https://github.com/brechtsanders/winlibs_mingw/releases/download/11.2.0-9.0.0-msvcrt-r5/winlibs-x86_64-posix-seh-gcc-11.2.0-mingw-w64-9.0.0-r5.zip) 
via the given links. Add TDM-GCC or WinLibs `bin` directory to your environment path. TDM-GCC also allows you to add to path during its installation. 
After this, run the following commands from the root of your Blade clone:

```terminal
cmake -B build -DCMAKE_MAKE_PROGRAM=mingw32-make -G "Unix Makefiles"
cmake --build build
```

The `blade` executable can be located in the `build/bin` folder. Simply add that folder to your system path to make it available
system-wide.

### Using Visual Studio

Install the 'Desktop Development with C++' workload using the Visual Studio installer. In the 'Installation Details'
section, make sure 'C++ CMake tools for Windows' checkbox is selected.

Should you wish to build from the command line, it is recommended to follow the next few steps in the
'Developer Command Prompt' or 'Developer PowerShell' shell for your version of Visual Studio.

You may instead prefer to open the project within the IDE, either by right-clicking the folder and selecting 'Open in
Visual Studio' or by choosing the 'Open Folder' option after the splash screen. In this case, you should select the
appropriate startup executable by choosing `blade.exe` from the 'Select Startup Item...' toolbar option in the bar above
the code editor.

### Using MSYS2 and MinGW

You may opt to use ported GCC or Clang compilers provided through the MSYS2 project.

This has only been tested using the `mingw64` toolchain. Open up a new `mingw64` terminal, and ensure you have the
required dependencies:

```sh
sudo pacman -Sy
sudo pacman -S --needed --noconfirm mingw-w64-x86_64-{cmake,ninja,toolchain}
```

Proceed to the [Configure](#configure) section to configure your CMake build, and follow the instructions as if you were
using these tools on a UNIX-like system.

## Autoinstall (Works on all OSX and Linux)

Run the following command (OSX):

```sh
bash <(curl -s https://raw.githubusercontent.com/blade-lang/blade/main/scripts/autoinstall-osx.sh)
```

Or in Linux:

```sh
bash <(curl -s https://raw.githubusercontent.com/blade-lang/blade/main/scripts/autoinstall-linux.sh)
```

## Configure

Configure CMake by creating an empty `build` directory in the source root directory, and running:

```sh
cmake -B build      # optionally, if you have `ninja-build` installed, you may use '-G Ninja' to speed up compile times.
```

CMake will have generated a Unix Makefiles, Ninja or Visual Studio project in the `build` directory.

## Build

Once the build files have been generated, either change to the `build` directory and run the appropriate `make`, `ninja`
or `msbuild` tool (the latter using the 'Developer Tools Command Prompt' on Windows), or ask CMake to call the correct
tool for you:

```sh
cmake --build build
```

The `blade` executable can be located in the `build` folder.
