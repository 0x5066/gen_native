cmake_minimum_required(VERSION 3.5)

option(LEGACY_MODE "Enable legacy mode (creates a 32-bit DLL)" ON)
set(WINAMP_INSTALL_DIR "C:/Program Files (x86)/WACUP" CACHE PATH "Root directory of Winamp installation")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

project(gen_native LANGUAGES CXX)
add_library(${PROJECT_NAME} SHARED native.cpp mainwnd.rc)
set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "") # Prevent CMake from prepending "lib" to the binary name 

if(LEGACY_MODE)
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "-m32" LINK_OPTIONS "-m32")
endif()

target_link_libraries(${PROJECT_NAME} -static shlwapi comctl32)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION ${WINAMP_INSTALL_DIR}/Plugins)