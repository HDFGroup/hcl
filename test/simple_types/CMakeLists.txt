# target_link_libraries(DistributedHashMapTest ${CMAKE_BINARY_DIR}/libhcl.so)
set(TEST_TYPE simple_types)
add_custom_target(${TEST_TYPE}_copy_hostfile)
add_custom_command(
        TARGET ${TEST_TYPE}_copy_hostfile
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
        ${CMAKE_CURRENT_BINARY_DIR}/hostfile)

add_custom_target(${TEST_TYPE}_copy_server_list)
add_custom_command(
        TARGET ${TEST_TYPE}_copy_server_list
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/server_list
        ${CMAKE_CURRENT_BINARY_DIR}/server_list)

set(examples unordered_map_test map_test queue_test priority_queue_test multimap_test set_test global_clock_test)

# Compile all examples
foreach (example ${examples})
    add_executable (${TEST_TYPE}_${example} ${example}.cpp util.h)
    add_dependencies(${TEST_TYPE}_${example} ${PROJECT_NAME})
    add_dependencies(${TEST_TYPE}_${example} ${TEST_TYPE}_copy_hostfile)
    add_dependencies(${TEST_TYPE}_${example} ${TEST_TYPE}_copy_server_list)
    set_target_properties(${TEST_TYPE}_${example} PROPERTIES ENVIRONMENT LD_PRELOAD=${CMAKE_BINARY_DIR}/libhcl.so)
    target_link_libraries(${TEST_TYPE}_${example} ${PROJECT_NAME})
endforeach()

message(INFO ${CMAKE_BINARY_DIR}/libhcl.so)

# Define MPI test case template
function(mpi target mpi_procs example ranks_per_process num_requests size_of_request server_on_node debug)
    set (test_parameters  -np ${mpi_procs} -f "${CMAKE_BINARY_DIR}/test/${TEST_TYPE}/hostfile" "${CMAKE_BINARY_DIR}/test/${TEST_TYPE}/${TEST_TYPE}_${example}" ${ranks_per_process} ${num_requests} ${size_of_request} ${server_on_node} ${debug} "${CMAKE_BINARY_DIR}/test/${TEST_TYPE}/server_list")
    add_test(NAME ${TEST_TYPE}_${target}_${example}_MPI_${mpi_procs}_${ranks_per_process}_${num_requests}_${size_of_request}_${server_on_node}_${debug} COMMAND "mpirun" ${test_parameters})
    set_tests_properties(${TEST_TYPE}_${target}_${example}_MPI_${mpi_procs}_${ranks_per_process}_${num_requests}_${size_of_request}_${server_on_node}_${debug} PROPERTIES ENVIRONMENT LD_PRELOAD=${CMAKE_BINARY_DIR}/libhcl.so)
endfunction()

# Define MPI test case
foreach (example ${examples})
    mpi(ares 2 ${example} 2 500 1000 1 0)
endforeach()
