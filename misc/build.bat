rem C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Tools\MSVC\14.26.28801\bin\Hostx64\x86\cl.exe
@echo off
cls
if not exist g:\build mkdir g:\build

cl >nul 2>&1 && (
  pushd g:\build
  rem -DSLINGAME_SHADOWDEBUG
  cl -nologo -Wall -wd4100 -wd5045 -wd4820 -Z7 g:\src\win32_slingame.c /link user32.lib gdi32.lib opengl32.lib xinput.lib winmm.lib dsound.lib
rem  echo ---------------
rem  cl -nologo -Wall -wd4100 -wd5045 -wd4820 -Z7 g:\src\sobj.c /link user32.lib
  popd
) || (
  echo Running vcvarsall...
  "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
  build.bat
)
