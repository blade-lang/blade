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

__b_get_profile () {
	read -p "Please introduce the full path to your profile (default=~/.bashrc): "

	READ_REPLY=`echo $REPLY | sed 's/ *$//g'`

	if [[ -z "$READ_REPLY" ]]
	then
		B_PROFILE_VARS=~/.bashrc
	else
		B_PROFILE_VARS="$READ_REPLY"
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

	git clone https://github.com/blade-lang/blade .
	cmake -B .
	cmake --build .

	printf "\nBlade downloaded. Installing...\n"

	__b_get_profile

	if [[ ! -w "$B_PROFILE_VARS" ]]; then
		sudo echo "export PATH=\"$BLADE_DIR/blade:\$PATH\"" >> "$B_PROFILE_VARS"
	else
		echo "export PATH=\"$BLADE_DIR/blade:\$PATH\"" >> "$B_PROFILE_VARS"
	fi

	echo "Done."
	export PATH="$BLADE_DIR/blade:$PATH"
}

__b_autoinstall
cd "$TMP_PWD"
