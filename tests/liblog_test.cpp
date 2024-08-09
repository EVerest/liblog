// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#include <exception>
#include <mutex>

#include <gtest/gtest.h>
#include <spdlog/sinks/base_sink.h>

#include <everest/logging.hpp>

namespace Everest {
namespace Logging {

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

    void flush_() override {}
    size_t counter = 0;
};

class LibLogUnitTest : public ::testing::Test {
protected:
    void SetUp() override {
    }

    void TearDown() override {
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

} // namespace Logging
} // namespace Everest
