@echo off
rem Klikalny start instalatora panelu Tuike.
rem Windows domyslnie nie pozwala uruchomic pliku .ps1 podwojnym klknieciem,
rem wiec ten plik robi to za uzytkownika (-ExecutionPolicy Bypass dotyczy
rem tylko tego jednego uruchomienia i nic nie zmienia w systemie).
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -File "%~dp0Zainstaluj-Tuike.ps1" %*
echo.
pause
