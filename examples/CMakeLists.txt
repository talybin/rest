# REST examples

file (GLOB EXAMPLES_SRC *.cpp)

function (MAKE_EXAMPLE example_source_file)
    get_filename_component (example_name ${example_source_file} NAME_WE)
    add_executable (${example_name} ${example_source_file})
    target_link_libraries(${example_name} rest ${Boost_LIBRARIES} Threads::Threads)
    install (TARGETS ${example_name} RUNTIME DESTINATION bin)
endfunction (MAKE_EXAMPLE)

if (EXAMPLES)
    foreach (example_source_file ${EXAMPLES_SRC})
        MAKE_EXAMPLE (${example_source_file})
    endforeach ()
endif ()

