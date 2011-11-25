@echo off
if "%1" == "" goto HELP

for /r %%a in (.) do (cd "%%a"&find "#include" *.c >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "# include" *.c >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#  include" *.c >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#   include" *.c >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#    include" *.c >> "%~1")
cd "%~dp1"

for /r %%a in (.) do (cd "%%a"&find "#include" *.cc >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "# include" *.cc >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#  include" *.cc >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#   include" *.cc >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#    include" *.cc >> "%~1")
cd "%~dp1"

for /r %%a in (.) do (cd "%%a"&find "#include" *.s >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "# include" *.s >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#  include" *.s >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#   include" *.s >> "%~1")
cd "%~dp1"
for /r %%a in (.) do (cd "%%a"&find "#    include" *.s >> "%~1")
cd "%~dp1"

sort < "%~1" > "kill.me"
move "kill.me" "%~1"
if exist "kill.me" del "kill.me"
goto EXIT

:HELP
echo usage: collect_headers_from_h {list file}

:EXIT
