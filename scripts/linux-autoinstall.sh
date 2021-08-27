#!/bin/bash
BLADE_DIR=~/.blade

autoinstall () {
	if test -d "$BLADE_DIR"; then
		read -p "$BLADE_DIR already exists. Do you want to remove it? [Yn] " -r

		if [[ ! $REPLY =~ ^[Yy]$ ]]
		then
			exit 1
		fi

		rm -rf "$BLADE_DIR"
	fi

	mkdir -p "$BLADE_DIR"

	git clone https://github.com/blade-lang/blade "$BLADE_DIR"
	cmake -B "$BLADE_DIR/build" -DCMAKE_BUILD_TYPE=Release
	cmake --build "$BLADE_DIR/build" --config Release

	echo "#!/bin/bash" | sudo tee /bin/blade > /dev/null
	echo "$BLADE_DIR/blade/bin/blade" | sudo tee -a /bin/blade > /dev/null
}

autoinstall
