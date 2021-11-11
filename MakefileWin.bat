@echo off
IF EXIST "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\BuildTools\VC\Auxiliary\Build\vcvarsall.bat" x64
) ELSE (
    call "C:\Program Files (x86)\Microsoft Visual Studio\2019\Community\VC\Auxiliary\Build\vcvarsall.bat" x64
)
set compilerflags=/Od /Zi /EHsc /MT
set includedirs=/I include
set linkerflags=/LIBPATH:libs/win glfw3.lib assimp-vc142-mt.lib zlib.lib IrrXML.lib gdi32.lib user32.lib Shell32.lib
cl.exe %compilerflags% %includedirs% include/glad/glad.c uSculpt.cpp /Fe:bin/uSculpt.exe /Fo:bin/ /Fd:bin/ /link %linkerflags% 
