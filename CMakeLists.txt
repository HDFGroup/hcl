cmake_minimum_required(VERSION 3.10)

project(hcl VERSION 1.0.0 DESCRIPTION "HCL: A library for flexible distributed data containers")

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -gdwarf-2 -g3")

option(HCL_ENABLE_RPCLIB "allow hcl to use RPCLIB" ON)
option(HCL_ENABLE_THALLIUM_TCP "allow hcl to use RPCLIB" OFF)
option(HCL_ENABLE_THALLIUM_ROCE "allow hcl to use RPCLIB" OFF)
option(BASKET_TIMER "Show timing information of library calls." OFF)
option(BASKET_TRACE "Show traces of library calls." OFF)

if(BASKET_ENABLE_RPCLIB)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBASKET_ENABLE_RPCLIB")
    message("BASKET_ENABLE_RPCLIB: ${BASKET_ENABLE_RPCLIB}")
elseif(BASKET_ENABLE_THALLIUM_TCP)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBASKET_ENABLE_THALLIUM_TCP")
    message("BASKET_ENABLE_THALLIUM_TCP: ${BASKET_ENABLE_THALLIUM_TCP}")
elseif(BASKET_ENABLE_THALLIUM_ROCE)
    set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -DBASKET_ENABLE_THALLIUM_ROCE")
    message("BASKET_ENABLE_THALLIUM_ROCE: ${BASKET_ENABLE_THALLIUM_ROCE}")
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

add_library(${PROJECT_NAME} SHARED ${BASKET_SRC})

set_target_properties(${PROJECT_NAME} PROPERTIES
    VERSION ${PROJECT_VERSION}
    SOVERSION 1
    PUBLIC_HEADER include/hcl.h)
install(DIRECTORY include/ DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if(BASKET_TIMER)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BASKET_TIMER)
endif()

if(BASKET_TRACE)
  target_compile_definitions(${PROJECT_NAME} PRIVATE BASKET_TRACE)
endif()

set_target_properties(${PROJECT_NAME} PROPERTIES LINKER_LANGUAGE CXX)

configure_file(hcl.pc.in hcl.pc @ONLY)

if(BASKET_ENABLE_RPCLIB)
    set(RPC_LIB_FLAGS -lrpc ${RPC_LIB_FLAGS})
endif()
if(BASKET_ENABLE_THALLIUM_ROCE OR BASKET_ENABLE_THALLIUM_TCP)
    set(RPC_LIB_FLAGS -lthallium -lmercury -lmercury_util -lmargo -labt ${RPC_LIB_FLAGS})
endif()

set(LIB_FLAGS ${RPC_LIB_FLAGS} -lmpi -lpthread -lrt)

# libs for NATS tests
#set(LIB_FLAGS ${LIB_FLAGS} -lnats)

# libs for Redis tests
#set(LIB_FLAGS ${LIB_FLAGS} -lev -L/home/kbateman/install/lib/ -L/home/kbateman/install/lib64/ -lhiredis -lredox)

target_link_libraries(hcl ${LIB_FLAGS})

install(TARGETS ${PROJECT_NAME}
    LIBRARY DESTINATION ${CMAKE_INSTALL_LIBDIR}
    PUBLIC_HEADER DESTINATION ${CMAKE_INSTALL_INCLUDEDIR})

if (DEFINED ENV{PKG_CONFIG_PATH})
   install(FILES ${CMAKE_BINARY_DIR}/hcl.pc
                  DESTINATION $ENV{PKG_CONFIG_PATH}/pkgconfig)
else()
   install(FILES ${CMAKE_BINARY_DIR}/hcl.pc
                  DESTINATION ${CMAKE_INSTALL_DATAROOTDIR}/pkgconfig)
endif()


option(BUILD_TEST "Build the unit tests" ON)
if(BUILD_TEST)
    enable_testing()
    add_subdirectory(test)
endif()
