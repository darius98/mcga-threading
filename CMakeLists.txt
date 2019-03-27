cmake_minimum_required(VERSION 3.12.4)
project(MCGA_Threading)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

add_library(mcga_threading STATIC
    src/delayed_invocation.cpp
    src/event_loop.cpp
    src/event_loop_thread.cpp
)
target_include_directories(mcga_threading PUBLIC include)

install(DIRECTORY include DESTINATION .)

option(BuildTests "Build tests using the KKTest library" OFF)
if (BuildTests)
    enable_testing()

    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

    function (add_kktest TEST_NAME)
        add_executable(${TEST_NAME}_test ${ARGN})
        target_link_libraries(${TEST_NAME}_test kktest mcga_threading)
        add_dependencies(check ${TEST_NAME}_test)
        add_test(NAME ${TEST_NAME}
            COMMAND ${TEST_NAME}_test --max-parallel-tests=10
        )
    endfunction ()

    add_kktest(event_loop tests/event_loop.cpp)
    add_kktest(event_loop_thread tests/event_loop_thread.cpp)
endif ()
