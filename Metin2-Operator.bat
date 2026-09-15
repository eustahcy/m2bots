@echo off
rem Konsola operatora: publikacja na GitHub i sterowanie serwerem na VPS.
rem Dla gracza jest Metin2-Launcher-GUI.bat - ten plik go nie zastepuje.
powershell -NoProfile -ExecutionPolicy Bypass -STA -File "%~dp0tools\Metin2-Operator-GUI.ps1"
if errorlevel 1 pause
