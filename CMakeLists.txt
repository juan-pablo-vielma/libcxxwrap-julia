cmake_minimum_required(VERSION 2.8.12)
project(JlCxx)

# Cmake policies
# ==============

if(POLICY CMP0074)
    # find_package() uses <PackageName>_ROOT variables.
    # This policy was introduced in CMake version 3.12.
    cmake_policy(SET CMP0074 NEW)
endif()

# Dependencies
# ============

set(CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR})
find_package(Julia REQUIRED)

set(CMAKE_INSTALL_RPATH "${CMAKE_INSTALL_PREFIX}/lib;${Julia_LIBRARY_DIR}")
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
set(CMAKE_MACOSX_RPATH 1)
set(CMAKE_LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/lib")

# includes
# ============
include(CheckCXXCompilerFlag)
include(CMakePackageConfigHelpers)
include(GenerateExportHeader)
include(GNUInstallDirs)
include(InstallRequiredSystemLibraries)

# Compilation flags
# =================

if(CMAKE_CXX_COMPILER_ID MATCHES "Clang" OR CMAKE_CXX_COMPILER_ID MATCHES "GNU" OR CMAKE_CXX_COMPILER_ID MATCHES "Intel")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wunused-parameter -Wextra -Wreorder")
    CHECK_CXX_COMPILER_FLAG("-std=c++14" HAS_CPP14_FLAG)

    if (HAS_CPP14_FLAG)
        set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
    else()
        message(FATAL_ERROR "Unsupported compiler -- jlcxx requires C++14 support!")
    endif()
endif()

set(CMAKE_CXX_STANDARD 14)

add_definitions(-DJULIA_ENABLE_THREADING)

if(WIN32 AND DEFINED ENV{MSYSTEM})
  if($ENV{MSYSTEM} STREQUAL "MINGW32")
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -march=pentium4 -static-libstdc++")
  endif()
endif()

set(CMAKE_INSTALL_LIBDIR "lib")

set(JLCXX_BUILD_EXAMPLES ON CACHE BOOL "Build the JlCxx examples")
set(JLCXX_BUILD_TESTS ON CACHE BOOL "Build the JlCxx tests")

# Source files
# ============

set(JLCXX_INCLUDE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/include)
set(JLCXX_SOURCE_DIR ${CMAKE_CURRENT_SOURCE_DIR}/src)

set(JLCXX_HEADERS
    ${JLCXX_INCLUDE_DIR}/jlcxx/array.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/const_array.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/jlcxx.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/jlcxx_config.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/functions.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/module.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/smart_pointers.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/tuple.hpp
    ${JLCXX_INCLUDE_DIR}/jlcxx/type_conversion.hpp
)

set(JLCXX_SOURCES
  ${JLCXX_SOURCE_DIR}/c_interface.cpp
  ${JLCXX_SOURCE_DIR}/jlcxx.cpp
  ${JLCXX_SOURCE_DIR}/functions.cpp
)

# Versioning
# ==========

file(STRINGS "${JLCXX_INCLUDE_DIR}/jlcxx/jlcxx_config.hpp" jlcxx_version_defines
    REGEX "#define JLCXX_VERSION_(MAJOR|MINOR|PATCH)")
foreach(ver ${jlcxx_version_defines})
    if(ver MATCHES "#define JLCXX_VERSION_(MAJOR|MINOR|PATCH) +([^ ]+)$")
        set(JLCXX_VERSION_${CMAKE_MATCH_1} "${CMAKE_MATCH_2}" CACHE INTERNAL "")
    endif()
endforeach()
set(${PROJECT_NAME}_VERSION
    ${JLCXX_VERSION_MAJOR}.${JLCXX_VERSION_MINOR}.${JLCXX_VERSION_PATCH})
message(STATUS "${PROJECT_NAME} version: v${${PROJECT_NAME}_VERSION}")

# Output
# ======

set(JLCXX_TARGET cxxwrap_julia)
add_library(${JLCXX_TARGET} SHARED ${JLCXX_SOURCES} ${JLCXX_HEADERS})

target_include_directories(${JLCXX_TARGET} PUBLIC
  $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
  $<BUILD_INTERFACE:${JLCXX_INCLUDE_DIR}>
  "$<BUILD_INTERFACE:${Julia_INCLUDE_DIRS}>"
)

target_link_libraries(${JLCXX_TARGET} $<BUILD_INTERFACE:${Julia_LIBRARY}>)
link_directories(${Julia_LIBRARY_DIR} ${Julia_LIBRARY_DIR}/julia)
set_target_properties(${JLCXX_TARGET} PROPERTIES
                      PUBLIC_HEADER "${JLCXX_HEADERS}"
                      COMPILE_DEFINITIONS "JLCXX_EXPORTS")
set_property(TARGET ${JLCXX_TARGET} PROPERTY VERSION ${${PROJECT_NAME}_VERSION})
set_property(TARGET ${JLCXX_TARGET} PROPERTY SOVERSION ${JLCXX_VERSION_MAJOR})
set_property(TARGET ${JLCXX_TARGET} PROPERTY
  INTERFACE_${JLCXX_TARGET}_MAJOR_VERSION ${JLCXX_VERSION_MAJOR})
set_property(TARGET ${JLCXX_TARGET} APPEND PROPERTY
  COMPATIBLE_INTERFACE_STRING ${JLCXX_TARGET}_MAJOR_VERSION
)
target_compile_definitions(${JLCXX_TARGET} PUBLIC "JULIA_ENABLE_THREADING")
target_compile_options(${JLCXX_TARGET} PUBLIC "$<$<COMPILE_LANGUAGE:CXX>:-std=c++14>" )

generate_export_header(${JLCXX_TARGET})

set(JLCXX_NAME ${JLCXX_TARGET})
if(WIN32)
    if(MSVC)
        set(JLCXX_LINK_FILE ${JLCXX_NAME}${CMAKE_LINK_LIBRARY_SUFFIX})
        set(JLCXX_RUNTIME_FILE ${JLCXX_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    else()
        set(JLCXX_LINK_FILE lib${JLCXX_NAME}.dll.a)
        set(JLCXX_RUNTIME_FILE lib${JLCXX_NAME}.dll)
    endif()
else()
    if(APPLE)
        target_link_libraries(${JLCXX_TARGET} "-framework CoreFoundation")
    endif()
    set(JLCXX_LINK_FILE lib${JLCXX_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
    set(JLCXX_RUNTIME_FILE lib${JLCXX_NAME}${CMAKE_SHARED_LIBRARY_SUFFIX})
endif()

# Installation
# ============

set(JLCXX_CMAKECONFIG_INSTALL_DIR "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}" CACHE STRING "install path for jlcxxConfig.cmake")


install(TARGETS ${JLCXX_TARGET} EXPORT JlCxxTargets
        ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
        LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
        RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
        PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}/jlcxx)

configure_package_config_file(${PROJECT_NAME}Config.cmake.in
                              "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
                              INSTALL_DESTINATION ${JLCXX_CMAKECONFIG_INSTALL_DIR})
write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
  VERSION ${${PROJECT_NAME}_VERSION}
  COMPATIBILITY AnyNewerVersion
)

configure_file(FindJulia.cmake ${CMAKE_CURRENT_BINARY_DIR}/FindJulia.cmake COPYONLY)
export(EXPORT JlCxxTargets
  FILE ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigExports.cmake
  NAMESPACE JlCxx::
)

install(EXPORT JlCxxTargets
  FILE ${PROJECT_NAME}ConfigExports.cmake
  NAMESPACE JlCxx::
  DESTINATION ${JLCXX_CMAKECONFIG_INSTALL_DIR}
)

install(FILES ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake
              ${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake
              ${CMAKE_CURRENT_SOURCE_DIR}/FindJulia.cmake
  DESTINATION ${JLCXX_CMAKECONFIG_INSTALL_DIR}
)

if(JLCXX_BUILD_EXAMPLES)
  add_subdirectory(examples)
endif()

if(JLCXX_BUILD_TESTS)
  enable_testing()
  add_subdirectory(test)
endif()

if(WIN32)
    string(SUBSTRING ${Julia_VERSION_STRING} 0 3 Julia_VERSION_SHORT)
    add_custom_target(create_zip
        COMMAND ${CMAKE_COMMAND} -E tar "cfv" "libcxxwrap-julia-${Julia_VERSION_SHORT}-msvc${Julia_WORD_SIZE}-v${${PROJECT_NAME}_VERSION}.zip" --format=zip "usr"
        DEPENDS install
        WORKING_DIRECTORY ${CMAKE_INSTALL_PREFIX}/..
        )
endif()
