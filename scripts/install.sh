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
  abort "Blade autoinstall is only supported on macOS and Linux."
fi

install_if_missing() {
  if [[ "$(command -v $@)" == "" ]]
  then
    echo "$@ is not installed. Attempting to install it!"

    # On Ubuntu with snap, snap is the correct way to get an up-to-date cmake version.
    if [[ "$@" == "cmake" && -x "$(command -v snap)" ]]; then snap install cmake --classic
    elif [ -x "$(command -v apk)" ]; then sudo apk add --no-cache $@ -y
    elif [ -x "$(command -v apt-get)" ]; then sudo apt-get install $@ -y
    elif [ -x "$(command -v dnf)" ]; then sudo dnf install $@ -y
    elif [ -x "$(command -v zypper)" ]; then sudo zypper install $@ -y
    elif [ -x "$(command -v yum)" ]; then sudo yum install $@ -y
    elif [ -x "$(command -v pacman)" ]; then sudo pacman -Sy $@
    elif [ -x "$(command -v brew)" ]; then brew install $@
    else
      echo "Failed to install dependencies. Package manager not found."
      abort "You must manually install $@ to continue"
    fi
  else
    echo "$@ is installed..."
  fi
}

install_blade() {
	# removing old/stale/partial objects
	echo "Removing old/stale/partial objects..."
	sudo rm /usr/local/bin/blade
	sudo rm -rf `pwd`/blade

	# cloning
	git clone https://github.com/blade-lang/blade.git
	cd blade

	# building
	cmake -B .
	cmake --build . -- -j 16

	# We are copying to .blade here instead of just moving it to
	# blade directly in case the user runs this script from the
	# home directory.
	cp -r blade $@/.blade

	cd ..
	rm -rf blade

	# Now we can move blade back to the home directory.
	mv $@/.blade $@/blade

	# Now link the blade executable to path
	sudo ln -s $@/blade/blade /usr/local/bin/blade
}

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
  install_if_missing 'readline'
else
  install_if_missing 'git'
  install_if_missing 'curl'
  install_if_missing 'libssl-dev'
  install_if_missing 'libreadline-dev'
  install_if_missing 'libcurl4-openssl-dev'
fi

#Install cmake dependency.
install_if_missing 'cmake'

install_blade "/home/$USER"
