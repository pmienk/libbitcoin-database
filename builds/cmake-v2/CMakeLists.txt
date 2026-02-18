#cmake_minimum_required(VERSION 3.22...3.30)
cmake_minimum_required(VERSION 3.30 FATAL_ERROR)

project(libbitcoin-database
  VERSION 4.0.0
  DESCRIPTION "Bitcoin Cross-Platform C++ Development Toolkit - base library"
  LANGUAGES C CXX
)

#set(Boost_VERBOSE true)

if (MSVC)
  set( CMAKE_STATIC_LIBRARY_PREFIX "lib" )
  set( CMAKE_SHARED_LIBRARY_PREFIX "lib" )
endif ()

add_library( bitcoin-database )
add_library( bitcoin::database ALIAS bitcoin-database )

# ──────────────────────────────────────────────────────────────
#   Options
# ─────────────────────────────────────────────────────────────

#option(BUILD_SHARED_LIBS "Build shared library instead of static" OFF)
option(with-tests "Build unit tests" ON)
option(with-tools "Compile with tools" ON)

set(LIBBITCOIN_CXX_STANDARD 20 CACHE STRING "Require C++ 20 standard.")

# ──────────────────────────────────────────────────────────────
#   Dependencies
# ──────────────────────────────────────────────────────────────

find_package(Boost 1.86.0 REQUIRED
  COMPONENTS
    unit_test_framework
)

find_package( bitcoin-system 4.0.0 REQUIRED )

# ──────────────────────────────────────────────────────────────
#   Library definition
# ──────────────────────────────────────────────────────────────

file(GLOB_RECURSE bitcoin_database_HEADERS CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/*.hpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../include/bitcoin/*.ipp"
)

file( GLOB_RECURSE bitcoin_database_SOURCES CONFIGURE_DEPENDS
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.cpp"
  "${CMAKE_CURRENT_SOURCE_DIR}/../../src/*.c"
)

target_sources( bitcoin-database PUBLIC
  FILE_SET HEADERS
  BASE_DIRS "${CMAKE_CURRENT_SOURCE_DIR}/../../include"
  FILES ${bitcoin_database_HEADERS}
)

target_sources( bitcoin-database PRIVATE
  ${bitcoin_database_SOURCES}
)

target_include_directories( bitcoin-database
  PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/../../include>
    $<INSTALL_INTERFACE:include>
)

target_compile_features(bitcoin-database PUBLIC cxx_std_${LIBBITCOIN_CXX_STANDARD})

target_compile_definitions(bitcoin-database
  PUBLIC
    $<$<BOOL:${with-ssl}>:-DWITH_SSL -DWOLFSSL_USER_SETTINGS>
  PRIVATE
    $<$<BOOL:${BUILD_SHARED_LIBS}>:BC_DLL>
    # Add other internal defines if needed, e.g. BC_USE_ICU
)

target_link_libraries(bitcoin-database
  PUBLIC
    bitcoin::system
)

# Very common flags for libbitcoin projects
target_compile_options(bitcoin-database
  PRIVATE
    $<$<CXX_COMPILER_ID:MSVC>:/permissive- /W3>
    $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
)

set_target_properties(bitcoin-database PROPERTIES
  VERSION   ${PROJECT_VERSION}
  SOVERSION ${PROJECT_VERSION_MAJOR}
  EXPORT_NAME database
)

# ──────────────────────────────────────────────────────────────
#   Tests
# ──────────────────────────────────────────────────────────────

if(with-tests)
  enable_testing()

  file(GLOB_RECURSE bitcoin_database_test_SOURCES
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.cpp"
    "${CMAKE_CURRENT_SOURCE_DIR}/../../test/*.c"
  )

  add_executable( bitcoin-database-test )

  target_sources( bitcoin-database-test PRIVATE
    ${bitcoin_database_test_SOURCES} )

  target_link_libraries( bitcoin-database-test PRIVATE
    bitcoin::database
    Boost::unit_test_framework
  )

  target_compile_options(bitcoin-database-test
    PRIVATE
      $<$<CXX_COMPILER_ID:MSVC>:/permissive- /W3>
      $<$<NOT:$<CXX_COMPILER_ID:MSVC>>:-Wall -Wextra -Wpedantic>
  )

  add_test( NAME bitcoin-database-test COMMAND bitcoin-database-test
    --run_test=*
    --log_level=warning
    --show_progress=no
    --detect_memory_leak=0
    --report_level=no
    --build_info=yes
  )

endif()

# ──────────────────────────────────────────────────────────────
#   Installation & CMake package config
# ──────────────────────────────────────────────────────────────

include(GNUInstallDirs)

install(TARGETS bitcoin-database
  EXPORT bitcoin-database-targets
  LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
  ARCHIVE DESTINATION ${CMAKE_INSTALL_LIBDIR}
  RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR}
  INCLUDES DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
  FILE_SET HEADERS
)

install(EXPORT bitcoin-database-targets
  NAMESPACE bitcoin::
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-database
  FILE bitcoin-database-targets.cmake
)

include(CMakePackageConfigHelpers)

configure_package_config_file(
  ${CMAKE_CURRENT_SOURCE_DIR}/bitcoin-database-config.cmake.in
  ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config.cmake
  INSTALL_DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-database
)

write_basic_package_version_file(
  bitcoin-database-config-version.cmake
  VERSION ${PACKAGE_VERSION}
  COMPATIBILITY AnyNewerVersion
)

install(
  FILES
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/bitcoin-database-config-version.cmake
  DESTINATION ${CMAKE_INSTALL_LIBDIR}/cmake/bitcoin-database
)
