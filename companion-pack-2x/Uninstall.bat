@echo off
powershell.exe -NoProfile -ExecutionPolicy Bypass -File "%~dp0tools\manage.ps1" -Mode Uninstall -ServerRoot "%~dp0.."
set "result=%errorlevel%"
pause
exit /b %result%
