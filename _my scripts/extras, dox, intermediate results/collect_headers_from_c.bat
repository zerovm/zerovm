@echo off
if "%1" == "" goto HELP

for /r %%a in (.) do (cd "%%a"&find "#include" *.c >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#include" *.cc >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "# include" *.c >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "# include" *.cc >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#  include" *.c >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#  include" *.cc >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#   include" *.c >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#   include" *.cc >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#    include" *.c >> "%~dpnx1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#    include" *.cc >> "%~dpnx1")
cd "%~dp1"

echo !!!!!!!!!!!!!!!!!
echo sort [ "%~1" ] "kill.me"
sort < "%~1" > "kill.me"
::move "kill.me" "%~1"
::if exist "kill.me" del "kill.me"
goto EXIT

:HELP
echo usage: collect_headers_from_c {list file}

:EXIT
