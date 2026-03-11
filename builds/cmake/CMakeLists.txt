###############################################################################
#  Copyright (c) 2014-2026 libbitcoin-database developers (see COPYING).
#
#         GENERATED SOURCE CODE, DO NOT EDIT EXCEPT EXPERIMENTALLY
#
###############################################################################
cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project( libbitcoin-database
  VERSION 4.0.0
  DESCRIPTION "Bitcoin High Performance Blockchain Database"
  LANGUAGES C CXX )

list( APPEND CMAKE_MODULE_PATH "${PROJECT_SOURCE_DIR}/modules" )

if ( MSVC )
  set( CMAKE_STATIC_LIBRARY_PREFIX "lib" )
  set( CMAKE_SHARED_LIBRARY_PREFIX "lib" )
endif()

#------------------------------------------------------------------------------
# Project options.
#------------------------------------------------------------------------------
option( with-test "Build test." ON )
option( with-tools "Build tools." ON )

#------------------------------------------------------------------------------
# Dependencies.
#------------------------------------------------------------------------------

if ( BUILD_SHARED_LIBS )
  set( bitcoin-system_USE_STATIC_LIBS "OFF" )
  set( Boost_USE_STATIC_LIBS "OFF" )
else()
  set( bitcoin-system_USE_STATIC_LIBS "ON" )
  set( Boost_USE_STATIC_LIBS "ON" )
endif()

find_package( bitcoin-system 4.0.0 REQUIRED )

find_package( Boost 1.86.0 REQUIRED
  COMPONENTS
    unit_test_framework
)
#------------------------------------------------------------------------------
# Compiler options.
#------------------------------------------------------------------------------

include(CheckCCompilerFlag)
include(CheckCXXCompilerFlag)
include(CheckCXXSourceCompiles)

check_cxx_compiler_flag( "-Wall" HAS_FLAG_WALL )
if ( !HAS_FLAG_WALL )
    message( FATAL_ERROR "Compiler does not support '-Wall'." )
endif()

check_cxx_compiler_flag( "-Wextra" HAS_FLAG_WEXTRA )
if ( !HAS_FLAG_WEXTRA )
    message( FATAL_ERROR "Compiler does not support '-Wextra'." )
endif()

check_cxx_compiler_flag( "-Wno-reorder" HAS_FLAG_WNO-REORDER )
if ( !HAS_FLAG_WNO-REORDER )
    message( FATAL_ERROR "Compiler does not support '-Wno-reorder'." )
endif()

check_cxx_compiler_flag( "-Wno-missing-field-initializers" HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
if ( !HAS_FLAG_WNO-MISSING-FIELD-INITIALIZERS )
    message( FATAL_ERROR "Compiler does not support '-Wno-missing-field-initializers'." )
endif()

check_cxx_compiler_flag( "-Wno-missing-braces" HAS_FLAG_WNO-MISSING-BRACES )
if ( !HAS_FLAG_WNO-MISSING-BRACES )
    message( FATAL_ERROR "Compiler does not support '-Wno-missing-braces'." )
endif()

check_cxx_compiler_flag( "-Wno-comment" HAS_FLAG_WNO-COMMENT )
if ( !HAS_FLAG_WNO-COMMENT )
    message( FATAL_ERROR "Compiler does not support '-Wno-comment'." )
endif()

check_cxx_compiler_flag( "-Wno-deprecated-copy" HAS_FLAG_WNO-DEPRECATED-COPY )
if ( !HAS_FLAG_WNO-DEPRECATED-COPY )
    message( FATAL_ERROR "Compiler does not support '-Wno-deprecated-copy'." )
endif()

if ( "${CMAKE_CXX_COMPILER_ID}" STREQUAL "Clang" )
  check_cxx_compiler_flag( "-Wno-mismatched-tags" HAS_FLAG_WNO-MISMATCHED-TAGS )
  if ( !HAS_FLAG_WNO-MISMATCHED-TAGS )
      message( FATAL_ERROR "Compiler does not support '-Wno-mismatched-tags'." )
  endif()
endif()

#------------------------------------------------------------------------------
# bitcoin-database library
#------------------------------------------------------------------------------
add_library( bitcoin-database )
add_library( bitcoin::database ALIAS bitcoin-database )

target_compile_features( bitcoin-database
  PUBLIC
    cxx_std_20
)

target_compile_options( bitcoin-database
  PRIVATE
    -Wall
    -Wextra
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-reorder>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-field-initializers>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-missing-braces>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-comment>
    $<$<COMPILE_LANGUAGE:CXX>:-Wno-deprecated-copy>
    $<$<CXX_COMPILER_ID:Clang>:-Wno-mismatched-tags>
)

file( GLOB_RECURSE bitcoin_database_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.h"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/*.ipp"
)

file( GLOB_RECURSE bitcoin_database_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp"
)

target_sources( bitcoin-database
  PUBLIC
    FILE_SET HEADERS
    BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
    FILES
      ${bitcoin_database_HEADERS}
  PRIVATE
    ${bitcoin_database_SOURCES}
)

target_include_directories( bitcoin-database
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

target_link_libraries( bitcoin-database
  PUBLIC
    bitcoin::system
)

set_target_properties(bitcoin-database
  PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION ${PROJECT_VERSION_MAJOR}
    EXPORT_NAME database
)

#------------------------------------------------------------------------------
# libbitcoin-database-tests tests
#------------------------------------------------------------------------------
if ( with-test )
  add_executable( libbitcoin-database-tests )

  enable_testing()

  add_test( NAME libbitcoin-database-tests
    COMMAND libbitcoin-database-tests
    --run_test=*
    --log_level=warning
    --show_progress=no
    --detect_memory_leak=0
    --report_level=no
    --build_info=yes
  )

  file( GLOB_RECURSE libbitcoin_database_tests_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.cpp"
  )

  target_sources( libbitcoin-database-tests
    PRIVATE
      ${libbitcoin_database_tests_SOURCES}
  )

  target_link_libraries( libbitcoin-database-tests
    PRIVATE
      Boost::unit_test_framework
      bitcoin::database
  )
endif()

#------------------------------------------------------------------------------
# initchain executable
#------------------------------------------------------------------------------
if ( with-tools )
  add_executable( initchain )

  file( GLOB_RECURSE initchain_SOURCES CONFIGURE_DEPENDS
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/initchain/*.c"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../tools/initchain/*.cpp"
  )

  target_sources( initchain
    PRIVATE
      ${initchain_SOURCES}
  )

  target_link_libraries( initchain
    PRIVATE
      bitcoin::database
  )
endif()
#------------------------------------------------------------------------------
# Installation routine.
#------------------------------------------------------------------------------
include( GNUInstallDirs )
include( CMakePackageConfigHelpers )

install( TARGETS bitcoin-database
  EXPORT bitcoin-database-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILE_SET HEADERS
)

install( EXPORT bitcoin-database-targets
  NAMESPACE bitcoin::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-database
  FILE bitcoin-database-targets.cmake
)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bitcoin-database-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-database
)

write_basic_package_version_file(
  "${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config-version.cmake"
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-database
)
