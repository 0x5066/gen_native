cmake_minimum_required(VERSION 3.5)

option(LEGACY_MODE "Enable legacy mode (creates a 32-bit DLL). Requires a 32-bit compiler." ON)
set(WINAMP_INSTALL_DIR "C:/Program Files (x86)/WACUP" CACHE PATH "Root directory of Winamp installation")

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED OFF)

project(gen_native LANGUAGES CXX)

if(MSVC)
    add_library(${PROJECT_NAME} SHARED native.cpp)
    set_target_properties(${PROJECT_NAME} PROPERTIES LINK_OPTIONS "${CMAKE_CURRENT_SOURCE_DIR}/mainwnd.res")
else()
    file(TOUCH "${CMAKE_CURRENT_BINARY_DIR}/mainwnd.rc")
    add_library(${PROJECT_NAME} SHARED native.cpp "${CMAKE_CURRENT_BINARY_DIR}/mainwnd.rc")
    add_custom_command(
        OUTPUT
            "${CMAKE_CURRENT_BINARY_DIR}/mainwnd.rc"
        COMMAND ${CMAKE_RC_COMPILER} "-i" "${CMAKE_CURRENT_SOURCE_DIR}/mainwnd.res" "-o" "${CMAKE_CURRENT_BINARY_DIR}/mainwnd.rc"
        DEPENDS
            "${CMAKE_CURRENT_SOURCE_DIR}/mainwnd.res"
    )
    add_custom_target(resources
        DEPENDS "${CMAKE_CURRENT_SOURCE_DIR}/mainwnd.res"
    )

    set_target_properties(${PROJECT_NAME} PROPERTIES COMPILE_OPTIONS "$<$<BOOL:${LEGACY_MODE}>:-m32>" LINK_OPTIONS "$<$<BOOL:${LEGACY_MODE}>:-m32>")
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES PREFIX "") # Prevent CMake from prepending "lib" to the binary name 
set_target_properties(${PROJECT_NAME} PROPERTIES SUFFIX ".dll") # Prevent cross-compilers from adding the wrong suffix (".so") to the binary name 
target_link_libraries(${PROJECT_NAME} -static shlwapi comctl32)

install(TARGETS ${PROJECT_NAME} RUNTIME DESTINATION "${WINAMP_INSTALL_DIR}/Plugins")
