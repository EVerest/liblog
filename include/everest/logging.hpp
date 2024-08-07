// SPDX-License-Identifier: Apache-2.0
// Copyright Pionix GmbH and Contributors to EVerest
#ifndef LOGGING_HPP
#define LOGGING_HPP
#include <spdlog/spdlog.h>

#include <boost/current_function.hpp>
#include <boost/exception/diagnostic_information.hpp>
#include <boost/log/attributes/named_scope.hpp>
#include <boost/log/support/exception.hpp>
#include <boost/throw_exception.hpp>

#include <exception>
#include <iostream>
#include <sstream>
#include <string>

namespace Everest {
namespace Logging {

enum severity_level {
    verbose,
    debug,
    info,
    warning,
    error,
    critical,
};

void init(const std::string& logconf);
void init(const std::string& logconf, std::string process_name);
void update_process_name(std::string process_name);
std::string trace();

struct LogSource {
    std::string file;
    int line = 0;
    std::string function;

    bool log_file = true;
    bool log_line = true;
    bool log_function = true;

    LogSource(const std::string& file, int line, const std::string& function) :
        file(file), line(line), function(function) {
    }

    LogSource(const std::string& function) : function(function), log_file(false), log_line(false) {
    }
};

class EverestLogger {
public:
    EverestLogger(const std::string& file, int line, const std::string& function, spdlog::level::level_enum level) :
        file(file), line(line), function(function), level(level), log_file(true), log_line(true), log_function(true) {
    }
    EverestLogger(const std::string& function, spdlog::level::level_enum level) :
        function(function), level(level), log_file(false), log_line(false), log_function(true) {
    }
    EverestLogger(spdlog::level::level_enum level) :
        level(level), log_file(false), log_line(false), log_function(false) {
    }

    ~EverestLogger() {
#ifdef LIBLOG_OMIT_FILE_AND_LINE_NUMBERS
        log_file = false;
        log_line = false;
#endif
        if (log_file and log_line and log_function) {
            spdlog::default_logger_raw()->log(spdlog::source_loc{file.c_str(), line, function.c_str()}, level,
                                              log.str());
        } else if ((not log_file) and (not log_line) and log_function) {
            spdlog::default_logger_raw()->log(spdlog::source_loc{"", 0, function.c_str()}, level, log.str());
        } else {
            spdlog::default_logger_raw()->log(level, log.str());
        }
    }

    template <typename T> EverestLogger& operator<<(T const& message) {
        log << message;
        return *this;
    }

    EverestLogger& operator<<(std::ostream& (*message)(std::ostream&)) {
        log << message;
        return *this;
    }

private:
    std::stringstream log;
    std::string file;
    int line = 0;
    std::string function;
    spdlog::level::level_enum level;
    bool log_file;
    bool log_line;
    bool log_function;
};

} // namespace Logging

// clang-format off
#define EVLOG_verbose (::Everest::Logging::EverestLogger(__FILE__, __LINE__, BOOST_CURRENT_FUNCTION, spdlog::level::trace))
#define EVLOG_debug (::Everest::Logging::EverestLogger(BOOST_CURRENT_FUNCTION, spdlog::level::debug))
#define EVLOG_info (::Everest::Logging::EverestLogger(spdlog::level::info))
#define EVLOG_warning (::Everest::Logging::EverestLogger(BOOST_CURRENT_FUNCTION, spdlog::level::warn))
#define EVLOG_error (::Everest::Logging::EverestLogger(BOOST_CURRENT_FUNCTION, spdlog::level::err))
#define EVLOG_critical (::Everest::Logging::EverestLogger(BOOST_CURRENT_FUNCTION, spdlog::level::critical))

} // namespace Everest

template <class T> constexpr void EVLOG_AND_THROW(const T& ex) {
    EVLOG_error << ex.what();
    BOOST_THROW_EXCEPTION(boost::enable_error_info(ex)
                            << boost::log::BOOST_LOG_VERSION_NAMESPACE::current_scope());
}

template <class T> constexpr void EVTHROW(const T& ex) {
    BOOST_THROW_EXCEPTION(boost::enable_error_info(ex)
                            << boost::log::BOOST_LOG_VERSION_NAMESPACE::current_scope());
}

#endif // LOGGING_HPP
