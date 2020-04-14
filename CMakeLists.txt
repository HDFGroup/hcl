cmake_minimum_required(VERSION 3.10)

project(hcl VERSION 1.0.0 DESCRIPTION "HCL: A library for flexible distributed data containers")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2 -g3")

option(HCL_ENABLE_RPCLIB "allow hcl to use RPCLIB" ON)
option(HCL_ENABLE_THALLIUM_TCP "allow hcl to use RPCLIB" OFF)
option(HCL_ENABLE_THALLIUM_ROCE "allow hcl to use RPCLIB" OFF)
option(HCL_TIMER "Show timing information of library calls." OFF)
option(HCL_TRACE "Show traces of library calls." OFF)

if(NOT HCL_INSTALL_DATA_DIR)
    set(HCL_INSTALL_DATA_DIR ${CMAKE_INSTALL_PREFIX}/share)
endif()

if(NOT HCL_INSTALL_LIB_DIR)
  set(HCL_INSTALL_LIB_DIR ${CMAKE_INSTALL_PREFIX}/lib)
endif()

if(NOT HCL_EXPORTED_TARGETS)
    set(HCL_EXPORTED_TARGETS "hcl-targets")
endif()

if(HCL_ENABLE_RPCLIB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHCL_ENABLE_RPCLIB")
    message("HCL_ENABLE_RPCLIB: ${HCL_ENABLE_RPCLIB}")
elseif(HCL_ENABLE_THALLIUM_TCP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHCL_ENABLE_THALLIUM_TCP")
    message("HCL_ENABLE_THALLIUM_TCP: ${HCL_ENABLE_THALLIUM_TCP}")
elseif(HCL_ENABLE_THALLIUM_ROCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DHCL_ENABLE_THALLIUM_ROCE")
    message("HCL_ENABLE_THALLIUM_ROCE: ${HCL_ENABLE_THALLIUM_ROCE}")
endif()

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -g")

include(GNUInstallDirs)
include_directories(src/hcl)
include_directories(src)
include_directories(include)
set(HCL_SRC  include/hcl/common/singleton.h
                include/hcl/common/debug.h
                include/hcl/common/enumerations.h
                include/hcl/common/macros.h
                include/hcl/common/configuration_manager.h
                src/hcl/common/debug.cpp
                include/hcl/common/constants.h
                include/hcl/common/typedefs.h
                include/hcl/common/data_structures.h
                src/hcl/common/data_structures.cpp
                include/hcl/communication/rpc_lib.h
                src/hcl/communication/rpc_lib.cpp
                include/hcl/unordered_map/unordered_map.h
                include/hcl/map/map.h
                include/hcl/multimap/multimap.h
                include/hcl/clock/global_clock.h
                src/hcl/clock/global_clock.cpp
                include/hcl/queue/queue.h
                include/hcl/priority_queue/priority_queue.h
                include/hcl/set/set.h
                include/hcl/sequencer/global_sequence.h
                src/hcl/sequencer/global_sequence.cpp
                include/hcl/communication/rpc_factory.h)

add_library(${PROJECT_NAME} SHARED ${HCL_SRC})
target_compile_options(${PROJECT_NAME} PRIVATE -Wall -Wextra)

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/hcl.h)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(HCL_TIMER)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HCL_TIMER)
endif()

if(HCL_TRACE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE HCL_TRACE)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

configure_file(hcl.pc.in hcl.pc @ONLY)

if(NOT APPLE)
  find_library(LIBRT rt)
  if(NOT LIBRT)
    message(FATAL_ERROR "librt is required for POSIX shared memory")
  endif()
endif()

find_library(LIBPTHREAD pthread)

target_link_libraries(${PROJECT_NAME} ${LIBRT} ${LIBPTHREAD})

if(HCL_ENABLE_RPCLIB)
    set(RPC_LIB_FLAGS -lrpc ${RPC_LIB_FLAGS})
    target_link_libraries(${PROJECT_NAME} ${RPC_LIB_FLAGS})
endif()

if(HCL_ENABLE_THALLIUM_ROCE OR HCL_ENABLE_THALLIUM_TCP)
    find_package(thallium CONFIG REQUIRED)
    if(thallium_FOUND)
        message(STATUS "found thallium at ${thallium_DIR}")
        target_link_libraries(${PROJECT_NAME} thallium)
    endif()
endif()

include(CMakePackageConfigHelpers)

configure_package_config_file(
    ${CMAKE_CURRENT_SOURCE_DIR}/CMake/hcl-config.cmake.in
    ${CMAKE_CURRENT_BINARY_DIR}/hcl-config.cmake
    INSTALL_DESTINATION ${HCL_INSTALL_DATA_DIR}/cmake/hcl
)

write_basic_package_version_file(
    ${CMAKE_CURRENT_BINARY_DIR}/hcl-config-version.cmake
    VERSION ${PROJECT_VERSION}
    COMPATIBILITY SameMajorVersion
)

install(FILES
    ${CMAKE_CURRENT_BINARY_DIR}/hcl-config.cmake
    ${CMAKE_CURRENT_BINARY_DIR}/hcl-config-version.cmake
    DESTINATION ${HCL_INSTALL_DATA_DIR}/cmake/hcl
)

install(TARGETS ${PROJECT_NAME}
    EXPORT ${HCL_EXPORTED_TARGETS}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR}
)

install(EXPORT ${HCL_EXPORTED_TARGETS}
    DESTINATION ${HCL_INSTALL_DATA_DIR}/cmake/hcl
    FILE ${HCL_EXPORTED_TARGETS}.cmake
)

if (DEFINED ENV{PKG_CONFIG_PATH} AND NOT "$ENV{PKG_CONFIG_PATH}" STREQUAL "")
  message(STATUS "PKG_CONFIG_PATH is set to: " $ENV{PKG_CONFIG_PATH})
   install(FILES ${CMAKE_BINARY_DIR}/hcl.pc
                  DESTINATION $ENV{PKG_CONFIG_PATH}/pkgconfig)
else()
   install(FILES ${CMAKE_BINARY_DIR}/hcl.pc
                  DESTINATION ${HCL_INSTALL_LIB_DIR}/pkgconfig)
                message(STATUS "PKG_CONFIG_PATH is not set" )
endif()

option(BUILD_TEST "Build the unit tests" ON)
if(BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
