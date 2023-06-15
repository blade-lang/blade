# Install and Uninstall actions

Nyssa provides two installation hook (`post_install` and `cli`) and one uninstallation hook (`pre_uninstall`) that allows package and library authors to customize the installation and uninstallation experience and do many things such as downloading and building extra dependencies that may be written in other programming languages.

### `post_install`

The `post_install` configuration allows package authors to specify a Blade script that should be run after the package has been extracted into its destination directory. To specify a script to run after installation, add the `post_install` option to the `nyssa.json` file.

```
...
"post_install": "my_cli_script.b"
```

The _<scirpt_name>_ must be a path or filename relative to the root of the package.

### `pre_uninstall`

The `pre_uninstall` configuration is much like the `post_install` configuration, except that it runs just before a package is uninstalled. You can add it to the `nyssa.json` file in the same way as the `post_install` option.

```
...
"pre_uninstall": "my_cli_script.b"
```

### `cli`

The `cli` installation hook allows package authors to specify a script that serves as the CLI entry point to the application. When the _CLI_ script is specified, a CLI entry will be created at `.blade` for local installations or at the root of Blade for global installations. This files will be automatically removed during uninstallation.

For example, the testing framework `qi` specifies a CLI entry point. For this reason, when you install `qi` locally, you can run the Qi command by simply running the command `.blade/qi` (or `.blade\qi` for Windows) to run your tests. This is made possible because during installation, Nyssa will automatically create the corresponding command-line entry file for you.

For applications installated globally, the application will become available on the user terminal **via the name** of the application provided that Blade has been added to path during installation.

For example,

```
"cli": "my_cli_script.b"
```
