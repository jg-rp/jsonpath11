cmake_minimum_required(VERSION 3.15...3.27)

project(jsonpath24
  VERSION 0.1.0
  LANGUAGES CXX
  DESCRIPTION "Fast JSONPath for Python"
  HOMEPAGE_URL "https://github.com/jg-rp/jsonpath24"
)

if (NOT SKBUILD)
  message(WARNING "\
  This CMake file is meant to be executed using 'scikit-build'. Running
  it directly will almost certainly not produce the desired result. If
  you are a user trying to install this package, please use the command
  below, which will install all necessary build dependencies, compile
  the package in an isolated environment, and then install it.
  =====================================================================
   $ pip install .
  =====================================================================
  If you are a software developer, and this is your own package, then
  it is usually much more efficient to install the build dependencies
  in your environment once and use the following command that avoids
  a costly creation of a new virtual environment at every compilation:
  =====================================================================
   $ pip install nanobind scikit-build-core[pyproject]
   $ pip install --no-build-isolation -ve .
  =====================================================================
  You may optionally add -Ceditable.rebuild=true to auto-rebuild when
  the package is imported. Otherwise, you need to re-run the above
  after editing C++ files.")
endif()

# Try to import all Python components potentially needed by nanobind
find_package(Python 3.8
  REQUIRED COMPONENTS Interpreter Development.Module
  OPTIONAL_COMPONENTS Development.SABIModule)

if (NOT CMAKE_BUILD_TYPE AND NOT CMAKE_CONFIGURATION_TYPES)
  set(CMAKE_BUILD_TYPE Release CACHE STRING "Choose the type of build." FORCE)
  set_property(CACHE CMAKE_BUILD_TYPE PROPERTY STRINGS "Debug" "Release" "MinSizeRel" "RelWithDebInfo")
endif()

# # Import nanobind through CMake's find_package mechanism
# find_package(nanobind CONFIG REQUIRED)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extern/nanobind)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/extern/libjsonpath)

nanobind_add_module(
  _jsonpath24

  # Target the stable ABI for Python 3.12+, which reduces
  # the number of binary wheels that must be built. This
  # does nothing on older Python versions
  STABLE_ABI

  # Build libnanobind statically and merge it into the
  # extension (which itself remains a shared library)
  #
  # If your project builds multiple extensions, you can
  # replace this flag by NB_SHARED to conserve space by
  # reusing a shared libnanobind across libraries
  NB_STATIC

  src/jsonpath24.cpp
  src/libjsonpath/node.cpp
  src/libjsonpath/path.cpp
)

target_link_libraries(_jsonpath24 PUBLIC jsonpath)

target_include_directories(_jsonpath24 PUBLIC 
  $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
  $<INSTALL_INTERFACE:include>
  ${PROJECT_SOURCE_DIR}/extern/libjsonpath/include
)

# Install directive for scikit-build-core
install(TARGETS _jsonpath24 LIBRARY DESTINATION jsonpath24)