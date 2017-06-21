@echo off
setlocal
call vars-vc10.bat
:: /?
dumpbin /exports build\libs\clbkenlm\shared\clbkenlm.dll
endlocal
