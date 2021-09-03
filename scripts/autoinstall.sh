#!/bin/bash
BLADE_DIR=~/.blade

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

	git clone https://github.com/blade-lang/blade "$BLADE_DIR"
	cmake -B "$BLADE_DIR/build" -DCMAKE_BUILD_TYPE=Release
	cmake --build "$BLADE_DIR/build" --config Release

	printf "\nBlade downloaded. Installing...\n"

	if [[ "$SHELL" == *zsh ]]; then
		B_PROFILE_VARS=~/.bash_profile
	elif [[ "$SHELL" == *bash ]]; then
		B_PROFILE_VARS=~/.zshrc
	else
		echo "Unsupported terminal. Please manually add \"$BLADE_DIR/build/bin/\" to your PATH."
		exit 1
	fi

	echo "export PATH=\"$BLADE_DIR/build/bin/:\$PATH\"" >> "$B_PROFILE_VARS"
	echo "Done. Restart your terminal or run \`. $B_PROFILE_VARS\` to reload PATH."
}

autoinstall
