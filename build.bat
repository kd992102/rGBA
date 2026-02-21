@echo off
REM ==============================================
REM rGBA emulator build script (new architecture)
REM ==============================================

echo [BUILD] Building rGBA emulator...

REM Compile all source files
gcc -I. -m32 -g -c main_new.c -o obj\main_new.o
if errorlevel 1 goto error

gcc -I. -m32 -g -c gba_core.c -o obj\gba_core.o
if errorlevel 1 goto error

gcc -I. -m32 -g -c gba_cpu.c -o obj\gba_cpu.o
if errorlevel 1 goto error

gcc -I. -m32 -g -c gba_cpu_arm_example.c -o obj\gba_cpu_arm_example.o
if errorlevel 1 goto error

gcc -I. -m32 -g -c gba_cpu_thumb.c -o obj\gba_cpu_thumb.o
if errorlevel 1 goto error

gcc -I. -m32 -g -c gba_memory.c -o obj\gba_memory.o
if errorlevel 1 goto error

REM Link executable
gcc -m32 -g obj\*.o -o bin\rGBA_new.exe -lmingw32 -lSDL2main -lSDL2
if errorlevel 1 goto error

echo [OK] Build complete: bin\rGBA_new.exe
goto end

:error
echo [ERROR] Build failed!
exit /b 1

:end
