cmake_minimum_required(VERSION 3.12.4)
project(MCGA_Threading)
set(CMAKE_CXX_STANDARD 17)

set(CMAKE_EXPORT_COMPILE_COMMANDS TRUE)
set(CMAKE_POSITION_INDEPENDENT_CODE TRUE)
set(CMAKE_CXX_VISIBILITY_PRESET hidden)

option(BuildTests "Build tests using the KKTest library" OFF)
option(BuildBenchmarks "Build the benchmarks" OFF)

include(FindThreads)

add_library(mcga_threading STATIC
    src/delayed_invocation.cpp
    src/event_loop.cpp
    src/event_loop_thread.cpp
)
target_include_directories(mcga_threading PUBLIC third_party/concurrentqueue)
target_include_directories(mcga_threading PUBLIC third_party/readerwriterqueue)
target_include_directories(mcga_threading PUBLIC include)
target_link_libraries(mcga_threading PUBLIC ${CMAKE_THREAD_LIBS_INIT})

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
    enable_testing()

    add_custom_target(check COMMAND ${CMAKE_CTEST_COMMAND})

    function (add_kktest TEST_NAME)
        add_executable(${TEST_NAME}_test ${ARGN})
        target_link_directories(${TEST_NAME}_test PRIVATE /usr/local/lib)
        target_include_directories(${TEST_NAME}_test PRIVATE /usr/local/include)
        target_link_libraries(${TEST_NAME}_test kktest mcga_threading)
        add_dependencies(check ${TEST_NAME}_test)
        add_test(NAME ${TEST_NAME}
            COMMAND ${TEST_NAME}_test --max-parallel-tests=10
        )
    endfunction ()

    add_kktest(event_loop tests/event_loop.cpp)
    add_kktest(event_loop_thread tests/event_loop_thread.cpp)
endif ()

if (BuildBenchmarks)
    function (add_benchmark BENCHMARK_NAME)
        add_executable(${BENCHMARK_NAME}_benchmark ${ARGN})
        target_link_libraries(${BENCHMARK_NAME}_benchmark mcga_threading)
    endfunction ()

    add_benchmark(event_loop_delay_error benchmarks/event_loop_delay_error.cpp)
endif ()
