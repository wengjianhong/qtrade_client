cmake_minimum_required(VERSION 3.22)
project(qtrade_client VERSION 0.1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

if(NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

include(GNUInstallDirs)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY ${CMAKE_BINARY_DIR}/bin)

# Installed binaries embed RUNPATH for prefix libs
set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/${CMAKE_INSTALL_LIBDIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH FALSE)

add_compile_options(-fPIC -Wall -Wextra -Wpedantic -O2)

find_package(Threads REQUIRED)
find_package(spdlog 1.16.0 REQUIRED)
find_package(nlohmann_json 3.11.0 REQUIRED)
find_package(cpputils 0.1.0 REQUIRED CONFIG)
find_package(qtrade_engine REQUIRED CONFIG)
find_package(qtrade_service REQUIRED CONFIG)

include(${CMAKE_SOURCE_DIR}/cmake/qtrade_client.cmake)

install(TARGETS qtrade_client
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
)

install(DIRECTORY ${CMAKE_SOURCE_DIR}/config/
  DESTINATION config
  FILES_MATCHING PATTERN "*.json"
)
