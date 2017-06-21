@echo off
setlocal
call vars-gradle.bat

set GRA_CACHE_MODULES=%GRADLE_USER_HOME%\caches\modules-2
set CUR_SUFFIX=clarabridge\clbkenlm-windows

rd /s /q %GRA_CACHE_MODULES%\metadata-2.23\descriptors\%CUR_SUFFIX%
rd /s /q %GRA_CACHE_MODULES%\files-2.1\%CUR_SUFFIX%
endlocal
