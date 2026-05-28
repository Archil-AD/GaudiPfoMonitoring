cmake_minimum_required(VERSION 3.23 FATAL_ERROR)

if(${CMAKE_SOURCE_DIR} STREQUAL ${CMAKE_BINARY_DIR})
    message(FATAL_ERROR "PfoMonitoringContent requires an out-of-source build.")
endif()

project(PfoMonitoring LANGUAGES CXX)

set(${PROJECT_NAME}_VERSION_MAJOR 01)
set(${PROJECT_NAME}_VERSION_MINOR 00)
set(${PROJECT_NAME}_VERSION_PATCH 00)
set(${PROJECT_NAME}_VERSION "${${PROJECT_NAME}_VERSION_MAJOR}.${${PROJECT_NAME}_VERSION_MINOR}.${${PROJECT_NAME}_VERSION_PATCH}")

include(GNUInstallDirs)

# load default settings from ILCSOFT_CMAKE_MODULES
FIND_PACKAGE( ILCUTIL COMPONENTS ILCSOFT_CMAKE_MODULES REQUIRED )

INCLUDE( ilcsoft_default_settings )


# ============================================================================
# Find required packages
# ============================================================================
find_package(PandoraSDK 03.00.00 REQUIRED)
message(STATUS "PandoraSDK found: ${PandoraSDK_FOUND}")

# Important: find_package respects CMAKE_PREFIX_PATH automatically
find_package(LCContent 03.00.00 REQUIRED)
message(STATUS "LCContent found: ${LCContent_FOUND}")
message(STATUS "LCContent_INCLUDE_DIRS: ${LCContent_INCLUDE_DIRS}")
message(STATUS "LCContent_LIBRARIES: ${LCContent_LIBRARIES}")

option(PANDORA_MONITORING "Build with Pandora monitoring support" OFF)
if(PANDORA_MONITORING)
    find_package(PandoraMonitoring 03.00.00 REQUIRED)
endif()

set(CMAKE_CXX_STANDARD 11 CACHE STRING "C++ Standard")
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(CMAKE_CXX_FLAGS "-Wall -Wextra -pedantic -Wno-long-long -Wno-sign-compare -Wshadow -fno-strict-aliasing ${CMAKE_CXX_FLAGS}")

include(CheckCXXCompilerFlag)
unset(COMPILER_SUPPORTS_CXX_FLAGS CACHE)
CHECK_CXX_COMPILER_FLAG(${CMAKE_CXX_FLAGS} COMPILER_SUPPORTS_CXX_FLAGS)

if(NOT COMPILER_SUPPORTS_CXX_FLAGS)
    message(FATAL_ERROR "The compiler ${CMAKE_CXX_COMPILER} does not support cxx flags ${CMAKE_CXX_FLAGS}")
endif()

# ============================================================================
# Collect source files
# ============================================================================
file(GLOB_RECURSE PROJECT_SRCS RELATIVE ${PROJECT_SOURCE_DIR} "src/*.cc")
file(GLOB_RECURSE headers RELATIVE ${CMAKE_CURRENT_SOURCE_DIR} "include/*.h")

# Separate Gaudi module sources to avoid double compilation if they are in src/
set(MODULE_SOURCES
    src/SavePfoMonitoringTree.cc
)

foreach(_src ${MODULE_SOURCES})
    list(REMOVE_ITEM PROJECT_SRCS ${_src})
endforeach()

# ============================================================================
# CREATE THE LIBRARY TARGET
# ============================================================================
add_library(${PROJECT_NAME} SHARED ${PROJECT_SRCS})
add_library(${PROJECT_NAME}::${PROJECT_NAME} ALIAS ${PROJECT_NAME})
set(${PROJECT_NAME}_LIBRARIES ${PROJECT_NAME}::${PROJECT_NAME})

# ============================================================================
# Set headers as public
# ============================================================================
if(headers)
    target_sources(${PROJECT_NAME} PUBLIC FILE_SET headers TYPE HEADERS BASE_DIRS "include" FILES ${headers})
endif()

# ============================================================================
# Prepare External Dependencies
# ============================================================================
set(PANDORA_LIBS "")

if(TARGET PandoraPFA::PandoraSDK)
    message(STATUS "Linking PandoraPFA::PandoraSDK")
    list(APPEND PANDORA_LIBS PandoraPFA::PandoraSDK)
