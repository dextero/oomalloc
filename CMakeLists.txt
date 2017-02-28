cmake_minimum_required(VERSION 2.8.5)
project(oomalloc)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -Wall -Wextra -std=c99")

set(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/lib)
set(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/bin)


include_directories(include)
add_library(${PROJECT_NAME} SHARED src/oomalloc.c)


install(FILES include/oomalloc.h DESTINATION include)
install(TARGETS ${PROJECT_NAME} DESTINATION lib)


find_package(GTest)
if(GTEST_FOUND)
    include_directories(${GTEST_INCLUDE_DIRS})
    add_executable(${PROJECT_NAME}_test)
    target_link_libraries(${PROJECT_NAME}_test ${GTEST_BOTH_LIBRARIES} ${PROJECT_NAME})

    enable_testing()
    add_test(${PROJECT_NAME}_test
             ${EXECUTABLE_OUTPUT_PATH}/${PROJECT_NAME}_test)
endif()
