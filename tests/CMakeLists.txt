include(CTest)

function(add_test_suite name)
    add_executable(${name} ${name}.cpp)
    target_link_libraries(${name} PRIVATE luft)
    add_test(NAME ${name} COMMAND ${name})
endfunction()

add_test_suite(basic)
