@echo off
rem Builds the core-layer tests on the PC. Needs Visual Studio 2022 Community.
rem
rem NOTE: keep this file pure ASCII. cmd.exe parses .bat with the system code
rem page (cp950 here), so Chinese in a rem line gets mangled and cmd tries to
rem execute the fragments as commands. Same rule as the .bat files in
rem rp2040-retro-dict. The Chinese write-up lives in README.md.
rem
rem /utf-8 is required: the tests use Chinese string literals on purpose,
rem because every bug worth catching here is on a multi-byte boundary.
call "C:\Program Files\Microsoft Visual Studio\2022\Community\VC\Auxiliary\Build\vcvars64.bat" >nul

cl /nologo /W4 /O2 /utf-8 /I%~dp0..\core /Fe:%~dp0test_textbuf.exe %~dp0test_textbuf.c %~dp0..\core\textbuf.c /Fo:%~dp0
if errorlevel 1 exit /b 1

cl /nologo /W4 /O2 /utf-8 /I%~dp0..\core /I%~dp0..\vendor /Fe:%~dp0test_glyph.exe %~dp0test_glyph.c %~dp0..\core\glyph.c /Fo:%~dp0
if errorlevel 1 exit /b 1

echo.
echo === textbuf ===
%~dp0test_textbuf.exe
if errorlevel 1 exit /b 1
echo.
echo === glyph ===
%~dp0test_glyph.exe

cl /nologo /W4 /O2 /utf-8 /I%~dp0..\core /I%~dp0..\vendor /Fe:%~dp0test_editor.exe %~dp0test_editor.c %~dp0..\core\editor.c %~dp0..\core\textbuf.c %~dp0..\vendor\ime.c /Fo:%~dp0
if errorlevel 1 exit /b 1
echo.
echo === editor ===
%~dp0test_editor.exe
