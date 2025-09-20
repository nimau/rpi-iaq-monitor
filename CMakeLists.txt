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

#############################################################################
# BSEC (Bosch Sensortec Environmental Cluster) - External Dependency
#############################################################################
# BSEC is a proprietary library from Bosch Sensortec that cannot be redistributed.
# Users must download it themselves from Bosch Sensortec and provide the path.

# Get BSEC_DIR from environment variable or CMake cache
set(BSEC_DIR "$ENV{BSEC_DIR}" CACHE PATH "Path to Bosch BSEC installation (must contain inc/ and lib/ subdirectories)")

if(NOT BSEC_DIR)
    message(FATAL_ERROR 
        "BSEC library not found. Please download the Bosch BSEC library and set BSEC_DIR.\n"
        "Steps:\n"
        "1. Download BSEC from Bosch Sensortec (search: 'Bosch BSEC Software Library')\n"
        "2. Accept the license and extract the archive\n"
        "3. Set environment variable: export BSEC_DIR=/path/to/extracted/BSEC\n"
        "4. Or pass to CMake: cmake -DBSEC_DIR=/path/to/extracted/BSEC .\n"
        "The BSEC directory must contain inc/ and lib/ subdirectories.")
endif()

# Find BSEC include directory
find_path(BSEC_INCLUDE_DIR 
    NAMES bsec_interface.h bsec_datatypes.h
    HINTS "${BSEC_DIR}/inc" "${BSEC_DIR}/include" "${BSEC_DIR}/src"
    DOC "BSEC include directory"
    REQUIRED
)

# Find BSEC library - try different possible locations and names
find_library(BSEC_LIBRARY
    NAMES algobsec bsec libalgobsec.a libbsec.a
    HINTS "${BSEC_DIR}/lib" 
          "${BSEC_DIR}/lib/gcc" 
          "${BSEC_DIR}/lib/Release" 
          "${BSEC_DIR}/binaries"
          "${BSEC_DIR}"
    DOC "BSEC library file"
    REQUIRED
)

# Find BME68x driver sources (usually bundled with BSEC)
find_path(BME68X_INCLUDE_DIR
    NAMES bme68x.h bme68x_defs.h
    HINTS "${BSEC_DIR}/src" 
          "${BSEC_DIR}/inc" 
          "${BSEC_DIR}/include"
          "${BSEC_DIR}/algo/normal_version/bin/gcc/Raspberry_Pi"
    DOC "BME68x driver include directory"
    REQUIRED
)

# Find BME68x source files
find_file(BME68X_SOURCE
    NAMES bme68x.c
    HINTS "${BSEC_DIR}/src"
          "${BSEC_DIR}/algo/normal_version/bin/gcc/Raspberry_Pi"
    DOC "BME68x driver source file"
    REQUIRED
)

find_file(BSEC_INTEGRATION_SOURCE
    NAMES bsec_integration.c
    HINTS "${BSEC_DIR}/src"
          "${BSEC_DIR}/algo/normal_version/bin/gcc/Raspberry_Pi"  
    DOC "BSEC integration source file"
    REQUIRED
)

message(STATUS "BSEC Found:")
message(STATUS "  BSEC_DIR: ${BSEC_DIR}")
message(STATUS "  BSEC_INCLUDE_DIR: ${BSEC_INCLUDE_DIR}")
message(STATUS "  BME68X_INCLUDE_DIR: ${BME68X_INCLUDE_DIR}")
message(STATUS "  BSEC_LIBRARY: ${BSEC_LIBRARY}")
message(STATUS "  BME68X_SOURCE: ${BME68X_SOURCE}")
message(STATUS "  BSEC_INTEGRATION_SOURCE: ${BSEC_INTEGRATION_SOURCE}")

#############################################################################
# Other Dependencies
#############################################################################

include(FetchContent)

# Fetch CPR if needed
FetchContent_Declare(cpr GIT_REPOSITORY https://github.com/libcpr/cpr.git
                         GIT_TAG 3b15fa82ea74739b574d705fea44959b58142eb8) # 1.10.5
FetchContent_MakeAvailable(cpr)

# spdlog
# yaml-cpp for configuration management
find_package(yaml-cpp QUIET)
if(NOT yaml-cpp_FOUND)
    message(STATUS "yaml-cpp not found system-wide, fetching from GitHub...")
    FetchContent_Declare(
        yaml-cpp
        GIT_REPOSITORY https://github.com/jbeder/yaml-cpp.git
        GIT_TAG 0.8.0
    )
    
    # Disable yaml-cpp tests and tools to speed up build
    set(YAML_CPP_BUILD_TESTS OFF CACHE BOOL "" FORCE)
    set(YAML_CPP_BUILD_TOOLS OFF CACHE BOOL "" FORCE)
    set(YAML_BUILD_SHARED_LIBS ON CACHE BOOL "" FORCE)
    
    FetchContent_MakeAvailable(yaml-cpp)
    message(STATUS "yaml-cpp fetched and configured")
endif()
find_package(spdlog REQUIRED)

#############################################################################
# Target Configuration
#############################################################################

add_executable(air-quality-monitor)

target_sources(air-quality-monitor 
    PRIVATE main.cpp
    PRIVATE ${BME68X_SOURCE}
    PRIVATE ${BSEC_INTEGRATION_SOURCE}
    PRIVATE ./src/air_quality_service.cpp
    PRIVATE ./src/homebridge_service.cpp
    PRIVATE ./src/simple_i2c_bus.cpp
    PRIVATE ./src/ConfigManager.cpp
)

target_include_directories(air-quality-monitor 
    PRIVATE ./include
    PRIVATE ./src
    PRIVATE ${BSEC_INCLUDE_DIR}
    PRIVATE ${BME68X_INCLUDE_DIR}
)

target_link_libraries(air-quality-monitor 
    PRIVATE ${BSEC_LIBRARY}
    PRIVATE cpr::cpr
    PRIVATE spdlog::spdlog
    PRIVATE yaml-cpp
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

# Apply BSEC timing fix automatically during build
add_custom_target(apply_bsec_fix
    COMMAND ${CMAKE_SOURCE_DIR}/scripts/apply_bsec_fix.sh
    COMMENT "Applying BSEC timing fix to suppress warnings"
    VERBATIM
)

# Make the main target depend on the BSEC fix
add_dependencies(air-quality-monitor apply_bsec_fix)
