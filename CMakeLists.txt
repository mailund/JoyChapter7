cmake_minimum_required(VERSION 3.5)
project(
    JoyChapter7
    LANGUAGES C
)

include(CTest)

add_executable(performance hash_functions.c hash_functions_performance.c)
add_test(
    NAME    performance 
    COMMAND performance
)

add_executable(rehash_test rehash_test.c open_addressing.c)
add_test(
    NAME    rehash_test 
    COMMAND rehash_test
)

add_executable(rehash_performance rehash_performance.c open_addressing.c)
add_test(
    NAME    open_addressing 
    COMMAND open_addressing
)
