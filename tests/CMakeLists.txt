setup_target_for_coverage_gcovr_html(
    NAME ${PROJECT_NAME}_gcovr_coverage
    EXECUTABLE ctest
    DEPENDENCIES ${PROJECT_NAME}_tests
    EXCLUDE "examples/*"
)

setup_target_for_coverage_gcovr_xml(
    NAME ${PROJECT_NAME}_gcovr_coverage_xml
    EXECUTABLE ctest
    DEPENDENCIES ${PROJECT_NAME}_tests
    EXCLUDE "examples/*"
)

setup_target_for_coverage_lcov(
    NAME ${PROJECT_NAME}_lcov_coverage
    EXECUTABLE ctest
    DEPENDENCIES ${PROJECT_NAME}_tests
    LCOV_ARGS "--ignore-errors=empty,missing"
    EXCLUDE "examples/*"
)

set(TEST_TARGET_NAME ${PROJECT_NAME}_tests)
add_executable(${TEST_TARGET_NAME})

target_include_directories(log
    PUBLIC
        $<BUILD_INTERFACE:${PROJECT_SOURCE_DIR}/lib>
        $<INSTALL_INTERFACE:${CMAKE_INSTALL_INCLUDEDIR}>
)

target_link_libraries(${TEST_TARGET_NAME} PRIVATE
        everest::log
        GTest::gtest_main
        GTest::gmock_main
)

target_sources(${TEST_TARGET_NAME} PRIVATE
    liblog_test.cpp
)

if (BUILD_BACKTRACE_SUPPORT)
    target_compile_definitions(${TEST_TARGET_NAME} PRIVATE WITH_LIBBACKTRACE)
endif()

add_test(${TEST_TARGET_NAME} ${TEST_TARGET_NAME})

# copy logging configs
configure_file(logging_configs/no_sinks.ini logging_configs/no_sinks.ini COPYONLY)
configure_file(logging_configs/console.ini logging_configs/console.ini COPYONLY)
configure_file(logging_configs/console_defaults.ini logging_configs/console_defaults.ini COPYONLY)
configure_file(logging_configs/textfile_verb.ini logging_configs/textfile_verb.ini COPYONLY)
configure_file(logging_configs/textfile_verb_rotate.ini logging_configs/textfile_verb_rotate.ini COPYONLY)
configure_file(logging_configs/textfile_verb_rotate_on_open.ini logging_configs/textfile_verb_rotate_on_open.ini COPYONLY)
configure_file(logging_configs/textfile_verb_broken_filename.ini logging_configs/textfile_verb_broken_filename.ini COPYONLY)
configure_file(logging_configs/textfile_verb_broken_format_string.ini logging_configs/textfile_verb_broken_format_string.ini COPYONLY)
configure_file(logging_configs/textfile_debg.ini logging_configs/textfile_debg.ini COPYONLY)
configure_file(logging_configs/textfile_info.ini logging_configs/textfile_info.ini COPYONLY)
configure_file(logging_configs/textfile_warn.ini logging_configs/textfile_warn.ini COPYONLY)
configure_file(logging_configs/textfile_erro.ini logging_configs/textfile_erro.ini COPYONLY)
configure_file(logging_configs/textfile_crit.ini logging_configs/textfile_crit.ini COPYONLY)
configure_file(logging_configs/textfile_off.ini logging_configs/textfile_off.ini COPYONLY)
configure_file(logging_configs/syslog.ini logging_configs/syslog.ini COPYONLY)
