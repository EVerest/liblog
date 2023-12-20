find_package(GTest REQUIRED)

set(TEST_NAME liblog_test)
add_executable(${TEST_NAME} liblog_test.cpp)

target_include_directories(${TEST_NAME} PUBLIC ${GTEST_INCLUDE_DIRS})
target_include_directories(log
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)
target_link_libraries(${TEST_NAME} PRIVATE
        ${GTEST_LIBRARIES}
        ${GTEST_MAIN_LIBRARIES}
)

target_link_libraries(${TEST_NAME} PRIVATE lib gtest_main)
gtest_discover_tests(${TEST_NAME})
