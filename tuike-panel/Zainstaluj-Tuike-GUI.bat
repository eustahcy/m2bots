@echo off
rem Klikalny start okienkowego instalatora panelu Tuike.
rem -WindowStyle Hidden: okno konsoli nie jest do niczego potrzebne, caly
rem przebieg instalacji widac w oknie programu.
setlocal
cd /d "%~dp0"
powershell -NoProfile -ExecutionPolicy Bypass -WindowStyle Hidden -File "%~dp0Zainstaluj-Tuike-GUI.ps1"
