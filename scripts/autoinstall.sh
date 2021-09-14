#!/bin/bash
BLADE_DIR=~/.blade
TMP_PWD="$( pwd )"

__b_check_versions () {
	if ! command -v cmake &> /dev/null
	then
		echo "CMake could not be found"
		exit
	fi
}

__b_autoinstall () {
	if [[ "$(whoami)" == "root" ]]; then
		echo "error: Running as root"
		exit 1
	fi

	__b_check_versions

	if [[ -d "$BLADE_DIR" ]] || [[ -f "$BLADE_DIR" ]]; then
		read -p "$BLADE_DIR already exists. Do you want to remove it? [Yn] " -r

		READ_REPLY=`echo $REPLY | sed 's/ *$//g'`

		if [[ ! -z "$READ_REPLY" ]] && [[ ! "$READ_REPLY" =~ ^[Yy]$ ]]
		then
			exit 1
		fi

		rm -rf "$BLADE_DIR"
  fi

	mkdir -p "$BLADE_DIR"
	cd "$BLADE_DIR"

	git clone https://github.com/blade-lang/blade . > /dev/null
	cmake -B build -DCMAKE_BUILD_TYPE=Release > /dev/null
	cmake --build build --config Release

	BLADE_BIN="$BLADE_DIR/build/bin"
	BLADE_ENV="$BLADE_DIR/env"

	printf "export PATH=\"$BLADE_BIN:\$PATH\"\n" >> "$BLADE_ENV"
	printf "BLADE_DIR=\"$BLADE_DIR\"\n" >> "$BLADE_ENV"
	printf "BLADE=\"$BLADE_BIN/blade\"\n" >> "$BLADE_ENV"

	B_LOAD_BLADE_ENV=". \"$BLADE_ENV\""

	if [[ "$( cat ~/.profile )" != *"$B_LOAD_BLADE_ENV" ]]; then
		if [ -f ~/.bash_profile ]; then
			echo "" >> ~/.bash_profile # newline
			echo "$B_LOAD_BLADE_ENV" >> ~/.bash_profile
		fi
	
		echo "" >> ~/.profile # newline
		echo "$B_LOAD_BLADE_ENV" >> ~/.profile
	fi

	echo "Done."
	. "$BLADE_ENV"
}

__b_autoinstall
cd "$TMP_PWD"
