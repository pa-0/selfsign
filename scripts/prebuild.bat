@echo off
pushd ..
call scripts\version.bat > res\version.h 2>nul
popd
exit /b 0