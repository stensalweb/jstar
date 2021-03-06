cmake_minimum_required(VERSION 3.9)
project(jstar)

# version
set(JSTAR_VERSION_MAJOR 1)
set(JSTAR_VERSION_MINOR 2)
set(JSTAR_VERSION_PATCH 3)

# set the c standard
set(CMAKE_C_STANDARD 99)

# options
option(JSTAR_INSTALL        "Generate install targets" ON)
option(JSTAR_COMPUTED_GOTOS "Use computed gotos for VM eval loop" ON)
option(JSTAR_NAN_TAGGING    "Use NaN tagging technique to store the VM internal type" ON)
option(JSTAR_DBG_PRINT_EXEC "Trace the execution of the VM" OFF)
option(JSTAR_DBG_PRINT_GC   "Trace the execution of the garbage collector" OFF)
option(JSTAR_DBG_STRESS_GC  "Stress the garbage collector by calling it on every allocation" OFF)

option(JSTAR_SYS   "Include the 'sys' module in the language" ON)
option(JSTAR_IO    "Include the 'io' module in the language" ON)
option(JSTAR_MATH  "Include the 'math' module in the language" ON)
option(JSTAR_DEBUG "Include the 'debug' module in the language" ON)
option(JSTAR_RE    "Include the 're' module in the language" ON)

# setup option.h
configure_file (
    "${PROJECT_SOURCE_DIR}/jstar/include/jstar/jstarconf.h.in"
    "${PROJECT_SOURCE_DIR}/jstar/include/jstar/jstarconf.h"
)

# set default build type if not specified
if(NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
    message(STATUS "Setting build type to 'Release' as none was specified.")
    set(CMAKE_BUILD_TYPE "Release" CACHE STRING "Choose the type of build." FORCE)
endif()

# Check for link time optimization support
if(NOT CMAKE_BUILD_TYPE STREQUAL "Debug")
    include(CheckIPOSupported)
    check_ipo_supported(RESULT LTO)
    if(LTO)
        message(STATUS "J* link-time optimization enabled")
    endif()
endif()

# set output directories
set(CMAKE_ARCHIVE_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/lib)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)
set(CMAKE_BUILD_RPATH_USE_ORIGIN   TRUE)

# set compiler flags
if(CMAKE_C_COMPILER_ID STREQUAL "GNU")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -Wno-unused-parameter")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -fomit-frame-pointer -s")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
elseif(CMAKE_C_COMPILER_ID MATCHES "Clang" AND NOT MSVC)
    set(CMAKE_C_FLAGS "-Wall -Wextra -Wno-unused-parameter")
    set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -O3 -fomit-frame-pointer -Qunused-arguments -s")
    set(CMAKE_EXE_LINKER_FLAGS_RELEASE "${CMAKE_EXE_LINKER_FLAGS_RELEASE} -s")
elseif(MSVC)
    add_definitions("-D_CRT_SECURE_NO_WARNINGS=1")
    set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} /wd4244 /wd4267")
endif()

# subprojects
add_subdirectory("cli")
add_subdirectory("jstar")

# ---- install targets ----

if(JSTAR_INSTALL)
    # install all build products
    install(TARGETS jstar libjstar libjstar_static
        LIBRARY DESTINATION lib
        ARCHIVE DESTINATION lib
        RUNTIME DESTINATION bin
    )

    # install header files
    install(DIRECTORY
        "jstar/include/" 
        DESTINATION include
        PATTERN "*.h.in" EXCLUDE
    )
endif()