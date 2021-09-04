#!/bin/bash
BLADE_DIR=~/.blade
TMP_PWD="$( pwd )"

autoinstall () {
	if [[ "$(whoami)" == "root" ]]; then
		echo "error: Running as root"
		exit 1
	fi

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

	git clone https://github.com/blade-lang/blade .
	cmake -B build -DCMAKE_BUILD_TYPE=Release
	cmake --build build --config Release

	printf "\nBlade downloaded. Installing...\n"

	echo "Done. Please add \"$BLADE_DIR/build/bin/\" to your PATH."
	export PATH="$BLADE_DIR/build/bin/:$PATH"
}

autoinstall
cd "$TMP_PWD"
