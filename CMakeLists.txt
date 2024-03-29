cmake_minimum_required(VERSION 3.18)
project(MCGA_threading)
set(CMAKE_CXX_STANDARD 20)

set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

option(MCGA_threading_tests "Build MCGA Threading tests" OFF)
option(MCGA_threading_benchmarks "Build MCGA Threading benchmarks" OFF)
option(MCGA_threading_benchmark_against_evpp "Include the evpp library's EventLoop and EventLoopPool in the MCGA Threading benchmark" OFF)
option(MCGA_threading_examples "Build MCGA Threading examples" OFF)

if (SANITIZER_COMPILE_OPTIONS)
    add_compile_options(${SANITIZER_COMPILE_OPTIONS})
endif ()
if (SANITIZER_LINK_OPTIONS)
    add_link_options(${SANITIZER_LINK_OPTIONS})
endif ()

include(FindThreads)

add_library(mcga_threading INTERFACE)
target_include_directories(mcga_threading
        INTERFACE SYSTEM third_party/concurrentqueue
        INTERFACE include
        )
target_link_libraries(mcga_threading INTERFACE ${CMAKE_THREAD_LIBS_INIT})

install(DIRECTORY include DESTINATION ..)
install(FILES third_party/concurrentqueue/concurrentqueue.h
        DESTINATION include/concurrentqueue.h)

if (MCGA_threading_tests)
    function(add_mcga_test TEST_NAME)
        add_executable(${TEST_NAME}_test ${ARGN})
    endfunction()

    add_executable(mcga_threading_test
            tests/constructs/event_loop_thread.cpp
            tests/constructs/event_loop_thread_pool.cpp
            tests/base/thread_pool_wrapper.cpp
            tests/base/thread_wrapper.cpp
            )
    target_link_libraries(mcga_threading_test mcga_test mcga_threading)
endif ()

if (MCGA_threading_benchmarks)
    function(add_benchmark BENCHMARK_NAME)
        set(BENCHMARK_TARGET mcga_threading_${BENCHMARK_NAME}_benchmark)
        add_executable(${BENCHMARK_TARGET} ${ARGN})
        target_link_libraries(${BENCHMARK_TARGET} mcga_threading)

        if (MCGA_threading_benchmark_against_evpp)
            target_compile_definitions(${BENCHMARK_TARGET} PUBLIC LINK_EVPP)
            target_link_directories(${BENCHMARK_TARGET} PUBLIC /usr/local/lib)
            target_include_directories(${BENCHMARK_TARGET} PUBLIC /usr/local/include)
            target_link_libraries(${BENCHMARK_TARGET} evpp_static event glog)
        endif ()
    endfunction()

    add_benchmark(event_loop_delay_error benchmarks/event_loop_delay_error.cpp)
    add_benchmark(simple_function benchmarks/simple_function.cpp)
    add_benchmark(object_processing benchmarks/object_processing.cpp)
endif ()

if (MCGA_threading_examples)
    function(add_example example_name)
        add_executable(mcga_threading_${example_name}_example examples/${example_name}.cpp)
        target_link_libraries(mcga_threading_${example_name}_example PUBLIC mcga_threading)
    endfunction()

    add_example(intervals)
    add_example(move_only_object_processor)
    add_example(own_processor)
endif ()
