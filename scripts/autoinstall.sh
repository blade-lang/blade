#!/bin/bash
BLADE_DIR=~/.blade
TMP_PWD="$( pwd )"

__b_get_profile () {
	read -p "Please introduce your profile (default=~/.bashrc): "

	READ_REPLY=`echo $REPLY | sed 's/ *$//g'`

	if [[ -z "$READ_REPLY" ]]
	then
		B_PROFILE_VARS=~/.bashrc
	else
		B_PROFILE_VARS="$READ_REPLY"
	fi
}

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

	__b_get_profile

	echo "$B_PROFILE_VARS"

	if [[ ! -w "$B_PROFILE_VARS" ]]; then
		sudo echo "export PATH=\"$BLADE_DIR/build/bin/:\$PATH\"" >> "$B_PROFILE_VARS"
	else
		echo "export PATH=\"$BLADE_DIR/build/bin/:\$PATH\"" >> "$B_PROFILE_VARS"
	fi

	echo "Done."
	export PATH="$BLADE_DIR/build/bin/:$PATH"
}

autoinstall
cd "$TMP_PWD"
