@echo off

echo Checking if vspkg is installed

where vcpkg.exe >nul 2>nul

IF NOT ERRORLEVEL 0 (
  echo Vcpkg not found in path
  echo Exciting...
  exit /b
)

echo Vcpkg is installed...
echo Checking processor architecture

if %PROCESSOR_ARCHITECTURE%==x86 (
  echo Processor architecture is x86...
  set LIBS=curl:x86-windows pcre2:x86-windows
) else (
  echo Processor architecture is x64...
  set LIBS=curl:x64-windows pcre2:x64-windows
)

echo\

for %%a in (%LIBS%) do (
  echo Installing %%a...
  echo\
  vcpkg.exe install %%a
  echo/
)

pause