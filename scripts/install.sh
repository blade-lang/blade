#!/bin/bash

set -u

abort() {
  printf "%s\n" "$@"
  exit 1
}

# Fail fast with a concise message when not using bash
# Single brackets are needed here for POSIX compatibility
# shellcheck disable=SC2292
if [ -z "${BASH_VERSION:-}" ]
then
  abort "Bash is required to interpret this script."
fi

# Check OS.
OS="$(uname)"
if [[ "${OS}" == "Linux" ]]
then
  IS_LINUX=1
elif [[ "${OS}" != "Darwin" ]]
then
  abort "Blade auto install is only supported on macOS and Linux."
fi

install_if_missing() {
  for value in "$@"
  do
    if [[ $(command -v "$value") == "" ]]
    then
      echo "$value is not installed. Attempting to install it!"

      if [[ "$value" == "cmake" && -x "$(command -v snap)" ]]; then
        # On Ubuntu with snap, snap is the correct way to get an up-to-date cmake version.
        snap install cmake --classic
      elif [ -x "$(command -v apk)" ]; then
        sudo apk add --no-cache "$value"
      elif [ -x "$(command -v apt-get)" ]; then
        sudo apt-get install "$value" -y
      elif [ -x "$(command -v dnf)" ]; then
        sudo dnf install "$value" -y
      elif [ -x "$(command -v zypper)" ]; then
        sudo zypper install "$value" -y
      elif [ -x "$(command -v yum)" ]; then
        sudo yum install "$value" -y
      elif [ -x "$(command -v pacman)" ]; then
        sudo pacman -Sy "$value"
      elif [ -x "$(command -v brew)" ]; then
        brew install "$value"
      else
        echo "Failed to install dependencies. Package manager not found."
        abort "You must manually install $value to continue"
      fi
    else
      echo "$value is already installed. Skipping..."
    fi
  done
}

install_build_env() {
  # shellcheck disable=SC2154
  if [[ $(command -v "make") == "" ]]
  then
    echo "Build environment is not setup. Setting it up"

    if [ -x "$(command -v apt-get)" ]; then
      sudo apt-get install build-essential -y
    elif [ -x "$(command -v apk)" ]; then
      sudo apk add --no-cache build-base
    elif [ -x "$(command -v dnf)" ]; then
      sudo dnf group install "C Development Tools and Libraries" -y
    elif [ -x "$(command -v zypper)" ]; then
      sudo zypper install -t pattern devel_basis -y
    elif [ -x "$(command -v yum)" ]; then
      sudo yum groups mark install "Development Tools" -y
      sudo yum groups mark convert "Development Tools" -y
      sudo yum groupinstall "Development Tools" -y
    elif [ -x "$(command -v pacman)" ]; then
      sudo pacman -Sy base-devel
    else
      echo "Failed to install dependencies. Package manager not found."
      abort "You must manually setup your build environment by installing make, c/c++ and related modules to continue"
    fi
  else
    echo "Build environment is already setup. Skipping..."
  fi
}

install_blade() {
  if [[ -d "$1" ]]; then
    mkdir -p "$1"
  fi

	# removing old/stale/partial objects
	STALE_PATHS=("/usr/local/bin/blade" "$(pwd)/blade" "$1/blade")
	for path in "${STALE_PATHS[@]}"
	do
	  if [[ -d "$path" ]]; then
	    echo "Removing old/stale/partial object '$path'..."
      sudo rm -rf "$path" || exit
    fi
  done

	# cloning
	git clone https://github.com/blade-lang/blade.git
	cd blade || exit

	# building
	if [[ "${OS}" == "Darwin" ]]
  then
	  cmake -B . -DOPENSSL_ROOT_DIR=/opt/homebrew/opt/openssl/ || exit
  else
	  cmake -B . || exit
  fi
	cmake --build . -- -j 16 || exit

	# We are copying to .blade here instead of just moving it to
	# blade directly in case the user runs this script from the
	# home directory.
	cp -r blade "$1/.blade"

	cd ..
	rm -rf blade

	# Now we can move blade back to the home directory.
	mv "$1/.blade" "$1/blade"

	# Now link the blade executable to path
	echo "Linking Blade..."
	sudo ln -s "$1/blade/blade" /usr/local/bin/blade
}

echo "Beginning installation of Blade..."

install_build_env

if [[ -z "${IS_LINUX-}" ]]
then

  #Install Homebrew if not installed...
  if [[ $(command -v 'brew') == "" ]]
  then
    echo "Homebrew not installed. Installing..."
    /bin/bash -c "$(curl -fsSL https://raw.githubusercontent.com/Homebrew/install/HEAD/install.sh)"
  else
    echo "Brew is installed. Using brew!"
  fi

  install_if_missing 'openssl'
else
  install_if_missing 'git' 'curl' 'libssl-dev' 'libcurl4-openssl-dev'
  if [[ $(command -v 'apt') != "" ]]; then
    install_if_missing 'zlib1g-dev'
  else
    install_if_missing 'zlib-devel'
  fi
fi

#Install cmake dependency.
install_if_missing 'cmake'

if [[ "${OS}" == "Darwin" ]]
then
  install_blade "/Users/$USER"
else
  install_blade "/home/$USER"
fi
