# target_link_libraries(DistributedHashMapTest ${CMAKE_BINARY_DIR}/libbasket.so)

set(examples unordered_map_test map_test queue_test priority_queue_test multimap_test set_test)
# global_clock_test global_sequence_test

add_custom_target(copy_hostfile)
add_custom_command(
        TARGET copy_hostfile
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/hostfile
        ${CMAKE_CURRENT_BINARY_DIR})

add_custom_target(copy_server_list)
add_custom_command(
        TARGET copy_server_list
        COMMAND ${CMAKE_COMMAND} -E copy
        ${CMAKE_CURRENT_SOURCE_DIR}/server_list
        ${CMAKE_CURRENT_BINARY_DIR})

# Compile all examples
foreach (example ${examples})
    add_executable (${example} ${example}.cpp util.h)
    add_dependencies(${example} ${PROJECT_NAME})
    add_dependencies(${example} copy_hostfile)
    add_dependencies(${example} copy_server_list)
    set_target_properties(${example} PROPERTIES ENVIRONMENT LD_PRELOAD=${CMAKE_BINARY_DIR}/libbasket.so)
    target_link_libraries(${example} ${PROJECT_NAME})
    set_target_properties (${example} PROPERTIES FOLDER test)
endforeach()

message(INFO ${CMAKE_BINARY_DIR}/libbasket.so)

# Define MPI test case template
function(mpi target mpi_procs example ranks_per_process num_requests size_of_request server_on_node debug)
    set (test_parameters  -np ${mpi_procs} -f "${CMAKE_BINARY_DIR}/test/hostfile" "${CMAKE_BINARY_DIR}/test/${example}" ${ranks_per_process} ${num_requests} ${size_of_request} ${server_on_node} ${debug})
    add_test(NAME ${target}_${example}_MPI_${mpi_procs}_${ranks_per_process}_${num_requests}_${size_of_request}_${server_on_node}_${debug} COMMAND "mpirun" ${test_parameters})
    set_tests_properties(${target}_${example}_MPI_${mpi_procs}_${ranks_per_process}_${num_requests}_${size_of_request}_${server_on_node}_${debug} PROPERTIES ENVIRONMENT LD_PRELOAD=${CMAKE_BINARY_DIR}/libbasket.so)
endfunction()

# Define MPI test case
foreach (example ${examples})
    mpi(ares 4 ${example} 2 500 1000 1 0)
endforeach()
