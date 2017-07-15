@echo off

SET CompilerFlags=/nologo /fp:fast /Z7 /MTd
REM SET CompilerFlags=/nologo /fp:fast /Z7 /MTd /Oi /Ox
SET LinkerFlags=/incremental:no user32.lib gdi32.lib

if not exist ..\build mkdir ..\build
pushd ..\build

cl.exe %CompilerFlags% ..\code\win32_snek.cpp /link %LinkerFlags%

popd
