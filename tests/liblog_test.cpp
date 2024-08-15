// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include <exception>
#include <filesystem>
#include <fstream>
#include <mutex>

#include <gmock/gmock.h>
#include <gtest/gtest.h>
#include <spdlog/sinks/base_sink.h>

#include <everest/exceptions.hpp>
#include <everest/logging.hpp>

namespace Everest {
namespace Logging {
namespace tests {

class TestSink : public spdlog::sinks::base_sink<std::mutex> {
public:
    // return number of log messages logged
    size_t count() {
        return this->counter;
    }

protected:
    void sink_it_(const spdlog::details::log_msg& message) override {
        this->counter += 1;
    }

    void flush_() override {
    }
    size_t counter = 0;
};

class LibLogUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
        auto logger = spdlog::default_logger();
        for (auto& sink : logger->sinks()) {
            sink->set_level(spdlog::level::off);
        }
    }

    void log_with_all_loglevels() {
        EVLOG_verbose << "This is a VERBOSE message.";
        EVLOG_debug << "This is a DEBUG message.";
        EVLOG_info << "This is a INFO message.";
        EVLOG_warning << "This is a WARNING message.";
        EVLOG_error << "This is a ERROR message.";
        EVLOG_critical << "This is a CRITICAL message.";
    }

    int count_log_entries(const std::filesystem::path& file_name) {
        std::ifstream log(file_name.string());
        EXPECT_TRUE(log.is_open());
        std::string line;
        auto count = 0;
        while (std::getline(log, line)) {
            count += 1;
        }
        log.close();
        return count;
    }
};

TEST_F(LibLogUnitTest, test_log_source_file_linenr_function) {
    auto log_source = LogSource("file", 42, "function");
    EXPECT_TRUE(log_source.log_file);
    EXPECT_TRUE(log_source.log_line);
    EXPECT_TRUE(log_source.log_function);
}

TEST_F(LibLogUnitTest, test_log_source_function) {
    auto log_source = LogSource("function");
    EXPECT_FALSE(log_source.log_file);
    EXPECT_FALSE(log_source.log_line);
    EXPECT_TRUE(log_source.log_function);
}

TEST_F(LibLogUnitTest, test_evthrow) {
    ASSERT_THROW(EVTHROW(std::runtime_error("hello there")), std::runtime_error);
}

TEST_F(LibLogUnitTest, test_evlog_and_throw) {
    ASSERT_THROW(EVLOG_AND_THROW(std::runtime_error("hello there")), std::runtime_error);
}

TEST_F(LibLogUnitTest, test_everest_logger) {
    auto test_sink = std::make_shared<TestSink>();
    auto test_logger = std::make_shared<spdlog::logger>("", test_sink);
    spdlog::set_default_logger(test_logger);
    spdlog::set_level(spdlog::level::level_enum::trace);
    EVLOG_verbose << "hello there";
    EVLOG_info << "hello there";
    EVLOG_warning << "hello there";
    ASSERT_EQ(test_sink->count(), 3);
}

TEST_F(LibLogUnitTest, test_init_wrong_filename) {
    auto test_sink = std::make_shared<TestSink>();
    auto test_logger = std::make_shared<spdlog::logger>("", test_sink);
    spdlog::set_default_logger(test_logger);
    spdlog::set_level(spdlog::level::level_enum::trace);
    ASSERT_THROW(Everest::Logging::init("this_file_does_not_exist", "liblog_test"), Everest::EverestConfigError);
}

TEST_F(LibLogUnitTest, test_init_no_sinks) {
    auto test_sink = std::make_shared<TestSink>();
    auto test_logger = std::make_shared<spdlog::logger>("", test_sink);
    spdlog::set_default_logger(test_logger);
    spdlog::set_level(spdlog::level::level_enum::trace);
    ASSERT_THROW(Everest::Logging::init("logging_configs/no_sinks.ini", "liblog_test"), Everest::EverestConfigError);
}

TEST_F(LibLogUnitTest, test_init_console_sink) {
    Everest::Logging::init("logging_configs/console.ini", "liblog_test");
    log_with_all_loglevels();
}

TEST_F(LibLogUnitTest, test_init_console_sink_defaults) {
    Everest::Logging::init("logging_configs/console_defaults.ini", "liblog_test");
    log_with_all_loglevels();
}

TEST_F(LibLogUnitTest, test_init_console_sink_no_process_name) {
    Everest::Logging::init("logging_configs/console.ini");
    log_with_all_loglevels();
}

TEST_F(LibLogUnitTest, test_init_console_sink_correct_filter) {
    Everest::Logging::init("logging_configs/console.ini", "EVerest");
    log_with_all_loglevels();
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_verb) {
    auto log_dir = std::filesystem::path("liblog_test_logs_verb");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_verb.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 7);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_debg) {
    auto log_dir = std::filesystem::path("liblog_test_logs_debg");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_debg.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 6);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_info) {
    auto log_dir = std::filesystem::path("liblog_test_logs_info");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_info.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 4);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_warn) {
    auto log_dir = std::filesystem::path("liblog_test_logs_warn");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_warn.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 3);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_erro) {
    auto log_dir = std::filesystem::path("liblog_test_logs_erro");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_erro.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 2);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_crit) {
    auto log_dir = std::filesystem::path("liblog_test_logs_crit");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_crit.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 1);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_off) {
    auto log_dir = std::filesystem::path("liblog_test_logs_off");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "FileLog00000.log";
    Everest::Logging::init("logging_configs/textfile_off.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    ASSERT_EQ(count_log_entries(file_name), 0);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_verb_rotate) {
    auto log_dir = std::filesystem::path("liblog_test_logs_verb_rotate");
    std::filesystem::remove_all(log_dir);
    Everest::Logging::init("logging_configs/textfile_verb_rotate.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    auto count = 0;
    for (auto& entry : std::filesystem::directory_iterator(log_dir)) {
        count += 1;
    }

    ASSERT_EQ(count, 5);
}

TEST_F(LibLogUnitTest, test_init_textfile_sink_verb_broken_filename) {
    auto log_dir = std::filesystem::path("liblog_test_logs_verb_rotate_broken_filename");
    std::filesystem::remove_all(log_dir);
    auto file_name = log_dir / "NoFormatString00000.log";
    Everest::Logging::init("logging_configs/textfile_verb_broken_filename.ini", "EVerest");
    log_with_all_loglevels();
    spdlog::default_logger()->flush();

    auto count = 0;
    for (auto& entry : std::filesystem::directory_iterator(log_dir)) {
        count += 1;
    }

    ASSERT_EQ(count, 5);

    ASSERT_EQ(count_log_entries(file_name), 1);
}

TEST_F(LibLogUnitTest, test_init_syslog_sink) {
    Everest::Logging::init("logging_configs/syslog.ini", "EVerest");
    log_with_all_loglevels();
}

TEST_F(LibLogUnitTest, test_init_syslog_sink_filtered) {
    Everest::Logging::init("logging_configs/syslog.ini", "Everest");
    log_with_all_loglevels();
}

TEST_F(LibLogUnitTest, test_trace) {
    auto trace_str = Everest::Logging::trace();
#ifdef WITH_LIBBACKTRACE
    EXPECT_THAT(trace_str,
                ::testing::StartsWith("#0: Everest::Logging::tests::LibLogUnitTest_test_trace_Test::TestBody()"));
#else
    ASSERT_EQ(trace_str, "Backtrace functionality not built in\n");
#endif
}

} // namespace tests
} // namespace Logging
} // namespace Everest
