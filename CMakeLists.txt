cmake_minimum_required(VERSION 3.5)

# Use a 32 bit compiler
option(LEGACY_MODE "Enable legacy mode (creates a 32-bit DLL)" ON)
set(WINAMP_INSTALL_DIR "C:/Program Files (x86)/WACUP" CACHE PATH "Root directory of Winamp installation")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

project(gen_native LANGUAGES CXX)

if(MSVC)
    add_library(${PROJECT_NAME} SHARED native.cpp)
    add_compile_definitions("/utf-8")
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_OPTIONS "${CMAKE_CURRENT_SOURCE_DIR}/mainwnd.res")
else()
    add_library(${PROJECT_NAME} SHARED native.cpp mainwnd.rc)
    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "$<$<BOOL:${LEGACY_MODE}>:-m32>" LINK_OPTIONS "$<$<BOOL:${LEGACY_MODE}>:-m32>")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "") # Prevent CMake from prepending "lib" to the binary name 
target_link_libraries(${PROJECT_NAME} -static shlwapi comctl32)
install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${WINAMP_INSTALL_DIR}/Plugins")
