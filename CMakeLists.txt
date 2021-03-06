cmake_minimum_required(VERSION 2.8.5)
project(oomalloc)

set(CMAKE_C_FLAGS "${CMAKE_C_FLAGS} -Wall -Wextra -std=c99")
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c++14")

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)

# oomalloc library
include_directories(include)
add_library(${PROJECT_NAME} SHARED src/oomalloc.c)
set_target_properties(${PROJECT_NAME} PROPERTIES
                      COMPILE_FLAGS "-nostdlib"
                      LINK_FLAGS "-nostdlib -Wl,--whole-archive")
target_link_libraries(${PROJECT_NAME} dl)

install(FILES include/oomalloc.h DESTINATION include)
install(TARGETS ${PROJECT_NAME} DESTINATION lib)

# tests
option(WITH_TESTS "Enable tests" OFF)
if(WITH_TESTS)
    set(BUILD_GTEST ON)
    set(BUILD_GMOCK OFF)
    add_subdirectory(deps/googletest)

    add_library(${PROJECT_NAME}_test_lib SHARED src/oomalloc.c)
    set_target_properties(${PROJECT_NAME}_test_lib PROPERTIES
                          COMPILE_FLAGS "-DOOMALLOC_TEST")
    target_link_libraries(${PROJECT_NAME}_test_lib dl)

    include_directories(deps/googletest/googletest/include
                        src)
    add_executable(${PROJECT_NAME}_test test/oomalloc.cpp)
    set_target_properties(${PROJECT_NAME}_test PROPERTIES
                          COMPILE_FLAGS "-DOOMALLOC_TEST")
    target_link_libraries(${PROJECT_NAME}_test gtest gtest_main ${PROJECT_NAME}_test_lib)

    enable_testing()
    add_test(${PROJECT_NAME}_test
             ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_test)
endif()
