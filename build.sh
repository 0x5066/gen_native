i686-w64-mingw32-cmake -B build -G Ninja -DWINAMP_INSTALL_DIR="$HOME/.wine/drive_c/Program Files (x86)/WACUPDev"
cmake --build build --config release --parallel
cmake --build build --target install