cmake_minimum_required(VERSION 3.25)

project(BookSim2 VERSION 2.0.0 LANGUAGES C CXX)

if(NOT MSVC)
  message(FATAL_ERROR "This rewrite currently supports MSVC(cl) on Windows only.")
endif()

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(BOOKSIM_SRC_DIR "${CMAKE_CURRENT_SOURCE_DIR}/src")

file(GLOB_RECURSE BOOKSIM_CPP_SOURCES CONFIGURE_DEPENDS
  "${BOOKSIM_SRC_DIR}/*.cpp"
)
list(REMOVE_ITEM BOOKSIM_CPP_SOURCES "${BOOKSIM_SRC_DIR}/main.cpp")

add_library(booksim_core STATIC
  ${BOOKSIM_CPP_SOURCES}
)

target_include_directories(booksim_core
  PUBLIC
    $<BUILD_INTERFACE:${BOOKSIM_SRC_DIR}>
    $<BUILD_INTERFACE:${BOOKSIM_SRC_DIR}/allocators>
    $<BUILD_INTERFACE:${BOOKSIM_SRC_DIR}/arbiters>
    $<BUILD_INTERFACE:${BOOKSIM_SRC_DIR}/networks>
    $<BUILD_INTERFACE:${BOOKSIM_SRC_DIR}/power>
    $<BUILD_INTERFACE:${BOOKSIM_SRC_DIR}/routers>
    $<INSTALL_INTERFACE:include/booksim2>
)

target_compile_features(booksim_core PUBLIC cxx_std_20)
target_compile_definitions(booksim_core PRIVATE _CRT_SECURE_NO_WARNINGS)
target_compile_options(booksim_core PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus)

set_target_properties(booksim_core PROPERTIES
  OUTPUT_NAME "booksim_core"
  ARCHIVE_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib"
)

add_executable(booksim "${BOOKSIM_SRC_DIR}/main.cpp")
target_link_libraries(booksim PRIVATE booksim_core)
target_compile_options(booksim PRIVATE /W4 /permissive- /EHsc /Zc:__cplusplus)
set_target_properties(booksim PROPERTIES
  OUTPUT_NAME "booksim"
  RUNTIME_OUTPUT_DIRECTORY "${BOOKSIM_SRC_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_DEBUG "${BOOKSIM_SRC_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELEASE "${BOOKSIM_SRC_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_RELWITHDEBINFO "${BOOKSIM_SRC_DIR}"
  RUNTIME_OUTPUT_DIRECTORY_MINSIZEREL "${BOOKSIM_SRC_DIR}"
)

add_library(Booksim::booksim_core ALIAS booksim_core)

include(CMakePackageConfigHelpers)
include(GNUInstallDirs)

install(TARGETS booksim_core booksim
  EXPORT BookSimTargets
  RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}"
  LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}"
  ARCHIVE DESTINATION "${CMAKE_INSTALL_LIBDIR}"
)

install(DIRECTORY "${BOOKSIM_SRC_DIR}/"
  DESTINATION "include/booksim2"
  FILES_MATCHING PATTERN "*.hpp"
)

install(EXPORT BookSimTargets
  FILE BookSimTargets.cmake
  NAMESPACE Booksim::
  DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/BookSim2"
)
