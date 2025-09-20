cmake_minimum_required(VERSION 3.27.0)
project(air-quality-monitor VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

# Determine CPU architecture for optimization flags
execute_process(COMMAND uname -m OUTPUT_VARIABLE ARCH OUTPUT_STRIP_TRAILING_WHITESPACE)
execute_process(COMMAND nproc OUTPUT_VARIABLE CPU_CORES OUTPUT_STRIP_TRAILING_WHITESPACE)

# Set build type to Release by default for performance
if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

# Compiler optimizations based on architecture
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set(CMAKE_CXX_FLAGS_RELEASE "-O3 -flto -DNDEBUG")
    set(CMAKE_C_FLAGS_RELEASE "-O3 -flto -DNDEBUG")
    
    # Architecture-specific optimizations
    if(ARCH MATCHES "aarch64")
        # ARM64 optimizations (Pi 4, Pi 5, etc.)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=cortex-a72 -mtune=cortex-a72")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mcpu=cortex-a72 -mtune=cortex-a72")
    elseif(ARCH MATCHES "armv7l")
        # ARM32 optimizations (Pi 3, etc.)
        set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -mcpu=cortex-a53 -mtune=cortex-a53")
        set(CMAKE_C_FLAGS_RELEASE "${CMAKE_C_FLAGS_RELEASE} -mcpu=cortex-a53 -mtune=cortex-a53")
    endif()
    
    message(STATUS "Building optimized Release version for ${ARCH}")
elseif(CMAKE_BUILD_TYPE STREQUAL "Debug")
    # Debug build with timing instrumentation
    set(CMAKE_CXX_FLAGS_DEBUG "-O0 -g -DENABLE_TIMING_DEBUG")
    set(CMAKE_C_FLAGS_DEBUG "-O0 -g")
    message(STATUS "Building Debug version with timing instrumentation")
endif()

# C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

include(FetchContent)

# Fetch CPR if needed
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8) # 1.10.5
FetchContent_MakeAvailable(cpr)

# spdlog
find_package(spdlog REQUIRED)

add_executable(air-quality-monitor)

target_sources(air-quality-monitor 
    PRIVATE main.cpp
    PRIVATE ./bsec/src/bme68x.c
    PRIVATE ./bsec/src/bsec_integration.c
    PRIVATE ./src/air_quality_service.cpp
    PRIVATE ./src/homebridge_service.cpp
    PRIVATE ./src/simple_i2c_bus.cpp
)

target_include_directories(air-quality-monitor 
    PRIVATE ./include
    PRIVATE ./src
    PRIVATE ./bsec/src
)

target_link_directories(air-quality-monitor 
    PRIVATE ./bsec/lib
    PRIVATE ./lib
)

target_link_libraries(air-quality-monitor 
    PRIVATE algobsec
    PRIVATE cpr::cpr
    PRIVATE spdlog::spdlog
    PRIVATE i2c
    PRIVATE rt  # Required for clock_nanosleep and high-resolution timers
)

# Link-time optimization for Release builds
if(CMAKE_BUILD_TYPE STREQUAL "Release")
    set_target_properties(air-quality-monitor PROPERTIES
        INTERPROCEDURAL_OPTIMIZATION TRUE
        LINK_FLAGS "-flto"
    )
    
    # Strip symbols in release builds for smaller binary
    add_custom_command(TARGET air-quality-monitor POST_BUILD
        COMMAND ${CMAKE_STRIP} $<TARGET_FILE:air-quality-monitor>
        COMMENT "Stripping debug symbols for release build"
    )
endif()

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
