cmake_minimum_required(VERSION 3.27.0)
project(air-quality-monitor VERSION 0.1.0 LANGUAGES C CXX)

include(CTest)
enable_testing()

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
)

set(CPACK_PROJECT_NAME ${PROJECT_NAME})
set(CPACK_PROJECT_VERSION ${PROJECT_VERSION})
include(CPack)
