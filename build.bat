@echo off
REM ==============================================
REM rGBA 模拟器编译脚本（新架构）
REM ==============================================

echo [编译] 编译 rGBA 模拟器...

REM 编译所有源文件
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

REM 链接生成可执行文件
gcc -m32 -g obj\*.o -o bin\rGBA_new.exe -lmingw32 -lSDL2main -lSDL2
if errorlevel 1 goto error

echo [成功] 编译完成！可执行文件: bin\rGBA_new.exe
goto end

:error
echo [错误] 编译失败！
exit /b 1

:end
