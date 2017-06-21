@echo off
setlocal
call vars-vc10.bat
cl @build\tmp\compileNgqExecutableNgqCpp\options.txt src\main\cpp\util\exception.cc
endlocal