else()
    message(STATUS "Using PandoraSDK via include/lib dirs")
    target_include_directories(${PROJECT_NAME} PUBLIC ${PandoraSDK_INCLUDE_DIRS})
    list(APPEND PANDORA_LIBS ${PandoraSDK_LIBRARIES})
endif()

if(TARGET PandoraPFA::LCContent)
    message(STATUS "Linking PandoraPFA::LCContent")
    list(APPEND PANDORA_LIBS PandoraPFA::LCContent)
elseif(TARGET LCContent::LCContent)
    message(STATUS "Linking LCContent::LCContent")
    list(APPEND PANDORA_LIBS LCContent::LCContent)
elseif(LCContent_LIBRARIES)
    message(STATUS "Using LCContent via include/lib dirs")
    target_include_directories(${PROJECT_NAME} PUBLIC ${LCContent_INCLUDE_DIRS})
    list(APPEND PANDORA_LIBS ${LCContent_LIBRARIES})
else()
    message(FATAL_ERROR "LCContent found but no target or libraries defined")
endif()

FIND_PACKAGE( Marlin 1.0 REQUIRED )
FIND_PACKAGE( MarlinUtil 01.10 REQUIRED )

set(MARLIN_LIBS "")
FOREACH( pkg Marlin MarlinUtil)
    IF( ${pkg}_FOUND )
       list(APPEND MARLIN_LIBS ${${pkg}_LIBRARIES})
       target_include_directories(${PROJECT_NAME} PUBLIC ${${pkg}_INCLUDE_DIRS})
    ENDIF()
ENDFOREACH()

target_link_libraries(${PROJECT_NAME} PUBLIC ${PANDORA_LIBS} ${MARLIN_LIBS})

# ============================================================================
# Define Gaudi module
# ============================================================================

# Find Gaudi and ROOT packages
# dependencies are declared like this
find_package(Gaudi REQUIRED)
find_package(ROOT COMPONENTS RIO Tree REQUIRED)
find_package(EDM4HEP REQUIRED)
find_package(k4FWCore REQUIRED)
#find_package(podio REQUIRED)

gaudi_add_module(GaudiPfoMonitoring
                 SOURCES ${MODULE_SOURCES}
                 LINK Gaudi::GaudiKernel
                      ${PROJECT_NAME}
                     # ${PANDORA_LIBS}
                     # ${MARLIN_LIBS}
                      k4FWCore::k4FWCore
                     # EDM4HEP::edm4hep
                     # podio::podio
                      ROOT::RIO
                      ROOT::Tree)

# ============================================================================
# Installation
# ============================================================================
install(TARGETS ${PROJECT_NAME}
    EXPORT ${PROJECT_NAME}Targets
    RUNTIME DESTINATION "${CMAKE_INSTALL_BINDIR}" COMPONENT bin
    LIBRARY DESTINATION "${CMAKE_INSTALL_LIBDIR}" COMPONENT shlib
    FILE_SET headers DESTINATION ${CMAKE_INSTALL_INCLUDEDIR} COMPONENT dev
)


# ============================================================================
# CMake Config Files for downstream projects
# ============================================================================
include(CMakePackageConfigHelpers)

configure_package_config_file(
    "${CMAKE_CURRENT_SOURCE_DIR}/cmake/${PROJECT_NAME}Config.cmake.in"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    INSTALL_DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    PATH_VARS CMAKE_INSTALL_INCLUDEDIR CMAKE_INSTALL_LIBDIR
)

write_basic_package_version_file(
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    VERSION ${${PROJECT_NAME}_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}Config.cmake"
    "${CMAKE_CURRENT_BINARY_DIR}/${PROJECT_NAME}ConfigVersion.cmake"
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT dev
)

install(EXPORT ${PROJECT_NAME}Targets
    FILE ${PROJECT_NAME}Targets.cmake
    NAMESPACE PandoraPFA::
    DESTINATION "${CMAKE_INSTALL_LIBDIR}/cmake/${PROJECT_NAME}"
    COMPONENT dev
)


# ============================================================================
# Set include directories
# ============================================================================
target_include_directories(${PROJECT_NAME} PUBLIC
    $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
    $<INSTALL_INTERFACE:include>
)
