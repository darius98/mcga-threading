cmake_minimum_required(VERSION 3.12.4)
project(MCGA_Threading)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

option(BuildTests "Build tests using the KKTest library" OFF)
option(BuildBenchmarks "Build the benchmarks" OFF)

include(FindThreads)

add_library(mcga_threading INTERFACE)
target_include_directories(mcga_threading
    INTERFACE third_party/concurrentqueue
    INTERFACE third_party/readerwriterqueue
    INTERFACE include
)
target_link_libraries(mcga_threading INTERFACE ${CMAKE_THREAD_LIBS_INIT})

install(DIRECTORY include DESTINATION .)
install(FILES third_party/concurrentqueue/concurrentqueue.h
                      DESTINATION include/concurrentqueue.h)
install(FILES third_party/concurrentqueue/blockingconcurrentqueue.h
                      DESTINATION include/blockingconcurrentqueue.h)
install(FILES third_party/readerwriterqueue/readerwriterqueue.h
                        DESTINATION include/readerwriterqueue.h)
install(FILES third_party/readerwriterqueue/atomicops.h
                        DESTINATION include/atomicops.h)

if (BuildTests)
    function (add_kktest TEST_NAME)
        add_executable(${TEST_NAME}_test ${ARGN})
    endfunction ()

    add_executable(mcga_threading_test
        tests/event_loop.cpp
        tests/event_loop_pool.cpp
        tests/thread_pool_wrapper.cpp
        tests/thread_wrapper.cpp
        tests/worker.cpp
        tests/worker_pool.cpp
    )
    target_link_directories(mcga_threading_test PRIVATE /usr/local/lib)
    target_include_directories(mcga_threading_test PRIVATE /usr/local/include)
    target_link_libraries(mcga_threading_test kktest mcga_threading)
endif ()

if (BuildBenchmarks)
    function (add_benchmark BENCHMARK_NAME)
        add_executable(${BENCHMARK_NAME}_benchmark ${ARGN})
        target_link_libraries(${BENCHMARK_NAME}_benchmark mcga_threading)
    endfunction ()

    add_benchmark(event_loop_delay_error benchmarks/event_loop_delay_error.cpp)
endif ()
