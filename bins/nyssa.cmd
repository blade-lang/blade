@ECHO OFF
SETLOCAL
SET ROOT="%~dp0"
SET NYSSA_DIR="%ROOT%\\apps\\nyssa\\cli.b"
SET BLADE_EXE="%ROOT%\\blade.exe"

%BLADE_EXE% %NYSSA_DIR% %*
EXIT /B 0
