i686-w64-mingw32-windres -i mainwnd.res -o mainwnd.rc
i686-w64-mingw32-windres -i mainwnd.rc -o mainwnd.res.o
i686-w64-mingw32-g++ -g3 -O0 -shared -o gen_native.dll native.cpp mainwnd.res.o -lgdi32 -mwindows -lcomctl32
#x86_64-w64-mingw32-windres -i mainwnd.res -o mainwnd_x64.rc
#x86_64-w64-mingw32-windres -i mainwnd_x64.rc -o mainwnd_x64.res.o
#x86_64-w64-mingw32-g++ -g3 -O0 -shared -o gen_native_x64.dll native.cpp mainwnd_x64.res.o -lgdi32 -mwindows
cp gen_native.dll "$HOME/.wine/drive_c/Program Files (x86)/Winamp/Plugins/gen_native.dll"
wine "C:\\Program Files (x86)\\Winamp\\winamp.exe"